/*
 Simple morphing clock watch face for Pebble Time

 2016 Fabio Arnold
*/

#include "clock.h"

static Window *s_main_window;
static Layer *s_clock_layer;
static Layer *s_menubar_layer;

// Battery data
static GBitmap *s_battery_bitmap;
static int s_battery_charge_percent = 0;
static bool s_battery_is_charging = false;

// Bluetooth data
static GBitmap *s_bt_bitmap;
static GBitmap *s_bt_disconnected_bitmap;
static bool s_bluetooth_connected = false;

// Clock data
static Clock *s_clock;

// Time in milliseconds between frames
#define FRAME_DELTA 13

static void clock_next_frame_handler(void *context) {
	// Draw a frame
	layer_mark_dirty(s_clock_layer);

	// Complete animation
	if (clock_has_frame(s_clock)) {
		// Continue in FRAME_DELTA ms with next frame
		app_timer_register(FRAME_DELTA, clock_next_frame_handler, /*context*/NULL);
	}
}

static void clock_update_time(TimeUnits units_changed, bool animated) {
	// Get a local tm structure
	time_t temp = time(NULL);
#if 0
	// Test code for animation debugging
	time_t debug_time = 1459113970;
	temp = 60*60*(temp - debug_time) + debug_time; // speed it up
	units_changed = SECOND_UNIT|MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT|YEAR_UNIT; // all
#endif
	struct tm *tick_time = localtime(&temp);

	clock_set_time(s_clock, tick_time, units_changed, animated);

	if (animated) {
		// Start the animation
		app_timer_register(FRAME_DELTA, clock_next_frame_handler, /*context*/NULL);
	}
}

static void clock_layer_update_proc(Layer *layer, GContext *ctx) {
	// Draw something
	GRect bounds = layer_get_bounds(layer);
	clock_draw(s_clock, ctx, bounds);
}

static void menubar_layer_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	GRect rect;

	// Draw black bar
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, /*corner_radius*/0, /*corner_mask*/0x0);

	// Draw battery
	GRect battery_bounds = gbitmap_get_bounds(s_battery_bitmap);
	battery_bounds.origin.x = bounds.size.w - battery_bounds.size.w - 4;
	graphics_context_set_compositing_mode(ctx, GCompOpSet);
	graphics_draw_bitmap_in_rect(ctx, s_battery_bitmap, battery_bounds);

	// Draw battery charge
	rect.origin.x = battery_bounds.origin.x+2; rect.origin.y = 5;
	rect.size.w = (14*s_battery_charge_percent + 99) / 100; rect.size.h = 6;
	graphics_context_set_fill_color(ctx, rect.size.w <= 3 ? GColorRed : GColorBrightGreen);
	graphics_fill_rect(ctx, rect, /*corner_radius*/0, /*corner_mask*/0x0);

	// Draw Bluetooth connection status
	GBitmap *bt_bitmap = s_bluetooth_connected ? s_bt_bitmap : s_bt_disconnected_bitmap;
	GRect bt_bounds = gbitmap_get_bounds(bt_bitmap);
	bt_bounds.origin.x = 4;
	graphics_draw_bitmap_in_rect(ctx, bt_bitmap, bt_bounds);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
	clock_update_time(units_changed, /*animated*/true);
}

static void battery_handler(BatteryChargeState charge) {
	s_battery_charge_percent = charge.charge_percent;
	s_battery_is_charging = charge.is_charging;
	// Redraw menubar
	layer_mark_dirty(s_menubar_layer);
}

static void bluetooth_handler(bool connected) {
	s_bluetooth_connected = connected;
	// Redraw menubar
	layer_mark_dirty(s_menubar_layer);
}

static void main_window_load(Window *window) {
	// Get info about the window
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	// Set window appearance
	window_set_background_color(window, GColorBlack); // GColorCobaltBlue);

	// Create clock layer
	s_clock_layer = layer_create(bounds);
	layer_set_update_proc(s_clock_layer, clock_layer_update_proc);
	layer_add_child(window_layer, s_clock_layer);

	// Create menubar layer
	s_menubar_layer = layer_create(GRect(0, 0, bounds.size.w, 16));
	layer_set_update_proc(s_menubar_layer, menubar_layer_update_proc);
	layer_add_child(window_layer, s_menubar_layer);

	// Register with TickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
	//tick_timer_service_subscribe(SECOND_UNIT, tick_handler);

	// Register with BatteryStateService
	battery_state_service_subscribe(battery_handler);
	battery_handler(battery_state_service_peek()); // Get initial battery state

	// Register Bluetooth with ConnectionService
	connection_service_subscribe((ConnectionHandlers) {
		.pebble_app_connection_handler = bluetooth_handler
	});
	bluetooth_handler(connection_service_peek_pebble_app_connection());
}

static void main_window_unload(Window *window) {
	// Unsubscribe all services
	tick_timer_service_unsubscribe();
	battery_state_service_unsubscribe();
	connection_service_unsubscribe();

	// Destroy all layers
	layer_destroy(s_clock_layer);
	layer_destroy(s_menubar_layer);
}

static void init(void) {
	// Create clock
	s_clock = clock_create();

	// Create images
	s_battery_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BATTERY);
	s_bt_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_CONNECTED);
	s_bt_disconnected_bitmap = gbitmap_create_with_resource(RESOURCE_ID_BT_DISCONNECTED);

	// Make sure the time is displayed from the start
	clock_update_time(/*units_changed*/SECOND_UNIT|MINUTE_UNIT|HOUR_UNIT|DAY_UNIT|MONTH_UNIT|YEAR_UNIT,
		/*animated*/false);

	// Create main window
	s_main_window = window_create();

	// Set handlers to manage the elements inside the window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});

	// Show window on the watch, with animated=true
	window_stack_push(s_main_window, /*animated*/true);
}

static void deinit(void) {
	// Destroy window
	window_destroy(s_main_window);

	// Destroy clock
	clock_destroy(s_clock);

	// Delete images
	gbitmap_destroy(s_battery_bitmap);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
