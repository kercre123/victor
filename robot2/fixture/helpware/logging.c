#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "helpware/logging.h"

#define HELPER_LOGFILE "/data/local/fixture/fixture.log"

static struct {
  bool enabled;
  int fd;
} gLogging = {0};

int fixture_log_start(const char* params, int len)
{
  if (!gLogging.fd) {
    gLogging.fd = open(HELPER_LOGFILE, O_WRONLY|O_CREAT);
    gLogging.enabled = true;
    fixture_log_writestring(">>logstart ");
    fixture_log_write(params, len);
  }
  return 0; //no errors.
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
  if (gLogging.enabled) {
    gLogging.enabled = false;
    close(gLogging.fd);
    gLogging.fd = 0;
  }
  return 0;
}
