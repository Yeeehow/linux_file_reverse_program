CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGETS = reverser controller

all: $(TARGETS)

reverser: reverser.c
	$(CC) $(CFLAGS) -o reverser reverser.c

controller: controller.c
	$(CC) $(CFLAGS) -o controller controller.c

clean:
	rm -f $(TARGETS)