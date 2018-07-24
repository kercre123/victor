#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#include "../spine/spine_hal.h"
#include "schema/messages.h"

extern const struct SpineMessageHeader* hal_read_frame();


#define VERSTRING_LEN 16

#define DEBUG_DFU
#ifdef DEBUG_DFU
#define dprint printf
#else
#define dprint(s, ...)
#endif


#define USAGE_STRING "USAGE:\n%s [-v] <filename> [-f]\n"\
                     "\tDownloads file to syscon\n"\
                     "\t(-v to read current version and exit)\n"\
                     "\t(-f to force update to older version)\n"


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
  version_UNKNOWN,
  version_OLDER,
  version_OK,
  version_NEWER,
};


FILE* gImgFilep = NULL;


void core_common_on_exit(void)
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
  core_common_on_exit();
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

enum VersionStatus  CheckVersion(uint8_t desiredVersion[], const uint8_t firmwareVersion[])
{
  ShowVersion("Installed", firmwareVersion);
  ShowVersion("Provided", desiredVersion);

  // TODO: THIS ISN'T ACTUALLY CHECKING
  return version_OLDER;  // for now, always assume older than desired
}


int IsGoodAck(struct AckMessage* msg)
{
   if (msg->status <= 0) {
    dprint("NACK = %d\n", msg->status);
    return 0;
   }
   return 1;
}

const struct SpineMessageHeader* SendCommand(uint16_t ctype, const void* data, int size, int retries)
 {
  do {
    hal_send_frame(ctype, data, size);

    const struct SpineMessageHeader* hdr = hal_wait_for_frame(PAYLOAD_ACK);
    if (IsGoodAck((struct AckMessage*)(hdr + 1))) {
      return hdr;
    }
  }
  while (retries-- > 0);
  return NULL;
}


void SendData(FILE* imgfile, int start_addr)
{
  while (!feof(imgfile)) {

    struct WriteDFU packet = {0};
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

      if (SendCommand(PAYLOAD_DFU_PACKET, &packet, sizeof(packet), 1) == NULL) {
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

  hal_send_frame(PAYLOAD_VERSION, NULL, 0);

  const uint8_t* version_ptr = NULL;
  const struct SpineMessageHeader* hdr;
  do {
     hdr = hal_read_frame();
     if (hdr && hdr->payload_type == PAYLOAD_ACK) {
        if (!IsGoodAck((struct AckMessage*)(hdr + 1))) {
          version_ptr = (const uint8_t*)"-----Erased-----";
        }
     }
     else if (hdr && hdr->payload_type == PAYLOAD_VERSION) {
       version_ptr = ((struct VersionInfo*)(hdr + 1))->app_version;
     }
  } while (!version_ptr);

  if (*argv[1] == '-') {
     if (strncmp(argv[1], "-v",2) == 0) {
       ShowVersion("Installed", version_ptr);
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

  if (CheckVersion(versionString, version_ptr) >= version_OK) {
    printf("Already running target version or newer\n\tuse '-f' to force update\n");
    //exit unless "-f" force flag
    if (argc <2 || strncmp(argv[2],"-f",2) != 0) {
       exit(0);
    }
  }
  //else needs update



  dprint("erasing installed image\n");
  if (SendCommand(PAYLOAD_ERASE, NULL, 0, 1) == NULL) {
    error_exit(app_FLASH_ERASE_ERROR, "Can't erase syscon flash");
  }

  SendData(gImgFilep, 0);

  dprint("requesting validation\n");
  if (SendCommand(PAYLOAD_VALIDATE, NULL, 0, 0) == NULL) {
    error_exit(app_VALIDATION_ERROR, "Image Validation Failed");
  }

  printf("Success!\n");
  core_common_on_exit();
  return 0;
}
