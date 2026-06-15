all: loader

loader: task2.o startup.o start.o
	ld -o loader task2.o startup.o start.o -L/usr/lib32 -lc -T linking_script -dynamic-linker /lib32/ld-linux.so.2

task2.o: task2.c
	gcc -m32 -Wall -g -c task2.c -o task2.o

startup.o: startup.s
	nasm -f elf32 startup.s -o startup.o

start.o: start.s
	nasm -f elf32 start.s -o start.o

clean:
	rm -f *.o loader