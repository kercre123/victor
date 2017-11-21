#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <termios.h>
#include <sys/time.h>

void enable_kbhit(bool enable)
{
  static struct termios oldt, newt;
  static bool active;

  if ( enable && !active)
  {
    tcgetattr( STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~( ICANON | ECHO );
    tcsetattr( STDIN_FILENO, TCSANOW, &newt);
    active = true;
  }
  else if (!enable && active) {
    tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
    active = false;
  }
}

int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;

  tv.tv_sec = 0;
  tv.tv_usec = 0;

  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);

  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);

}
