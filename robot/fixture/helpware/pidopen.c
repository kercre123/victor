#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "core/clock.h"


#define SHELLNAME "/bin/sh"

#define MAX_ARGS 16+3


int pidopen(const char* processname, const char* argstr, pid_t* pid_out)
{
  pid_t pid = 0;
  int pipefd[2];

  pipe(pipefd); //create a pipe
  pid = fork(); //span a child process
  if (pid == 0)
  {
    //space separate argstr into args array
    char* argcopy = strdup(argstr); //make a writeable copy
    char* args[MAX_ARGS];

    int i = 0;
    args[i++]= SHELLNAME;
    args[i++] = (char*)processname;
    char** ap = &args[i++];
    while (ap < &args[MAX_ARGS] && (*ap = strsep(&argcopy, " ")) != NULL) {
      if (**ap != '\0') { ap++; }
    }
    free(argcopy);

    // Child. redirect std output to pipe, launch script
    close(pipefd[0]);
    dup2(pipefd[1], STDOUT_FILENO);
//    execv(SHELLNAME, args);
    execv(args[0], args);
  }
  //Only parent gets here. make tail nonblocking and return;
  *pid_out = pid;
  close(pipefd[1]);
  fcntl(pipefd[0], F_SETFL, fcntl(pipefd[0], F_GETFL) | O_NONBLOCK);
  return pipefd[0];
}


int pidclose(pid_t pid, bool force)
{
  if (force) {
    printf("killing %d\n",pid);
    kill(pid, SIGKILL);
  }
  int status;
  //or wait for the child process to terminate
  waitpid(pid, &status, 0);
  printf("waitpid returned \n");
  return force?-70:WEXITSTATUS(status);
}

int wait_for_data(int fd, int max_sec) {
  fd_set fdset;
  struct timeval timeout = {max_sec,500000};
  FD_ZERO(&fdset);
  FD_SET(fd, &fdset);
  int ret = select(fd+1,&fdset,NULL,NULL,&timeout);
  return (ret>0);
}


//#define SELF_TEST
#ifdef SELF_TEST

int main(int argc, const char* argv[]) {

  int pid, pfd;
  pfd = pidopen("./headprogram", &pid);
  bool timedout = false;

  int timeout_sec = 2;

  uint64_t now = steady_clock_now();
  uint64_t expiration = now +(timeout_sec*NSEC_PER_SEC);


  if (pfd>0) {
    uint64_t scnow = steady_clock_now();
    printf("%ju of %ju\n", scnow-now, expiration-now);

    char buffer[512];
    int n;
    do {
      if (wait_for_data(pfd, 0)) {
      n = read(pfd, buffer, 512);
      if (n>0) {
        printf("%.*s", n, buffer);

      }
      }
      scnow = steady_clock_now();
      printf("%ju of %ju\n", scnow-now, expiration-now);
      if (scnow > expiration) {
        printf("TIMEOUT after %d sec\n", timeout_sec);
        timedout = true;
        break;
      }

    } while (n>0 || n==EAGAIN);
    return pidclose(pid, timedout);
  }
  return -1;
}

#endif
