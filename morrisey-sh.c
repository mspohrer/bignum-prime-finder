// simple shell
// This program takes whatever is input and executes it as a single command.
// This is problematic. While "ls" works, try "ls -FC" or "ls "
// Based on APUE3e p. 12
// 1. Remove fgets and replace with read.
// 2. Remove printf and replace with write.
// 3. Remove call to error() and replace with write, strerror, and exit.
// 4. If command path is not specified, then use the PATH environment
//    variable to find the command.
// 5. Add "exit" command to leave the shell
// 6. Replace execlp with execv.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <error.h>


typedef struct call{
  long max;
  char *buf;
  long numWords;
  char ** words;
} scall;

static int
Fork()
{
  pid_t pid;

  if ((pid = fork()) < 0)
    error(EXIT_FAILURE, errno, "fork error");
  return(pid);
}
static void
Malloc(char **buf, uint size)
{
  *buf = malloc(size);
  memset(*buf, 0, size);
  if (buf == NULL)
    {
      error(EXIT_FAILURE, errno, "fork error");
    }
}
static void
Parse(scall *sc)
{
  int i;
  char *word;

  // counts the number of words
  i = 0;
  sc->numWords = 1;
  while(sc->buf[i] != 0)
  {
    if((sc->buf[i] == ' ') && (sc->buf[i - 1] != ' '))
      ++sc->numWords;
    ++i;
  }
  
  sc->words = malloc((sc->numWords-1) * sizeof(char*));
  memset(sc->words, 0, sc->numWords - 1);
  i = 0;
  while((word = strtok_r(sc->buf, " ", &sc->buf)) != NULL)
  {
    Malloc(&sc->words[i], strlen(word));
    strncpy(sc->words[i], word, strlen(word));
    memset(word, 0, strlen(word));
    ++i;
  }
  if(word) 
  {
    free(word);
    word = NULL;
  }
}

static void
Read(scall *sc)
{
  ssize_t ret;
  Malloc(&sc->buf, sc->max);
  if ((ret = read(STDIN_FILENO, sc->buf, sc->max)) < 0)
    {
    error(EXIT_FAILURE, errno, "read error");
    }
  sc->buf[strlen(sc->buf)-1] = 0; // chomp '\n'

  Parse(sc);
}

int
main(void)
{
  scall sc;
  sc.max = sysconf(_SC_LINE_MAX);
  char *prompt = "% ";
  long MAX = sysconf(_SC_LINE_MAX);
  char buf[MAX];
  pid_t pid;
  int status;

  do {
    printf("%% ");
    fflush(STDIN_FILENO);
    Read(&sc);
    buf[strlen(buf)-1] = 0; // chomp '\n'
    pid = Fork();
    if (pid == 0) {  // child
      execlp(buf, buf, (char *)NULL);
      error(EXIT_FAILURE, errno, "exec failure");
    }
    // parent
    if ((pid = waitpid(pid, &status, 0)) < 0)
      error(EXIT_FAILURE, errno, "waitpid error");
  } while(1);
  exit(EXIT_SUCCESS);
}
