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
    mpz_fdiv_q_ui(increment, number, CHILD_COUNT);
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

/*static void
Pipe(int *fd)
{
  if(pipe(fd) < 0)
  {
    perror("Pipe()");
    exit(EXIT_FAILURE);
  }
}*/

int
is_prime(mpz_t num_to_check)
{
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
  while(mpz_cmp(dividend, up_limit) <= 0)
  {
    mpz_cdiv_r(remainder, num_to_check, dividend);

    // if remainder == 0, reult is fail
    if(mpz_cmp_ui(remainder,0) == 0) return 0;
    
    mpz_add_ui(dividend, dividend, 2);
  }

  return 1;
}
    
void
finder(mpz_t begin, mpz_t end)
{
  mpz_t start, stop, remainder, num_to_check;
  int result;

  // converts the char * passed from ./boss to mpz_t 
  // for start and stop
  // I kept this exactly the same as with having children 
  // to kee the overheads resulting from the algorithm as
  // close to equal as possible.
  mpz_init_set(start, begin);
  mpz_init_set(stop, end);
  mpz_init_set(num_to_check, start);
  mpz_init_set(remainder, start);

  if(mpz_cdiv_r_ui(remainder, num_to_check, 2) == 0) 
    mpz_add_ui(num_to_check, num_to_check, 1);

  while(mpz_cmp(num_to_check,stop) <= 0)
  {
    result = is_prime(num_to_check);

    if(result == 1)
      gmp_printf("%Zd is prime\n", num_to_check);

    mpz_add_ui(num_to_check, num_to_check, 2);
  }
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
  int i,opt, options[NUM_OPTS];

  for(i = 0; i < NUM_OPTS; ++i)
    options[i] = 0;

  while((opt = getopt (argc,argv, "tc:p:")) != -1)
    switch (opt) 
    {
      case 't':
        // to run with time checks
        options[0] = 1;
        CHILD_COUNT = 1;
        break;
      case 'c':
        CHILD_COUNT = atoi(optarg);
        options[1] = 1;
        break;
      case 'p':
        PTHREAD_COUNT = atoi(optarg);
        options[2] = 1;
        break;
      default:
        printf("Usage:\n ./boss [OPTIONS]\n"
            "-t run with a time check to test speeds \n"
            "-c [INTEGER] select number of children to use\n"
            "-p [INTEGER] select number of threads to use\n");
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
      // mpz_add adds the last two variables and stores it in 
      // the first variable
      call_child(start, stop);
      mpz_add_ui(start, stop, 1);
      mpz_add(stop, start, increment);
    }
    for(i = 0; i < CHILD_COUNT; ++i)
      wait(NULL);
  }

  exit(EXIT_SUCCESS);
}
