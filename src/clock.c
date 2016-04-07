#include "clock.h"
#include "digit_transitions.h"

static const int s_date_tile_size = 2;
static const int s_time_tile_size = 7;

// If this is 1 hours will be on top and minutes below
#define TIME_VARIATION1 1
static const int s_hours_tile_size = 13;
static const int s_minutes_tile_size = 7;

static void digit_init(Digit *digit, int tile_size) {
	digit->value = 0;
	digit->prv_value = 0;
	digit->anim = tile_size; // animation completed
	digit->tile_size = tile_size;
}

static void digit_set_value(Digit *digit, int value, bool animated) {
	if (digit->value != value) {
		digit->prv_value = digit->value;
		digit->value = value;
		digit->anim = 0; // start animation
	}
	if (!animated) {
		digit->anim = digit->tile_size;
	}
}

static void swap_int(int *i0, int *i1) {
	int tmp = *i0;
	*i0 = *i1;
	*i1 = tmp;
}

static void digit_draw(Digit *digit, GContext *ctx, int pos_x, int pos_y) {
	int lower = digit->prv_value;
	int higher = digit->value;
	int alpha0 = digit->anim;
	int alpha1 = digit->tile_size - alpha0;

	if (higher < lower) { // invert animation
		swap_int(&lower, &higher);
		swap_int(&alpha0, &alpha1);
	}

	int transition = get_digit_transition(lower, higher);
	if (transition == -1) { // none found
		// make digit appear instantaneous
		if (digit->value < 9) { // use i of i to i+1
			alpha0 = 0;
			alpha1 = digit->tile_size;
			transition = digit->value;
		} else { // use 9 of 0 to 9 transition
			alpha0 = digit->tile_size;
			alpha1 = 0;
			transition = 12;
		}
	}

	GRect rect; rect.size.w = digit->tile_size; rect.size.h = digit->tile_size;
	for (int i = 0; i < 14; i++) {
		rect.origin.x = pos_x
			+ alpha1 * digit_tile_coords[28 * transition + i].x
			+ alpha0 * digit_tile_coords[28 * transition + 14 + i].x;
		rect.origin.y = pos_y
			+ alpha1 * digit_tile_coords[28 * transition + i].y
			+ alpha0 * digit_tile_coords[28 * transition + 14 + i].y;
		graphics_fill_rect(ctx, rect, 0, 0x0);
	}

	// next frame
	if (digit->anim < digit->tile_size) {
		digit->anim++;
	}
}

static void draw_colon(GContext *ctx, int pos_x, int pos_y, int tile_size) {
	GRect rect;
	rect.size.w = tile_size; rect.size.h = tile_size;
	rect.origin.x = pos_x; rect.origin.y = pos_y + tile_size;
	graphics_fill_rect(ctx, rect, 0, 0x0);
	rect.origin.y = pos_y + 3 * tile_size;
	graphics_fill_rect(ctx, rect, 0, 0x0);
}

static void draw_dash(GContext *ctx, int pos_x, int pos_y, int tile_size) {
	GRect rect;
	rect.size.w = 2*tile_size; rect.size.h = tile_size;
	rect.origin.x = pos_x; rect.origin.y = pos_y + 2*tile_size;
	graphics_fill_rect(ctx, rect, 0, 0x0);
}

Clock *clock_create() {
	Clock *clock = (Clock *)malloc(sizeof(Clock));

#if WITH_DATE
	digit_init(&clock->year[0], s_date_tile_size);
	digit_init(&clock->year[1], s_date_tile_size);
	digit_init(&clock->year[2], s_date_tile_size);
	digit_init(&clock->year[3], s_date_tile_size);
	digit_init(&clock->month[0], s_date_tile_size);
	digit_init(&clock->month[1], s_date_tile_size);
	digit_init(&clock->day[0], s_date_tile_size);
	digit_init(&clock->day[1], s_date_tile_size);
#endif

	// 24h format
#if TIME_VARIATION1
	digit_init(&clock->hours[0], s_hours_tile_size);
	digit_init(&clock->hours[1], s_hours_tile_size);
	digit_init(&clock->minutes[0], s_minutes_tile_size);
	digit_init(&clock->minutes[1], s_minutes_tile_size);
#else
	digit_init(&clock->hours[0], s_time_tile_size);
	digit_init(&clock->hours[1], s_time_tile_size);
	digit_init(&clock->minutes[0], s_time_tile_size);
	digit_init(&clock->minutes[1], s_time_tile_size);
#endif
	digit_init(&clock->seconds[0], s_time_tile_size);
	digit_init(&clock->seconds[1], s_time_tile_size);

	return clock;
}

void clock_destroy(Clock *clock) {
	free(clock);
}

bool clock_has_frame(Clock *clock) {
	return clock->hours[1].anim < clock->hours[1].tile_size
		|| clock->minutes[1].anim < clock->minutes[1].tile_size;
}

#if WITH_DATE
static void clock_set_date(Clock *clock, struct tm *tick_time, TimeUnits units_changed, bool animated) {
	if (units_changed & DAY_UNIT) {
		digit_set_value(&clock->day[0], tick_time->tm_mday/10, animated);
		digit_set_value(&clock->day[1], tick_time->tm_mday%10, animated);

		if (units_changed & MONTH_UNIT) {
			int month = 1+tick_time->tm_mon; // tm_mon is 0 to 11
			digit_set_value(&clock->month[0], month/10, animated);
			digit_set_value(&clock->month[1], month%10, animated);

			if (units_changed & YEAR_UNIT) {
				int year = 1900+tick_time->tm_year; // tm_year is current year - 1900
				digit_set_value(&clock->year[3], year%10, animated); year /= 10;
				digit_set_value(&clock->year[2], year%10, animated); year /= 10;
				digit_set_value(&clock->year[1], year%10, animated); year /= 10;
				digit_set_value(&clock->year[0], year%10, animated);
			}
		}
	}
}
#endif

void clock_set_time(Clock *clock, struct tm *tick_time, TimeUnits units_changed, bool animated) {
	// Update clock's digits
	if (units_changed & SECOND_UNIT) {
		digit_set_value(&clock->seconds[0], tick_time->tm_sec/10, animated);
		digit_set_value(&clock->seconds[1], tick_time->tm_sec%10, animated);

		if (units_changed & MINUTE_UNIT) {
			digit_set_value(&clock->minutes[0], tick_time->tm_min/10, animated);
			digit_set_value(&clock->minutes[1], tick_time->tm_min%10, animated);

			if (units_changed & HOUR_UNIT) {
				digit_set_value(&clock->hours[0], tick_time->tm_hour/10, animated);
				digit_set_value(&clock->hours[1], tick_time->tm_hour%10, animated);

#if WITH_DATE
				// Update date
				clock_set_date(clock, tick_time, units_changed, animated);
#endif
			}
		}
	}
}

// TODO: Handle bounds.origin if necessary (simply offset pos)
void clock_draw(Clock *clock, GContext *ctx, GRect bounds) {
	graphics_context_set_fill_color(ctx, GColorWhite);

	// Draw time
#if TIME_VARIATION1
	const int tiles_x = 7; // HH or MM
	const int tiles_y = 5; // per row
	const int padding = s_hours_tile_size+2; // space between rows
	const int offset_y = 6;

	int pos_x = (bounds.size.w - tiles_x*s_hours_tile_size) / 2; // center
	int pos_y = offset_y + (bounds.size.h - tiles_y*(s_hours_tile_size + s_minutes_tile_size) - padding) / 2;
	graphics_context_set_fill_color(ctx, GColorFolly);
	digit_draw(&clock->hours[0], ctx, pos_x, pos_y); pos_x += 4*s_hours_tile_size;
	digit_draw(&clock->hours[1], ctx, pos_x, pos_y);
	pos_x = (bounds.size.w - tiles_x*s_minutes_tile_size) / 2; // center
	pos_y += tiles_y*s_hours_tile_size + padding;
	graphics_context_set_fill_color(ctx, GColorWhite);
	digit_draw(&clock->minutes[0], ctx, pos_x, pos_y); pos_x += 4*s_minutes_tile_size;
	digit_draw(&clock->minutes[1], ctx, pos_x, pos_y);
#else
	const int time_tiles_x = 3+1+3+1+1+1+3+1+3; // HH:MM

	int pos_x = (bounds.size.w - time_tiles_x*s_time_tile_size) / 2; // center
	int pos_y = (bounds.size.h - 5*s_time_tile_size) / 2;

	digit_draw(&clock->hours[0], ctx, pos_x, pos_y); pos_x += 4*s_time_tile_size;
	digit_draw(&clock->hours[1], ctx, pos_x, pos_y); pos_x += 4*s_time_tile_size;
	draw_colon(ctx, pos_x, pos_y, s_time_tile_size); pos_x += 2*s_time_tile_size;
	digit_draw(&clock->minutes[0], ctx, pos_x, pos_y); pos_x += 4*s_time_tile_size;
	digit_draw(&clock->minutes[1], ctx, pos_x, pos_y);

	// TODO: Make seconds display optional
	//digit_draw(&clock->seconds[0], ctx, 0, 16);
	//digit_draw(&clock->seconds[1], ctx, 4*4, 16);
#endif

#if WITH_DATE
	// Draw date
	const int date_tiles_x = 15+1+2+1+7+1+2+1+7; // YYYY-MM-DD

	pos_x = (bounds.size.w - date_tiles_x*s_date_tile_size) / 2; // center
	pos_y += 5*s_time_tile_size + 2*s_date_tile_size;

	digit_draw(&clock->year[0], ctx, pos_x, pos_y); pos_x += 4*s_date_tile_size;
	digit_draw(&clock->year[1], ctx, pos_x, pos_y); pos_x += 4*s_date_tile_size;
	digit_draw(&clock->year[2], ctx, pos_x, pos_y); pos_x += 4*s_date_tile_size;
	digit_draw(&clock->year[3], ctx, pos_x, pos_y); pos_x += 4*s_date_tile_size;
	draw_dash(ctx, pos_x, pos_y, s_date_tile_size); pos_x += 3*s_date_tile_size;
	digit_draw(&clock->month[0], ctx, pos_x, pos_y); pos_x += 4*s_date_tile_size;
	digit_draw(&clock->month[1], ctx, pos_x, pos_y); pos_x += 4*s_date_tile_size;
	draw_dash(ctx, pos_x, pos_y, s_date_tile_size); pos_x += 3*s_date_tile_size;
	digit_draw(&clock->day[0], ctx, pos_x, pos_y); pos_x += 4*s_date_tile_size;
	digit_draw(&clock->day[1], ctx, pos_x, pos_y);
#endif
}
