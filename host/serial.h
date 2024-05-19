#ifndef __SERIAL_H
#define __SERIAL_H

void serialInit(void);
void serialWrite(const int id, const char buffer[], const int size);
void serialRead(const int id);

#endif
