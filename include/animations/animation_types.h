#ifndef ANIMATION_TYPES_H
#define ANIMATION_TYPES_H
#include <stdint.h>
#include <Arduino.h>

typedef struct {
    const uint16_t frameCount;
    const uint8_t width; 
    const uint8_t height; 
    const uint8_t *frameDurations; 
    const uint16_t *animationFrames; 
} Animation;

#endif
