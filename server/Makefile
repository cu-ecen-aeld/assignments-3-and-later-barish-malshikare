CROSS_COMPILE?=
CC = $(CROSS_COMPILE)gcc

all: aesdsocket

aesdsocket:aesdsocket.o
	@echo "compiler is $(CC)"
	$(CC) -o  aesdsocket aesdsocket.o

aesdsocket.o:aesdsocket.c
	$(CC) -c aesdsocket.c
clean:
	rm -f aesdsocket.o aesdsocket

debug:
	@echo "Using compiler :$(CC)"

	
