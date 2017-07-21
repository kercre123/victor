#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>

#include "../spine/spine_hal.h"
#include "clad/spine/spine_protocol.h"
#define VERSTRING_LEN 16

#define DEBUG_DFU
#ifdef DEBUG_DFU
#define dprint printf
#else
#define dprint(s, ...)
#endif


#define USAGE_STRING "USAGE:\n%s [-v] <filename>\n"\
                     "\tDownloads file to syscon\n"\
                     "\t(-v to read current version)\n"


void printhex(const uint8_t* bytes, int len)
{
  while (len-- > 0) {
    printf("%02x", *bytes++);
  }
}


enum DfuAppErrorCode {
  app_SUCCESS = 0,
  app_USAGE = -1,
  app_FILE_OPEN_ERROR = -2,
  app_FILE_READ_ERROR = -3,
  app_SEND_DATA_ERROR = -4,
  app_HAL_INIT_ERROR = -5,
  app_FLASH_ERASE_ERROR = -6,
  app_VALIDATION_ERROR = -7,
};

enum VersionStatus {
  version_OK,
  version_OLDER,
  version_NEWER,
  version_UNKNOWN,
};


FILE* gImgFilep = NULL;


void on_exit(void)
{
   if (gImgFilep) {
      fclose(gImgFilep);
      gImgFilep = NULL;
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


void ShowVersion(const char* typename, const uint8_t version[])
{
  int i;
  printf("%s Version = ", typename);
  printhex(version, VERSTRING_LEN);
  printf(" [");
  for (i = 0; i < VERSTRING_LEN; i++) {
    putchar(isprint(version[i]) ? version[i] : '*');
  }
  printf("]\n");
}

enum VersionStatus  CheckVersion(uint8_t desiredVersion[], VersionInfo* firmwareVersion)
{
  ShowVersion("Installed", firmwareVersion->app_version);
  ShowVersion("Provided", desiredVersion);

  // TODO: THIS ISN'T ACTUALLY CHECKING
  return version_OLDER;  // for now, always assume older than desired
}


int IsGoodAck(AckMessage* msg)
{
  if (msg->status != Ack_ACK_PAYLOAD) {
    dprint("NACK = %d\n", msg->status);
    return 0;
  }
  return 1;
}

const SpineMessageHeader* SendCommand(uint16_t ctype, void* data, int size, int retries)
{
  do {
    hal_send_frame(ctype, data, size);

    const SpineMessageHeader* hdr = hal_get_frame(PayloadId_PAYLOAD_ACK);
    if (IsGoodAck((AckMessage*)(hdr + 1))) {
      return hdr;
    }
  }
  while (retries-- > 0);
  return NULL;
}


void SendData(FILE* imgfile, int start_addr)
{
  while (!feof(imgfile)) {

    WriteDFU packet = {0};
    size_t itemcount = fread(&(packet.data[0]), sizeof(uint32_t), 256, imgfile);
    size_t databytes = itemcount * sizeof(uint32_t);
    dprint("read %zd words (%zd bytes)\n", itemcount, databytes);

    if (itemcount < 256) {
      if (ferror(imgfile)) {
        error_exit(app_FILE_READ_ERROR, "imgfile read error: %s", strerror(errno));
      }
    }

    if (itemcount  > 0) {
      packet.address = start_addr;
      packet.wordCount = itemcount;

      dprint("writing %d words (%zd bytes) for @%x\n", packet.wordCount, databytes, packet.address);
      dprint("\t[%x ...  %x]\n", packet.data[0], packet.data[packet.wordCount - 1]);

      if (SendCommand(PayloadId_PAYLOAD_DFU_PACKET, &packet, sizeof(packet), 1) == NULL) {
        error_exit(app_SEND_DATA_ERROR, "Syscon won't accept DFU Payload");
      }
      start_addr += databytes;
    }
  }
  return;
}


int main(int argc, const char* argv[])
{
  if (argc <= 1) {
    error_exit(app_USAGE, USAGE_STRING, argv[0]);
  }

  dprint("Initializing spine\n");

  SpineErr result = hal_init(SPINE_TTY, SPINE_BAUD);
  if (result != err_OK) {
    error_exit(app_HAL_INIT_ERROR, "Can't init spine hal. (%d)", result);
  }

  dprint("requesting installed version\n");

  hal_send_frame(PayloadId_PAYLOAD_VERSION, NULL, 0);

  const SpineMessageHeader* hdr;
  hdr = hal_get_frame(PayloadId_PAYLOAD_VERSION);

  if (*argv[1] == '-') {
    if (strcmp(argv[1], "-v") == 0) {
      ShowVersion("Installed", ((VersionInfo*)(hdr + 1))->app_version);
    }
    return 0;
  }

  dprint("opening file\n");
  gImgFilep = fopen(argv[1], "rb");
  if (!gImgFilep) {
    error_exit(app_FILE_OPEN_ERROR, "Can't open %s", argv[1]);
    exit(-2);
  }

  dprint("reading image version\n");
  uint8_t versionString[VERSTRING_LEN];
  size_t nchars = fread(versionString, sizeof(uint8_t), VERSTRING_LEN, gImgFilep);
  if (nchars != VERSTRING_LEN) {
    error_exit(app_FILE_READ_ERROR, "Can't read from data file %s", argv[1]);
  }

  if (CheckVersion(versionString, (VersionInfo*)(hdr + 1)) == version_OK) {
    printf("Already running target version\n\n");
    exit(0);
  }
  //else needs update

  dprint("erasing installed image\n");
  if (SendCommand(PayloadId_PAYLOAD_ERASE, NULL, 0, 1) == NULL) {
    error_exit(app_FLASH_ERASE_ERROR, "Can't erase syscon flash");
  }

  SendData(gImgFilep, 0);

  dprint("requestig validation\n");
  if (SendCommand(PayloadId_PAYLOAD_VALIDATE, NULL, 0, 0) == NULL) {
    error_exit(app_VALIDATION_ERROR, "Image Validation Failed");
  }

  printf("Success!\n");
  on_exit();
  return 0;
}
