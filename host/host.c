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

typedef struct _PIXEL_t
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} PIXEL_t;

typedef struct _CommandDrawPanel_t{
  uint8_t panelId;
  uint8_t bufferId;
  uint8_t reserved[2];
  uint8_t pixelMap[64 * 64 * 3];
} CommandDrawPanel_t;

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
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

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

void setBlocking(int fd, int should_block)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(fd, &tty) != 0)
    {
        fprintf(stderr, "error %s from tggetattr", strerror(errno));
        return;
    }

    tty.c_cc[VMIN]  = should_block ? 1 : 0;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if(tcsetattr(fd, TCSANOW, &tty) != 0) fprintf(stderr, "error %s setting term attributes", strerror(errno));
}

void makePanelRed(PIXEL_t panel[64][64])
{
  for(int y = 0; y < 64; y++)
  {
    for(int x = 0; x < 64; x++)
    {
      panel[y][x].red = 255;
      panel[y][x].green = 0;
      panel[y][x].blue = 0;
    }
  }
}

void makePanelOrange(PIXEL_t panel[64][64])
{
  for(int y = 0; y < 64; y++)
  {
    for(int x = 0; x < 64; x++)
    {
      panel[y][x].red = 255;
      panel[y][x].green = 255;
      panel[y][x].blue = 0;
    }
  }
}

void makePanelGreen(PIXEL_t panel[64][64])
{
  for(int y = 0; y < 64; y++)
  {
    for(int x = 0; x < 64; x++)
    {
      panel[y][x].red = 0;
      panel[y][x].green = 255;
      panel[y][x].blue = 0;
    }
  }
}

void makePanelAqua(PIXEL_t panel[64][64])
{
  for(int y = 0; y < 64; y++)
  {
    for(int x = 0; x < 64; x++)
    {
      panel[y][x].red = 0;
      panel[y][x].green = 255;
      panel[y][x].blue = 255;
    }
  }
}

void makePanelBlue(PIXEL_t panel[64][64])
{
  for(int y = 0; y < 64; y++)
  {
    for(int x = 0; x < 64; x++)
    {
      panel[y][x].red = 0;
      panel[y][x].green = 0;
      panel[y][x].blue = 255;
    }
  }
}

void makePanelViolet(PIXEL_t panel[64][64])
{
  for(int y = 0; y < 64; y++)
  {
    for(int x = 0; x < 64; x++)
    {
      panel[y][x].red = 255;
      panel[y][x].green = 0;
      panel[y][x].blue = 255;
    }
  }
}

int main(int argc, char *argv[])
{
    PIXEL_t display[6][64][64];
    char recieve_buffer[1024];
    //char *portname = "/dev/ttyUSB1";
    char *portname = "/dev/ttyACM0";
    int swidth = 64 * 3;
    int sheight = 64 * 2;
    int serial_size;

    int fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0)
    {
        fprintf(stderr, "error %d opening %s: %s", errno, portname, strerror(errno));
        return 0;
    }

    setInterfaceAttribs(fd, B115200, 1);  // set speed to 115,200 bps, 8n1 (no parity)
    setBlocking(fd, 1);                    // set block on read

    // Setup test Panels
    makePanelRed(display[0]);
    makePanelOrange(display[1]);
    makePanelGreen(display[2]);
    makePanelAqua(display[3]);
    makePanelBlue(display[4]);
    makePanelViolet(display[5]);

    // Draw to computer screen
    G_init_graphics(swidth, sheight);
    for(int n = 0; n < 6; n++)
    {
      int x0 = (n % 3) * 64;
      int y0 = (n / 3) * 64;
      for(int y = 0; y < 64; y++)
      {
        for(int x = 0; x < 64; x++)
        {
          G_rgb(display[n][y][x].red/255, display[n][y][x].green/255, display[n][y][x].blue/255);
          G_point(x0+x, y0+y);
        }
      }
    }

    CommandDrawPanel_t packet;
    for(int i = 0; i < 6; i++)
    {
      packet.panelId = i+1;
      memcpy(packet.pixelMap, display[i], sizeof(display[i]));
      serial_size = write(fd, &packet, sizeof(packet));
      //printf("Sending packet: %d\n", serial_size);
      serial_size = write(fd, "\n", 1);
      serial_size = read(fd, recieve_buffer, sizeof(recieve_buffer));
      printf(recieve_buffer);
      sleep(1);
    }
    write(fd, "d\n", 2);
    read(fd, recieve_buffer, sizeof(recieve_buffer));
    printf(recieve_buffer);

    G_wait_key();

    return 0;
}
