#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <assert.h>
#include <termios.h>


#define MAX_FIRMWARE_SZ 0xa0000
#define FIXTURE_TTY "/dev/ttyHSL1"
#define FIXTURE_BAUD B1000000



#define DEBUG_DFU
#ifdef DEBUG_DFU
#define dprint printf
#else
#define dprint(s, ...)
#endif
#define SERIAL_STREAM_DEBUG_IN 1
#define SERIAL_STREAM_DEBUG_OUT 0


#define USAGE_STRING "USAGE:\n%s [-v] <filename> [-f]\n"  \
  "\tDownloads file to fixture stm-32\n"                  \
  "\t(-v to read current version and exit)\n"             \
  "\t(-f to force update to older version)\n"

// All the global resources to be cleaned up on exit
static struct {
  int serialFd;
  int imageFd;
  const uint8_t* safefile;
} gDFU = {0};


enum DfuAppErrorCode {
  app_SUCCESS = 0,
  app_USAGE = -1,
  app_FILE_OPEN_ERROR = -2,
  app_FILE_READ_ERROR = -3,
  app_SEND_DATA_ERROR = -4,
  app_INIT_ERROR = -5,
  app_FLASH_ERASE_ERROR = -6,
  app_VALIDATION_ERROR = -7,
  app_FILE_SIZE_ERROR = -8,
  app_MEMORY_ERROR = -9,
  app_IO_ERROR = -10,
};



//Clean up open file handles and memory
void on_exit(void)
{
  if (gDFU.imageFd) {
    close(gDFU.imageFd);
    gDFU.imageFd = 0;
  }
  if (gDFU.serialFd) {
    close(gDFU.serialFd);
    gDFU.serialFd = 0;
  }
  if (gDFU.safefile) {
    free((void*)gDFU.safefile);
    gDFU.safefile = 0;
  }
}

void error_exit(enum DfuAppErrorCode code, const char* msg, ...)
{
  va_list args;

  printf("ERROR %d: ", code);
  va_start(args, msg);
  vprintf(msg, args);
  va_end(args);
  printf("\n\n");
  on_exit();
  exit(code);
}


//Opens serial port at `devicename` with given `baud` enum. Returns file descriptor.
int serial_init(const char* devicename, int baud)
{
  assert(devicename!=NULL);
   
  int serialFd = open(devicename, O_RDWR | O_NONBLOCK);
  if (serialFd <= 0) {
    error_exit(app_INIT_ERROR, "Can't init serial port at %s. (%d)",
               FIXTURE_TTY, serialFd);
  }
  gDFU.serialFd = serialFd;

  /* Configure device */
  {
    struct termios cfg;
    if (tcgetattr(gDFU.serialFd, &cfg)) {
      error_exit(app_IO_ERROR, "tcgetattr() failed");
    }
    cfmakeraw(&cfg);

    cfsetispeed(&cfg, baud);
    cfsetospeed(&cfg, baud);

    cfg.c_cflag |= (CS8 | CSTOPB);    // Use N82 bit words

    printf("configuring port %s (fd=%d)\n", devicename, gDFU.serialFd);

    if (tcsetattr(gDFU.serialFd, TCSANOW, &cfg)) {
      error_exit(app_IO_ERROR, "tcsetattr() failed");
    }
  }
  return gDFU.serialFd;
}


int serial_write(int serial_fd, const uint8_t* buffer, int len)
{
#if SERIAL_STREAM_DEBUG_OUT
  printf("sending %d chars: ", len);
  int i;
  for (i = 0; i < len ; i++) {
    printf("%c", buffer[i]);
  }
  printf("\n");
#endif
  int r =  write(serial_fd, buffer, len);
  return r;
}

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

    int i;
    for (i = 0; i < result; i++) {
      printf("%c", buffer[i]);
    }
  }
#endif
  return result;
}




const uint8_t* read_safefile(int safefp, int* szOut)
{
  uint8_t imagebuffer[MAX_FIRMWARE_SZ];
  int n_read = read(safefp, imagebuffer, MAX_FIRMWARE_SZ);
  if (n_read < 0) {
    error_exit(app_FILE_READ_ERROR, "Error reading file : %d", errno);
  }
  uint8_t eof;
  int remaining_bytes = read(safefp, &eof, 1);
  if (remaining_bytes) {
    error_exit(app_FILE_SIZE_ERROR, "Image File too big ( %d > %d) ", n_read, MAX_FIRMWARE_SZ);
  }

  uint8_t* buffer = malloc(n_read);
  if (!buffer) {
    error_exit(app_MEMORY_ERROR, "Error allocating memory (%d bytes)", n_read);
  }
  memcpy(buffer, imagebuffer, n_read);
  *szOut = n_read;
  return buffer;
}



int readAck(int serialFd)
{

  uint8_t ackByte = 0;
  int nread = 0;
  while (nread < 1) {
    nread = serial_read(serialFd, &ackByte, 1);
  }
  return ('1' == ackByte);
}


//Reads chars from `serialFd` until all chars in target` string matched.
//Returns total number read.
int readTo(int serialFd, const char* target)
{
  assert(serialFd > 0);
  assert(target != NULL);
  int count = 0;
  while (*target) {
    char data;
    int nread = serial_read(serialFd, (uint8_t*)&data, 1);
    if (nread == 1 && data == *target) {
      target++;
    }
    count += nread;
  }
  return count;
}


int main(int argc, const char* argv[])
{
  if (argc <= 1) {
    error_exit(app_USAGE, USAGE_STRING, argv[0]);
  }

  dprint("Initializing comms\n");

  serial_init(FIXTURE_TTY, FIXTURE_BAUD);

  //verify existing one first?

  dprint("opening file\n");
  gDFU.imageFd = open(argv[1], O_RDONLY);
  if (!gDFU.imageFd) {
    error_exit(app_FILE_OPEN_ERROR, "Can't open %s", argv[1]);
  }
  int safefilesz = 0;
  gDFU.safefile = read_safefile(gDFU.imageFd, &safefilesz);


  // Attention
  dprint("Sending escape\n");
  char esc[] = {27, 0};
  serial_write(gDFU.serialFd, esc, 1);
  serial_write(gDFU.serialFd, esc, 1);

  dprint("Waiting for response\n");
  readTo(gDFU.serialFd, ">");


  //Exit Command Line mode
  dprint("Sending Exit\n");
  serial_write(gDFU.serialFd, "Exit\n", 5);
  sleep(1); // 1 sec to drain buffers
  int n_read = 0;
  do {
    uint8_t tempbuf[1];
    n_read = serial_read(gDFU.serialFd, tempbuf, 1);
  }
  while (n_read > 0);

  // Download.
  dprint("Sending new image\n");

  // Erase is 7.8s worst case, 6.5s typical case at voltage level 2 * 5 128KB blocks
  //TODOO : add this to serial open code              port.ReadTimeout = 13000;

  // Send Tag A
  int index;
  for (index = 0; index < 4; index++) {
    serial_write(gDFU.serialFd, gDFU.safefile + index, 1);
    if (!readAck(gDFU.serialFd)) {
      error_exit(app_VALIDATION_ERROR, "HeadAck %d\n", index);
    }

  }
  // Send the remaining data blocks
  while (index < safefilesz) {
    int count = 2048 + 32;
    if (index == 4) {
      count -= 4;  // Account for Tag A being sent first
    }
    serial_write(gDFU.serialFd, gDFU.safefile + index, count);
    index += count;
    if (!readAck(gDFU.serialFd)) {
      error_exit(app_VALIDATION_ERROR, "BlockAck %d\n", index);
    }
  }
  printf("Flash successful\n");

  on_exit();
  return 0;
}



