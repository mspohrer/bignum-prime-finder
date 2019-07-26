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

static void
Pipe(int *fd)
{
  if(pipe(fd) < 0)
  {
    perror("Pipe()");
    exit(EXIT_FAILURE);
  }
}

void
call_child(mpz_t start, mpz_t stop)
{
  pid_t pid;
  char *beg = 0;
  char *end = 0;
  int fd[2];

  Pipe(fd);

  beg = mpz_get_str(beg, DECIMAL, start);
  end = mpz_get_str(end, DECIMAL, stop);

  if((pid = Fork()) == 0)
  {
    dup2(fd[0], STDIN_FILENO);
    close(fd[1]);
printf("fork kid\n");
    if(execl("./finder", "./finder", beg, end, NULL) == -1)
    {
      perror("execl() in call_child()");
      exit(EXIT_FAILURE);
    }
  }
printf("fork parent\n");
  dprintf(fd[1], "Got it%d\n", fd[1]); 
  fflush(NULL);
  close(fd[0]);
  close(fd[1]);
}


int
main(int argc, char *argv[])
{
  mpz_t number, start, stop, increment;
  int i,opt, options[NUM_OPTS];

  for(i = 0; i < NUM_OPTS; ++i)
    options[i] = 0;

  while((opt = getopt (argc,argv, "tc:")) != -1)
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
      default:
        printf("Usage:\n ./boss [OPTIONS]\n"
            "-t run with a time check to test speeds \n"
            "-c [INTEGER] select number of children to use\n");
        exit(EXIT_FAILURE);
    }

  
  get_num(number);

printf("fork parent\n");
  // init_set_ui initializes and sets the variable with the 
  // unsigned long passed to it
  mpz_init_set_ui(start, 1);
  mpz_init_set_ui(increment, 1);
  mpz_init_set_ui(stop, 1);

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
