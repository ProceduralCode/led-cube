main: serial.o hsv.o sierpinski.c ../common/defines.h ../common/typedefs.h ../common/packet.h
	gcc sierpinski.c serial.o hsv.o -lm -lX11 -g -o main
serial.o: ../common/serial.h ../common/serial.c
	gcc ../common/serial.c -g -c
hsv.o: ../common/hsv.h ../common/hsv.c
	gcc ../common/hsv.c -g -c
clean:
	rm -f *.o main
