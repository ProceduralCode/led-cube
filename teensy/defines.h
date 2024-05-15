#ifndef __DEFINES_H
#define __DEFINES_H

/******************************************************************************
 * Defines
******************************************************************************/
#define COLOR_DEPTH 24
#define PANEL_SIZE 64
#define PANEL_AREA PANEL_SIZE * PANEL_SIZE
#define PANELS_HORIZONTAL 3
#define PANELS_VERTICAL 2
#define kMatrixWidth PANEL_SIZE * PANELS_HORIZONTAL
#define kMatrixHeight PANEL_SIZE * PANELS_VERTICAL
#define kRefreshDepth 36
#define kDmaBufferRows 4
#define INPUT_RESERVE PANEL_AREA * 4
#define LED_PIN 13
//#define LED_ENABLED

#endif
