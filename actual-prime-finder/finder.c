// goes through every odd number from 3 to the square root
// of num_to_check. If the num_to_check is diveded evenly
// by the dividend, then 0 is returned and num_to_check 
// is not prime. 
// Otherwise the number being checked is prime and 1 is
// returned.

#include "prime-finder.h"

//struct node *head;
mpz_t INCREMENT;
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
 // if(rem != 0)
//    gmp_printf("%Zd is prime\n", num_to_check);
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
is_prime_wrapper(void *arg)
{
  mpz_t start, stop;
  mpz_init_set_str(start, arg, DECIMAL);
    gmp_printf("%Zd\n", start);
  mpz_init(stop);
  mpz_add(stop, start, INCREMENT);
  
  while(mpz_cmp(start,stop) <= 0)
  {
    is_prime(start);
    mpz_add_ui(start, start, 2);
  }

  pthread_exit(NULL);
}

// oof, this is kind of wonky. When passing iterated numbers through 
// pthread_create, the pointer sometimes is pointing to the same data
// as the previously created thread. The setup with the num[] prevents 
// prevents that from happening. Concurrency is a pain!
void
threads(mpz_t start, mpz_t stop)
{
  int k, ret;
  pthread_t ptid[PTHREAD_COUNT];
  char *starts[PTHREAD_COUNT];
  mpz_t diff;
  
  mpz_init(INCREMENT);
  mpz_init(diff);
  
  mpz_sub(diff, stop, start);
  mpz_cdiv_q_ui(INCREMENT, diff, PTHREAD_COUNT);

  memset(starts, 0, sizeof(starts)); 

  for(k = 0; k < PTHREAD_COUNT; ++k)
  {
    starts[k] = mpz_get_str(starts[k], DECIMAL, start);
    mpz_add(start, start, INCREMENT);
    mpz_add_ui(start, start, 1);
  }

  for(k = 0; k < PTHREAD_COUNT; ++k)
  {
    //printf("threads %Zd\n", starts[k]);
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
}

  //gmp_printf("increment %Zd\n", increment);

  /*
  while(!head);
  struct node *temp = NULL;
  while(mpz_cmp_ui(temp->num, 0) != 0){
    while(!head);
    pthread_mutex_lock(&head->lock);
    temp = head;
    head = head->next;
    //temp->next = NULL;
    pthread_mutex_unlock(&temp->lock);
    is_prime(temp->num);
    mpz_clear(temp->num);
    pthread_mutex_destroy(&temp->lock);
    free(temp);
  }
  sleep(1);
  while(head) 
  {
    gmp_printf("%Zd\n", head->num);
    head = head->next;
  }
  while(mpz_cmp(num_to_check,stop) <= 0)
  {
    is_prime(num_to_check);
    mpz_add_ui(num_to_check, num_to_check, 2);
  }
  */
  /*
struct node *
init_node()
{
  struct node *temp = malloc(sizeof(struct node));
  mpz_init(temp->num);
  pthread_mutex_init(&temp->lock, NULL);
  pthread_mutex_lock(&temp->lock);
  temp->checked = 0;
  temp->next = NULL;
  return temp;
}
  for(k = 0; k < PTHREAD_COUNT; ++k)
    printf("threads %s\n", starts[k]);
  for(k = 0; k < PTHREAD_COUNT; ++k)
    gmp_printf("threads %Zd\n", range[k]);
  for(k = 0; k < PTHREAD_COUNT; ++k){
    ret = pthread_join(ptid[k], NULL);
    if(ret != 0){
      perror("threads()");
      exit(EXIT_FAILURE);
    }
  }
  */

/*
  //struct node *tail = NULL;
  //head = NULL;
  if(PTHREAD_COUNT > 1) 
    mpz_fdiv_q_ui(increment, number, CHILD_COUNT);

  if(PTHREAD_COUNT < 2) 
    mpz_init_set(stop, number);
  else 
  {
    mpz_init(stop);
    mpz_add(stop, start, increment);

  for(int j = 0; j < PTHREAD_COUNT; ++j)
  {
    ret = pthread_create(&ptid[j], NULL, &is_prime_wrapper, NULL);
    if(ret != 0){
      perror("threads()");
      exit(EXIT_FAILURE);
    }
  }
  */
  /*
  while(mpz_cmp(num_to_check,stop) <= 0)
  {
    if(!head) {
      head = init_node();
      mpz_set(head->num, num_to_check);
      pthread_mutex_unlock(&head->lock);
      tail = head;
      head->next = NULL;
    } else {
      tail->next = init_node();
      mpz_set(tail->num, num_to_check);
      pthread_mutex_unlock(&tail->lock);
      tail->next = NULL;
      tail = tail->next;
    }
    mpz_add_ui(num_to_check, num_to_check, 2);
  }
  tail = init_node();
  mpz_set_ui(tail->num, 0);
  tail->next = NULL;
  pthread_mutex_unlock(&tail->lock);
  */

int
main(int argc, char **argv)
{
  mpz_t start, stop, remainder, num_to_check;
  char buf[BUFSIZ], buf2[MAX_LINE_IN];
  int read_in;
  FILE *filein;

  // converts the char * passed from ./boss to mpz_t 
  // for start and stop

  if (argc == 4) {
    mpz_init_set_str(start, argv[1], 10);
    printf("value from argv[1]: %s\n", argv[1]);
    mpz_init_set_str(stop, argv[2], 10);
    printf("value from argv[2]: %s\n", argv[2]);
    PTHREAD_COUNT = atoi(argv[3]);
  }
  else {
    filein = fdopen(STDIN_FILENO, "r");
    //printf("reading child\n");
    //read_in = read(STDIN_FILENO, &buf, sizeof(buf)-1);
    fgets(buf2, MAX_LINE_IN, filein);

    if (buf2[strlen(buf2)-1] == '\n')
      buf2[strlen(buf2) - 1] = 0;
    printf("start:%s\n", buf2);
    mpz_init_set_str(start, buf2, 10);
    fgets(buf2, MAX_LINE_IN, filein);
    if (buf2[strlen(buf2)-1] == '\n')
      buf2[strlen(buf2) - 1] = 0;
    //read(STDIN_FILENO, &buf, sizeof(buf)-1);
    //buf[strlen(buf)] = 0;
    //printf("end:%s\n", buf2);
    mpz_init_set_str(stop, buf2, 10);
    PTHREAD_COUNT = atoi(argv[1]);
  }
  mpz_init_set(num_to_check, start);
  mpz_init(remainder);
  mpz_cdiv_r_ui(remainder, num_to_check, 2);

  // makes start odd to ensure only odd numbers are checked
  if(mpz_cmp_ui(remainder, 0) == 0) 
    mpz_add_ui(num_to_check, num_to_check, 1);

  if(PTHREAD_COUNT == 0)
    no_threads(num_to_check, stop);
  else
    threads(num_to_check, stop);

  mpz_clears(start, stop, remainder, num_to_check, NULL);
  //write(STDOUT_FILENO, "-1\n", 2);
}
