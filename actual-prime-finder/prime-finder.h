#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <error.h>
#include <limits.h>
#include <gmp.h>

// the number of numbers each child is responsible to check if prime
int CHILD_COUNT = 0;
int TEST = 0;
#define DECIMAL 10
#define NUM_OPTS 2
#define MAX_LINE_IN sysconf(_SC_LINE_MAX)

