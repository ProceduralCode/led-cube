/******************************************************************************
 * ttyACM0
 *****************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include "FPToolkit.c"
#include "defines.h"
#include "packet.h"

typedef struct _PIXEL_t
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} PIXEL_t;

int setInterfaceAttribs (int fd, int speed, int parity)
{
    struct termios tty;
    if (tcgetattr (fd, &tty) != 0)
    {
        fprintf(stderr, "error %s from tcgetattr", strerror(errno));
        return -1;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    // disable IGNBRK for mismatched speed tests; otherwise receive break
    // as \000 chars
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 1;            // read() waits until at least a character is read
    tty.c_cc[VTIME] = 0;            // no timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0)
    {
        fprintf(stderr, "error %d from tcsetattr", errno);
        return -1;
    }

    return 0;
}

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
    char recieve_buffer[1024];
    //char *portname = "/dev/ttyUSB1";
    char *portname = "/dev/ttyACM0";
    int swidth = PANEL_SIZE * PANELS_HORIZONTAL;
    int sheight = PANEL_SIZE * PANELS_VERTICAL;
    int serial_size;

    // Open the connection to the USB device (teensy)
    // The file descriptor, fd will be very important to identify
    // this unique serial port from any other we may open.
    // fd is likely to be 3 since 0 = stdin, 1 = stdout, and 2 = stderr
    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        fprintf(stderr, "error %d opening %s: %s", errno, portname, strerror(errno));
        return 0;
    }

    setInterfaceAttribs(fd, B115200, 1);  // set speed to 115,200 bps, 8n1 (no parity)

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
      serial_size = write(fd, &packet, sizeof(packet));
      serial_size = write(fd, "\n", 1);
      serial_size = read(fd, recieve_buffer, sizeof(recieve_buffer));
      printf(recieve_buffer);
    }
    // Send command to update the panels
    write(fd, "d\n", 2);
    read(fd, recieve_buffer, sizeof(recieve_buffer));
    printf(recieve_buffer);

    G_wait_key();

    write(fd, "c\n", 2);
    read(fd, recieve_buffer, sizeof(recieve_buffer));
    printf(recieve_buffer);

    return 0;
}
