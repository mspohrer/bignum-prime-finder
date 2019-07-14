//Tacy Bechtel and Matthew Spohrer

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
  if (argc < 2) {
    printf("%s", "Usage: ./lfl [child count]\n");
    return -1;
  }
  time_t buf;
  int childCount = atoi(argv[1]);
  int ret, fd, pid;
  //printf("%d\n", childCount);
  int filesize = 1024;
  int offset = filesize / childCount;
  FILE *fp = fopen("lfl.txt", "w");
  int * fds = malloc(childCount);
  FILE **fps = malloc(childCount);
  fd = fileno(fp);
  
  for (int i = 0; i < childCount; ++i)
  {
    if((ret = Fork()) == 0)
    {
      //printf("child\n");
      int childOffset = i * offset;
      int childEnd =  (i + 1) * offset -1;
      //char buffer[offset - 1];
      //fds[childCount] = dup(fd);
      fps[childCount] = fopen("lfl.txt", "w");
      fds[childCount] = fileno(fps[childCount]);
      //setvbuf(fps[childCount], buffer, _IOLBF, offset - 1);
      fseek(fps[childCount], childOffset, SEEK_SET);
      pid = getpid();
      Time(&buf);
      //printf("hi\n");
      //dprintf(fd, "PID: %d\t\t Current time: %ld\n", pid, buf);
      //dprintf(fd, "PID: %d\t\t Current time: %ld\n", pid, buf);
      while ((childOffset += dprintf(fds[childCount], "PID: %d\t\t Current time: %ld\n", pid, buf)) + LOGSIZE <= childEnd) {
        //printf("PID: %d\t Offset: %d\n", pid, childOffset);
        sleep(1);
        Time(&buf);
      }
      if (childOffset > childEnd)
        printf("wrote outside buffer\n");
      /*
      while(childOffset++ < childEnd) {
        dprintf(fds[childCount], '\0');
      }
      */
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

