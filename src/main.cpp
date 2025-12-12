#include "Encoder.h"
#include "animations/animations.h"
#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>

#define DATAPIN 13
#define mw 16
#define mh 16
#define MAX_BRIGHTNESS 90
#define ANIMATION_SPEED 150        // Speed in percent compared to original animation speed
int animation_change_interval = 5; // Time in seconds to change to the next animation. Signed so minMax works correctly on decrements
#define NUMMATRIX (mw * mh)

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

// Variables to track the current state of the animation
uint8_t currentAnimationIndex = 0;
uint16_t currentFrame = 0;
unsigned long lastFrameChangeTime = 0;
unsigned long lastAnimationChangeTime = millis();
int8_t brightness = MAX_BRIGHTNESS; // This is signed so minMax works correctly on decrements

// Message display management
uint16_t messageColor = matrix->Color(0, 255, 0);
unsigned long messageClearTime = 0;
String message = "";

// Function Prototypes
void playCurrentFrame(const Animation *anim);
void drawRGBBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h);
void showMessage(const String &msg, unsigned long duration_ms);
int minMax(int val, int minVal, int maxVal);
void turnOnDisplay();

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<NEOPIXEL, DATAPIN>(matrixleds, NUMMATRIX);
  matrix->begin();
  matrix->setBrightness(MAX_BRIGHTNESS);
  matrix->setTextWrap(false);
  matrix->setTextColor(messageColor);
  encoder1.begin();
  encoder2.begin();
}

void loop() {
  int shadeOfGray = map(messageClearTime - millis(), 0, 1000, 0, 255);
  matrix->setTextColor(matrix->Color(shadeOfGray, shadeOfGray, shadeOfGray));
  static unsigned long lastAnimationChangeTime = millis();

  // Get the current animation structure
  const Animation *currentAnim = &allAnimations[currentAnimationIndex];

  uint8_t duration_units = pgm_read_byte(currentAnim->frameDurations + currentFrame);
  uint32_t delay_ms = (uint32_t)duration_units * 100.0f * (100.0f / (float)ANIMATION_SPEED);

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
    currentAnimationIndex = (currentAnimationIndex + enc1_counter + TOTAL_ANIMATIONS) % TOTAL_ANIMATIONS;
    currentFrame = 0; // Reset frame counter for new animation
    enc1_counter = 0; // Reset encoder counter state

    Serial.println("Switched to animation index: " + String(currentAnimationIndex + 1));

    // Show animation index on the display but only if auto advance is disabled
    // It gets annoying to have the index pop up every time in normal mode
    if (!autoAdvanceEnabled) {
      showMessage(String(currentAnimationIndex + 1), 1000);
    }
    
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
  const uint32_t bytes_per_frame = (uint32_t)anim->width * anim->height * 3;
  const uint32_t start_offset = currentFrame * bytes_per_frame;
  const uint8_t *current_frame_addr = anim->animationFrames + start_offset;

  // Draw the bitmap for the current frame
  drawRGBBitmap(0, 0, current_frame_addr, anim->width, anim->height);

  if (millis() < messageClearTime) {
    matrix->setCursor(0, 0);
    matrix->print(message);
  }
  matrix->show();
}

void drawRGBBitmap(int16_t x, int16_t y, const uint8_t *bitmap, int16_t w, int16_t h) {
  uint32_t byteIndex = 0;

  for (uint16_t pixel = 0; pixel < w * h; pixel++) {
    uint8_t r, g, b;

    // Read the next three bytes (R, G, B) from the PROGMEM address
    r = pgm_read_byte(bitmap + byteIndex++);
    g = pgm_read_byte(bitmap + byteIndex++);
    b = pgm_read_byte(bitmap + byteIndex++);

    // Color conversion (RGB888 to RGB565)
    // Map(0-255) to 5-bit (0-31), 6-bit (0-63), 5-bit (0-31)
    b = map(b, 0, 255, 0, 31);
    g = map(g, 0, 255, 0, 63);
    r = map(r, 0, 255, 0, 31);

    // Combine into 16-bit RGB565 format (R:11-15, G:5-10, B:0-4)
    RGB_bmp_fixed[pixel] = (r << 11) | (g << 5) | b;
  }

  // The matrix library function reads from the RAM buffer (RGB_bmp_fixed)
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
  if (ignoreEncoder1Button) {
    ignoreEncoder1Button = false;
    return;
  }
  autoAdvanceEnabled = !autoAdvanceEnabled;
  showMessage(autoAdvanceEnabled ? ">>" : String(currentAnimationIndex), 1000);

  // If we are in situation where brightness is zero, and user pressed a button - restore it
  turnOnDisplay();
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

int minMax(int val, int minVal, int maxVal) {
  if (val < minVal)
    return minVal;
  if (val > maxVal)
    return maxVal;
  return val;
}