/**
 * File: fork_and_exec.cpp
 *
 * Author: seichert
 * Created: 11/7/2017
 *
 * Description: Fork, Exec a process and capture output
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

static pthread_mutex_t sMutex = PTHREAD_MUTEX_INITIALIZER;
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

static int WaitForChild(int parent_ptty, pid_t child_pid, int* child_status, std::ostream& out) {
  int status = 0;
  struct pollfd poll_fds[] = {
    [0] = {
      .fd = parent_ptty,
      .events = POLLIN,
    },
  };
  int rc = 0;
  bool found_child = false;

  while (!found_child) {
    if (TEMP_FAILURE_RETRY(poll(poll_fds, ARRAY_SIZE(poll_fds), -1)) < 0) {
      out.flush();
      return -1;
    }

    if (poll_fds[0].revents & POLLIN) {
      char buffer[4096];
      ssize_t bytesRead = TEMP_FAILURE_RETRY(read(parent_ptty, buffer, sizeof(buffer)));
      if (bytesRead > 0) {
        out.write(buffer, bytesRead);
      }
    }

    if (poll_fds[0].revents & POLLHUP) {
      int ret;

      ret = waitpid(child_pid, &status, WNOHANG);
      if (ret < 0) {
        return errno;
      }
      if (ret > 0) {
        found_child = true;
      }
    }
  }

  if (child_status != NULL) {
    *child_status = status;
  } else {
    if (WIFEXITED(status)) {
      rc = WEXITSTATUS(status);
    } else {
      rc = -ECHILD;
    }
  }

  return rc;
}

int ForkAndExec(const std::vector<std::string>& args, std::ostream& out)
{
  pid_t pid;
  int parent_ptty;
  int child_ptty;
  sigset_t blockset;
  sigset_t oldset;
  int rc = 0;
  int status = 0;

  rc = pthread_mutex_lock(&sMutex);
  if (rc) {
    return rc;
  }

  parent_ptty = TEMP_FAILURE_RETRY(open("/dev/ptmx", O_RDWR));
  if (parent_ptty < 0) {
    pthread_mutex_unlock(&sMutex);
    return -1;
  }

  char child_devname[64];
  if (grantpt(parent_ptty)
      || unlockpt(parent_ptty)
      || ptsname_r(parent_ptty, child_devname, sizeof(child_devname)) != 0) {
    close(parent_ptty);
    pthread_mutex_unlock(&sMutex);
    return -1;
  }

  child_ptty = TEMP_FAILURE_RETRY(open(child_devname, O_RDWR));
  if (child_ptty < 0) {
    close(parent_ptty);
    pthread_mutex_unlock(&sMutex);
    return -1;
  }

  sigemptyset(&blockset);
  sigaddset(&blockset, SIGQUIT);
  sigaddset(&blockset, SIGINT);
  pthread_sigmask(SIG_BLOCK, &blockset, &oldset);

  pid = fork();
  if (pid < 0) {
    close(child_ptty);
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    close(parent_ptty);
    pthread_mutex_unlock(&sMutex);
    return -1;
  } else if (!pid) {
    pthread_mutex_unlock(&sMutex);
    pthread_sigmask(SIG_SETMASK, &oldset, NULL);
    close(parent_ptty);

    dup2(child_ptty, 1);
    dup2(child_ptty, 2);
    close(child_ptty);
    ExecChild(args);
  } else {
    sChildPid = pid;
    close(child_ptty);
    rc = WaitForChild(parent_ptty, pid, &status, out);
    sChildPid = -1;
  }

  pthread_sigmask(SIG_SETMASK, &oldset, NULL);
  close(parent_ptty);
  pthread_mutex_unlock(&sMutex);
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