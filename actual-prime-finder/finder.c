// goes through every odd number from 3 to the square root
// of num_to_check. If the num_to_check is diveded evenly
// by the dividend, then 0 is returned and num_to_check 
// is not prime. 
// Otherwise the number being checked is prime and 1 is
// returned.

#include "prime-finder.h"


// checks if the number sent to it is a prime number. Starts at
// 3 and goes until the square root of the number being checked. 
void 
is_prime(mpz_t num_to_check)
{
  int result = 0;
  int rem = 0;
  mpz_t dividend, up_limit, remainder;

  mpz_init_set_ui(dividend, 3);
  mpz_init_set_ui(up_limit, 1);
  mpz_init_set_ui(remainder, 1);

  // mpz_root gets the root and truncates to the int
  // returns 0 if truncated
  if(mpz_root(up_limit, num_to_check, 2) == 0) 
    mpz_add_ui(up_limit, up_limit,1);
  
  // mpz_cmp compares arg1 to arg, returns positive if arg1 > arg2
  // 0 if arg1 == arg2 and negative if arg1 < arg2
  while((result = mpz_cmp(dividend, up_limit)) <= 0)
  {
    mpz_cdiv_r(remainder, num_to_check, dividend);

    // if remainder == 0, reult is fail
    if((rem = mpz_cmp_ui(remainder,0)) == 0) break;
    
    mpz_add_ui(dividend, dividend, 2);
  }
  if(rem != 0)
    gmp_printf("%Zd ", num_to_check);
}

// if no threads are requested by the user, the single, main thread
// of control cycles through the range checking if prime.
void
no_threads(mpz_t num_to_check, mpz_t stop)
{
  while(mpz_cmp(num_to_check,stop) <= 0)
  {
    is_prime(num_to_check);
    mpz_add_ui(num_to_check, num_to_check, 2);
  }
}

// wrapper that allows pthread_create to call the is_prime function
void *
is_prime_wrapper(void *arg)
{
  mpz_t start, stop, remainder;
  mpz_init_set_str(start, arg, DECIMAL);
  mpz_init(stop);
  mpz_init(remainder);
  mpz_add(stop, start, INCREMENT);

  // ensures never checking even numbers
  if(mpz_cdiv_r_ui(remainder, start, 2) == 0)
    mpz_add_ui(start,start,1);
  
  while(mpz_cmp(start,stop) <= 0)
  {
    is_prime(start);
    mpz_add_ui(start, start, 2);
  }

  mpz_clears(remainder, stop, start, NULL);
  pthread_exit(NULL);
}

// populates an array with start values as char * then passes them to
// pthread_create. The managing thread then waits via pthread_join
void
threads(mpz_t start, mpz_t stop)
{
  int k, ret;
  pthread_t ptid[PTHREAD_COUNT];
  char *starts[PTHREAD_COUNT];

  memset(starts, 0, sizeof(starts)); 

  // populates the array with string representations of the mpz 
  // These values are the starting values of the ranges the 
  // threads will test.
  for(k = 0; k < PTHREAD_COUNT; ++k)
  {
    starts[k] = mpz_get_str(starts[k], DECIMAL, start);
    mpz_add(start, start, INCREMENT);
    mpz_add_ui(start, start, 1);
  }

  // passes the start values to the threads
  for(k = 0; k < PTHREAD_COUNT; ++k)
  {
    ret = pthread_create(&ptid[k], NULL, &is_prime_wrapper, (void *) starts[k]);
    if(ret != 0){
      perror("threads()");
      exit(EXIT_FAILURE);
    }
  }

  // wait for threads
  for(k = 0; k < PTHREAD_COUNT; ++k){
    ret = pthread_join(ptid[k], NULL);
    if(ret != 0){
      perror("threads()");
      exit(EXIT_FAILURE);
    }
  }
  for(k = PTHREAD_COUNT - 1; k >= 0; --k)
    if(starts[k]) free(starts[k]);
}

int
main(int argc, char **argv)
{
  mpz_t start, stop, remainder, num_to_check, diff;
  int pipe_size = fcntl(STDIN_FILENO, F_GETPIPE_SZ);
  char buf[pipe_size];
  FILE *filein;

  // converts the char * passed from ./boss to mpz_t 
  // for start and stop

  if (argc == 4) {
    mpz_init_set_str(start, argv[1], 10);
    //printf("value from argv[1]: %s\n", argv[1]);
    mpz_init_set_str(stop, argv[2], 10);
    //printf("value from argv[2]: %s\n", argv[2]);
    PTHREAD_COUNT = atoi(argv[3]);
  }
  else {
    filein = fdopen(STDIN_FILENO, "r");
    fgets(buf, MAX_LINE_IN, filein);

    if (buf[strlen(buf)-1] == '\n')
      buf[strlen(buf) - 1] = 0;
    //printf("start:%s\n", buf);
    mpz_init_set_str(start, buf, 10);
    fgets(buf, MAX_LINE_IN, filein);
    if (buf[strlen(buf)-1] == '\n')
      buf[strlen(buf) - 1] = 0;
    //printf("end:%s\n", buf);
    mpz_init_set_str(stop, buf, 10);
    PTHREAD_COUNT = atoi(argv[1]);
  }
  mpz_init(remainder);
  mpz_init(diff);
  mpz_init(INCREMENT);
  mpz_init_set(num_to_check, start);
  mpz_cdiv_r_ui(remainder, num_to_check, 2);

  // makes start odd to ensure only odd numbers are checked
  if(mpz_cmp_ui(remainder, 0) == 0) 
    mpz_add_ui(num_to_check, num_to_check, 1);

  if(PTHREAD_COUNT == 0)
    no_threads(num_to_check, stop);
  else {
    mpz_sub(diff, stop, start);
    mpz_cdiv_q_ui(INCREMENT, diff, PTHREAD_COUNT);
    threads(num_to_check, stop);
  }

  mpz_clears(start, stop, remainder, num_to_check, diff, INCREMENT, NULL);
}
