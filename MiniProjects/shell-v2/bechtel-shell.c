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

struct pipe {
  char *left;
  char *right;
};
  

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

void
Read(int where_from, char *buf, int size)
{
  if (read(where_from, buf, size) < 0) {
    perror("read()");
    exit(EXIT_FAILURE);
  }
}

int
Parse(char **argv, char *buf, char *token)
{
  int i = 0; 
  token = NULL;

  //Beging parsing user arguments
  token = strtok(buf," ");
  do {
    //create argv array from user arguments
    argv[i] = malloc(ARGMAX);
    memset(argv[i], 0, ARGMAX);
    strncpy(argv[i], token, strlen(token));
    i++;
    //continue parsing
    token = strtok(NULL, " ");
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

int
main(void)
{
  long MAX = sysconf(_SC_LINE_MAX);
  char buf[MAX];
  pid_t pid;
  int status, i, accessed;
  char *argv[ARGMAX];
  char *token, *make_cmd, *consume_env;
  char *env = getenv("PATH");

  do {
    //Reset buffer and access indicator on each pass
    memset(buf, 0, MAX);
    accessed = 0;

    //Write prompt line
    Write(STDOUT_FILENO, "% ", 2);

    //Read in user arguments
    Read(STDIN_FILENO, buf, MAX);

    buf[strlen(buf)-1] = 0; // chomp '\n'

    //If no user input, return to the top
    if (strlen(buf) == 0)
      continue;

    // Parses the command line entry, returns the number of tokens
    i = Parse(argv,buf,token);

    // Exit from commandline
    if (strncmp(argv[0], "exit", 4) == 0) {
      //FREE YOUR MEMORY, KIDS
      for (int j=0; j<i; j++) {
        free(argv[j]);
        argv[j] = NULL;
      }
      exit(EXIT_SUCCESS);
    }

    status = 0;
    //if user provided path, no need to parse it.
    if(access(argv[0], X_OK) == 0) {
      accessed = 1;
      pid = Fork();
      if (pid == 0)   // child
        Execv(argv[0], argv);
      // parent
      else Waitpid(pid, status, 0);
      break;
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

    }
    
    //FREE YOUR ALLOCATED MEMORY, KIDS
    for (int j=0; j<i; j++) {
      free(argv[j]);
      argv[j] = NULL;
    }
    free(consume_env);
    free(make_cmd);

  } while(1);
  exit(EXIT_SUCCESS);
}

