#ifndef __PACKET_H
#define __PACKET_H

#include "defines.h"

typedef struct _CommandDrawPanel_t
{
    uint8_t panelId;
    uint8_t bufferId;
    uint8_t reserved[2];
    uint8_t pixelMap[PANEL_AREA * COLORS];
} CommandDrawPanel_t;

typedef struct _CommandGetPixel_t
{
    uint8_t x, y;
} CommandGetPixel_t;

#endif
