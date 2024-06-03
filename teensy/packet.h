#include <stdint.h>
#ifndef __PACKET_H
#define __PACKET_H

#include "defines.h"

typedef struct _RGB
{
	uint8_t red, green, blue;
} RGB;

typedef struct _CommandDrawPanel_t
{
    uint8_t panelId;
    uint8_t bufferId;
    uint8_t reserved[2];
    RGB pixelMap[PANEL_SIZE][PANEL_SIZE];
} CommandDrawPanel_t;

typedef struct _CommandGetPixel_t
{
    uint8_t comamnd;
    uint8_t x, y;
} CommandGetPixel_t;

typedef struct _CommandFillScreen_t
{
    uint8_t command;
    uint8_t red, green, blue;
} CommandFillScreen_t;

#endif
