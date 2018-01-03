#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "helpware/logging.h"

#define LOGFILE_PATH "/data/local/fixture/logs/"
#define SEQFILE      LOGFILE_PATH "sequence.id"
#define HELPER_LOGFILE LOGFILE_PATH "fixture%04d.log"
#define MAX_LOGFILE_NAMELEN (sizeof(HELPER_LOGFILE)+9)

#define FIXTURE_LOG_NOT_OPEN  432  //TODO from global error allocation

static struct {
  bool enabled;
  int fd;
  int start_cnt;
} gLogging = {0,0,0};


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
  if (!gLogging.fd) {
    int n = get_sequence_number(SEQFILE);
    char filename[MAX_LOGFILE_NAMELEN];

    struct stat st={0};
    if (stat(LOGFILE_PATH, &st)==-1) {
      mkdir(LOGFILE_PATH, 0x700);
    }
    snprintf(filename, MAX_LOGFILE_NAMELEN, HELPER_LOGFILE, n);
    gLogging.fd = open(filename, O_WRONLY|O_CREAT);
  }
  return (gLogging.fd == 0);
}

void fixture_log_terminate(void)
{
  if (gLogging.fd) {
    fixture_log_stop("",0);
    close(gLogging.fd);
    gLogging.fd = 0;
  }
}

int fixture_log_start(const char* params, int len)
{
  char buf[100];
  
  if (gLogging.fd) {
    if( !gLogging.enabled ) {
      gLogging.enabled = true;
      snprintf(buf, sizeof(buf), "\n------------------------------ log start %i ------------------------------\n", ++gLogging.start_cnt);
      fixture_log_writestring(buf);
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
  if (gLogging.enabled) {
    write(gLogging.fd, textstring, len);
  }
}

int fixture_log_stop(const char* params, int len)
{
  gLogging.enabled = false;
  return 0;
}
