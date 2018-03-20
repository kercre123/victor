 /* #include <stdbool.h> */
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>
/* #include <string.h> */

#include "schema/messages.h"
#include "spine_crc.h"
#include "spine_hal.h"

#define SKIP_CRC_CHECK 0

#define SPINE_MAX_BYTES 1280

#define BODY_TAG_PREFIX ((uint8_t*)&SyncKey)
#define SPINE_TAG_LEN sizeof(SpineSync)
#define SPINE_PID_LEN sizeof(PayloadId)
#define SPINE_LEN_LEN sizeof(uint16_t)
#define SPINE_HEADER_LEN (SPINE_TAG_LEN+SPINE_PID_LEN+SPINE_LEN_LEN)
#define SPINE_CRC_LEN sizeof(crc_t)

static_assert(1, "true");
static_assert(SPINE_HEADER_LEN == sizeof(struct SpineMessageHeader), "bad define");
static_assert(SPINE_MAX_BYTES >= SPINE_HEADER_LEN + sizeof(struct BodyToHead) + SPINE_CRC_LEN, "bad define");
static_assert(SPINE_MAX_BYTES >= SPINE_HEADER_LEN + sizeof(struct HeadToBody) + SPINE_CRC_LEN, "bad_define");


static const SpineSync SyncKey = SYNC_BODY_TO_HEAD;

static struct HalGlobals {
  uint8_t inbuffer[SPINE_MAX_BYTES];
  struct SpineMessageHeader outheader;
  int fd;
  int errcount;
} gHal;


/************* Error Handling *****************/
#define spine_error(code, fmt, args...)   (LOGE( fmt, ##args)?(code):(code))
#ifdef CONSOLE_DEBUG_PRINTF
#define spine_debug(fmt, args...)  printf(fmt, ##args)
#else
#define spine_debug(fmt, args...)  (LOGD( fmt, ##args))
#endif

#define EXTENDED_SPINE_DEBUG 0
#if EXTENDED_SPINE_DEBUG
#define spine_debug_x spine_debug
#else
#define spine_debug_x(fmt, args...)
#endif

/************* SERIAL INTERFACE ***************/

static void hal_serial_close()
{
  LOGD("close(fd = %d)", gHal.fd);
  //TODO: restore?:  tcsetattr(gHal.fd, TCSANOW, &gHal.oldcfg))
  close(gHal.fd);
  gHal.fd = 0;
}


SpineErr hal_serial_open(const char* devicename, long baudrate)
{
  if (gHal.fd != 0) {
    return spine_error(err_ALREADY_OPEN, "hal serial port in use, close other first");
  }

  spine_debug("opening serial port\n");

  gHal.fd = open(devicename, O_RDWR | O_NONBLOCK);
  if (gHal.fd < 0) {
    return spine_error(err_CANT_OPEN_FILE, "Can't open %s", devicename);
  }

  spine_debug("configuring serial port\n");

  /* Configure device */
  {
    struct termios cfg;
    if (tcgetattr(gHal.fd, &cfg)) {
      hal_serial_close();
      return spine_error(err_TERMIOS_FAIL, "tcgetattr() failed");
    }
//?    memcpy(gHal.oldcfg,cfg,sizeof(gHal.oldcfg)); //make backup;

    cfmakeraw(&cfg);

    platform_set_baud(gHal.fd, cfg, baudrate);

    cfg.c_cflag |= (CS8 | CSTOPB);    // Use N82 bit words

    LOGD("configuring port %s (fd=%d)", devicename, gHal.fd);

    if (tcsetattr(gHal.fd, TCSANOW, &cfg)) {
      hal_serial_close();
      return spine_error(err_TERMIOS_FAIL, "tcsetattr() failed");
    }
  }
  spine_debug("serial port OK\n");
  return err_OK;
}


int hal_serial_read(uint8_t* buffer, int len)   //->bytes_recieved
{

  int result = read(gHal.fd, buffer, len);
  if (result < 0) {
    if (errno == EAGAIN) { //nonblocking no-data
      usleep(HAL_SERIAL_POLL_INTERVAL_US); //wait a bit.
      result = 0; //not an error
    }
  }
  return result;
}


int hal_serial_send(const uint8_t* buffer, int len)
{
  if (len) {
    return write(gHal.fd, buffer, len);
  }
  return 0;
}


/************* PROTOCOL SYNC ***************/

enum MsgDir {
  dir_SEND,
  dir_READ,
};

//checks for valid tag, returns expected length, -1 on err
static int get_payload_len(PayloadId payload_type, enum MsgDir dir)
{
  switch (payload_type) {
  case PAYLOAD_MODE_CHANGE:
    return 0;
    break;
  case PAYLOAD_DATA_FRAME:
    return (dir == dir_SEND) ? sizeof(struct HeadToBody) : sizeof(struct BodyToHead);
    break;
  case PAYLOAD_VERSION:
    return (dir == dir_SEND) ? 0 : sizeof(struct VersionInfo);
    break;
  case PAYLOAD_ACK:
    return sizeof(struct AckMessage);
    break;
  case PAYLOAD_ERASE:
    return 0;
    break;
  case PAYLOAD_VALIDATE:
    return 0;
    break;
  case PAYLOAD_DFU_PACKET:
    return sizeof(struct WriteDFU);
    break;
  default:
    break;
  }
  return -1;
}




//Creates header for frame
static const uint8_t* spine_construct_header(PayloadId payload_type,  uint16_t payload_len)
{
#ifndef NDEBUG
  int expected_len = get_payload_len(payload_type, dir_SEND);
  assert(expected_len >= 0); //valid type
  assert(expected_len == payload_len);
  assert(payload_len <= (SPINE_MAX_BYTES - SPINE_HEADER_LEN - SPINE_CRC_LEN));
#endif

  struct SpineMessageHeader* hdr = &gHal.outheader;
  hdr->sync_bytes = SYNC_HEAD_TO_BODY;
  hdr->payload_type = payload_type;
  hdr->bytes_to_follow = payload_len;
  return (uint8_t*) hdr;
}


//Function: Examines `buf[idx]` and determines if it is part of sync seqence.
//Prereq: The first `idx` chars in `buf` are partial valid sync sequence.
//Returns: Length of partial valid sync sequence. possible values = 0..idx+1
static int spine_sync(const uint8_t* buf, unsigned int idx)
{
  if (idx < SPINE_TAG_LEN) {
    if (buf[idx] != BODY_TAG_PREFIX[idx]) {
      return 0; //none of the characters so far are good.
    }
  }
  idx++; //accept rest of characters unless proven otherwise
  if (idx == SPINE_HEADER_LEN) {
    struct SpineMessageHeader* candidate = (struct SpineMessageHeader*)buf;
    int expected_len = get_payload_len(candidate->payload_type, dir_READ);
    if (expected_len < 0 || (expected_len != candidate->bytes_to_follow)) {
      LOGE("spine_header %x %x %x : %d", candidate->sync_bytes,
           candidate->payload_type,
           candidate->bytes_to_follow,
           expected_len);
      //bad header,
      //we need to check the length bytes for the beginning of a sync word
      unsigned int pos = idx - SPINE_LEN_LEN;
      int match = 0;
      while (pos < idx) {
        if (buf[pos] == BODY_TAG_PREFIX[match]) {
          match++;
          pos++;
        }
        else if (match > 0) {  match = 0; }
        else { pos++; }
      }
      // at this point we know that the last `match` chars rcvd into length position are a possible sync tag.
      // but there is no need to copy them to beginning of `buf` b/c we can't get here unless
      //  buf already contains good tag. So just return the number of matches
      return match;
    }
  }
  return idx;
}


/************* PUBLIC INTERFACE ***************/

SpineErr hal_init(const char* devicename, long baudrate)
{
  gHal.errcount = 0;
  gHal.fd = 0;
  SpineErr r = hal_serial_open(devicename, baudrate);
#ifdef SPINE_TTY_LEGACY
  if ((r == err_TERMIOS_FAIL) || (r == err_CANT_OPEN_FILE)) {
    if (strncmp(devicename, SPINE_TTY, strlen(SPINE_TTY)) == 0) {
      // If we're running on an old OS version, try the legacy ttyHSL1 device
      r = hal_serial_open(SPINE_TTY_LEGACY, baudrate);
    }
  }
#endif
  return r;
}


// Scan the whole payload for sync, to recover after dropped bytes,
int hal_resync_partial(int start_offset, int len) {
   /*  ::: Preconditions :::
       gHal.inbuffer contains len recieved bytes.
       the first start_offset bytes do not need to be scanned
       ::: Postconditions :::
       the first valid sync header and any following bytes up to `len`
       -  or a partial sync header ending at the `len`th byte -
       have been copied to the beginning of gHal.inbuffer.
       returns the number of copied bytes
   */
    unsigned int i;
    unsigned int index = 0;
    for (i = start_offset; i+index < len; ) {
       spine_debug_x(" %02x", gHal.inbuffer[i+index]);
       unsigned int i2 = spine_sync(gHal.inbuffer+i, index);
       if (i2 <= index) { //no match, or restarted match at `i2` chars before last scanned char
          i+=index-i2+1;
       }
       else if (i2 == SPINE_HEADER_LEN) { //whole sync!
          index = len-i; //consider rest of buffer valid.
          break;
       }
       index = i2;
    }
    //at this point we have scanned `i`+`index` chars. the last `index` of them are valid.
    if (index) {
       memmove(gHal.inbuffer, gHal.inbuffer+i, index);
    }
    spine_debug("\n%u dropped bytes\n", start_offset+i-index);

    return index;
}



//gathers most recently queued frame,
//Spins until valid frame header is recieved.
const struct SpineMessageHeader* hal_read_frame()
{
  static unsigned int index = 0;

  //spin here pulling single characters until whole sync rcvd
  while (index < SPINE_HEADER_LEN) {

    int rslt = hal_serial_read(gHal.inbuffer + index, 1);
    if (rslt > 0) {
      index = spine_sync(gHal.inbuffer, index);
    }
    else if (rslt < 0) {
      if ((gHal.errcount++ & 0x3FF) == 0) { //TODO: at somepoint maybe we handle this?
        LOGI("spine_read_error %d", rslt);
      }
    }
    else {
       return NULL; //wait a bit more
    }
  } //endwhile


  //At this point we have a valid message header. (spine_sync rejects bad lengths and payloadTypes)
  // Collect the right number of bytes.
  unsigned int payload_length = ((struct SpineMessageHeader*)gHal.inbuffer)->bytes_to_follow;
  unsigned int total_message_length = SPINE_HEADER_LEN + payload_length + SPINE_CRC_LEN;

  spine_debug_x("%d byte payload\n", payload_length);

  while (index < total_message_length) {
    int rslt = hal_serial_read(gHal.inbuffer + index, total_message_length - index);
    //spine_debug_x("%d bytes rcvd\n", rslt);
    if (rslt > 0) {
      index += rslt;
    }
    else if (rslt < 0) {
      if ((gHal.errcount++ & 0x3FF) == 0) { //TODO: at somepoint maybe we handle this?
        LOGI("spine_payload_read_error %d", rslt);
      }
    }
    else {
       return NULL; //wait a bit more
    }
  }

  spine_debug_x("%d bytes rcvd\n", index);

  //now we just have to validate CRC;
  crc_t expected_crc = *((crc_t*)(gHal.inbuffer + SPINE_HEADER_LEN + payload_length));
  crc_t true_crc = calc_crc(gHal.inbuffer + SPINE_HEADER_LEN, payload_length);
  if (expected_crc != true_crc && !SKIP_CRC_CHECK) {
    spine_debug("\nspine_crc_error: calc %08x vs data %08x\n", true_crc, expected_crc);
    LOGI("spine_crc_error %08x != %08x", true_crc, expected_crc);


    // Scan the whole payload for sync, to recover after dropped bytes,
    index = hal_resync_partial(SPINE_HEADER_LEN, total_message_length);
    return NULL;
  }

  spine_debug_x("found frame %04x!\r", ((struct SpineMessageHeader*)gHal.inbuffer)->payload_type);
  spine_debug_x("payload start: %08x!\r", *(uint32_t*)(((struct SpineMessageHeader*)gHal.inbuffer)+1));
  index = 0; //get ready for next one
  return ((struct SpineMessageHeader*)gHal.inbuffer);
}


//pulls out frames
const void* hal_get_next_frame(int32_t timeout_ms) {
  timeout_ms *= 1000 / HAL_SERIAL_POLL_INTERVAL_US;
  const struct SpineMessageHeader* hdr;
  do {
    if (timeout_ms>0 && --timeout_ms==0) {
      LOGE("TIMEOUT in hal_get_next_frame() TIMEOUT");
      return NULL;
    }
    hdr = hal_read_frame();
  } while (!hdr);
  return hdr;
}

const void* hal_get_frame(uint16_t type, int32_t timeout_ms)
{
  while (1) {
    const struct SpineMessageHeader* hdr;
    hdr = hal_get_next_frame(timeout_ms);
    if (!hdr || hdr->payload_type == type) {
      return hdr;
    }
  }
  return NULL;
}

const void* hal_wait_for_frame(uint16_t type)
{
  const void* ret;
  do {
    ret = hal_get_frame(type, INT32_MAX);
  } while (!ret);
  return ret;
}


void hal_send_frame(PayloadId type, const void* data, int len)
{
  const uint8_t* hdr = spine_construct_header(type, len);
  crc_t crc = calc_crc(data, len);
  if (hdr) {
    spine_debug_x("sending %x packet (%d bytes) CRC=%08x\n", type, len, crc);
    hal_serial_send(hdr, SPINE_HEADER_LEN);
    hal_serial_send(data, len);
    hal_serial_send((uint8_t*)&crc, sizeof(crc));
  }
}

void hal_set_mode(int new_mode)
{
  printf("Sending Mode Change %x\n", PAYLOAD_MODE_CHANGE);
  hal_send_frame(PAYLOAD_MODE_CHANGE, NULL, 0);
}

#ifdef STANDALONE_TEST

//gcc -g -DSTANDALONE_TEST -I ../../syscon spine_hal.c spine_crc.c -o spine_test

int main(int argc, const char* argv[])
{
   gHal.fd = open("unittest.dat", O_RDONLY);
   const struct SpineMessageHeader* hdr = hal_get_frame(PAYLOAD_ACK, 1000);
   assert(hdr && hdr->payload_type == PAYLOAD_ACK);
}
#endif
