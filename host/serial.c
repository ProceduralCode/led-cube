#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include "serial.h"

int serialFilter(const struct dirent *entry);
int setInterfaceAttribs(const int fd, const int speed, const int parity);

// I don't expect there to be more than 4 teensy devices
static int fd[4];
static int total_fd;
static fd_set g_writefds, g_readfds;

// Buffer to make serialRead self contained
static char recieve_buffer[4096];

// Constants to find the USB serial ports
static char basedir[] = "/dev/";
static char portname[] = "ttyACM"; //"ttyUSB1";

/******************************************************************************
 * serialInit() will eit the program if it is unable to open any USB serial
 * ports
 *****************************************************************************/
int serialInit()
{
    char port[32] = {0};
    struct dirent **serial_devices = NULL;
    int numUSB = scandir(basedir, &serial_devices, serialFilter, alphasort);

    if(numUSB > -1)
    {
        printf("Found %d ports:", numUSB);
        for(int i = 0; i < numUSB; i++)
        {
            printf(" %s ", serial_devices[i]->d_name);
        }
        printf("\n");
    }
    else
    {
        fprintf(stderr, "error %d opening %s: %s\n", errno, portname, strerror(errno));
        exit(0);
    }

    total_fd = numUSB;
    FD_ZERO(&g_writefds);
    FD_ZERO(&g_readfds);

    for(int i = 0; i < total_fd; i++)
    {
        strcat(port, basedir);
        strcat(port, serial_devices[i]->d_name);
        // Open the connection to the USB device (teensy)
        // The file descriptor, fd will be very important to identify
        // this unique serial port from any other we may open.
        // fd is likely to be 3 since 0 = stdin, 1 = stdout, and 2 = stderr
        fd[i] = open(port, O_RDWR | O_NOCTTY | O_SYNC);
        if (fd[i] < 0)
        {
            fprintf(stderr, "error %d opening %s: %s\n", errno, port, strerror(errno));
            exit(0);
        }

        printf("Setting fd%d:%s\n", fd[i], port);
        setInterfaceAttribs(fd[i], B115200, 1);  // set speed to 115,200 bps, 8n1 (no parity)
        memset(port, '\0', strlen(port));
    }

    for(int i = 0; i < total_fd-1; i++)
    {
        uint64_t id_1 = serialGetID(i);
        uint64_t id_2 = serialGetID(i+1);
        if(id_1 > id_2)
        {
            int temp = fd[i];
            fd[i] = fd[i+1];
            fd[i+1] = temp;
        }
    }

    for(int i = 0; i < numUSB; i++)
    {
        free(serial_devices[i]);
        //uint64_t id_1 = serialGetID(i);
        //printf("fd%d: 0x%llX\n", fd[i], id_1);
    }
    free(serial_devices);

    return total_fd;
}

void serialDeinit()
{
    for(int i = 0; i < total_fd-1; i++)
    {
        close(fd[i]);
    }
}

int serialFilter(const struct dirent *entry)
{
    return (strncmp(entry->d_name, portname, sizeof(portname)-1) == 0);
}

int setInterfaceAttribs(const int fd, const int speed, const int parity)
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

void serialWrite(const int id, const char buffer[], const int size)
{
    int count;
    int fds;
    int is_ready;

    // Let's use a select call to enxure that the serial port is ready
    do {
        FD_SET(fd[id], &g_writefds);
        fds = select(fd[id]+1, NULL, &g_writefds, NULL, NULL);
        if(fds == -1) fprintf(stderr, "error %d selecting device%d: %s\n", errno, id, strerror(errno));
        is_ready = FD_ISSET(fd[id], &g_writefds);
    } while(!is_ready);
    count = write(fd[id], buffer, size);
    if(count == -1)
    {
        fprintf(stderr, "error %d writing to port[%d]: %s\n", errno, id, strerror(errno));
        exit(0);
    }
    // Let's use a select call to enxure that the serial port is ready
    do {
        FD_SET(fd[id], &g_writefds);
        fds = select(fd[id]+1, NULL, &g_writefds, NULL, NULL);
        if(fds == -1) fprintf(stderr, "error %d selecting device%d: %s\n", errno, id, strerror(errno));
        is_ready = FD_ISSET(fd[id], &g_writefds);
    } while(!is_ready);
    count = write(fd[id], "\n", 1);
    if(count == -1)
    {
        fprintf(stderr, "error %d writing to port%d: %s\n", errno, id, strerror(errno));
        exit(0);
    }
}

void serialRead(const int id, const int print)
{
    int count;
    int fds;
    int is_ready;

    // Let's use a select call to enxure that the serial port is ready
    do {
        FD_SET(fd[id], &g_readfds);
        fds = select(fd[id]+1, &g_readfds, NULL, NULL, NULL);
        if(fds == -1) fprintf(stderr, "error %d selecting device%d: %s\n", errno, id, strerror(errno));
        is_ready = FD_ISSET(fd[id], &g_readfds);
    } while(!is_ready);
    count = read(fd[id], recieve_buffer, sizeof(recieve_buffer));
    if(count > -1)
    {
        int i = 0;
        while(recieve_buffer[i] != '\n' && i < count)
        {
            if(print) printf("%c", recieve_buffer[i]);
            i++;
        }
        if(print) printf("\n");
    }
    else
    {
        fprintf(stderr, "error %d reading port[%d]: %s\n", errno, id, strerror(errno));
        exit(0);
    }
    //fsync(fd[id]);
    fflush(stdout);
}

uint64_t serialGetID(const int id)
{
    int count;
    int fds;
    int is_ready;
    uint64_t device_id;

    do {
        FD_SET(fd[id], &g_writefds);
        fds = select(fd[id]+1, NULL, &g_writefds, NULL, NULL);
        if(fds == -1) fprintf(stderr, "error %d selecting device%d: %s\n", errno, id, strerror(errno));
        is_ready = FD_ISSET(fd[id], &g_writefds);
    } while(!is_ready);
    count = write(fd[id], "i\n", 2);
    do {
        FD_SET(fd[id], &g_readfds);
        fds = select(fd[id]+1, &g_readfds, NULL, NULL, NULL);
        if(fds == -1) fprintf(stderr, "error %d selecting device%d: %s\n", errno, id, strerror(errno));
        is_ready = FD_ISSET(fd[id], &g_readfds);
    } while(!is_ready);
    count = read(fd[id], recieve_buffer, sizeof(recieve_buffer));
    device_id = *((uint64_t*)recieve_buffer);

    return device_id;
}
