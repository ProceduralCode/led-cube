main: serial.o koch.c ../common/defines.h ../common/typedefs.h ../common/packet.h
	gcc koch.c serial.o -lm -lX11 -g -o main
serial.o: ../common/serial.h ../common/serial.c
	gcc ../common/serial.c -g -c
clean:
	rm -f *.o main
