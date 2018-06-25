CC = gcc -g
CFLAGS = -std=c99 -O2 -Wall -Wpedantic -Werror 
CFLAGS += -DCONS_BATCH
CFLAGS += -DBACKTRACKING
CFLAGS += -DADAPTIVE
#CFLAGS = -O2 -pthread -std=c99
OBJECTS = fifo.o main.o

hack: main.c
	$(CC) -O2 -pthread -std=c99 fifo.c main.c -lpthread -lrt -o main

all: main

main: $(OBJECTS)
	$(CC) $(OBJECTS) -lrt -lpthread -o main

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

fifo.o: fifo.c
	$(CC) $(CFLAGS) -c fifo.c

clean:
	rm -f *.o main
