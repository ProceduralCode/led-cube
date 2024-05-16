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
SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, SM_PANELTYPE_HUB75_64ROW_MOD32SCAN, SM_HUB75_OPTIONS_NONE);
//SMARTMATRIX_ALLOCATE_BUFFERS(matrix, kMatrixWidth, kMatrixHeight, kRefreshDepth, kDmaBufferRows, SM_PANELTYPE_HUB75_64ROW_MOD32SCAN, SM_HUB75_OPTIONS_FM6126A_RESET_AT_START);
SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(drawingLayer, kMatrixWidth, kMatrixHeight, COLOR_DEPTH, SM_BACKGROUND_OPTIONS_NONE);
String g_inputString = "";      // a String to hold incoming data
bool g_stringComplete = false;  // whether the string is complete
char g_command = 0;
int  g_commandLength = 0;
uint8_t g_buffer[sizeof(CommandDrawPanel_t)];

void setup()
{
  matrix.addLayer(&drawingLayer);
  matrix.begin();
  matrix.setBrightness(0x40);

  drawingLayer.fillScreen({0x00, 0x00, 0x00});
  drawingLayer.swapBuffers();

#ifdef LED_ENABLED
  pinMode(LED_PIN, OUTPUT);
#endif
  Serial.begin(115200);
  g_inputString.reserve(INPUT_RESERVE);
}

void loop()
{
  checkSerial();
  processCommand();
}

void processCommand()
{
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
      CommandDrawPanel_t *packet = (CommandDrawPanel_t *)g_buffer;
      Serial.printf("Updating Panel %d %d\n", packet->panelId, packet->bufferId);
      updatePanel(packet);
      break;
    }
    case 'c':
      Serial.println("Clearing Panels");
      drawingLayer.fillScreen({0x00, 0x00, 0x00});
      drawingLayer.swapBuffers();
      break;
    case 'd':
      Serial.println("Moving to display memory");
      drawingLayer.swapBuffers();
      break;
    case 'f':
      Serial.printf("FPS: %d\n", matrix.getRefreshRate());
      break;
    case 'r':
      drawingLayer.fillScreen({0xff, 0x00, 0x00});
      drawingLayer.swapBuffers();
      Serial.println("Make Red");
      break;
    case 'g':
      drawingLayer.fillScreen({0x00, 0xff, 0x00});
      drawingLayer.swapBuffers();
      Serial.println("Make Green");
      break;
    case 'b':
      drawingLayer.fillScreen({0x00, 0x00, 0xff});
      drawingLayer.swapBuffers();
      Serial.println("Make Blue");
      break;
    case 'w':
      drawingLayer.fillScreen({0xff, 0xff, 0xff});
      drawingLayer.swapBuffers();
      Serial.println("Make White");
      break;
    case 't':
#ifdef LED_ENABLED
      Serial.println("Blinking");
      blink();
#else
      {
      rgb24 color(drawingLayer.readPixel(63, 63));
      Serial.printf("0x%02x%02x%02x\n", color.red, color.green, color.blue);
      }
#endif
      break;
    default:
      Serial.printf("Unknown command 0x%X with size %d\n", g_command, g_commandLength);
  }
  g_command = 0;
}

void serialEvent()
{
  // Wait until we process the last serial string
  if(g_stringComplete) return;

  while (Serial.available() > 0) {
    // get the new byte:
    char inChar = (char)Serial.read();

    // add it to the inputString:
    g_inputString += inChar;

    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n')
    {
      g_stringComplete = true;
      g_inputString[g_inputString.length()-1] = '\0';
    }
  }
}

void checkSerial()
{
  if(g_command != 0) return;

  if(g_stringComplete)
  {
    g_command = g_inputString[0];
    g_commandLength = g_inputString.length() - 1;
    memcpy(g_buffer, g_inputString.c_str(), g_commandLength);

    // cleanup for the next command
    g_inputString = "";
    g_stringComplete = false;
  }
}

void updatePanel(CommandDrawPanel_t *const packet)
{
  int x0 = ((packet->panelId - 1) % PANELS_HORIZONTAL) * PANEL_SIZE;
  int y0 = ((packet->panelId - 1) / PANELS_HORIZONTAL) * PANEL_SIZE;
  rgb24 color;

  for(int y = 0; y < PANEL_SIZE; y++)
  {
    for(int x = 0; x < PANEL_SIZE; x++)
    {
      color.red   = packet->pixelMap[COLORS*y * PANEL_SIZE + COLORS*x + 0];
      color.green = packet->pixelMap[COLORS*y * PANEL_SIZE + COLORS*x + 1];
      color.blue  = packet->pixelMap[COLORS*y * PANEL_SIZE + COLORS*x + 2];
      drawingLayer.drawPixel(x0 + x, y0 + y, color);
    }
  }
  //Serial.printf("Updating %d,%d 0x%02x%02x%02x\n", x0, y0, color.red, color.green, color.blue);
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
