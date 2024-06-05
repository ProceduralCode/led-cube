#ifndef __PACKET_H
#define __PACKET_H

#include <stdint.h>
#include "defines.h"
#include "typedefs.h"

typedef struct _CommandDrawPanel_t{
    uint8_t panelId;
    uint8_t bufferId;
    uint8_t reserved[2];
    RGB pixelMap[PANEL_SIZE][PANEL_SIZE];
} CommandDrawPanel_t;

typedef struct _CommandGetPixel_t
{
    uint8_t command;
    uint8_t x, y;
} CommandGetPixel_t;

typedef struct _CommandFillScreen_t
{
    uint8_t command;
    RGB rgb;
} CommandFillScreen_t;

#endif
