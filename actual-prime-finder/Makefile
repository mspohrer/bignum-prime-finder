CC=gcc
CFLAGS= -Wall
DFLAGS= -g
PROGS= boss finder

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $(DFLAGS) $< -o $@ -lgmp

clean:
	rm $(PROGS)
