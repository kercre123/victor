#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/wait.h>

#include "core/common.h"
#include "core/serial.h"
#include "core/clock.h"
#include "core/lcd.h"
#include "helpware/helper_text.h"
#include "helpware/display.h"
#include "helpware/logging.h"
#include "helpware/pidopen.h"


#define FIXTURE_TTY "/dev/ttyHSL1"
#define FIXTURE_BAUD B1000000


#define LINEBUFSZ 1024

static int gSerialFd;

#define SEND_CONSOLE   0x1
#define SEND_LOG       0x2
#define SEND_SERIAL    0x4
#define SEND_CL        (SEND_CONSOLE | SEND_LOG) /*most incomming & error data is sent to console + log*/
#define SEND_CLS       (SEND_CONSOLE | SEND_LOG | SEND_SERIAL) /*most outgoing data is sent to all 3 targets*/
void write_multiple(int targets, const char* textstring, int len)
{
  if( targets & SEND_CONSOLE ) {
    printf("%.*s", len, textstring);
    fflush(stdout);
  }
  if( targets & SEND_LOG ) {
    fixture_log_write(textstring, len);
  }
  if( (targets & SEND_SERIAL) && gSerialFd >= 0 ) {
    serial_write(gSerialFd, (uint8_t*)textstring, len);
  }
}

int printf_multiple(int targets, const char* format, ...)
{
  va_list argptr;
  va_start(argptr, format);
  
  char buf[256];
  int formatlen = vsnprintf(buf, sizeof(buf), format, argptr);
  int validlen = formatlen < sizeof(buf) ? formatlen : sizeof(buf); //MIN(formatlen, sizeof(buf))
  va_end(argptr);
  
  if( validlen > 0 ) //neg on encoding err
    write_multiple(targets, buf, validlen );
  
  if( validlen < formatlen ) {
    const char err_msg[] = "--PRINTF_MULTIPLE BUFFER OVERFLOW--\n";
    write_multiple(SEND_CL, err_msg, sizeof(err_msg));
  }
  
  return formatlen;
}

int shellcommand(const char* command, int timeout_sec) {
  int retval = -666;
  uint64_t expiration = steady_clock_now()+(timeout_sec*NSEC_PER_SEC);

  printf_multiple(SEND_CL, "-BEGIN SHELL- \"%s\" (%is)\n", command, timeout_sec);
  
  int pid;
  int pfd = pidopen("./headprogram", &pid);
  bool timedout = false;

  if (pfd>0) {
    uint64_t scnow = steady_clock_now();
    char buffer[512];
    int n;
    do {
      if (wait_for_data(pfd, 0)) {
        n = read(pfd, buffer, 512);
        if (n>0) {
          printf("%.*s", n, buffer);
          fixture_log_write(buffer,n);

        }
      }
      scnow = steady_clock_now();
      if (scnow > expiration) {
        printf("TIMEOUT after %d sec\n", timeout_sec);
        fixture_log_writestring("TIMEOUT ");
        timedout = true;
        break;
      }
    } while (n>0 );
    retval = pidclose(pid, timedout);
  }

  printf_multiple(SEND_CL, "--END SHELL-- \"%s\"\n", command);
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
  return helper_lcdset_command_parse("0 \n", 3);
}
int handle_logstart_command(const char* cmd, int len) {
  return fixture_log_start(cmd, len);
}
int handle_logstop_command(const char* cmd, int len) {
  return fixture_log_stop(cmd, len);
  return 0;
}

//#define STATIC_ASSERT(COND,MSG) typedef char static_assertion_##MSG[(COND)?1:-1]
//STATIC_ASSERT(LONG_MAX > 0xFFFFffff); //require >32-bit long for parsing u32 nums with strtol()

int handle_dutprogram_command(const char* cmd, int len) {
  char *end;
  const char *next = cmd;
  
  //first arg is timeout value in seconds (%u)
  errno = 0;
  long timeout_sec = strtol(next, &end, 10);
  if( errno != 0 || end <= next || timeout_sec > INT_MAX || timeout_sec < 1 /*INT_MIN*/ ) {
    printf("timeout = %ld, errno = %d, end = next+%d\n", timeout_sec, errno, end-next );
    timeout_sec = INT_MAX;
    return 2; //report formatting error
  }
  printf("timeout = %ds\n", (int)timeout_sec);
  len -= (end-next);
  next = end;
  
  //2nd arg is 32-bit esn (%08x)
  errno = 0;
  unsigned long esn = len > 0 ? strtoul(next, &end, 16) : 0;
  if( errno != 0 || end <= next || esn > 0xFFFFffff || esn < 1 ) {
    printf("esn = %lx, errno = %d, end = next+%d\n", esn, errno, end-next );
    esn = 0;
    return 2; //report formatting error
  }
  printf("esn = %08x\n", (uint32_t)esn);
  len -= (end-next);
  next = end;
  
  char dutcmd[25];
  snprintf(dutcmd, sizeof(dutcmd), "./headprogram %08x", (uint32_t)esn);
  return shellcommand(dutcmd, (int)timeout_sec);
}

int handle_shell_timeout_test_command(const char* cmd, int len) {
  //return handle_dutprogram_command(cmd, len);
  printf("shell test disabled\n"); fflush(stdout);
  fixture_log_writestring("shell test disabled\n");
  return 0;
}

int read_file_(const char* filepath, char* out_buf, int buf_sz, int* out_len)
{
  *out_len = 0;
  out_buf[0] = '\0';
  int fd = open(filepath, O_RDONLY);
  
  if (fd > 0) {
    *out_len = read(fd, out_buf, buf_sz);
    close(fd);
  } else {
    printf_multiple(SEND_CL, "file open error: '%s' errno=%i fd=%i\n", filepath, errno, fd);
    return errno;
  }
  
  return 0;
}

#define READ_FILE_BUF_SZ 40
char* read_file_line1_(const char* filepath)
{
  static char buf[READ_FILE_BUF_SZ+1];
  
  int rlen = 0;
  int err = read_file_(filepath, buf, READ_FILE_BUF_SZ, &rlen);
  buf[READ_FILE_BUF_SZ] = '\0';
  
  if( err == 0 )
  {
    int linelen = 0;
    if( rlen > 0 ) {
      char *s = buf;
      while( *s > 0x1f && *s < 0x7e ) { linelen++; s++; } //stop at non-printable char or EOL
    }
    buf[linelen] = '\0';
    return buf;
  }
  
  return NULL;
}

int handle_get_emmcdl_ver_command(const char* cmd, int len)
{
  char* version = read_file_line1_("/data/local/fixture/emmcdl/version");
  printf_multiple(SEND_CLS, ":%s\n", (version ? version : "file-error"));
  return 0;
}

int handle_get_temperature_command(const char* cmd, int len)
{
  //>>get_temperature 4 0 1 ... [zone list]
  //cat thermal_zone*/type to see a list of valid zones
  //printf("DEBUG,GET-TEMP: cmd=%08x, len=%i, cmd+len=%08x\n", (uint32_t)cmd, len, (uint32_t)(cmd+len) );
  //printf("DEBUG,GET-TEMP: '%.*s' (%i)\n", len, cmd, len);
  
  char* next = (char*)cmd;
  while(next < cmd+len) //each argument is zone # to report
  {
    errno = 0;
    char *endptr;
    int zone = strtol(next, &endptr, 10); //enforce base10
    //printf("DEBUG,GET-TEMP: zone=%i, next=%08x, endptr-delta=%i, errno=%i\n", zone, (uint32_t)next, (uint32_t)(endptr-next), errno);
    
    char *temperature = NULL;
    if( errno == 0 && zone >= 0 && zone <= 999 && endptr > next ) {
      char filepath[50];
      snprintf(filepath, sizeof(filepath), "/sys/devices/virtual/thermal/thermal_zone%u/temp", zone);
      temperature = read_file_line1_(filepath);
    } else {
      zone = -1;
    }
    printf_multiple(SEND_CLS, ":%i %s\n", zone, (temperature ? temperature : "-1"));
    next = endptr > next ? endptr : next+1;
  }
  
  //fflush(stdout);
  return 0;
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
  REGISTER_COMMAND(shell_timeout_test),
  {"shell-timeout-test", 18, handle_shell_timeout_test_command},
  REGISTER_COMMAND(get_emmcdl_ver),
  REGISTER_COMMAND(get_temperature),
 /* ^^ insert new commands here ^^ */
  {0}
};

const char* fixture_command_parse(const char*  command, int len) {
  static char responseBuffer[LINEBUFSZ];

//  printf("\tparsing  \"%.*s\"\n", len, command);
  if(len > 1 && command[len-1]=='\n') //strip trailing newline
    len--;

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
  snprintf(responseBuffer+2+i, LINEBUFSZ-2-i, " %d\n", -1);
  return responseBuffer;

}

const char* find_line(const char* buf, int buflen, const char** last)
{
  if (!buflen) {return NULL;}
  assert(buf <= *last && *last <= buf+buflen);
  int remaining = buflen - (*last - buf);
  const char* token = memchr(*last, '\n', remaining);
  if (token) {
    *last = token+1;
    return buf;
  }
  return NULL;
}

int linecount = 0 ;
int fixture_serial(int serialFd) {
  static char linebuf[LINEBUFSZ+1];
  const char* response = NULL;
  static int linelen = 0;
  int nread = serial_read(serialFd, (uint8_t*)linebuf+linelen, LINEBUFSZ-linelen);
  if (nread<=0) { return 0; }
  const char* endl = linebuf+linelen;


  printf("%.*s", nread, linebuf+linelen);
  fflush(stdout);
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
    linecount++;
    if (line[0]=='>' && line[1]=='>') {
      response = fixture_command_parse(line+2, endl-line-2);
      if (response) {
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
      if (linebuf[0]=='>'&& linebuf[1]=='>') {
        const char * response = fixture_command_parse(linebuf+2, endl-linebuf-2);
        if (response) {
          printf("~%s", response);
        }
      }
      linelen = 0;
    }
  }
  return 0;
}


void on_exit(void)
{
  if (gSerialFd >= 0) {
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
  signal(SIGKILL, safe_quit);

  lcd_init();
  lcd_set_brightness(20);
  display_init();
  fixture_log_init();

  gSerialFd = serial_init(FIXTURE_TTY, FIXTURE_BAUD);

  serial_write(gSerialFd, (uint8_t*)"\x1b\x1b\n", 4);
  serial_write(gSerialFd, (uint8_t*)"reset\n", 6);

  enable_kbhit(1);
  
  //process bootup
  exit = fixture_serial(gSerialFd);
  exit |= user_terminal();
  printf("helper build " __DATE__ " " __TIME__ "\n");
  fflush(stdout);
  
  while (!exit)
  {
    exit = fixture_serial(gSerialFd);
    exit |= user_terminal();
 }

  on_exit();

  return 0;


}
