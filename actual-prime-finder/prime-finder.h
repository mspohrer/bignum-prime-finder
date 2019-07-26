#include <errno.h>
#include <error.h>
#include <gmp.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// the number of numbers each child is responsible to check if prime
int CHILD_COUNT = 0;
int PTHREAD_COUNT = 0;
int TEST = 0;
#define DECIMAL 10
#define NUM_OPTS 3
#define MAX_LINE_IN sysconf(_SC_LINE_MAX)

void Pthread_create(pthread_t*,pthread_attr_t *, void *(*start_rtn)(void *), void *restrict arg);
void is_prime(mpz_t num_to_check);
void no_threads(mpz_t num_to_check, mpz_t stop);
void threads(mpz_t num_to_check, mpz_t stop);

