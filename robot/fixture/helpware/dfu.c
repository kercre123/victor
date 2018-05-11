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

#include "core/common.h"
#include "core/serial.h"


#define MAX_FIRMWARE_SZ 0xa0000
//#define FIXTURE_TTY "/dev/ttyHSL1"
#define FIXTURE_TTY "/dev/ttyHS0"
#define FIXTURE_BAUD B1000000




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



//Clean up open file handles and memory
void core_common_on_exit(void)
{
  if (gDFU.imageFd) {
    close(gDFU.imageFd);
    gDFU.imageFd = 0;
  }
  if (gDFU.serialFd) {
    serial_close(gDFU.serialFd);
    gDFU.serialFd = 0;
  }
  if (gDFU.safefile) {
    free((void*)gDFU.safefile);
    gDFU.safefile = 0;
  }
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

  dprintf("Initializing comms\n");

  gDFU.serialFd = serial_init(FIXTURE_TTY, FIXTURE_BAUD);

  //verify existing one first?

  dprintf("opening file\n");
  gDFU.imageFd = open(argv[1], O_RDONLY);
  if (gDFU.imageFd <=0) {
    error_exit(app_FILE_OPEN_ERROR, "Can't open '%s' (%d)", argv[1], errno);
  }
  int safefilesz = 0;
  gDFU.safefile = read_safefile(gDFU.imageFd, &safefilesz);

  // Attention
  dprintf("Sending escape\n");
  uint8_t esc[] = {27, 0};
  serial_write(gDFU.serialFd, esc, 1);
  serial_write(gDFU.serialFd, esc, 1);

  dprintf("Waiting for response\n");
  readTo(gDFU.serialFd, ">");


  //Exit Command Line mode
  dprintf("Sending Exit\n");
  serial_write(gDFU.serialFd, (uint8_t*)"Exit\n", 5);
  sleep(1); // 1 sec to drain buffers
  int n_read = 0;
  do {
    uint8_t tempbuf[1];
    n_read = serial_read(gDFU.serialFd, tempbuf, 1);
  }
  while (n_read > 0);

  // Download.
  dprintf("Sending new image\n");

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

  core_common_on_exit();
  return 0;
}



