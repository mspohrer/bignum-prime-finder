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
#include <error.h>
#include <limits.h>

#define ARGMAX 30   //max size of argument I am accepting

static int
Fork()
{
  pid_t pid;

  if ((pid = fork()) < 0)
    error(EXIT_FAILURE, errno, "fork error");
  return(pid);
}

void
Write(int where_to, char *buf, int size)
{
  if (write(where_to, buf, size) < 0) {
    perror("write()");
    exit(EXIT_FAILURE);
  }
}

int
Read(int where_from, char *buf, int size)
{
  int c, i = 0;
  char * exitMsg = "\nTo exit, please type \"exit\"\n";
  do {
    c = getc(stdin);
    if (c == EOF) {
      Write(STDOUT_FILENO, exitMsg, strlen(exitMsg));
      return 1;
    }
    buf[i] = c;
    i++;
  } while ( c != '\n' && c != EOF && i < size);
  buf[i] = '\0';
  return 0;
}

void
free_array(char **argv, int i)
{
  //FREE YOUR MEMORY, KIDS
  for (int j=0; j<i; j++) {
    free(argv[j]);
    argv[j] = NULL;
  }
}

int
Parse(char **argv, char *buf, char *delim)
{
  int i = 0;
  char *token = NULL;

  //Beging parsing user arguments
  token = strtok(buf,delim);
  do {
    //create argv array from user arguments
    argv[i] = malloc(ARGMAX);
    memset(argv[i], 0, ARGMAX);
    strncpy(argv[i], token, strlen(token));
    i++;
    //continue parsing
    token = strtok(NULL, delim);
  } while (token != NULL && i < 20);
  //exec will want last argument to be null (when to stop)
  argv[i] = NULL;
  return i;
}

void
Execv(char *cmd, char **argv)
{
  execv(cmd, argv);
  write(STDERR_FILENO, strerror(errno), strlen(strerror(errno)));
  write(STDOUT_FILENO, "\n", 1);
  exit(EXIT_FAILURE);
}

void
Waitpid(pid_t pid, int status,int option)
{
  if ((pid = waitpid(pid, &status, option)) < 0)
    write(STDERR_FILENO, strerror(errno), strlen(strerror(errno)));
}

// goes checks the command and exec process
void
exec_checks(char **argv, int pid)
{
  int status,accessed;
  char *token, *make_cmd, *consume_env;
  char *env = getenv("PATH");

  status = 0;
  accessed = 0;
  //if user provided path, no need to parse it.
  if(access(argv[0], X_OK) == 0) {
    accessed = 1;
    if (pid == -1)
      pid = Fork();
    if (pid == 0)   // child
      Execv(argv[0], argv);
    // parent
    else Waitpid(pid, status, 0);
  }
  else {
    //copy of env so program can loop until exited (doesn't destroy original env)
    consume_env = malloc(strlen(env) + 1);
    strncpy(consume_env, env, strlen(env));
    //allocate path holder
    make_cmd = malloc(PATH_MAX);
    //begin parsing path options
    token = strtok(consume_env, ":");
    do {
      memset(make_cmd, 0, PATH_MAX);
      //try to find command form user down each path variable
      strcat(make_cmd, token);
      strcat(make_cmd, "/");
      strncat(make_cmd, argv[0], strlen(argv[0]));
      //if it's there and we have execute rights, fork and run the command
      if(access(make_cmd, X_OK) == 0) {
        accessed = 1;
        if (pid == -1)
          pid = Fork();
        if (pid == 0)  // child
          Execv(make_cmd, argv);
        // parent
        else Waitpid(pid, status, 0);
        break;
        }
      //continue parsing
      token = strtok(NULL, ":");
    } while (token != NULL);
    //give user output if unable to execute entry
    if (token == NULL && accessed == 0) {
      write(STDOUT_FILENO, "Unable to process request\n", 26);
    }
    free(consume_env);
    free(make_cmd);
  }
}

void
Pipe(int *fd)
{
  if(pipe(fd) == -1){
    perror("Pipe()");
    exit(EXIT_FAILURE);
  }
}

// called in the case there are pipes. It breaks the command
// line entry into the seperate commands, opens a pipe, and
// forks. At this point, the stdio fd's are changed to those
// formed by the pipe call so the exec'ing functions will use
// those fd's for their IO.
void
pipes(char **argv, char **cmds, char *buf)
{
  int i = Parse(cmds, buf, "|");
  int fd[2];
  int k, rw;
  pid_t pid = 0;

  rw = 1;
  for(k = 0; k < i; ++k){
    if(rw == 1) Pipe(fd);
    fprintf(stderr,"%d % d after Pipe\n", fd[0], fd[1]);
    if((pid = Fork()) == 0){
      if(k != 0) {
        fprintf(stderr,"%d % d after Pipe in\n", fd[0], fd[1]);
        close(STDIN_FILENO);
        dup(fd[0]);
      }
      if(k < i - 1){
        fprintf(stderr,"%d % d after Pipe out\n", fd[0], fd[1]);
        close(STDOUT_FILENO);
        dup(fd[1]);
      }
      close(fd[0]);
      close(fd[1]);
      Parse(argv, cmds[k], " ");
      exec_checks(argv, pid);
    }
    close(fd[rw]);
    --rw;
    if(rw < 0) rw = 1;
  }
  for(k = 0; k < i; ++k) wait(NULL);
}


int
main(void)
{
  long MAX = sysconf(_SC_LINE_MAX);
  char buf[MAX];
  int i;
  char *argv[ARGMAX];
  char *cmds[ARGMAX];
  pid_t pid;

  do {
    //Reset buffer and access indicator on each pass
    memset(buf, 0, MAX);

    //Write prompt line
    Write(STDOUT_FILENO, "% ", 2);

    //Read in user arguments
    if (Read(STDIN_FILENO, buf, MAX))
      continue;

    buf[strlen(buf)-1] = 0; // chomp '\n'

    //If no user input, return to the top
    if (strlen(buf) == 0)
      continue;
    // checks if pipes exist in the commandline entry and if so
    // directs the flow of control to handle piping otherwise
    // it handles the single command
    if(strchr(buf, '|')){
      pipes(argv,cmds, buf);
    }
    else {
      i = Parse(argv,buf, " ");
      // Exit from commandline
      if (strncmp(argv[0], "exit", 4) == 0) {
        free_array(argv, i);
        exit(EXIT_SUCCESS);
      }
//      pid = Fork();
      pid = -1;
      exec_checks(argv, pid);
      free_array(argv, i);
    }



  } while(1);
  exit(EXIT_SUCCESS);
}
