CC=gcc
CFLAGS= -Wall
DFLAGS= -g
PROGS= lfl

all: $(PROGS)

%: %.c
	$(CC) $(CFLAGS) $(DFLAGS) $< -o $@

clean:
	rm $(PROGS)
