#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <sys/time.h>

#include "core/common.h"
#include "core/serial.h"
#include "core/clock.h"
#include "core/lcd.h"
#include "helpware/helper_text.h"
#include "helpware/display.h"
#include "helpware/logging.h"


#define FIXTURE_TTY "/dev/ttyHSL1"
#define FIXTURE_BAUD B1000000


#define LINEBUFSZ 255

int wait_for_data(int fd, int max_sec) {
  fd_set fdset;
  struct timeval timeout = {max_sec,0};
  FD_ZERO(&fdset);
  FD_SET(fd, &fdset);
  int ret = select(1,&fdset,NULL,NULL,&timeout);
  return (ret>0);
}



int shellcommand(const char* command, int timeout_sec) {
  int retval = -666;
  uint64_t expiration = steady_clock_now()+(timeout_sec*NSEC_PER_SEC);

  fixture_log_writestring("-BEGIN SHELL- ");
  fixture_log_writestring(command);
  fixture_log_writestring("\n");

  FILE* pp = popen("./headprogram", "r");
  if (pp) {
    int pfd = fileno(pp);

    char buffer[512];
    while (!feof(pp)) {
      if (wait_for_data(pfd, 1)) { //wait 1 second
        if (fgets(buffer, 512, pp) != NULL) {
          printf("%s", buffer);
          fixture_log_writestring(buffer);
        }
      }
      else if (steady_clock_now() > expiration) {
        printf("TIMEOUT after %d sec\n", timeout_sec);
        fixture_log_writestring("TIMEOUT");
        break;
      }
    }
    retval = pclose(pp);
  }

  fixture_log_writestring("--END SHELL--");
  fixture_log_writestring(command);
  fixture_log_writestring("\n");

  return retval;

}

int handle_lcdset_command(const char* cmd, int len) {
  return helper_lcdset_command_parse(cmd, len);
}
int handle_lcdshow_command(const char* cmd, int len) {
  return helper_lcdshow_command_parse(cmd, len);
}
int handle_lcdclr_command(const char* cmd, int len) {
  //"clr" is same as "set 0"
  printf("handle lcdclr\n");
  return helper_lcdset_command_parse("0 \n", 3);
}
int handle_logstart_command(const char* cmd, int len) {
  return fixture_log_start(cmd, len);
}
int handle_logstop_command(const char* cmd, int len) {
  return fixture_log_stop(cmd, len);
}



int handle_dutprogram_command(const char* cmd, int len) {
  char* num_end;
  long timeout_sec = strtol(cmd, &num_end, 10);
  printf("timeout = %ld\n", timeout_sec);
  if (num_end == cmd || timeout_sec == 0) {
    timeout_sec = LONG_MAX;
  }
  return shellcommand("./headprogram", timeout_sec);

}


#define REGISTER_COMMAND(s) {#s, sizeof(#s)-1, handle_##s##_command}


typedef int (*CommandParser)(const char*, int);

typedef struct CommandHandler_t {
  const char* name;
  const int len;
  const CommandParser handler;
} CommandHandler;

static const CommandHandler handlers[] = {
  REGISTER_COMMAND(lcdset),
  REGISTER_COMMAND(lcdshow),
  REGISTER_COMMAND(lcdclr),
  REGISTER_COMMAND(logstart),
  REGISTER_COMMAND(logstop),
  REGISTER_COMMAND(dutprogram),
 /* ^^ insert new commands here ^^ */
  {0}
};

const char* fixture_command_parse(const char*  command, int len) {
  static char responseBuffer[LINEBUFSZ];

//  printf("parsing \"%s\"\n", command);

  const CommandHandler* candidate = &handlers[0];

  while (candidate->name) {
    if (len >= candidate->len &&
        strncmp(command, candidate->name, candidate->len)==0)
    {
      int status = candidate->handler(command+candidate->len, len-candidate->len);
      snprintf(responseBuffer, LINEBUFSZ, "<<%s %d\n", candidate->name, status);
      return responseBuffer;
    }
    candidate++;
  }
  printf("NO MATCH");
  //not recognized, echo back invalid command with error code
  char* endcmd = memchr(command, ' ', len);
  if (endcmd) { len = endcmd - command; }
  int i;
  responseBuffer[0]='<';
  responseBuffer[1]='<';
  for (i=0;i<len;i++)
  {
    responseBuffer[2+i]=*command++;
  }
  snprintf(responseBuffer+i, LINEBUFSZ-i, " %d\n", -1);
  return responseBuffer;

}

const char* find_line(const char* buf, int buflen, const char** last)
{
  if (!buflen) {return NULL;}
//  printf("looking for newline in \"%.*s\" (%d)\n",
//         buflen, *last, buflen);
//  printf("%x + %x <=? %x\n", buf, buflen, *last);
  assert(buf <= *last && *last <= buf+buflen);
  int remaining = buflen - (*last - buf);
  const char* token = memchr(*last, '\n', remaining);
  if (token) {
//     printf("found at %d\n", token-*last);
    *last = token+1;
    return buf;
  }
//  printf("not found\n");
  return NULL;
}

int fixture_serial(int serialFd) {
  static char linebuf[LINEBUFSZ+1];
  const char* response = NULL;
  static int linelen = 0;
  int nread = serial_read(serialFd, (uint8_t*)linebuf+linelen, LINEBUFSZ-linelen);
  if (nread<=0) { return 0; }
  const char* endl = linebuf+linelen;


  printf("%.*s", nread, linebuf+linelen);
  fixture_log_write(linebuf+linelen, nread);

  linelen+=nread;
  if (linelen >= LINEBUFSZ)
  {
    printf("TOO MANY CHARACTERS, truncating to %d\n", LINEBUFSZ);
    linelen = LINEBUFSZ;
    linebuf[linelen] = '\n';
  }
  const char* line = find_line(linebuf, linelen, &endl);
  while (line) {
    if (line[0]=='>' && line[1]=='>') {
      response = fixture_command_parse(line+2, endl-line-2);
      if (response) {
        printf("%s", response);
        fixture_log_writestring(response);
        serial_write(serialFd, (uint8_t*)response, strlen(response));
      }
    }
    linelen-=(endl-line);
    line = find_line(endl, linelen, &endl);
  }
  if (linelen) {
    memmove(linebuf, endl, linelen);
  }
  return 0;
}


static int gSerialFd;


#define LINEBUFSZ 255




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

int user_terminal(void) {
  static int linelen = 0;
  static char linebuf[LINEBUFSZ+1];
  if (kbhit()) {
    int nread = read(0, linebuf+linelen, LINEBUFSZ-linelen);
    if (nread<0) { return 1; }
    int i;
    for (i=0;i<nread;i++) {
      putchar(linebuf[linelen+i]);
    }
    fflush(stdout);
    serial_write(gSerialFd, (uint8_t*)linebuf+linelen, nread);

    char* endl = memchr(linebuf+linelen, '\n', nread);
    if (!endl) {
      linelen+=nread;
      if (linelen >= LINEBUFSZ)
      {
        printf("TOO MANY CHARACTERS, truncating to %d\n", LINEBUFSZ);
        endl = linebuf+LINEBUFSZ-1;
        *endl = '\n';
      }
    }
    if (endl) {
      if (strncmp(linebuf, "quit", 4)==0)  {
        return 1;
      }
      linelen = 0;
    }
  }
  return 0;
}


void on_exit(void)
{
  if (gSerialFd) {
    close(gSerialFd);
  }
  fixture_log_terminate();
  enable_kbhit(0);
}


void safe_quit(int n)
{
  error_exit(app_USAGE, "Caught signal %d \n", n);
}


int main(int argc, const char* argv[])
{
  bool exit = false;

  signal(SIGINT, safe_quit);

  lcd_init();
  lcd_set_brightness(20);
  display_init();
  fixture_log_init();

  gSerialFd = serial_init(FIXTURE_TTY, FIXTURE_BAUD);



  serial_write(gSerialFd, (uint8_t*)"\x1b\x1b\n", 4);
  serial_write(gSerialFd, (uint8_t*)"reset\n", 6);


  enable_kbhit(1);
  while (!exit)
  {
    exit = fixture_serial(gSerialFd);
    exit |= user_terminal();
 }

  on_exit();

  return 0;


}
