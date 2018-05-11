#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <termios.h>
#include <errno.h>

#include "core/common.h"
#include "core/serial.h"

#define SERIAL_STREAM_DEBUG_IN 0
#define SERIAL_STREAM_DEBUG_OUT 0

/// Opens serial port at `devicename` with given `baud` enum.
/// Returns file descriptor.
int serial_init(const char* devicename, int baud)
{
  assert(devicename!=NULL);

  int serialFd = open(devicename, O_RDWR | O_NONBLOCK);
  if (serialFd <= 0) {
    error_exit(app_INIT_ERROR, "Can't init serial port at %s. (%d)",
               devicename, serialFd);
  }

  dprintf("configuring port %s (fd=%d)\n", devicename, serialFd);
  /* Configure device */
  {
    struct termios cfg;
    if (tcgetattr(serialFd, &cfg)) {
      close(serialFd);
      error_exit(app_IO_ERROR, "tcgetattr() failed");
    }
    cfmakeraw(&cfg);
    cfsetispeed(&cfg, baud);
    cfsetospeed(&cfg, baud);

    cfg.c_cflag |= (CS8 | CSTOPB);    // Use N82 bit words

    if (tcsetattr(serialFd, TCSANOW, &cfg)) {
      close(serialFd);
      error_exit(app_IO_ERROR, "tcsetattr() failed");
    }
  }
  return serialFd;
}

/// Writes `len` bytes from `buffer` to port `serial_fd`.
/// Returns count of bytes written.
int serial_write(int serial_fd, const uint8_t* buffer, int len)
{
#if SERIAL_STREAM_DEBUG_OUT
  dprintf("sending %d chars: ", len);
  int i;
  for (i = 0; i < len ; i++) {
    dprintf("%c", buffer[i]);
  }
  dprintf("\n");
#endif
  int r =  write(serial_fd, buffer, len);
  return r;
}

/// Attempts to read `len` bytes into `buffer` from port `serial_fd`.
/// Non-blocking.
/// Returns count of bytes recieved, or <0 on error.
int serial_read(int serial_fd, uint8_t* buffer, int len) //->bytes_rcvd
{
  int result = read(serial_fd, buffer, len);
  if (result < 0) {
    if (errno == EAGAIN) { //nonblocking no-data
      result = 0; //not an error
    }
    else {
      error_exit(app_IO_ERROR, "Serial read eror %d\n", errno);
    }
  }
#if SERIAL_STREAM_DEBUG_IN
  if (result > 0) {
    dprintf("rcvd %d chars: ",result);

    int i;
    for (i = 0; i < result; i++) {
      dprintf("%c", buffer[i]);
    }
    dprintf("\n");
  }
#endif
  return result;
}

void serial_close(int serialFd)
{
  close(serialFd);
}
