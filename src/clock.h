#define WITH_DATE 0

#include <pebble.h>

typedef struct {
	char value;
	char prv_value;
	char anim; // from 0 to tile_size, 0 means prv_value, tile_size means value
	char tile_size;
} Digit;

typedef struct {
#if WITH_DATE
	// Date
	Digit year[4];
	Digit month[2];
	Digit day[2];
#endif

	// Time
	Digit hours[2];
	Digit minutes[2];
	Digit seconds[2];
} Clock;

Clock *clock_create();
void clock_destroy(Clock *clock);

// Returns true if there are still animation frames
bool clock_has_frame(Clock *clock);

void clock_set_time(Clock *clock, struct tm *tick_time, TimeUnits units_changed, bool animated);
void clock_draw(Clock *clock, GContext *ctx, GRect bounds);
