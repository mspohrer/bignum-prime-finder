// Tacy Bechtel and Matthew Spohrer
// Prime Finder
//
// This program takes in a very large number and finds the
// prime numbers existing below that number
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
#define CHILD_COUNT 5
#define DECIMAL 10
#define MAX_LINE_IN sysconf(_SC_LINE_MAX)

void
Fgets(char *buf, int size, FILE *where_from)
{
  if(!fgets(buf, size, where_from))
  {
    perror("Fgets()");
    exit(EXIT_FAILURE);
  }
}

static int
Fork()
{
  pid_t pid;

  if ((pid = fork()) < 0)
    error(EXIT_FAILURE, errno, "fork error");
  return(pid);
}

void
get_num(mpz_t number)
{
  char buf[MAX_LINE_IN];
  int exponent;

  printf("Enter the number 'x' for 2^x where 2^x will be the max number"
      "I will check if prime: ");

  Fgets(buf, MAX_LINE_IN, stdin);
  exponent = atoi(buf);
  
  // initializes and sets the <number>
  // sets <number> equal to 2^x where x is equal to the number
  // entered by the user
  mpz_init_set_ui(number, 1);
  mpz_mul_2exp(number, number, exponent);
}

void
call_child(mpz_t start, mpz_t stop)
{
  pid_t pid;
  char *beg = 0;
  char *end = 0;

  beg = mpz_get_str(beg, DECIMAL, start);
  end = mpz_get_str(end, DECIMAL, stop);

  if((pid = Fork()) == 0)
  {
    if(execl("./finder", "./finder", beg, end, NULL) == -1)
    {
      perror("execl() in call_child()");
      exit(EXIT_FAILURE);
    }
  }
}


int
main()
{
  mpz_t number, start, stop, increment;
  int i;

  get_num(number);
  
  // init_set_ui initializes and sets the variable with the 
  // unsigned long passed to it
  mpz_init_set_ui(start, 0);
  mpz_init_set_ui(increment, 0);
  mpz_init_set_ui(stop, 0);

  // sets the amount to increment to pass each child
  // fdiv = floor of the div (instead of ceiling and whatnot)
  // q = quotient (instead of remainder)
  // ui = unsigned long
  mpz_fdiv_q_ui(increment, number, CHILD_COUNT);

  // sets the start value to 1 + the previous child's stop
  // value
  for(i = 0; i < CHILD_COUNT; ++i)
  {
    // mpz_add adds the last two variables and stores it in 
    // the first variable
    mpz_add_ui(stop, stop, 1);
    mpz_add(start, start, stop);
    mpz_add(stop, start, increment);
    call_child(start, stop);
  }

  for(i = 0; i < CHILD_COUNT; ++i)
    wait(NULL);

  exit(EXIT_SUCCESS);
}
  /*printf("increment = ");
  mpz_out_str(stdout, DECIMAL, increment);
  printf("\n");*/
/*
p. 30 https://gmplib.org/gmp-man-6.1.2.pdf

mpz_init (<variable>) initialize before assigning GMP variables
mpz_inits (<variable1>, <variable2>,...)

mpz_set (mpz_t rop, const mpz_t op) set value of rop from op
mpz_set_str (mpz_t rop, const char *str, int base) Set the value ofropfromstr, 
      a null-terminated C string in basebase.  White space is allowedin the string, and is simply ignored

mpz_clear (<variable>) free the memory when finished
mpz_clears (<variable1>, <variable2>,...)


mpz_t multi precision int (bigger than big int)
mpz_mul (x,x,x) squares x and stores it in x
mpz_add (<variable>, ...)
mpz_sub (<variable>, ...)
mpz_cdiv_r (mpz_t r, const mpz_t n, const mpz_t d) divide n by d, r is remainder

mpz_t n;
mpz_init (<variable);
.
.
.
mpz_clear (<variable>);*/
