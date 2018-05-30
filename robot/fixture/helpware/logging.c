#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

#include <sys/stat.h>

#include "core/clock.h"
#include "helpware/logging.h"

#define LOGFILE_PATH "/data/local/fixture/logs/"
#define SEQFILE      LOGFILE_PATH "sequence.id"
#define HELPER_LOGFILE LOGFILE_PATH "fixture%04d.log"
#define MAX_LOGFILE_NAMELEN (sizeof(HELPER_LOGFILE)+9)

#define FIXTURE_LOG_NOT_OPEN  432  //TODO from global error allocation
#define LOG_BUFSZ 256

static struct {
  int fd;
  bool enabled;
  int start_cnt;
  uint64_t start_time;
  int ccnt; //character count within the current line
} gLogging = {-1,0,0};


unsigned int get_sequence_number(const char* seqfile)
{
  int fd = open(seqfile, O_RDWR|O_CREAT);
  if (fd<0) { return 0; }
  unsigned int seqnum;
  if (read(fd, &seqnum,sizeof(seqnum)) != sizeof(seqnum))  {
    seqnum = 0;
  }
  ++seqnum;
  lseek(fd, 0, SEEK_SET);
  write(fd, &seqnum, sizeof(seqnum));
  close(fd);
  return seqnum;
}


int fixture_log_init(void)
{
  if (gLogging.fd < 0) {
    memset(&gLogging, 0, sizeof(gLogging));
    int n = get_sequence_number(SEQFILE);
    char filename[MAX_LOGFILE_NAMELEN];

    struct stat st={0};
    if (stat(LOGFILE_PATH, &st)==-1) {
      mkdir(LOGFILE_PATH, 0x700);
    }
    snprintf(filename, MAX_LOGFILE_NAMELEN, HELPER_LOGFILE, n);
    gLogging.fd = open(filename, O_WRONLY|O_CREAT);

    if( gLogging.fd >= 0 )
      write(gLogging.fd, "helper build " __DATE__ " " __TIME__ "\n", strlen("helper build " __DATE__ " " __TIME__ "\n"));
  }
  return (gLogging.fd < 0);
}

void fixture_log_terminate(void)
{
  if (gLogging.fd >= 0) {
    fixture_log_stop("",0);
    close(gLogging.fd);
    gLogging.fd = -1;
  }
}

int fixture_log_start(const char* params, int len)
{
  char buf[100];

  if (gLogging.fd >= 0) {
    if( !gLogging.enabled ) {
      gLogging.enabled = true;
      snprintf(buf, sizeof(buf), "\n------------------------------ log start %i ------------------------------\n", ++gLogging.start_cnt);
      write(gLogging.fd, buf, strlen(buf));

      gLogging.ccnt = 0;
      gLogging.start_time = steady_clock_now(); //beginning of log time
      fixture_log_writestring(">>logstart ");
      fixture_log_write(params, len);
    }
    return 0; //no errors.
  }
  return FIXTURE_LOG_NOT_OPEN;
}

void fixture_log_writestring(const char* textstring)
{
  fixture_log_write(textstring, strlen(textstring));
}

void fixture_log_write(const char* textstring, int len)
{
  static char logbuf[LOG_BUFSZ];

  if (gLogging.enabled)
  {
    //parse chunky stream into smooth lines
    while( len )
    {
      bool forcewrap = 0;
      char* endl = memchr( textstring, '\n', len);
      int oplen = !endl ? len : endl - textstring + 1; //up to and including newline
      if( oplen + gLogging.ccnt > LOG_BUFSZ ) //force line wrap if we overflow buffer
        forcewrap = 1, oplen = LOG_BUFSZ - gLogging.ccnt;

      //append to buffer
      memcpy( &logbuf[gLogging.ccnt], textstring, oplen );
      gLogging.ccnt += oplen;
      textstring += oplen;
      len -= oplen;

      //Dump full lines to the logfile
      if( forcewrap || logbuf[gLogging.ccnt-1] == '\n' )
      {
        //prepend timestamp
        char prefix[20];
        uint64_t time_us = (steady_clock_now() - gLogging.start_time)/1000;
        snprintf(prefix, sizeof(prefix), "[%03u.%06u] ", (uint32_t)(time_us/1000000), (uint32_t)(time_us%1000000) );
        write(gLogging.fd, prefix, strlen(prefix));

        //write buffered line
        write(gLogging.fd, logbuf, gLogging.ccnt);
        gLogging.ccnt = 0;
        if( forcewrap )
          write(gLogging.fd, "\n", 1);
      }

    }
  }
}

int fixture_log_stop(const char* params, int len)
{
  if( gLogging.enabled && gLogging.ccnt ) //flush log buffer
    fixture_log_write("\n",1);

  gLogging.enabled = false;
  return 0;
}
