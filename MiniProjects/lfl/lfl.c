//Tacy Bechtel and Matthew Spohrer
//ALSP
//Lock free logging

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

#define LOGSIZE strlen("PID: \t\tCurrent time: \n") + sizeof(int) + sizeof(long int)

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
  if (argc < 4) {
    printf("%s", "Usage: ./lfl [child count] [size] [logfile name]\n");
    return -1;
  }
  time_t buf;
  int childCount = atoi(argv[1]);
  int ret, pid;
  int filesize = atoi(argv[2]);
  char * filename = argv[3];
  int offset = filesize / childCount;
  FILE *fp = fopen(filename, "w");
  int * fds = malloc(childCount);
  FILE **fps = malloc(childCount);
  
  for (int i = 0; i < childCount; ++i)
  {
    if((ret = Fork()) == 0)
    {
      int childOffset = i * offset;
      int childEnd =  (i + 1) * offset -1;
      fps[childCount] = fopen(filename, "w");
      fds[childCount] = fileno(fps[childCount]);
      fseek(fps[childCount], childOffset, SEEK_SET);
      pid = getpid();
      Time(&buf);
      while ((childOffset += dprintf(fds[childCount], "PID: %d\t\t Current time: %ld\n", pid, buf)) + LOGSIZE <= childEnd) {
        sleep(1);
        Time(&buf);
      }
      if (childOffset > childEnd) {
        printf("wrote outside buffer\n");
        exit(-1);
      }
      exit(0);
    }
  }

  if (ret != 0) {
    for (int i = 0; i < childCount; ++i) {
      ret = wait(NULL);
      if (ret){
        perror("Wait()");
        exit(ret);
      }
    }
    printf("All children exited!\n");
  }

  fclose(fp);
  return 0;
}

