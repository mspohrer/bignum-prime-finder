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
int ACCT_ON = 0;

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
  } while ( c != '\n' && c != EOF && c != 0 && i < size);
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
exitbuiltin(char **argv, int argc)
{
  //char argc = sizeof(argv) / sizeof(argv[0]);
  //Write(STDOUT_FILENO, &argc, sizeof(argc));
  free_array(argv, argc);
  exit(EXIT_SUCCESS);
}

int
runbuiltin(char **argv, int argc)
{
  ACCT_ON = 1;
  return 0;
}

//Code borrowed from xv6 sh.c
typedef int funcPtr_t(char **, int);
typedef struct {
  char       *cmd;
  funcPtr_t  *name;
} dispatchTableEntry_t;


dispatchTableEntry_t fdt[] = {
  {"exit", exitbuiltin},
  {"run", runbuiltin}
};
int FDTcount = sizeof(fdt) / sizeof(fdt[0]); // # entris in FDT

void
dobuiltin(char **argv, int argc) {
  int i;

  for (i=0; i<FDTcount; i++)
    if (strncmp(argv[0], fdt[i].cmd, strlen(fdt[i].cmd)) == 0)
     (*fdt[i].name)(argv, argc);
}

//End of code borrowed from xv6

int
useBuiltin(char **argv, int argc)
{
  int len = strlen(argv[0]);
  if (strncmp(argv[0], "exit", len < 4 ? len : 4) == 0 || strncmp(argv[0], "run", len < 3 ? len : 3) == 0) {
    dobuiltin(argv, argc);
    return 1;
  }
  return 0;
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
  } while (token != NULL && i < ARGMAX);
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
  if (ACCT_ON) {
    struct rusage *rusage = NULL;
    if ((pid = wait4(pid, &status, option, rusage)) < 0)
      write(STDERR_FILENO, strerror(errno), strlen(strerror(errno)));
  }
  else {
    if ((pid = waitpid(pid, &status, option)) < 0)
      write(STDERR_FILENO, strerror(errno), strlen(strerror(errno)));
  }
}

// goes checks the command and exec process
void
exec_checks(char **argv, int pid, int argc)
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
  else if (useBuiltin(argv, argc)) {
    Write(STDOUT_FILENO, "found\n", 6);
    return;
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
  int cmd_cnt = Parse(cmds, buf, "|");
  int fd[cmd_cnt-2][2];
  int k,j;
  pid_t pid = 0;

  // parent - sets up
  for(j=0; j < cmd_cnt-1; ++j)
    Pipe(fd[j]);

  for(k = 0; k < cmd_cnt; ++k){
    if((pid = Fork()) == 0){ // child
      // if child is the first entered in the command line
      if(k == 0){
        // set its output to the first fd output number
        dup2(fd[k][1], STDOUT_FILENO);
        // close all unused fd's for the child proc
        for(j=0; j<cmd_cnt-1; ++j){
          close(fd[j][0]);
          close(fd[j][1]);
        }
        // Parse and Exec
        j = Parse(argv, cmds[k], " ");
        exec_checks(argv, pid, j);
        // if the newly forked child is neither the first or last
      } else if (k < cmd_cnt -1) {
        // change its stdout to the kth out fd
        dup2(fd[k][1], STDOUT_FILENO);
        // change its stdin to the kth-1 in fd.
        // this sets it to read in from the last childs output.
        dup2(fd[k-1][0], STDIN_FILENO);
        // close all uneccesary fd's
        for(j=0; j<cmd_cnt-1; ++j){
          close(fd[j][0]);
          close(fd[j][1]);
        }
        // Parse and Exec
        j = Parse(argv, cmds[k], " ");
        exec_checks(argv, pid, j);
        // last command entered in the chain of pipes
      } else {
        // switches the stdin of the las
        dup2(fd[k-1][0], STDIN_FILENO);
        for(j=0; j<cmd_cnt-1; ++j){
          close(fd[j][0]);
          close(fd[j][1]);
        }
        // Parse and Exec
        j = Parse(argv, cmds[k], " ");
        exec_checks(argv, pid, j);
      }
    }
  }
  // parent. close all fds
  for(j=0; j<cmd_cnt-1; ++j){
    close(fd[j][0]);
    close(fd[j][1]);
  }
  for(k = 0; k < cmd_cnt; ++k) 
    wait(NULL);
  free_array(cmds, cmd_cnt);
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
      /* TODO REMOVE THIS WHEN BUILTINS DONE
      // Exit from commandline
      if (strncmp(argv[0], "exit", 4) == 0) {
        free_array(argv, i);
        exit(EXIT_SUCCESS);
      }
      */
//      pid = Fork();
      pid = -1;
      exec_checks(argv, pid, i);
      free_array(argv, i);
    }



  } while(1);
  exit(EXIT_SUCCESS);
}
