#include "Encoder.h"
#include "animations/animations.h"
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <FastLED.h>
#include <WiFiManager.h>
#include "time.h"
#include <FastLED_NeoMatrix.h>
#include "settings.h"
#include "Clocks.h"

// Turn on debug statements to the serial output
#define DEBUG 1

#if DEBUG
  #define PRINT(s, x)     \
    {                     \
      Serial.print(F(s)); \
      Serial.print(x);    \
    }
  #define PRINTS(x) Serial.print(F(x))
  #define PRINTX(x) Serial.println(x, HEX)
#else
  #define PRINT(s, x)
  #define PRINTS(x)
  #define PRINTX(x)
#endif

CRGB matrixleds[NUMMATRIX];

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(matrixleds, mw, mh, NEO_MATRIX_BOTTOM + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG);

// --- Encoders management ---
// --- ENCODER 1 (Left Encoder) ---
#define ENC1_A 14      // GPIO 14 (A channel)
#define ENC1_B 27      // GPIO 27 (B channel)
#define ENC1_BUTTON 26 // GPIO 26

// --- ENCODER 2 (Right Encoder) ---
#define ENC2_A 33      // GPIO 33 (A channel)
#define ENC2_B 25      // GPIO 25 (B channel)
#define ENC2_BUTTON 32 // GPIO 32

volatile int enc1_counter = 0; // This counter will be modified in the ISR and controls animation switching
volatile int enc2_counter = 0; // This counter will be modified in the ISR and controls animation change interval

volatile bool autoAdvanceEnabled = true;
volatile bool animationEnabled = true;

// Forward declarations of ISR callback functions
void IRAM_ATTR onEncoder1Button();
void IRAM_ATTR onEncoder2Button();

Encoder encoder1(ENC1_A, ENC1_B, ENC1_BUTTON, &enc1_counter, onEncoder1Button);
Encoder encoder2(ENC2_A, ENC2_B, ENC2_BUTTON, &enc2_counter, onEncoder2Button);

bool ignoreEncoder1Button = false;
bool ignoreEncoder2Button = false;

// --- Global Animation Management ---
// Buffer for the converted RGB565 data (e.g. 16x16 pixels * 2 bytes/pixel)
static uint16_t RGB_bmp_fixed[mw * mh];
int animation_change_interval = INITIAL_ANIMATION_INTERVAL; // Time in seconds to change to the next animation. Signed so minMax works correctly on decrements

// Variables to track the current state of the animation
uint8_t currentAnimationIndex = 0;
uint16_t currentFrame = 0;
unsigned long lastFrameChangeTime = 0;
unsigned long lastAnimationChangeTime = millis();
int8_t brightness = MAX_BRIGHTNESS; // This is signed so minMax works correctly on decrements
bool displayClock = false;
uint8_t clockMode = 0; // 0 - Digital, 1 - Ring, 2 - Bars, 3 - Analog

// Message display management
uint16_t messageColor = matrix->Color(0, 255, 0);
unsigned long messageClearTime = 0;
String message = "";

// --- Wifi ---
WiFiManager wm;

// Function Prototypes
void playCurrentFrame(const Animation *anim);
void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);
void showMessage(const String &msg, unsigned long duration_ms);
int minMax(int val, int minVal, int maxVal);
void turnOnDisplay();
void checkTimeSync();
void encoder1LongPressCheck();
void drawClock();

void setup() {
  Serial.begin(115200);
  delay(200);

  // Wifi setup
  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP   
  // Uncomment to reset settings
  // wm.resetSettings();
  wm.setConfigPortalBlocking(false);
  wm.setConfigPortalTimeout(10);
  if(wm.autoConnect("WIFI-CONFIG-AP")){
      Serial.println("wiFi connected");
  }
  else {
      Serial.println("Config portal running");
  }

  FastLED.addLeds<NEOPIXEL, DATAPIN>(matrixleds, NUMMATRIX);
  matrix->begin();
  matrix->setBrightness(MAX_BRIGHTNESS);
  matrix->setTextWrap(false);
  matrix->setTextColor(messageColor);
  encoder1.begin();
  encoder2.begin();
}

void loop() {
  wm.process();
  checkTimeSync();

  encoder1LongPressCheck();

  int shadeOfGray = map(messageClearTime - millis(), 0, 1000, 0, 255);
  matrix->setTextColor(matrix->Color(shadeOfGray, shadeOfGray, shadeOfGray));
  static unsigned long lastAnimationChangeTime = millis();

  // Get the current animation structure
  const Animation *currentAnim = &allAnimations[currentAnimationIndex];

  uint8_t duration_units = pgm_read_byte(currentAnim->frameDurations + currentFrame);
  uint32_t delay_ms = (uint32_t)duration_units * 100.0f * (100.0f / (float)ANIMATION_SPEED);

  if (displayClock) {
    drawClock();
  }
  else {
    // Check if animation is Enabled and advance frame if it's time to display the next frame
    if (millis() - lastFrameChangeTime >= delay_ms) {
      lastFrameChangeTime = millis();
      playCurrentFrame(currentAnim);

      // Increment frame counter, wrap around if necessary
      if (animationEnabled) {
        currentFrame++;
        if (currentFrame >= currentAnim->frameCount) {
          currentFrame = 0;
        }
      }
    }
  }

  // Check if it's time to auto-advance to the next animation
  if (millis() - lastAnimationChangeTime >= animation_change_interval * 1000) {
    lastAnimationChangeTime = millis();

    // Move to the next animation but only if auto-advance is enabled and animation is enabled
    if (autoAdvanceEnabled && animationEnabled) {
      currentAnimationIndex = (currentAnimationIndex + 1) % TOTAL_ANIMATIONS;
      currentFrame = 0; // Reset frame counter for new animation
    }
  }

  // Encoder1 controls animation selection
  if (enc1_counter != 0) {
    turnOnDisplay();

    if (displayClock) {
      clockMode = clockMode + enc1_counter;
      Serial.println("Clock mode changed to: " + String(clockMode));
    }
    else {
      currentAnimationIndex = (currentAnimationIndex + enc1_counter + TOTAL_ANIMATIONS) % TOTAL_ANIMATIONS;
      currentFrame = 0; // Reset frame counter for new animation

      Serial.println("Switched to animation index: " + String(currentAnimationIndex + 1));

      // Show animation index on the display but only if auto advance is disabled
      // It gets annoying to have the index pop up every time in normal mode
      if (!autoAdvanceEnabled) {
        showMessage(String(currentAnimationIndex + 1), 1000);
      }
    }

    enc1_counter = 0; // Reset encoder counter state
  }

  // Encoder 2 controls either brightness or animation change interval
  if (enc2_counter != 0) {

    if (encoder2.currentlyPressed) {
      // If the button is currently pressed, raise a flag to ignore the next button press event
      ignoreEncoder2Button = true;

      // brightness += (enc2_counter/abs(enc2_counter)) * 5;
      brightness += enc2_counter * 5;
      brightness = minMax(brightness, 0, MAX_BRIGHTNESS);
      matrix->setBrightness(brightness);
      Serial.println("Brightness set to: " + String(brightness) + "%");
      showMessage(String(brightness) + "%", 1000);

    } else {
      // If we are in situation where brightness is zero, and user turned the encoder - restore it
      turnOnDisplay();

      animation_change_interval += enc2_counter; // Change interval by 1 second per step
      animation_change_interval = minMax(animation_change_interval, 1, 60);

      Serial.println("Animation change interval: " + String(animation_change_interval) + " s");
      showMessage(String(animation_change_interval) + "s", 1000);
    }
    enc2_counter = 0;
  }
}

// --- Frame Playback Function ---
void playCurrentFrame(const Animation *anim) {

  // Calculate the start of the current frame data in PROGMEM
  const uint32_t words_per_frame = (uint32_t)anim->width * anim->height;
  const uint32_t start_offset = currentFrame * words_per_frame;
  const uint16_t *current_frame_addr = anim->animationFrames + start_offset;

  // Draw the bitmap for the current frame
  drawRGBBitmap(0, 0, current_frame_addr, anim->width, anim->height);

  if (millis() < messageClearTime) {
    matrix->setCursor(0, 0);
    matrix->print(message);
  }
  matrix->show();
}

void drawRGBBitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h) {
  // Bitmap points to PROGMEM array of uint16_t RGB565 values
  const uint16_t *bitmap16 = (const uint16_t *)bitmap;
  for (uint16_t pixel = 0; pixel < w * h; pixel++) {
    // Read 16-bit value from PROGMEM
    RGB_bmp_fixed[pixel] = pgm_read_word(bitmap16 + pixel);
  }
  matrix->drawRGBBitmap(x, y, RGB_bmp_fixed, w, h);
}

void turnOnDisplay() {
  if (brightness == 0) {
    brightness = minMax(50, 0, MAX_BRIGHTNESS); // Set to 50% or MAX_BRIGHTNESS if it is below 50%
    matrix->setBrightness(brightness);
    showMessage(String(brightness) + "%", 1000);
  }
}

void IRAM_ATTR onEncoder1Button() {
  if (displayClock) {
    Serial.println("Doing nothing, clock display active");
    return;
  }
  
  if (ignoreEncoder1Button) {
    ignoreEncoder1Button = false;
    return;
  }
  autoAdvanceEnabled = !autoAdvanceEnabled;
  showMessage(autoAdvanceEnabled ? ">>" : String(currentAnimationIndex), 1000);

  // If we are in situation where brightness is zero, and user pressed a button - restore it
  turnOnDisplay();
}

void encoder1LongPressCheck() {
  static unsigned long lastEncoderPressTime = 0;
  // Handle a case of encoder1 is just pressed
  if (encoder1.currentlyPressed && lastEncoderPressTime == 0) {
    lastEncoderPressTime = millis();
  }
  if (encoder1.currentlyPressed == false && lastEncoderPressTime != 0) {
    lastEncoderPressTime = 0;
  }
  if (lastEncoderPressTime != 0 && millis() - lastEncoderPressTime >= 500) {
    displayClock = !displayClock;
    lastEncoderPressTime = 0;
    ignoreEncoder1Button = true;

    Serial.println(displayClock ? "Clock display enabled" : "Animation display enabled");
  }

}

void IRAM_ATTR onEncoder2Button() {
  if (ignoreEncoder2Button) {
    ignoreEncoder2Button = false;
    return;
  }

  animationEnabled = !animationEnabled;

  // If we are in situation where brightness is zero, and user pressed a button - restore it
  turnOnDisplay();
}

void showMessage(const String &msg, unsigned long duration_ms) {
  message = msg;
  messageClearTime = millis() + duration_ms;
}

void drawClock() {
  switch (clockMode%4) {
    case 0:
      drawDigitalClock(matrix);
      break;
    case 1:
      drawRingClock(matrix);
      break;
    case 2:
      drawBarsClock(matrix);
      break;
    case 3:
      drawAnalogClock(matrix);
      break;
    default:
      break;
  }

  if (millis() < messageClearTime) {
    matrix->setCursor(0, 0);
    matrix->print(message);
  }
  matrix->show();
}

int minMax(int val, int minVal, int maxVal) {
  if (val < minVal)
    return minVal;
  if (val > maxVal)
    return maxVal;
  return val;
}

void checkTimeSync() {
  static bool timeSynced = false;

  if (!timeSynced) {
    if (WiFi.status() == WL_CONNECTED) {
      // Init and get the time
      configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER);
      Serial.println("Getting local time");

      struct tm timeinfo;
      if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
      }
      
      timeSynced = true;
      Serial.print("Local time: ");
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");

    }
  }


}