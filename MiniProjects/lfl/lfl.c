//Tacy Bechtel and Matthew Spohrer

//Lock free logging

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

void
Time(time_t *buf)
{
  if(time(buf) < 0)
    perror("Time()");
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
  time_t *buf = NULL;
  int childCount = atoi(argv[1]);
  int ret, fd, pid;
  //printf("%d\n", childCount);
  int filesize = 4096;
  int offset = filesize / childCount;
  FILE *fp = fopen("lfl.txt", "w");
  fd = fileno(fp);
  
  for (int i = 0; i < childCount; ++i)
  {
    if((ret = Fork()) == 0)
    {
      //printf("child\n");
      int childStart = i * offset;
      int childEnd =  (i + 1) * offset -1;
      int childOffset = 0;
      //printf("%d\n", childStart);
      printf("%d\n", childEnd);
      fd = dup(fd);
      fp = fopen("lfl.txt", "w");
      fseek(fp, childOffset, childStart);
      pid = getpid();
      Time(buf);
      printf("hi");
      dprintf(fd, "PID: %d/t/t Current time: %ln\n", pid, buf);
      while ((childOffset += dprintf(1, "PID: %d/t/t Current time: %ln\n", pid, buf)) <= childEnd)
        printf("%d\n", childOffset);
      exit(0);
    }
  }

  if (ret != 0) {
    for (int i = 0; i < childCount; ++i) {
      wait(NULL);
    }
    printf("All children exited!\n");
  }



  fclose(fp);
  return 0;
}

