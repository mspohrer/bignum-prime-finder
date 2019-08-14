// Tacy Bechtel and Matthew Spohrer
// Prime Finder
//
// This program takes in a very large number and finds the
// prime numbers existing below that number
#include "prime-finder.h"

void Pipe(int *fd);
int Polling(int fds[CHILD_COUNT + 2][2]);

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
  mpz_t range;
  mpz_init_set_ui(increment, 1);
  if(CHILD_COUNT > 1) {
    mpz_init(range);
    mpz_sub(range, number, start);
    mpz_fdiv_q_ui(increment, range, CHILD_COUNT);
  }
  //mpz_init_set_ui(start, 1);

  if(CHILD_COUNT < 2) 
    mpz_init_set(stop, number);
  else 
  {
    mpz_init(stop);
    mpz_add(stop, start, increment);
  }
}

int
get_num(mpz_t min_exp, mpz_t max_exp)
{
  char buf[MAX_LINE_IN];
  int start, exponent;
  
  buf[0] = 0;
  while (buf[0] > '9' || buf[0] < '0') {
    printf("Enter the number 'x' for 2^x where 2^x will be the minimum\n"
          "value checked: ");
    Fgets(buf, MAX_LINE_IN, stdin);
  }
  start = atoi(buf);
  // initializes and sets the <number>
  // sets <number> equal to 2^x where x is equal to the number
  // entered by the user
  mpz_init_set_ui(min_exp, 1);
  mpz_mul_2exp(min_exp, min_exp, start);
  
  buf[0] = 0;
  while (buf[0] > '9' || buf[0] < '0') {
    printf("Enter the number 'x' for 2^x where 2^x will be the maximum\n"
          "value checked: ");
    Fgets(buf, MAX_LINE_IN, stdin);
  }
  exponent = atoi(buf);

  if (start >= exponent) {
    perror("Max must be greater than min");
    exit(EXIT_FAILURE);
  }
  
  // initializes and sets the <number>
  // sets <number> equal to 2^x where x is equal to the number
  // entered by the user

  mpz_init_set_ui(max_exp, 1);
  mpz_mul_2exp(max_exp, max_exp, exponent);

  return exponent;
}

// I kept this exactly the same as with having children 
// to kee the overheads resulting from the algorithm as
// close to equal as possible.
void
finder(mpz_t begin, mpz_t end)
{
  mpz_t start, stop, remainder, num_to_check;
  int fds[3][2], i, pid, stdout_reopen;
  CHILD_COUNT = 1;
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
  
  //create pipes for polling.
  for (i = 0; i < 3; i++) {
    Pipe(fds[i]);
  }
  //polling must happen in a child process
  if ((pid = Fork()) == 0) {
    Polling(fds);
    exit(EXIT_SUCCESS);
  }
  else
  {

    //hold stdout, as we will reinstate it after finding the primes
    stdout_reopen = dup(STDOUT_FILENO);

    //connect to pipe for polling file i/o
    dup2(fds[0][1], STDOUT_FILENO);

    if(PTHREAD_COUNT == 0){
      no_threads(num_to_check, stop);
    }
    else
      threads(num_to_check, stop);

    write(fds[CHILD_COUNT][1], "done", 4);
    wait(NULL);
  }
  dup2(stdout_reopen, STDOUT_FILENO);
  mpz_clears(start, stop, remainder, num_to_check, NULL);
}

// File i/o through epolling
int
Polling(int fds[CHILD_COUNT + 2][2])
{
  int i, j, poll_fd, file_out, ready_count, all_done = 0;
  int read_len;
  mode_t mode;
  char buf[MAX_LINE_IN]; //, buf2[MAX_LINE_IN];
  struct epoll_event ev, events[EPOLL_MAX];

  //open log file
  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  file_out = open("./prime-log.log", O_WRONLY | O_CREAT | O_TRUNC, mode);

  //set up epoll
  poll_fd = epoll_create(1);
  for (i = 0;  i < CHILD_COUNT + 1; i++) {
    ev.events = EPOLLIN;
    ev.data.fd = fds[i][0];
    if (epoll_ctl(poll_fd, EPOLL_CTL_ADD, fds[i][0], &ev) == -1) {
      perror("epoll_ctl()");
      exit(EXIT_FAILURE);
    }
  }
  //if using the daemon, tell the original parent that the daemon is ready
  // to begin file io
  if (IODAEMON == 1)
    write(fds[CHILD_COUNT + 1][1], "ready", strlen("ready") + 1);

  //now we poll!
  for (;;) {
    ready_count = epoll_wait(poll_fd, events, EPOLL_MAX, 0);
    if (ready_count == -1) {
      perror("epoll_wait()");
      exit(EXIT_FAILURE);
    }
    // exit condition - all children exited
    if (ready_count == 0 && all_done == 1)
      return(0);
    for (j = 0; j < ready_count; j++) {
      //if reading from parent process, all children have exited
      if (events[j].data.fd == fds[CHILD_COUNT][0]) {
        all_done = 1;
        read_len = read(events[j].data.fd, buf, MAX_LINE_IN);
      } else {
        read_len = read(events[j].data.fd, buf, MAX_LINE_IN);
        write(file_out, buf, read_len);
      }
    }
  }
}

//create io daemon
void
io_daemonize(int fds[CHILD_COUNT + 1][2])
{
  int pid;
  int fd0, fd1, fd2; 

  //don't want to exit the program to create the daemon,
  //so fork once before starting daemon work
  if ((pid = Fork()) == 0) {

    umask(0);

    if ((pid = Fork()) != 0) {
      exit(0);
    }

    setsid();

    if ((pid = Fork()) != 0) {
      exit(0);
    }
    
    //close stdio files, open to dev/null
    close(STDIN_FILENO);
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup2(0, STDOUT_FILENO);
    fd2 = dup2(0, STDERR_FILENO);

    // don't want to see "unused variable message, so check that
    // they opened correctly
    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
      exit(EXIT_FAILURE);
    }
    
    //call polling function.
    Polling(fds);
    exit(EXIT_SUCCESS);
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

// pipe values to children
void
pipes(mpz_t start, mpz_t stop, mpz_t increment, mpz_t max_exp)
{
  int i, j, fds[CHILD_COUNT][2], ios[CHILD_COUNT + 1][2];
  pid_t pid; //, pids[CHILD_COUNT];
  //int mpzsize = mpz_sizeinbase(10
  char *beg = 0;
  char *end = 0;
  char *buf[100];
  char pthread[10];
  sprintf(pthread, "%d", PTHREAD_COUNT);

  for (i = 0; i < CHILD_COUNT; i++) {
    Pipe(fds[i]);
    Pipe(ios[i]);
  }
  Pipe(ios[CHILD_COUNT]);
  if (CHILD_COUNT == 0) {
    return;
  }

  if (IODAEMON == 1) {
    // need one more pipe for daemon
    Pipe(ios[CHILD_COUNT + 1]);
    //start daemon
    io_daemonize(ios);
    // when read in happens, daemon is ready
    read(ios[CHILD_COUNT + 1][0], buf, 100);
  }

  for(i = 0; i < CHILD_COUNT; i++) {
    if ((pid = Fork()) == 0) {
      //pipe io to correct places
      dup2(fds[i][0], STDIN_FILENO);
      dup2(ios[i][1], STDOUT_FILENO);
      // close unneeded pipes
      for (j = 0;  j < CHILD_COUNT; j++) {
        close(fds[j][0]);
        close(fds[j][1]);
        close(ios[j][0]);
        close(ios[j][1]);
      }
      //exec the finder program
      if (execl("./finder", "./finder", pthread, NULL) < 0)
      {
        perror("execl() in call_child()");
        exit(EXIT_FAILURE);
      }
    }
  }
  for(i = 0; i < CHILD_COUNT; i++) {
    // allocate space for mpzs in char arrays
    beg = malloc(mpz_sizeinbase(start, DECIMAL));
    end = malloc(mpz_sizeinbase(stop, DECIMAL));
    // convert mpzs to strings for piping
    beg = mpz_get_str(beg, DECIMAL, start);
    end = mpz_get_str(end, DECIMAL, stop);
    //write range to children
    write(fds[i][1], beg, strlen(beg));
    write(fds[i][1], "\n", 1);
    write(fds[i][1], end, strlen(end));
    write(fds[i][1], "\n", 1);

    //increment start and stop to next range
    mpz_add_ui(start, stop, 1);
    mpz_add(stop, start, increment);
    if(mpz_cmp(stop, max_exp) > 0)
      mpz_set(stop, max_exp);
  }

  if (beg != 0) {
    free(beg);
    beg = NULL;
  }

  if (end != NULL) {
    free(end);
    end = NULL;
  }

  if (IODAEMON == 0) {
    // polling must be done in a separate process
    if ((pid = Fork()) == 0) {
      Polling(ios);
      exit(EXIT_SUCCESS);
    } else {
      for(i = 0; i < CHILD_COUNT; i++) {
        pid = wait(NULL);
      }
      //polling child knows to shut down
      write(ios[CHILD_COUNT][0], "done", 5);
    }
  } else {
    for(i = 0; i < CHILD_COUNT + 1; i++) {
      pid = wait(NULL);
    } 
    //polling child knows to shut down
    write(ios[CHILD_COUNT][0], "done", 5);
  }
}

// calls the child process.
void
call_child(mpz_t start, mpz_t stop, int fds[2])
{
  pid_t pid;
  char *beg = 0;
  char *end = 0;
  char pthread[10];
  sprintf(pthread, "%d", PTHREAD_COUNT);

  beg = mpz_get_str(beg, DECIMAL, start);
  end = mpz_get_str(end, DECIMAL, stop);

  if((pid = Fork()) == 0)
  {
    dup2(fds[1], STDOUT_FILENO);
    if(execl("./finder", "./finder", beg, end, pthread, NULL) == -1)
    {
      perror("execl() in call_child()");
      exit(EXIT_FAILURE);
    }
  }
  if(beg) free(beg);
  if(beg) free(end);
}

// if not piping values, use this
void
no_pipes(mpz_t start, mpz_t stop, mpz_t increment, mpz_t max_exp) {
  int i, pid;
  int fds[CHILD_COUNT + 2][2];
  mpz_t diff;

  //make sure increment is set correctly
  mpz_init(diff);
  mpz_sub(diff, max_exp, start);
  mpz_cdiv_q_ui(increment, diff, CHILD_COUNT);

  // create pipes for polling
  for(i=0; i < CHILD_COUNT + 2; i++)
  {
    Pipe(fds[i]);
  }
  
  for(i = 0; i < CHILD_COUNT; ++i)
  {
    mpz_add(stop, start, increment);
    if(mpz_cmp(stop, max_exp) > 0)
      mpz_set(stop, max_exp);
    call_child(start, stop, fds[i]);
    mpz_add_ui(start, stop, 1);
  }

  // polling in separate function
  if ((pid = Fork()) == 0 ) {
    Polling(fds);
    exit(EXIT_SUCCESS);
  } else {
    for(i = 0; i < CHILD_COUNT; ++i)
      wait(NULL);
    //tell polling to exit when it's done reading/writing
    write(fds[CHILD_COUNT][0], "done", 4);
  }
}

int
main(int argc, char *argv[])
{
  mpz_t start, stop, increment, max_exp, diff;
  struct timeval time_start, time_stop, time_diff;

  int opt;

  if(!argv[1])
    {
        printf("Usage:\n ./boss [OPTIONS]\n"
            "-c [INTEGER] select number of children to use\n"
            "-t [INTEGER] select number of threads to use\n"
            "-d run with an IO daemon\n"
            "-p pass values to child processes using pipes\n");
        exit(EXIT_FAILURE);
    }

  while((opt = getopt (argc,argv, "t:c:pd")) != -1)
    switch (opt) 
    {
      case 'c':
        CHILD_COUNT = atoi(optarg);
        break;
      case 't':
        PTHREAD_COUNT = atoi(optarg);
        break;
      case 'd':
        IODAEMON = 1;
        break;
      case 'p':
        PIPES = 1;
        break;
      default:
        exit(EXIT_FAILURE);
    }

  get_num(start, max_exp);

  init_numbers(increment, start, stop, max_exp);
  mpz_init(INCREMENT);
  mpz_init(diff);
        
  gettimeofday(&time_start, NULL);
  // if the user want no children, the finder is called here
  // otherwise call the number of children asked for.
  if(CHILD_COUNT == 0) {
    if(PTHREAD_COUNT > 0){
      mpz_sub(diff, stop, start);
      mpz_cdiv_q_ui(INCREMENT, diff, PTHREAD_COUNT);
    } 
    finder(start, stop);
  } else if (PIPES == 1) {
    pipes(start, stop, increment, max_exp);
  } else {
    no_pipes(start, stop, increment, max_exp);
  }

  gettimeofday(&time_stop, NULL);

  timersub(&time_stop, &time_start, &time_diff);

  //sleep(5);
  printf("%ld Seconds; %ld Microseconds\n",time_diff.tv_sec, time_diff.tv_usec);
  mpz_clears(max_exp, start, stop, increment, INCREMENT, NULL);

  exit(EXIT_SUCCESS);
}

void *
is_prime_wrapper(void *arg)
{
  mpz_t start, stop, remainder;
  mpz_init_set_str(start, arg, DECIMAL);
  mpz_init(stop);
  mpz_init(remainder);
  mpz_add(stop, start, INCREMENT);
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

void
threads(mpz_t start, mpz_t stop)
{
  int k, ret;
  pthread_t ptid[PTHREAD_COUNT];
  char *starts[PTHREAD_COUNT];
  
  memset(starts, 0, sizeof(starts)); 

  for(k = 0; k < PTHREAD_COUNT; ++k)
  {
    starts[k] = mpz_get_str(starts[k], DECIMAL, start);
    mpz_add(start, start, INCREMENT);
    mpz_add_ui(start, start, 1);
  }

  for(k = 0; k < PTHREAD_COUNT; ++k)
  {
    ret = pthread_create(&ptid[k], NULL, &is_prime_wrapper, (void *) starts[k]);
    if(ret != 0){
      perror("threads()");
      exit(EXIT_FAILURE);
    }
  }
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

  //gmp_fprintf(stderr, "here %Zd\n", num_to_check);
  fflush(NULL);
  if(rem != 0)
    gmp_printf("%Zd ", num_to_check);
  fflush(NULL);

  mpz_clears(dividend, up_limit, remainder, NULL);
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
