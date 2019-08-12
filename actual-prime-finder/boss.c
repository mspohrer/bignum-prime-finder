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
  
  printf("Enter the number 'x' for 2^x where 2^x will be the minimum\n"
         "value checked: ");
  Fgets(buf, MAX_LINE_IN, stdin);
  start = atoi(buf);
  // initializes and sets the <number>
  // sets <number> equal to 2^x where x is equal to the number
  // entered by the user
  mpz_init_set_ui(min_exp, 1);
  mpz_mul_2exp(min_exp, min_exp, start);

  printf("Enter the number 'x' for 2^x where 2^x will be the maximum\n"
         "value checked: ");
  Fgets(buf, MAX_LINE_IN, stdin);
  exponent = atoi(buf);

  if (start > exponent) {
    perror("get_num()");
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

int
Polling(int fds[CHILD_COUNT + 2][2])
{
  int i, j, poll_fd, file_out, ready_count, all_done = 0;
  int read_len;
  mode_t mode;
  char buf[MAX_LINE_IN]; //, buf2[MAX_LINE_IN];
  struct epoll_event ev, events[EPOLL_MAX];

  mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
  file_out = open("./prime-log.log", O_WRONLY | O_CREAT | O_TRUNC, mode);

  poll_fd = epoll_create(1);
  for (i = 0;  i < CHILD_COUNT + 1; i++) {
    //write(STDOUT_FILENO, &fds[i][0], sizeof(int));
    //printf("%i\n", fds[i][0]);
    //printf("%i\n", poll_fd);
    ev.events = EPOLLIN;
    ev.data.fd = fds[i][0];
    if (epoll_ctl(poll_fd, EPOLL_CTL_ADD, fds[i][0], &ev) == -1) {
      perror("epoll_ctl()");
      exit(EXIT_FAILURE);
    }
  }
  if (IODAEMON == 1)
    write(fds[CHILD_COUNT + 1][1], "ready", strlen("ready") + 1);
  for (;;) {
    ready_count = epoll_wait(poll_fd, events, EPOLL_MAX, 0);
    if (ready_count == -1) {
      perror("epoll_wait()");
      exit(EXIT_FAILURE);
    }
    if (ready_count == 0 && all_done == 1)
      return(0);
    //Fgets(buf, MAX_LINE_IN, fdopen(ready_fd, "r"));
    for (j = 0; j < ready_count; j++) {
      //printf("%i\n", events[j].data.fd);
      //memset(buf, 0, MAX_LINE_IN);
      if (events[j].data.fd == CHILD_COUNT + 1) {
        all_done = 1;
        read_len = read(events[j].data.fd, buf, MAX_LINE_IN);
      } else {
        read_len = read(events[j].data.fd, buf, MAX_LINE_IN);
      //if (strncmp(buf, "-1", 2) == 0) {
        //ev.data = (int) events[j].data.fd;
        //epoll_ctl(poll_fd, EPOLL_CTL_DEL, events[j].data.fd, &ev);
        //ended += 1;
        //write(file_out, "End of one input\n", 17);
      //} else {
          //prime_count += 1;
          //write(STDOUT_FILENO, buf, strlen(buf) + 1);
        write(file_out, buf, read_len);
      }
          //sprintf(buf2, "%ld", strlen(buf));
          //write(file_out, buf2, strlen(buf2) + 1);
      //}
    }
    //if (ended >= CHILD_COUNT) {
      //return(ended);
    //}
  }
}

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

    close(STDIN_FILENO);
    fd0 = open("/dev/null", O_RDWR);
    fd1 = dup2(0, STDOUT_FILENO);
    fd2 = dup2(0, STDERR_FILENO);

    if (fd0 != 0 || fd1 != 1 || fd2 != 2) {
      exit(EXIT_FAILURE);
    }

    if (Polling(fds) >= CHILD_COUNT) {
      exit(EXIT_SUCCESS);
    } else {
      exit(EXIT_FAILURE);
    }
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
  int i, j, fds[CHILD_COUNT][2], ios[CHILD_COUNT + 1][2];
  pid_t pid; //, pids[CHILD_COUNT];
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
    io_daemonize(ios);
    read(ios[CHILD_COUNT + 1][0], buf, 100);
  }

  for(i = 0; i < CHILD_COUNT; i++) {
    if ((pid = Fork()) == 0) {
      dup2(fds[i][0], STDIN_FILENO);
      dup2(ios[i][1], STDOUT_FILENO);
      for (j = 0;  j < CHILD_COUNT; j++) {
        close(fds[j][0]);
        close(fds[j][1]);
        close(ios[j][0]);
        close(ios[j][1]);
      }
      if (execl("./finder", "./finder", pthread, NULL) < 0)
      {
        perror("execl() in call_child()");
        exit(EXIT_FAILURE);
      }
    }
  }
  for(i = 0; i < CHILD_COUNT; i++) {
    beg = mpz_get_str(beg, DECIMAL, start);
    end = mpz_get_str(end, DECIMAL, stop);
    write(fds[i][1], beg, strlen(beg) + 1);
    write(fds[i][1], "\n", 1);
    sleep(2);
    write(fds[i][1], end, strlen(end) + 1);
    write(fds[i][1], "\n", 1);
    mpz_add_ui(start, stop, 1);
    mpz_add(stop, start, increment);
  }
  if (beg != 0)
    free(beg);

  if (end != 0)
    free(end);

  if (IODAEMON == 0) {
    if ((pid = Fork()) == 0) {
      Polling(ios);
    }
    for(i = 0; i < CHILD_COUNT; i++) {
      pid = wait(NULL);
    }
  } else {
    for(i = 0; i < CHILD_COUNT + 1; i++) {
      pid = wait(NULL);
    } 
  }
  write(ios[CHILD_COUNT][0], "done", 5);
}

// calls the child process.
void
call_child(mpz_t start, mpz_t stop)
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
    if(execl("./finder", "./finder", beg, end, pthread, NULL) == -1)
    {
      perror("execl() in call_child()");
      exit(EXIT_FAILURE);
    }
  }
  if(beg) free(beg);
  if(beg) free(end);
}

int
main(int argc, char *argv[])
{
  mpz_t number, start, stop, increment, max_exp;
  struct timeval time_start, time_stop, time_diff;

  int i, opt; //, options[NUM_OPTS], power;

  //for(i = 0; i < NUM_OPTS; ++i)
    //options[i] = 0;


  if(!argv[1])
    {
        printf("Usage:\n ./boss [OPTIONS]\n"
            "-t run with a time check to test speeds \n"
            "-c [INTEGER] select number of children to use\n"
            "-p [INTEGER] select number of threads to use\n"
            "-d run with an IO daemon\n"
            "-x pass values to child processes using pipes\n");
        exit(EXIT_FAILURE);
    }

  while((opt = getopt (argc,argv, "tc:p:dx")) != -1)
    switch (opt) 
    {
      case 't':
        // to run with time checks
        //options[0] = 1;
        if (CHILD_COUNT == 0)
          CHILD_COUNT = 1;
        break;
      case 'c':
        CHILD_COUNT = atoi(optarg);
        break;
      case 'p':
        PTHREAD_COUNT = atoi(optarg);
        break;
      case 'd':
        IODAEMON = 1;
        //options[3] = 1;
        break;
      case 'x':
        PIPES = 1;
        break;
      default:
        exit(EXIT_FAILURE);
    }

  
  get_num(start, max_exp);

  init_numbers(increment, start, stop, max_exp);
        
  gettimeofday(&time_start, NULL);
  // if the user want no children, the finder is called here
  // otherwise call the number of children asked for.
  if(CHILD_COUNT == 0) {
    finder(start, stop);
  } else if (PIPES == 1) {
    pipes(start, stop, increment);
  } else {
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

  gettimeofday(&time_stop, NULL);

  timersub(&time_stop, &time_start, &time_diff);

  printf("%ld Seconds; %ld Microseconds\n",time_diff.tv_sec, time_diff.tv_usec);
  mpz_clears(max_exp, start, stop, increment, NULL);

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
