CC = gcc
CFLAGS = -g -O  -Wall
 
main:work.o tpool.o service.o 
	$(CC) $^ -o $@
%.o:%.c
	$(CC) $(CFLAGS) -c $^
clean:
	rm -rf main service.o work.o tpool.o