#ifndef PIDOPEN_H
#define PIDOPEN_H


int pidopen(const char* processname, const char* argstr, pid_t* pid_out);
int pidclose(pid_t pid, bool force);
int wait_for_data(int fd, int max_sec);

#endif//PIDOPEN_H
