#include "time.h"
#include <FastLED_NeoMatrix.h>

#define HOURS_COLOR 255,0,0
#define QUARTERS_COLOR 100,0,0
#define MINUTE_COLOR 0,255,0
#define SECONDS_COLOR 0,0,255
#define GREY_COLOR 100,100,100

void drawAnalogClock(FastLED_NeoMatrix *matrix);
void drawRingClock(FastLED_NeoMatrix *matrix);
void drawBarsClock(FastLED_NeoMatrix *matrix);
void drawDigitalClock(FastLED_NeoMatrix *matrix);