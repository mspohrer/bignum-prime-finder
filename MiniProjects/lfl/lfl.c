//Tacy Bechtel and Matthew Spohrer

//Lock free logging

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

static time_t 
Time(time_t *buf)
{
  time_t t;
  if((t = time()) < 0)
    perror("Time()");
  return pid;
}

static int
Fork()
{
  pid_t pid;

  if((pid = fork()) < 0)
    perror("Fork()");
  return pid;
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    printf("%s", "Usage: ./lfl [child count]\n");
    return -1;
  }
  time_t *buf;
  int childCount = atoi(argv[1]);
  int ret, fd, pid;
  printf("%d\n", childCount);
  
  FILE *fp = fopen("lfl.txt", "w");
  fd = fileno(fp);

  if((ret = Fork()) == 0)
  {
    printf("child\n");
    fd = dup(fp);
    pid = getpid();
    buf = Time(&buf);
    


  }

  else
    wait(NULL);

  fclose(fd);
  return 0;
}

