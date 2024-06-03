#ifndef __PACKET_H
#define __PACKET_H

#include <stdint.h>
#include "defines.h"

typedef struct _CommandDrawPanel_t{
    uint8_t panelId;
    uint8_t bufferId;
    uint8_t reserved[2];
    uint8_t pixelMap[PANEL_AREA * COLORS];
} CommandDrawPanel_t;

typedef struct _CommandFillScreen_t
{
    uint8_t command;
    uint8_t red, green, blue;
} CommandFillScreen_t;

#endif
