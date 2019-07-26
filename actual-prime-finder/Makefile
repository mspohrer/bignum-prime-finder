CC=gcc
CFLAGS= -Wall
DFLAGS= -g -pthread
PROGS= boss finder

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $(DFLAGS) $< -o $@ -lgmp

clean:
	rm $(PROGS)
