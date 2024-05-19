main: serial.o host.c
	gcc host.c serial.o -lm -lX11 -g -o main
serial.o: serial.h serial.c
	gcc serial.c -c
clean:
	rm -f *.o main
