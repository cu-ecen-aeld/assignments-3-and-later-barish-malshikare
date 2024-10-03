CROSS_COMPILE?=
CC = $(CROSS_COMPILE)gcc
writer:writer.o
	@echo "compiler is $(CC)"
	$(CC) -o  writer writer.o

writer.o:writer.c
	$(CC) -c writer.c
clean:
	rm writer.o writer

debug:
	@echo "Using compiler :$(CC)"

	
