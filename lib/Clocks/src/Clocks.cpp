#include "Clocks.h"

#include <math.h>

void drawAnalogClock(FastLED_NeoMatrix *matrix) {
  const int width = 16;
  const int height = 16;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  // Clear the matrix
  matrix->fillScreen(0);

  // Clock center
  int cx = width / 2;
  int cy = height / 2;
  int radius = 7;

  // Draw clock face (circle)
  matrix->drawCircle(cx, cy, radius, matrix->Color(GREY_COLOR));

  // Draw hour hand
  float hour = timeinfo.tm_hour % 12 + timeinfo.tm_min / 60.0;
  float hour_angle = (hour / 12.0) * 2 * PI - PI / 2;
  int hx = cx + cos(hour_angle) * (radius - 4);
  int hy = cy + sin(hour_angle) * (radius - 4);
  matrix->drawLine(cx, cy, hx, hy, matrix->Color(HOURS_COLOR));

  // Draw minute hand
  float minute = timeinfo.tm_min + timeinfo.tm_sec / 60.0;
  float min_angle = (minute / 60.0) * 2 * PI - PI / 2;
  int mx = cx + cos(min_angle) * (radius - 2);
  int my = cy + sin(min_angle) * (radius - 2);
  matrix->drawLine(cx, cy, mx, my, matrix->Color(MINUTE_COLOR));

  // Draw second hand
  float sec_angle = (timeinfo.tm_sec / 60.0) * 2 * PI - PI / 2;
  int sx = cx + cos(sec_angle) * (radius - 1);
  int sy = cy + sin(sec_angle) * (radius - 1);
  matrix->drawLine(cx, cy, sx, sy, matrix->Color(SECONDS_COLOR));

  // FastLED.show();
}

static void drawPerimeter(FastLED_NeoMatrix *matrix, int start, int end, uint16_t color) {
  int n = end - start;
  // Top row
  for (int x = start; x < end; ++x)
    matrix->drawPixel(x, start, color);
  // Right column
  for (int y = start + 1; y < end - 1; ++y)
    matrix->drawPixel(end - 1, y, color);
  // Bottom row
  for (int x = end - 1; x >= start; --x)
    matrix->drawPixel(x, end - 1, color);
  // Left column
  for (int y = end - 2; y > start; --y)
    matrix->drawPixel(start, y, color);
}

// Helper to get the (x, y) for a given index along a square ring perimeter
static void getPerimeterXY(int ringSize, int idx, int &x, int &y) {
  // ringSize: 16, 14, 12
  int start = (16 - ringSize) / 2;
  int end = start + ringSize;
  int perim = (ringSize - 1) * 4;

  idx = (idx + ringSize / 2) % perim;

  if (idx < ringSize - 1) { // Top
    x = start + idx;
    y = start;

  } else if (idx < (ringSize - 1) * 2) {
    x = end - 1;
    y = start + (idx - (ringSize - 1));
  } else if (idx < (ringSize - 1) * 3) {
    x = end - 1 - (idx - (ringSize - 1) * 2);
    y = end - 1;
  } else if (idx < (ringSize - 1) * 4) {
    x = start;
    y = end - 1 - (idx - (ringSize - 1) * 3);
  } else {
    x = start + idx;
    y = start;
  }
}

void drawRingClock(FastLED_NeoMatrix *matrix) {
  const int width = 16;
  const int height = 16;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  matrix->fillScreen(0);

  // Draw backgrounds
  drawPerimeter(matrix, 0, 16, matrix->Color(GREY_COLOR)); // seconds ring (grey)
  drawPerimeter(matrix, 1, 15, matrix->Color(0, 0, 0));    // minutes ring (black)
  drawPerimeter(matrix, 2, 14, matrix->Color(GREY_COLOR)); // hours ring (grey)

  // Draw seconds pixel (outer ring, 60 positions - 2x16 + 2x14=60)
  int sec_idx = (timeinfo.tm_sec % 60);
  int sx, sy;
  getPerimeterXY(16, sec_idx, sx, sy);
  matrix->drawPixel(sx, sy, matrix->Color(SECONDS_COLOR));

  // Draw minutes pixel (middle ring, 52 positions - 2x14 + 2x12=52)
  int min_idx = map(timeinfo.tm_min, 0, 59, 0, 51);
  int mx, my;
  getPerimeterXY(14, min_idx, mx, my);
  matrix->drawPixel(mx, my, matrix->Color(MINUTE_COLOR));

  // Draw hours pixel (inner ring, 44 positions-  2x12 + 2x10=44)
  // We can convert 12-hour time to 44 positions by multiplying by 4 and adding quarter hours, then map 48 possible positions to 44 pixels
  int hour12 = timeinfo.tm_hour % 12;
  int hour_idx = map((hour12 * 4) + (timeinfo.tm_min / 15), 0, 47, 0, 43); // 12*4=48, each 15 min advances 1
  int hx, hy;
  getPerimeterXY(12, hour_idx, hx, hy);
  matrix->drawPixel(hx, hy, matrix->Color(HOURS_COLOR));
}

void drawBarsClock(FastLED_NeoMatrix *matrix) {
  const int width = 16;
  const int height = 16;

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  matrix->fillScreen(0);

  // top 2 and bottom 2 rows are filled with grey (60,60,60)
  matrix->fillRect(0, 0, width, 2, matrix->Color(GREY_COLOR));
  matrix->fillRect(0, height - 2, width, 2, matrix->Color(GREY_COLOR));

  // Draw hours bar (left side)
  int hourHeight = timeinfo.tm_hour % 12;
  int quarterWidth = timeinfo.tm_min / 15;
  if (hourHeight > 0) {
    matrix->fillRect(0, height - 2 - hourHeight, 4, hourHeight, matrix->Color(HOURS_COLOR));
  }
  matrix->fillRect(0, height - 2 - hourHeight - 1, quarterWidth, 1, matrix->Color(QUARTERS_COLOR));

  // Draw minutes bar (center side)
  int minutesFullLines = timeinfo.tm_min / 5;
  int minutesRemainder = timeinfo.tm_min % 5;
  if (minutesFullLines > 0) {
    matrix->fillRect(5, height - 2 - minutesFullLines, 5, minutesFullLines, matrix->Color(MINUTE_COLOR));
  }
  matrix->fillRect(5, height - 2 - minutesFullLines - 1, minutesRemainder, 1, matrix->Color(MINUTE_COLOR));

  // Draw seconds bar (right side)
  int secondsFullLines = timeinfo.tm_sec / 5;
  int secondsRemainder = timeinfo.tm_sec % 5;
  if (secondsFullLines > 0) {
    matrix->fillRect(width - 5, height - 2 - secondsFullLines, 5, secondsFullLines, matrix->Color(SECONDS_COLOR));
  }
  matrix->fillRect(width - 5, height - 2 - secondsFullLines - 1, secondsRemainder, 1, matrix->Color(SECONDS_COLOR));
}

void drawDigitalClock(FastLED_NeoMatrix *matrix) {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }

  matrix->fillScreen(0);

  char timeStrH[3];
  snprintf(timeStrH, sizeof(timeStrH), "%02d", timeinfo.tm_hour);
  char timeStrM[3];
  snprintf(timeStrM, sizeof(timeStrM), "%02d", timeinfo.tm_min);

  matrix->setTextColor(matrix->Color(255, 255, 255)); // White color
  matrix->setTextSize(1);

  matrix->setCursor(2, 0);
  matrix->print(timeStrH);
  if (timeinfo.tm_sec % 2 == 0) {
    // Blink colon every second 
    matrix->setCursor(12, 0);
    matrix->print(":");
  }
  matrix->setCursor(2, 8);
  matrix->print(timeStrM);
}