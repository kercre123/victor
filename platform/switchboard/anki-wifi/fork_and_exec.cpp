/**
 * File: fork_and_exec.cpp
 *
 * Author: seichert
 * Created: 11/7/2017
 *
 * Description: Fork and Exec a process
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "fork_and_exec.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define ARRAY_SIZE(x)   (sizeof(x) / sizeof(*(x)))

namespace Anki {

static pid_t sChildPid = -1;

static int ExecChild(const std::vector<std::string>& args) {
  char* argv_child[args.size() + 1];

  for (size_t i = 0 ; i < args.size() ; ++i) {
    argv_child[i] = (char *) malloc(args[i].size() + 1);
    strcpy(argv_child[i], args[i].c_str());
  }
  argv_child[args.size()] = NULL;

  int rc = execvp(argv_child[0], argv_child);
  fprintf(stderr, "%s: %s\n", argv_child[0], strerror(errno));
  for (size_t i = 0 ; i < args.size() + 1 ; ++i) {
    free(argv_child[i]);
  }
  _exit(-1);
  return rc;
}

int ForkAndExec(const std::vector<std::string>& args)
{
  pid_t pid;
  int rc;

  pid = fork();
  if (pid < 0) {
    return -1;
  } else if (!pid) {
    rc = ExecChild(args);
  } else {
    sChildPid = pid;
    int status;
    pid_t w = TEMP_FAILURE_RETRY(waitpid(pid, &status, 0));
    if (w == -1) {
      return errno;
    }
    if (WIFEXITED(status)) {
      rc = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
      rc = -(WTERMSIG(status));
    } else {
      rc = -ECHILD;
    }
    sChildPid = -1;
  }
  return rc;
}


void KillChildProcess() {
  if (sChildPid > 0) {
    (void) kill(sChildPid, SIGINT);
    (void) kill(sChildPid, SIGQUIT);
    (void) kill(sChildPid, SIGKILL);
    sChildPid = -1;
  }
}

} // namespace Anki
