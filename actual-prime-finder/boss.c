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

int
get_num(mpz_t number)
{
  char buf[MAX_LINE_IN];
  int exponent;

  printf("This is just for now to experiment with huge numbers.\n"
      "I got tired of entering 30 digit numbers by hand. \nEnter the"
      "number 'x' for 2^x where 2^x will be the max number\n"
      "I will check if prime: ");

  Fgets(buf, MAX_LINE_IN, stdin);
  exponent = atoi(buf);
  
  // initializes and sets the <number>
  // sets <number> equal to 2^x where x is equal to the number
  // entered by the user
  mpz_init_set_ui(number, 1);
  mpz_mul_2exp(number, number, exponent);
  return exponent;
}

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

  // makes start odd to ensure only odd numbers are checked
  if(mpz_cdiv_r_ui(remainder, num_to_check, 2) == 0) 
    mpz_add_ui(num_to_check, num_to_check, 1);

  if(PTHREAD_COUNT == 0)
    no_threads(num_to_check, stop);
  else
    threads(num_to_check, stop);
  mpz_clears(start, stop, remainder, num_to_check, NULL);
}

void
io_daemonize(int fds[CHILD_COUNT][2])
{
  int i, pid;
  int fd0, fd1, fd2, file_out;
  struct rlimit rl;
  mode_t mode;

  for(i = 0; i < CHILD_COUNT; i++) {
    close(fds[i][1]);
  }

  //don't want to exit the program to create the daemon,
  //so fork once before starting daemon work
  if ((pid = Fork()) == 0) {

    umask(0);

    if (getrlimit(RLIMIT_NOFILE, &rl) < 0) {
      perror("io_daemonize()");
      exit(EXIT_FAILURE);
    }

    if ((pid = Fork()) != 0) {
      exit(0);
    }

    setsid();

    if ((pid = Fork()) != 0) {
      exit(0);
    }

    if (rl.rlim_max == RLIM_INFINITY)
      rl.rlim_max = 1024;
    for (i = 0; i < rl.rlim_max; i++) {
      close(i);
    }
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup(0);
    fd2 = dup(0);

    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    file_out = open("./prime-log.log", O_WRONLY | O_CREAT | O_TRUNC, mode);

    while (1) { pause(); }
  }
}

void
Pipe(int *fd)
{
  if(pipe(fd) < 0){
    perror("Pipe()");
    exit(EXIT_FAILURE);
  }
}

void
pipes(mpz_t start, mpz_t stop, mpz_t increment)
{
  int i, j, fds[CHILD_COUNT][2];
  pid_t pid, pids[CHILD_COUNT];
  char *beg = 0;
  char *end = 0;

  for (i = 0; i < CHILD_COUNT; i++) {
    Pipe(fds[i]);
    //fds[i][0] = 0;
    //fds[i][1] = 0;
  }
  if (CHILD_COUNT == 0) {
    return;
  }

  if (IODAEMON == 1) {
    io_daemonize(fds);
  }

  for(i = 0; i < CHILD_COUNT; i++) {
    if ((pid = Fork()) == 0) {
  //printf("%d\n", CHILD_COUNT);
      dup2(fds[i][0], STDIN_FILENO);
      //dup2(fds[i][1], STDOUT_FILENO);
      for (j = 0;  j < CHILD_COUNT; j++) {
        if (fds[j][0] != 0) {
          close(fds[j][0]);
          close(fds[j][1]);
        }
      }
      if (execl("./finder", "./finder", NULL) < 0)
      {
        perror("execl() in call_child()");
        exit(EXIT_FAILURE);
      }
    } else {
      //printf("Parent!\n");
      pids[i] = pid;
      close(fds[i][0]);
    }
  }
  for(i = 0; i < CHILD_COUNT; i++) {
    //TODO NON-BLOCKING I/O
    beg = mpz_get_str(beg, DECIMAL, start);
    end = mpz_get_str(end, DECIMAL, stop);
    write(fds[i][1], beg, strlen(beg) + 1);
    write(fds[i][1], "\n", 1);
    //write(STDOUT_FILENO, beg, strlen(beg) + 1);
    //write(STDOUT_FILENO, "\n", 1);
    sleep(2);
    write(fds[i][1], end, strlen(end) + 1);
    write(fds[i][1], "\n", 1);
    //write(STDOUT_FILENO, end, strlen(end) + 1);
    //write(STDOUT_FILENO, "\n", 1);
    mpz_add_ui(start, stop, 1);
    mpz_add(stop, start, increment);
    //printf("%d\n", CHILD_COUNT);
  }
  if (beg != 0)
    free(beg);

  if (end != 0)
    free(end);

  //printf("%d\n", CHILD_COUNT);
  for(i = 0; i < CHILD_COUNT; i++) {
    pid = wait(NULL);
    printf("%d exited\n", pid);
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
  int i,opt, options[NUM_OPTS], power;

  for(i = 0; i < NUM_OPTS; ++i)
    options[i] = 0;

  if(!argv[1])
    {
        printf("Usage:\n ./boss [OPTIONS]\n"
            "-t run with a time check to test speeds \n"
            "-c [INTEGER] select number of children to use\n"
            "-p [INTEGER] select number of threads to use\n"
            "-d run with an IO daemon\n");
        exit(EXIT_FAILURE);
    }

  while((opt = getopt (argc,argv, "tc:p:d")) != -1)
    switch (opt) 
    {
      case 't':
        // to run with time checks
        options[0] = 1;
        if (CHILD_COUNT == 0)
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
      case 'd':
        IODAEMON = 1;
        options[3] = 1;
        break;
      default:
        exit(EXIT_FAILURE);
    }

  
  power = get_num(number);

  init_numbers(increment, start, stop, number);
        
  // if the user want no children, the finder is called here
  // otherwise call the number of children asked for.
  if(CHILD_COUNT == 0)
    finder(start, stop);
  else /*if (power < 435409)  //max exponent for childcount 1
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
  } else //if number too big to pass directly, pipe to children */
  {
    pipes(start, stop, increment);
  }

  mpz_clears(number, start, stop, increment, NULL);
  exit(EXIT_SUCCESS);
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

  if(result == 0 && rem != 0)
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

void
threads(mpz_t num_to_check, mpz_t stop)
{
  int j, k, i;
  pthread_t ptid[PTHREAD_COUNT];
  void *rval = NULL;

  i = 0;
  while(mpz_cmp(num_to_check,stop) <= 0)
  {

    for(j = i; j < PTHREAD_COUNT; ++j){
      pthread_create(&ptid[k], NULL, &is_prime_wrapper, &num_to_check);
      ++i;
    }

    for(k = 0; k < PTHREAD_COUNT; ++k){
      pthread_join(ptid[k], rval);
      --i;
    }
  }
}
