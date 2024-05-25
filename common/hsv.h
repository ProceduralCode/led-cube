#ifndef __HSV_H
#define __HSV_H

#include <stdint.h>
#include "typedefs.h"

void hsv2rgb(RGB *const rgb, uint16_t hue, uint8_t saturation, uint8_t value);

#endif
