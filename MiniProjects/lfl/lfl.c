//Tacy Bechtel and Matthew Spohrer
//ALSP
//Lock free logging

// For our program, we created a log file with fopen in the 
// parent process then fopen in each of the child processes
// to prevent children from switching in and using the 
// previously running child's offset. 
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>

// the size of each output
#define LOGSIZE strlen("PID: \t\tCurrent time: \n") + sizeof(int) + sizeof(long int)

// helper for the fopen() call
FILE *
Fopen(char *filename, char *mode)
{
  FILE *buf = NULL;
  if((buf = fopen(filename, mode)) == NULL)
  {
    perror("Fopen()");
    exit(EXIT_FAILURE);
  }
  return buf;
}

// helper for the time() call
void
Time(time_t *buf)
{
  if(time(buf) < 0)
  {
    perror("Time()");
    exit(EXIT_FAILURE);
  }
}

// helper for the fork() call
static int
Fork()
{
  pid_t pid;

  if((pid = fork()) < 0)
  {
    perror("Fork()");
    exit(EXIT_FAILURE);
  }
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
  FILE *fp = Fopen(filename, "w");
  
  // forks the number of children entered form the command line
  for (int i = 0; i < childCount; ++i)
  {
    //if child
    if((ret = Fork()) == 0)
    {
      int childOffset = i * offset;
      int childEnd =  (i + 1) * offset -1;

      // creates a seperate file directory entry for each child
      // to prevent outputs from different child processes using the 
      // offsets from child processes when switched between the 
      // fseek and completion of the output
      FILE *fp2 = Fopen(filename, "r+");
      int fd = fileno(fp2);

      // finds where in the log to begin output. Beginning of log +
      // childOffset
      fseek(fp2, childOffset, SEEK_SET);

      // get info to output
      pid = getpid();
      Time(&buf);

      // while the child still has room to write, keep writing
      // the sleep call was added to show the changes in the 
      // times being outputted
      while ((childOffset += dprintf(fd, "PID: %d\t\tCurrent time: %ld\n", 
              pid, buf)) + LOGSIZE <= childEnd) {
        sleep(1);
        Time(&buf);
      }

      // check to ensure no overwrites
      if (childOffset > childEnd)
        printf("wrote outside buffer\n");

      // fills the holes with null characters
      while(childOffset++ < childEnd) {
        dprintf(fd, 0);
      }

      fclose(fp2);
      exit(0);
    }
  }

  // if parent, wait
  if (ret != 0) {
    for (int i = 0; i < childCount; ++i) {
      wait(NULL);
    }
    printf("All children exited!\n");
  }

  fclose(fp);
  return 0;
}

