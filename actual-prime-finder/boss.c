// Tacy Bechtel and Matthew Spohrer
// Prime Finder
//
// This program takes in a very large number and finds the
// prime numbers existing below that number
#include "prime-finder.h"

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

// sets the amount to increment to pass each child, the start
// and stopping numbers.
// init_set_ui initializes and sets the variable with the 
// unsigned long passed to it
// fdiv = floor of the div (instead of ceiling and whatnot)
// q = quotient (instead of remainder)
// ui = unsigned long
void
init_numbers(mpz_t increment, mpz_t start, mpz_t stop, mpz_t number)
{
  mpz_init_set_ui(increment, 1);
  if(CHILD_COUNT > 1) 
    mpz_cdiv_q_ui(increment, number, CHILD_COUNT);
  mpz_init_set_ui(start, 1);
  if(CHILD_COUNT < 2) 
    mpz_init_set(stop, number);
  else 
  {
    mpz_init(stop);
    mpz_add(stop, start, increment);
  }
}

void
get_num(mpz_t number)
{
  char buf[MAX_LINE_IN];
  int exponent;

  printf("This is just for now to experiment with huge numbers."
      "I got tired of entering 30 digit numbers by hand. Enter the"
      "number 'x' for 2^x where 2^x will be the max number"
      "I will check if prime: ");

  Fgets(buf, MAX_LINE_IN, stdin);
  exponent = atoi(buf);
  
  // initializes and sets the <number>
  // sets <number> equal to 2^x where x is equal to the number
  // entered by the user
  mpz_init_set_ui(number, 1);
  mpz_mul_2exp(number, number, exponent);
}

/*static void
Pipe(int *fd)
{
  if(pipe(fd) < 0)
  {
    perror("Pipe()");
    exit(EXIT_FAILURE);
  }
}*/

// I kept this exactly the same as with having children 
// to kee the overheads resulting from the algorithm as
// close to equal as possible.
void
finder(mpz_t begin, mpz_t end)
{
  mpz_t start, stop, remainder, num_to_check;

  // converts the char * passed from ./boss to mpz_t 
  // for start and stop
  mpz_init_set(start, begin);
  mpz_init_set(stop, end);
  mpz_init_set(num_to_check, start);
  mpz_init_set(remainder, start);
  mpz_mod_ui(remainder, num_to_check, 2);

  // makes start odd to ensure only odd numbers are checked
  if(mpz_cmp_ui(remainder, 0) == 0) 
    mpz_add_ui(num_to_check, num_to_check, 1);

  if(PTHREAD_COUNT == 0)
    no_threads(num_to_check, stop);
  else
    threads(num_to_check, stop);
  mpz_clears(start, stop, remainder, num_to_check, NULL);
}

// calls the child process.
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
main(int argc, char *argv[])
{
  mpz_t number, start, stop, increment;
  int i,opt;

  if(!argv[1])
    {
        printf("Usage:\n ./boss [OPTIONS]\n"
            "-t run with a time check to test speeds \n"
            "-c [INTEGER] select number of children to use\n"
            "-p [INTEGER] select number of threads to use\n");
        exit(EXIT_FAILURE);
    }

  while((opt = getopt (argc,argv, "tc:p:")) != -1)
    switch (opt) 
    {
      case 't':
        // to run with time checks
        CHILD_COUNT = 1;
        break;
      case 'c':
        CHILD_COUNT = atoi(optarg);
        break;
      case 'p':
        PTHREAD_COUNT = atoi(optarg);
        break;
      default:
        exit(EXIT_FAILURE);
    }

  
  get_num(number);

  init_numbers(increment, start, stop, number);
        
  // if the user want no children, the finder is called here
  // otherwise call the number of children asked for.
  if(CHILD_COUNT == 0)
    finder(start, stop);
  else 
  {
    for(i = 0; i < CHILD_COUNT; ++i)
    {
      call_child(start, stop);
      if(CHILD_COUNT == 1) break;
      mpz_add_ui(start, stop, 1);
      mpz_add(stop, start, increment);
      if(mpz_cmp(stop,number) > 0)
        mpz_set(stop, number);
    }
    for(i = 0; i < CHILD_COUNT; ++i)
      wait(NULL);
  }

  mpz_clears(number, start, stop, increment, NULL);
  exit(EXIT_SUCCESS);
}

void *
is_prime_wrapper(void *num)
{
  mpz_t num_to_check;
  mpz_init_set_str(num_to_check, num, DECIMAL);
  is_prime(num_to_check);
  mpz_clear(num_to_check);
  pthread_exit(NULL);
}

// oof, this is kind of wonky. When passing iterated numbers through 
// pthread_create, the pointer sometimes is pointing to the same data
// as the previously created thread. The setup with the num[] prevents 
// prevents that from happening. Concurrency is a pain!
void
threads(mpz_t num_to_check, mpz_t stop)
{
  int j, k, i, ret;
  pthread_t ptid[PTHREAD_COUNT];
  void *rval = NULL;
  char *num[PTHREAD_COUNT];
  
  for(k = 0; k < PTHREAD_COUNT; ++k)
    num[k] = 0;

  k = 0;
  i = 0;
  while(mpz_cmp(num_to_check,stop) <= 0)
  {

    for(j = i; j < PTHREAD_COUNT; ++j){
      if(num[k]) num[k] = 0;
      num[k] = mpz_get_str(num[k], DECIMAL, num_to_check);
      ret = pthread_create(&ptid[j], NULL, &is_prime_wrapper,(void*)num[k]);
      if(ret != 0){
        perror("threads()");
        exit(EXIT_FAILURE);
      }
      mpz_add_ui(num_to_check, num_to_check, 2);
      ++i;
      ++k;
      if(k >= PTHREAD_COUNT) k = 0;
    }

    for(k = 0; k < PTHREAD_COUNT; ++k){
      ret = pthread_join(ptid[k], rval);
      if(ret != 0){
        perror("threads()");
        exit(EXIT_FAILURE);
      }
      --i;
    }
  }
}

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
    gmp_printf("%Zd is prime\n", num_to_check);

  mpz_clears(dividend, up_limit, remainder, NULL);
  if(PTHREAD_COUNT > 0) pthread_exit(NULL);
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
