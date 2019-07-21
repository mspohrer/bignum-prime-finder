CC=gcc
CFLAGS= -Wall
DFLAGS= -g -lgmp
PROGS= prime-finder

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $(DFLAGS) $< -o $@

clean:
	rm $(PROGS)
