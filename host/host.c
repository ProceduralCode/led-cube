/******************************************************************************
 * ttyACM0
 *****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include "FPToolkit.c"
#include "defines.h"
#include "packet.h"
#include "serial.h"

typedef struct _PIXEL_t
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} PIXEL_t;

void fillPanel(PIXEL_t panel[PANEL_SIZE][PANEL_SIZE], PIXEL_t color)
{
  for(int y = 0; y < PANEL_SIZE; y++)
  {
    for(int x = 0; x < PANEL_SIZE; x++)
    {
      panel[y][x] = color;
    }
  }
}

int main(int argc, char *argv[])
{
    PIXEL_t display[PANELS][PANEL_SIZE][PANEL_SIZE];
    int swidth = PANEL_SIZE * PANELS_HORIZONTAL;
    int sheight = PANEL_SIZE * PANELS_VERTICAL;

    serialInit();

    // Setup test Panels
    fillPanel(display[0], (PIXEL_t){255,   0,   0});
    fillPanel(display[1], (PIXEL_t){255, 255,   0});
    fillPanel(display[2], (PIXEL_t){  0, 255,   0});
    fillPanel(display[3], (PIXEL_t){  0, 255, 255});
    fillPanel(display[4], (PIXEL_t){  0,   0, 255});
    fillPanel(display[5], (PIXEL_t){255,   0, 255});

    // Draw to computer screen
    G_init_graphics(swidth, sheight);
    for(int n = 0; n < PANELS; n++)
    {
      int x0 = (n % COLORS) * PANEL_SIZE;
      int y0 = (n / COLORS) * PANEL_SIZE;
      for(int y = 0; y < PANEL_SIZE; y++)
      {
        for(int x = 0; x < PANEL_SIZE; x++)
        {
          G_rgb(display[n][y][x].red/255, display[n][y][x].green/255, display[n][y][x].blue/255);
          G_point(x0+x, y0+y);
        }
      }
    }

    // Send pixel map(s) to the teensy
    CommandDrawPanel_t packet;
    for(int i = 0; i < PANELS; i++)
    {
      packet.panelId = i+1;
      memcpy(packet.pixelMap, display[i], sizeof(display[i]));
      serialWrite(0, (char*)&packet, sizeof(packet));
      serialRead(0);
    }
    // Send command to update the panels
    serialWrite(0, "d", 1);
    serialRead(0);

    G_wait_key();

    serialWrite(0, "c", 1);
    serialRead(0);

    return 0;
}
