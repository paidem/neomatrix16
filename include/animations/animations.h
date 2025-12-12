// animations.h - Master file with all animation includes and array
#ifndef MASTER_ANIMATIONS_H
#define MASTER_ANIMATIONS_H

#include "animation_types.h"

#include "fireworks.h"
#include "chip.h"
#include "ducks_colors.h"
#include "frog.h"
#include "hearts.h"

// Global array of all available animations
const Animation allAnimations[] = { fireworksAnimation, chipAnimation, ducks_colorsAnimation, frogAnimation, heartsAnimation };
const uint8_t TOTAL_ANIMATIONS = sizeof(allAnimations) / sizeof(Animation);

#endif // MASTER_ANIMATIONS_H