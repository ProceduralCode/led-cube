#ifndef __SERIAL_H
#define __SERIAL_H

int serialInit(void);
void serialDeinit(void);
void serialWrite(const int id, const char buffer[], const int size);
void serialRead(const int id, const int print);
uint64_t serialGetID(const int id);

#endif
