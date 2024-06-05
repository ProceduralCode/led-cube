#ifndef __SERIAL_H
#define __SERIAL_H

#include <stdint.h>

int serialInit(void);
void serialDeinit(void);
void serialWrite(const int id, const char buffer[], const int size);
void serialRead(const int id, const int print);
uint64_t serialGetID(const int id);
void serialGetPixel(const int id, const int x, const int y);

#endif
