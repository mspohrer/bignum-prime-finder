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

static void
scallConst(scall * sc)
{
  sc->buf = NULL;
  sc->max = sysconf(_SC_LINE_MAX);
  sc->numWords = 0;
  sc->words = NULL;
}

static void
Write(int io, char *p)
{
  ssize_t ret;
  ret = write(io, p, strlen(p)); 
  fflush(NULL);
  if (ret < 0)
    {
      write(STDERR_FILENO, "Error in Write: ", 17);
      write(STDERR_FILENO,strerror(errno),strlen(strerror(errno)));
      write(STDERR_FILENO, "\n", 1);
      fflush(NULL);
      exit(EXIT_FAILURE);
    }
}

static int
Fork()
{
  pid_t pid;

  if ((pid = fork()) < 0)
    {
      Write(STDERR_FILENO, "Error in Fork: ");
      Write(STDERR_FILENO,strerror(errno));
      Write(STDERR_FILENO, "\n");
      exit(EXIT_FAILURE);
    }
  return(pid);
}

static void
Malloc(char **buf, uint size)
{
  *buf = malloc(size);
  memset(*buf, 0, size); 
  if (buf == NULL)
    {
      Write(STDERR_FILENO, "Error in Malloc: ");
      Write(STDERR_FILENO,strerror(errno));
      Write(STDERR_FILENO, "\n");
      exit(EXIT_FAILURE);
    }
}


static char*
Getenv(void)
{
  char *PATH = getenv("PATH");
  if (PATH == NULL)
    {
      Write(STDERR_FILENO, "Error in Getenv: ");
      Write(STDERR_FILENO,strerror(errno));
      Write(STDERR_FILENO, "\n");
      exit(EXIT_FAILURE);
    }
  return PATH;
}

static void
Execv(scall * sc)
{
  char *PATH = Getenv();
  int numPath = 0;
  char **path;
  char *word;
  int i, ret;
  char *buf;

  // counts the number of words
  i = 0;
  while(PATH[i] != 0)
  {
    if(PATH[i] == ':')
      ++numPath;
    ++i;
  }

  i = (numPath) * sizeof(char*);
  path = malloc(i);
  memset(path, 0, i);
  i = 0;
  while((word = strtok_r(PATH, ":", &PATH)) != NULL)
  {
    Malloc(&path[i], strlen(word));
    strncpy(path[i], word, strlen(word));
    ++i;
  }

  for(i = 0; i <= numPath; ++i)
  {
    ret = (strlen(path[i]) + strlen(sc->words[0])) * sizeof(char);
    buf = malloc(ret);
    memset(buf, 0, ret);
    strncpy(buf, path[i], strlen(path[i]));
    strncat(buf, "/", 1);
    strncat(buf, sc->words[0], strlen(sc->words[0]));
    if((ret = access(buf, X_OK)) >= 0)
      break;
  }

  execv(buf,sc->words);
  perror("Execv");
  exit(EXIT_FAILURE);
}

static void
Parse(scall *sc)
{
  int i;
  char *word;
  char *save = sc->buf;

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
  while((word = strtok_r(save, " ", &save)) != NULL)
  {
    Malloc(&sc->words[i], strlen(word));
    strncpy(sc->words[i], word, strlen(word));
    memset(word, 0, strlen(word));
    ++i;
  }
  int k;
  for(k = 0; k < i; ++k)
    printf("%s ", sc->words[k]);
  printf("%d \n", k);
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
      Write(STDERR_FILENO, "Error in Read: ");
      Write(STDERR_FILENO,strerror(errno));
      Write(STDERR_FILENO, "\n");
      exit(EXIT_FAILURE);
    }
  sc->buf[strlen(sc->buf)-1] = 0; // chomp '\n'

  Parse(sc);
}

void 
Waitpid(pid_t pid, int *status)
{
  if ((pid = waitpid(pid, status, 0)) < 0)
  {
    Write(STDERR_FILENO, "Error in waitpid: ");
    Write(STDERR_FILENO,strerror(errno));
    Write(STDERR_FILENO, "\n");
    exit(EXIT_FAILURE);
  }
}

static void
Exit(scall * sc)
{
  for(int i = sc->numWords -1; i >=0; --i)
    free(sc->words[i]);
  free(sc->words);
  free(sc->buf);
  exit(EXIT_SUCCESS);
}

int
main(void)
{
  scall sc;
  pid_t pid;
  char *prompt = "% ";
  int status;

  status = 0;
  scallConst(&sc);

  do {
    Write(STDOUT_FILENO, prompt);
    fflush(NULL);
    Read(&sc);

    if(strncmp(sc.words[0], "exit", strlen(sc.words[0]) - 1) == 0)
    {
      Exit(&sc);
    }

    pid = Fork();
    if (pid == 0)   // child
      Execv(&sc);
    else            // parent
      Waitpid(pid, &status);

  } while(1);
}
