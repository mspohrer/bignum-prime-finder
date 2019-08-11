// goes through every odd number from 3 to the square root
// of num_to_check. If the num_to_check is diveded evenly
// by the dividend, then 0 is returned and num_to_check 
// is not prime. 
// Otherwise the number being checked is prime and 1 is
// returned.

#include "prime-finder.h"

void 
is_prime(mpz_t num_to_check)
{
  int result = 0;
  int rem = 0;
  mpz_t dividend, up_limit, remainder;
  char *prime = 0;

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

  if(rem != 0) {
    //prime = mpz_get_str(prime, DECIMAL, num_to_check);
    //printf("%s is prime\n", prime);
    gmp_printf("%Zd is prime\n", num_to_check);
  }
}

void
no_threads(mpz_t num_to_check, mpz_t stop)
{
  while(mpz_cmp(num_to_check,stop) <= 0)
  {
    is_prime(num_to_check);
    mpz_add_ui(num_to_check, num_to_check, 2);
  }
}

void *
is_prime_wrapper(void *num)
{
  mpz_t num_to_check;
  mpz_set_str(num_to_check, num, DECIMAL);
  is_prime(num);
  mpz_clear(num_to_check);
  pthread_exit(NULL);
}

void
threads(mpz_t num_to_check, mpz_t stop)
{
  int j, k, i;
  pthread_t ptid[PTHREAD_COUNT];
  void *rval = NULL;

  i = 0;
  while(mpz_cmp(num_to_check,stop) <= 0)
  {
    for(j = i; j < PTHREAD_COUNT; ++j)
    {
      pthread_create(&ptid[k], NULL, &is_prime_wrapper, &num_to_check);
      ++i;
    }

    for(k = 0; k < PTHREAD_COUNT; ++k)
    {
      pthread_join(ptid[k], rval);
      --i;
    }
  }
}

int
main(int argc, char **argv)
{
  mpz_t start, stop, remainder, num_to_check;
  char buf[1024];
  int c, i;

  // converts the char * passed from ./boss to mpz_t 
  // for start and stop
  if (argc == 3) {
    mpz_init_set_str(start, argv[1], 10);
    //printf("value from argv[1]: %s\n", argv[1]);
    mpz_init_set_str(stop, argv[2], 10);
    //printf("value from argv[2]: %s\n", argv[2]);
  }
  else {
    //printf("reading child\n");
    read(STDIN_FILENO, &buf, sizeof(buf)-1);
    buf[strlen(buf)] = 0;
    //printf("start:%s\n", buf);
    mpz_init_set_str(start, buf, 10);
    read(STDIN_FILENO, &buf, sizeof(buf)-1);
    buf[strlen(buf)] = 0;
    //printf("end:%s\n", buf);
    mpz_init_set_str(stop, buf, 10);
  }
  mpz_init_set(num_to_check, start);
  mpz_init_set(remainder, start);

  // makes start odd to ensure only odd numbers are checked
  if(mpz_cdiv_r_ui(remainder, num_to_check, 2) == 0) 
    mpz_add_ui(num_to_check, num_to_check, 1);

  if(PTHREAD_COUNT == 0)
    no_threads(num_to_check, stop);
  else
    threads(num_to_check, stop);
  mpz_clears(start, stop, remainder, num_to_check, NULL);
}
