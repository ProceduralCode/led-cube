#include <MatrixHardware_Teensy4_ShieldV5.h>
#include <SmartMatrix.h>
#include <usb_serial.h>
#include "defines.h"
#include "packet.h"

/******************************************************************************
 * Prototypes
******************************************************************************/
void checkSerial();

/******************************************************************************
 * Globals
******************************************************************************/
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, SM_PANELTYPE_HUB75_64ROW_MOD32SCAN, SM_HUB75_OPTIONS_FM6126A_RESET_AT_START);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(drawingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, SM_BACKGROUND_OPTIONS_NONE);
bool g_stringComplete = false;  // whether the string is complete
char g_command = 0;
int  g_commandLength = 0;
volatile int g_index;
uint8_t g_Buffer[INPUT_RESERVE];
uint8_t g_RecieveBuffer[sizeof(CommandDrawPanel_t)];
uint8_t g_SendBuffer[128];

void setup()
{
  drawingLayer.enableColorCorrection(false);
  matrix.addLayer(&drawingLayer);
  matrix.begin();
  matrix.setBrightness(0x20);

  drawingLayer.fillScreen({0x00, 0x00, 0x00});
  drawingLayer.swapBuffers();

  Serial.begin(1000000);
}

void loop()
{
  checkSerial();
  processCommand();
}

void processCommand()
{
  int len = 0;

  switch(g_command)
  {
    case 0: // This is an empty command. No processing.
      break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    {
      CommandDrawPanel_t *packet = (CommandDrawPanel_t *)g_RecieveBuffer;
      updatePanel(packet);
      sprintf(g_SendBuffer, "Updating Panel %d %d\n", packet->panelId, packet->bufferId);
      len = len + strlen(g_SendBuffer);
      break;
    }
    case 'c':
      drawingLayer.fillScreen({0x00, 0x00, 0x00});
      drawingLayer.swapBuffers();
      sprintf(g_SendBuffer, "Clearing Panels\n");
      len = len + strlen(g_SendBuffer);
      break;
    case 'd':
      drawingLayer.swapBuffers();
      sprintf(g_SendBuffer, "Moving to display memory\n");
      len = len + strlen(g_SendBuffer);
      break;
    case 'f':
      sprintf(g_SendBuffer, "FPS: %d\n", matrix.getRefreshRate());
      len = len + strlen(g_SendBuffer);
      break;
    case 'r':
      drawingLayer.fillScreen({0xff, 0x00, 0x00});
      drawingLayer.swapBuffers();
      sprintf(g_SendBuffer, "Make Red\n");
      len = len + strlen(g_SendBuffer);
      break;
    case 'g':
      drawingLayer.fillScreen({0x00, 0xff, 0x00});
      drawingLayer.swapBuffers();
      sprintf(g_SendBuffer, "Make Green\n");
      len = len + strlen(g_SendBuffer);
      break;
    case 'b':
      drawingLayer.fillScreen({0x00, 0x00, 0xff});
      drawingLayer.swapBuffers();
      sprintf(g_SendBuffer, "Make Blue\n");
      len = len + strlen(g_SendBuffer);
      break;
    case 'w':
      drawingLayer.fillScreen({0xff, 0xff, 0xff});
      drawingLayer.swapBuffers();
      sprintf(g_SendBuffer, "Make White\n");
      len = len + strlen(g_SendBuffer);
      break;
    case 't':
      {
      sprintf(g_SendBuffer, "ID: 0x%X%X\n", HW_OCOTP_CFG0, HW_OCOTP_CFG1);
      len = len + strlen(g_SendBuffer);
      }
      break;
    case 'i':
      g_SendBuffer[0] = (0x000000FF & HW_OCOTP_CFG1) >> 0;
      g_SendBuffer[1] = (0x0000FF00 & HW_OCOTP_CFG1) >> 8;
      g_SendBuffer[2] = (0x00FF0000 & HW_OCOTP_CFG1) >> 16;
      g_SendBuffer[3] = (0xFF000000 & HW_OCOTP_CFG1) >> 24;
      g_SendBuffer[4] = (0x000000FF & HW_OCOTP_CFG0) >> 0;
      g_SendBuffer[5] = (0x0000FF00 & HW_OCOTP_CFG0) >> 8;
      g_SendBuffer[6] = (0x00FF0000 & HW_OCOTP_CFG0) >> 16;
      g_SendBuffer[7] = (0xFF000000 & HW_OCOTP_CFG0) >> 24;
      g_SendBuffer[8] = '\n';
      len = len + 9;
      //Serial.printf("ID: 0x%llX\n", *((uint64_t*)g_SendBuffer));
      break;
    case 'p':
    {
      CommandGetPixel_t *packet = (CommandGetPixel_t *)g_RecieveBuffer;
      rgb24 pixel = drawingLayer.readPixel(packet->x, packet->y);
      g_SendBuffer[0] = pixel.red;
      g_SendBuffer[1] = pixel.green;
      g_SendBuffer[2] = pixel.blue;
      g_SendBuffer[3] = '\n';
      len = len + 4;
      break;
    }
    case 'z':
    {
      CommandFillScreen_t *packet = (CommandFillScreen_t *)g_RecieveBuffer;
      rgb24 color(packet->red, packet->green, packet->blue);
      drawingLayer.fillScreen(color);
      drawingLayer.swapBuffers();
      sprintf(g_SendBuffer, "RGB: %3d %3d %3d\n", packet->red, packet->green, packet->blue);
      len = len + strlen(g_SendBuffer);
      break;
    }
    default:
      sprintf(g_SendBuffer, "Unknown command 0x%X with size %d\n", g_command, g_commandLength);
      len = len + strlen(g_SendBuffer);
  }

  // Check if there's a response to send
  if(len > 0)
  {
    Serial.write(g_SendBuffer, len);
    Serial.send_now();
  }

  // Clear the command flag for the next incoming packet
  g_command = 0;
}

void serialEvent()
{
  // Wait until we process the last serial string
  if(g_stringComplete) return;

  while (Serial.available() > 0) {
    // get the new byte:
    int unfiltered = Serial.read();

    g_Buffer[g_index] = unfiltered;
    g_index++;

    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (unfiltered == '\n')
    {
      g_stringComplete = true;
      g_Buffer[g_index-1] = '\0';
    }
  }
}

void checkSerial()
{
  // Block if we are processing a command already
  if(g_command != 0) return;

  if(g_stringComplete)
  {
    g_command = g_Buffer[0];
    // Remove the serial terminator
    g_commandLength = g_index - 1;
    memcpy(g_RecieveBuffer, g_Buffer, g_commandLength);

    // cleanup for the next command
    g_index = 0;
    g_stringComplete = false;
  }
}

void updatePanel(CommandDrawPanel_t *const packet)
{
  // We figure out which of the panels in the row we are on and then multiply an offset to get to the
  // x corner of a particular panel
  int x0 = ((packet->panelId - 1) % PANELS_HORIZONTAL) * PANEL_SIZE;
  // We do something similar for the y corner except we divide to reflect that we at wrapping around
  // to the panel underneath even though the panels are laid out linearly
  int y0 = ((packet->panelId - 1) / PANELS_HORIZONTAL) * PANEL_SIZE;
  rgb24 color;

  for(int y = 0; y < PANEL_SIZE; y++)
  {
    for(int x = 0; x < PANEL_SIZE; x++)
    {
      color.red   = packet->pixelMap[y][x].red;
      color.green = packet->pixelMap[y][x].green;
      color.blue  = packet->pixelMap[y][x].blue;
      drawingLayer.drawPixel(x0 + x, y0 + y, color);
      //if(x==2 && y==61) Serial.printf("Updating %3d,%3d 0x%02x%02x%02x %d %d\n", x0+x, y0+y, color.red, color.green, color.blue, g_RecieveBuffer[11722], g_Buffer[11722]);
    }
  }
}

void blink()
{
#ifdef LED_ENABLED
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(100);
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
#endif
}
