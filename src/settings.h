#ifndef SETTINGS_H
#define SETTINGS_H

#define NTP_SERVER "pool.ntp.org"

#define GMT_OFFSET 7200       // Seconds offset from GMT. 7200 = GMT+2
#define DAYLIGHT_OFFSET 3600  // Seconds offset for daylight savings time. 3600 = +1 hour in winter

#define DATAPIN 13            // The data line for the NeoPixel matrix
#define mw 16                 // Matrix width in pixels
#define mh 16                 // Matrix height in pixels
#define MAX_BRIGHTNESS 90     // Maximum brightness of the matrix
#define ANIMATION_SPEED 150   // Speed in percent compared to original animation speed

#define NUMMATRIX (mw * mh)

#endif // SETTINGS_H