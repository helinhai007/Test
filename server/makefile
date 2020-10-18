CC = gcc
CFLAGS = -g -O  -Wall
 
main:work.o tpool.o service.o 
	$(CC) $^ -o $@	-lpthread
%.o:%.c
	$(CC) $(CFLAGS) -c $^	-std=c99
clean:
	rm -rf main service.o work.o tpool.o