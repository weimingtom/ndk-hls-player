all: hello

CC:=arm-linux-androideabi-gcc
LD:=arm-linux-androideabi-ld


hello: hello.o
	$(LD) hello.o -o hello

hello.o:
	$(CC) hello.c -o hello.o -c


