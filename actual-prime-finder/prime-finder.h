#include <errno.h>
#include <error.h>
#include <gmp.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>

// the number of numbers each child is responsible to check if prime
int CHILD_COUNT = 0;
int PTHREAD_COUNT = 0;
int TEST = 0;
int IODAEMON = 0;
int PIPES = 0;
#define DECIMAL 10
#define NUM_OPTS 4
#define MAX_LINE_IN sysconf(_SC_LINE_MAX)
#define EPOLL_MAX 10

struct node {
  mpz_t num;
  struct node *next;
  int checked;
  pthread_mutex_t lock;
};


void is_prime(mpz_t num_to_check);
void no_threads(mpz_t num_to_check, mpz_t stop);
void threads(mpz_t num_to_check, mpz_t stop);

