#include "hal/spine/spine.h"
#include "hal/spine/spine_crc.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <termios.h>

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

void spine_init(spine_ctx_t spine)
{
    memset(spine, 0, sizeof(struct spine_ctx));
    spine->fd = -1;
}

void spine_destroy(spine_ctx_t spine)
{
    spine_close(spine);
    memset(spine, 0, sizeof(struct spine_ctx));
    spine->fd = -1;
}

static SpineErr spine_open_internal(spine_ctx_t spine, struct spine_params params)
{
    assert(spine != NULL);

    if (spine->fd >= 0) {
        return spine_error(err_ALREADY_OPEN, "hal serial port in use, close other first");
    }

    spine_debug("opening serial port\n");

    spine->fd = open(params.devicename, O_RDWR);
    if (spine->fd == -1) {
        return spine_error(err_CANT_OPEN_FILE, "Can't open %s", params.devicename);
    }

    spine_debug("configuring serial port\n");

    /* Configure device */
    {
        struct termios cfg;
        if (tcgetattr(spine->fd, &cfg)) {
            spine_close(spine);
            return spine_error(err_TERMIOS_FAIL, "tcgetattr() failed");
        }

        cfmakeraw(&cfg);

        platform_set_baud(spine->fd, cfg, params.baudrate);

        cfg.c_cflag |= (CS8 | CSTOPB);    // Use N82 bit words

        LOGD("configuring port %s (fd=%d)", params.devicename, spine->fd);

        if (tcsetattr(spine->fd, TCSANOW, &cfg)) {
            spine_close(spine);
            return spine_error(err_TERMIOS_FAIL, "tcsetattr() failed");
        }
    }
    spine_debug("serial port OK\n");
    return err_OK;
}

SpineErr spine_open(spine_ctx_t spine, struct spine_params params)
{
    SpineErr r = spine_open_internal(spine, params);
#ifdef SPINE_TTY_LEGACY
    if ((r == err_TERMIOS_FAIL) || (r == err_CANT_OPEN_FILE)) {
        if (strncmp(params.devicename, SPINE_TTY, strlen(SPINE_TTY)) == 0) {
            // If we're running on an old OS version, try the legacy ttyHSL1 device
            struct spine_params legacy_params = {
                .baudrate = params.baudrate,
                .devicename = SPINE_TTY_LEGACY
            };
            r = spine_open_internal(spine, legacy_params);
        }
    }
#endif
    return r;
}

int spine_close(spine_ctx_t spine)
{
    LOGD("close(fd = %d)", spine->fd);
    if (spine->fd == -1) {
        return 0;
    }
    int r = close(spine->fd);
    if (r == 0) {
        spine->fd = -1;
    }
    return r;
}

// get file descriptor associated with spine I/O
int spine_get_fd(spine_ctx_t spine)
{
    return spine->fd;
}

// read data from spine
ssize_t spine_read_all(int fd, void* buf, size_t buf_len)
{
    ssize_t bytes_read = 0;
    ssize_t r = 0;
    uint8_t* readbuf = (uint8_t*)buf;

    do {
        ssize_t remaining = (buf_len - bytes_read);
        if (remaining <= 0) {
            break;
        }
        r = read(fd, readbuf, remaining);
        if (r > 0) {
            bytes_read += r;
            readbuf += r;
        }
    } while (r > 0);

    if (errno != EAGAIN) {
        // read error, set result to r (-1). errno will be available to caller
        bytes_read = r;
    }

    return bytes_read;
}

ssize_t spine_read(spine_ctx_t spine)
{
    return spine_read_all(spine->fd, spine->buf_rx, sizeof(spine->buf_rx));
}

// send data into spine for processing
ssize_t spine_receive_data(spine_ctx_t spine, const void* bytes, size_t len)
{
    size_t next_offset = spine->rx_cursor;
    size_t remaining = sizeof(spine->buf_rx) - next_offset;

    if (len > remaining) {
        LOGE("spine_receive_data.overflow :: %u", len - remaining);
        spine->rx_cursor = 0;
        // BRC: add a flag to indicate a reset (for using in parsing?)
    }

    uint8_t* rx = spine->buf_rx + next_offset;
    memcpy(rx, bytes, len);
    spine->rx_cursor = next_offset + len;

    // printf("spine_receive_data %u :: rx_cursor at %u\n", len, spine->rx_cursor);

    return len;
}

ssize_t spine_parse_header(const void* inbuf, size_t inbuf_len, struct SpineMessageHeader* out_header)
{
    // search for header
    uint8_t* bytes = (uint8_t*)inbuf;
    ssize_t offset = 0;
    for(offset = 0; offset < inbuf_len; ++offset) {
        bytes += offset;
        SpineSync* sync = (SpineSync*)(bytes);
        if (SYNC_BODY_TO_HEAD == *sync) {
            break;
        }
    }

    size_t remaining = inbuf_len - offset;
    if (remaining == 0) {
        return -1;
    }

    out_header = (struct SpineMessageHeader*)bytes;

    return offset;
}

bool spine_frame_is_valid(struct spine_frame_b2h* frame)
{
    uint16_t payload_len = frame->header.bytes_to_follow;
    const uint8_t* payload_bytes = (const uint8_t*)&frame->payload;
    crc_t true_crc = calc_crc(payload_bytes, payload_len);
    crc_t expected_crc = frame->footer.checksum;
    return (expected_crc == true_crc);
}

ssize_t spine_parse_b2h_frame(const void* inbuf, size_t inbuf_len, struct spine_frame_b2h* out_frame)
{
    uint8_t* bytes = (uint8_t*)inbuf;
    ssize_t offset = 0;
    struct SpineMessageHeader* header = NULL;
    while((offset >= 0) && (offset < inbuf_len)) {
        // Find next header
        header = NULL;
        bytes += offset;
        ssize_t remaining = (inbuf_len - offset);
        offset = spine_parse_header(bytes, remaining, header);

        if (offset <= 0) {
            break;
        }
        if (PAYLOAD_DATA_FRAME != header->payload_type) {
            continue;
        }

        struct spine_frame_b2h* candidate = (struct spine_frame_b2h*)bytes;
        if (spine_frame_is_valid(candidate)) {
            break;
        }
    }

    if (header != NULL) {
        out_frame = (struct spine_frame_b2h*)(bytes + offset);
    } else {
        offset = -1;
    }

    return offset;
}

enum MsgDir {
    dir_SEND,
    dir_READ,
};

//checks for valid tag, returns expected length, -1 on err
int spine_get_payload_len(PayloadId payload_type, enum MsgDir dir)
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
  case PAYLOAD_CONT_DATA:
    return sizeof(struct ContactData);
    break;
  case PAYLOAD_SHUT_DOWN:
    return 0;
    break;
  default:
    break;
  }
  return -1;
}

void spine_set_rx_cursor(spine_ctx_t spine, ssize_t offset)
{
    if (offset > 0) {
        ssize_t remaining = sizeof(spine->buf_rx) - offset;
        assert(remaining >= 0);
        if (remaining >= 0) {
            const uint8_t* rx = (const uint8_t*)spine->buf_rx + offset;
            memmove(spine->buf_rx, rx, remaining);
        }
        spine->rx_cursor -= offset;
        memset(spine->buf_rx + spine->rx_cursor, 0x55, sizeof(spine->buf_rx) - spine->rx_cursor);
    } else if (offset < 0) {
        // discard all data
        spine->rx_cursor = 0;
        memset(spine->buf_rx, 0x55, sizeof(spine->buf_rx));
    }
}

ssize_t spine_parse_frame(spine_ctx_t spine, void *out_buf, size_t out_buf_len, size_t* out_len)
{
    // printf("spine_parse_frame: bytes available %u\n", spine->rx_cursor);
    const size_t rx_len = spine->rx_cursor;

    // Is there any data to process?
    if (rx_len == 0) {
        // indicate that we are waiting
        spine_debug_x("no data\n");
        return 0;
    }

    // Start from the beginning of the rx buffer
    uint8_t* rx = spine->buf_rx;

    // Search for a valid sync sequence
    ssize_t search_len = rx_len - sizeof(SYNC_BODY_TO_HEAD);
    search_len = (search_len < 0) ? 0 : search_len;
    ssize_t sync_index = -1;
    ssize_t i;
    SpineSync syncB2H = SYNC_BODY_TO_HEAD;

    for(i = 0; i < search_len; ++i) {
        const uint8_t* bytes = rx + i;
        const uint8_t* sync = (const uint8_t*)(&syncB2H);
        if (sync[0] == bytes[0] &&
            sync[1] == bytes[1] &&
            sync[2] == bytes[2] &&
            sync[3] == bytes[3])
        {
            sync_index = i;
//            spine_debug_x("found header\n");
            break;
        }
    }

    // No sync sequence
    if (sync_index == -1) {
        // throw away all data
        spine_debug_x("no sync\n");

        spine_set_rx_cursor(spine, -1);
        return -1;
    }

    size_t offset = sync_index;
    ssize_t rx_remaining = rx_len - offset;

    // Not enough data to valid header
    if (rx_remaining < sizeof(struct SpineMessageHeader)) {
        // reset to start of header & wait
        //printf("set_rx_cursor: %u\n", sync_index);
        spine_set_rx_cursor(spine, sync_index);
//        spine_debug_x("wait for header\n");
        return 0;
    }

    // set `rx` to the beginning of the packet
    // offset
    rx = (spine->buf_rx + sync_index);

    // Validate payload data
    const struct SpineMessageHeader* header = (const struct SpineMessageHeader*)rx;
    int expected_payload_len = spine_get_payload_len(header->payload_type, dir_READ);

    // Payload type is invalid or payload len is invalid
    if (expected_payload_len == -1 ||
        header->bytes_to_follow != expected_payload_len) {
        // skip current sync
        spine_debug("invalid payload: expected=%u | observed=%u\n", expected_payload_len, header->bytes_to_follow);
        spine_set_rx_cursor(spine, sync_index + sizeof(SYNC_BODY_TO_HEAD));
        return -1;
    }

    offset = sync_index + sizeof(struct SpineMessageHeader);
    rx_remaining = rx_len - offset;

    // Not enough data to validate payload
    size_t required_data_len = expected_payload_len + sizeof(struct SpineMessageFooter);
    if (rx_remaining < required_data_len) {
        // partial frame: wait for more data
//        spine_debug_x("wait for frame\n");

        #if 0  // HACK attempt to support partial frame update
        size_t bytes_processed = spine->rx_cursor - sync_index;
        size_t partial_len = (bytes_processed > out_buf_len) ? out_buf_len : bytes_processed;
        // copy partial data
        if (out_buf != NULL) {
            assert(out_buf_len >= partial_len);
            memcpy(out_buf, rx, partial_len);
        }
        if (out_len != NULL) {
            *out_len = partial_len;
        }
        #endif

        spine_set_rx_cursor(spine, sync_index);

        return 0;
    }
    spine_debug_x("full slug\n");

    const size_t frame_len =
        sizeof(struct SpineMessageHeader) + expected_payload_len + sizeof(struct SpineMessageFooter);

    // Calculate crc of payload
    const uint8_t* payload_bytes = rx + sizeof(struct SpineMessageHeader);
    crc_t true_crc = calc_crc(payload_bytes, expected_payload_len);

    // Extract crc from message
    const uint8_t* crc_bytes = rx + sizeof(struct SpineMessageHeader) + expected_payload_len;
    struct SpineMessageFooter* footer = (struct SpineMessageFooter*)crc_bytes;
    crc_t expected_crc = footer->checksum;

    // Invalid CRC
    if (true_crc != expected_crc) {
        // throw away header
      LOGW("invalid crc: expected=%08x | observed=%08x [type %x]", expected_crc, true_crc, header->payload_type);
        spine_set_rx_cursor(spine, sync_index + sizeof(SYNC_BODY_TO_HEAD));
        return -1;
    }

    // At this point we have a valid frame.

    // Copy data to output buffer
    if (out_buf != NULL) {
        assert(out_buf_len >= frame_len);
        memcpy(out_buf, rx, frame_len);
    }

    // Reset RX buffer
    spine_set_rx_cursor(spine, sync_index + frame_len);

    spine_debug_x("==> found frame %x \n", header->payload_type);

    return frame_len;
}

ssize_t spine_construct_header(PayloadId payload_type, uint16_t payload_len, struct SpineMessageHeader* outHeader)
{
#ifndef NDEBUG
  int expected_len = spine_get_payload_len(payload_type, dir_SEND);
  assert(expected_len >= 0); //valid type
  assert(expected_len == payload_len);
  assert(payload_len <= (SPINE_BUFFER_MAX_LEN - sizeof(struct SpineMessageHeader) - sizeof(struct SpineMessageFooter)));
#endif

  if (outHeader == NULL) {
      return -1;
  }

  struct SpineMessageHeader* hdr = outHeader;
  hdr->sync_bytes = SYNC_HEAD_TO_BODY;
  hdr->payload_type = payload_type;
  hdr->bytes_to_follow = payload_len;

  return sizeof(struct SpineMessageHeader);
}

ssize_t spine_make_h2b_frame(const struct HeadToBody* h2b_payload, struct spine_frame_h2b* out_frame)
{
    size_t payload_len = sizeof(struct HeadToBody);
    ssize_t r = spine_construct_header(PAYLOAD_DATA_FRAME, payload_len, &(out_frame->header));
    if (r == -1) {
        return -1;
    }

    memmove(&(out_frame->payload), h2b_payload, payload_len);
    const uint8_t* payload_bytes = (const uint8_t*)h2b_payload;
    out_frame->footer.checksum = calc_crc(payload_bytes, payload_len);

    return payload_len;
}


ssize_t spine_write(spine_ctx_t spine, const void* data, size_t len)
{
    const uint8_t* bytes = (const uint8_t*)data;
    ssize_t remaining = len;
    ssize_t written = 0;
    ssize_t wr = 0;
    while(remaining > 0) {
        wr = write(spine->fd, bytes, remaining);
        if (wr <= 0) {
            break;
        }
        bytes += wr;
        written += wr;
        remaining = len - written;
    }

    return (wr < 0) ? wr : written;
}

ssize_t spine_write_frame(spine_ctx_t spine, PayloadId type, const void* data, int len)
{
  uint8_t* outBytes = (uint8_t*)spine->buf_tx;
  const ssize_t outBufferLen = sizeof(spine->buf_tx);
  ssize_t remaining = outBufferLen;

  struct SpineMessageHeader* outHeader = (struct SpineMessageHeader*)outBytes;
  ssize_t r = spine_construct_header(type, len, outHeader);
  if (r < 0) {
    return r;
  }
  outBytes += r;
  remaining -= r;

  if (len > 0) {
    if (len > remaining) {
        return -1;
    }
    memmove(outBytes, data, len);
    outBytes += len;
    remaining -= len;
  }

  if (remaining < sizeof(crc_t)) {
      return -1;
  }
  crc_t crc = calc_crc(data, len);
  memmove(outBytes, (uint8_t*)&crc, sizeof(crc));
  outBytes += sizeof(crc);

  return spine_write(spine, spine->buf_tx, (outBytes - spine->buf_tx));
}

ssize_t spine_write_h2b_frame(spine_ctx_t spine, const struct HeadToBody* h2b_payload)
{
    struct spine_frame_h2b* out_frame = (struct spine_frame_h2b*)spine->buf_tx;
    ssize_t r = spine_make_h2b_frame(h2b_payload, out_frame);
    if (r <= 0) {
        return r;
    }

    return spine_write(spine, spine->buf_tx, sizeof(struct spine_frame_h2b));
}

ssize_t spine_write_ccc_frame(spine_ctx_t spine, const struct ContactData* ccc_payload)
{
    struct SpineMessageHeader* out_frame = (struct SpineMessageHeader*)spine->buf_tx;
    size_t payload_len = sizeof(struct ContactData);
    ssize_t r = spine_construct_header(PAYLOAD_CONT_DATA, payload_len, out_frame);
    if (r == -1) {
        return -1;
    }

    uint8_t* payload_start = (uint8_t*)(out_frame+1);
    memmove(payload_start, ccc_payload, payload_len);
    crc_t* csp = (crc_t*)(payload_start+payload_len);
    *csp = calc_crc(payload_start, payload_len);

    r = payload_len;
    if (r <= 0) {
        return r;
    }

    return spine_write(spine, spine->buf_tx,
                       sizeof(struct SpineMessageHeader) + sizeof(struct ContactData) + sizeof(struct SpineMessageFooter));
}

ssize_t spine_set_mode(spine_ctx_t spine, int new_mode)
{
  printf("Sending Mode Change %x\n", PAYLOAD_MODE_CHANGE);
  ssize_t r = spine_write_frame(spine, PAYLOAD_MODE_CHANGE, NULL, 0);
  spine_debug_x("spine_set_mode return %d\n", r);
  return r;
}

ssize_t spine_shutdown(spine_ctx_t spine)
{
  ssize_t r = spine_write_frame(spine, PAYLOAD_SHUT_DOWN, NULL, 0);
  spine_debug_x("spine_shutdown return %d\n", r);
  return r;
}

#ifdef DEBUG_SPINE_TEST
//
// TEST
//

struct spine_ctx gSpine;
#define RATEMASK (16-1) //(64-1)

void handle_incoming_frame(const struct spine_frame_b2h* frame)
{
   const struct BodyToHead* data = &frame->payload;

   crc_t data_crc = calc_crc((const uint8_t*)data, sizeof(struct BodyToHead));
   crc_t expected_crc = frame->footer.checksum;
   if (data_crc != expected_crc) {
       printf("CRC mismatch: expected=%08x | actual=%08x\n", expected_crc, data_crc);
   }

   static uint32_t lastfc = 0;
   if (data->framecounter != lastfc+1)
   {
      LOGD("DROPPED FRAMES after %d until %d\n", lastfc, data->framecounter);
      printf("DROPPED FRAMES after %d until %d\n", lastfc, data->framecounter);
   }
   if (lastfc<1) {
     int i;
     uint8_t* dp=(uint8_t*)data;
     for (i=0;i<sizeof(struct BodyToHead);i++)
     {
       printf("%02x ",*dp++);
     }
     printf("%08x\n", *(crc_t*)(dp));
     printf("CRC = %08x\n",calc_crc((const uint8_t*)data, sizeof(struct BodyToHead)));
   }

   lastfc = data->framecounter;
   static uint8_t printcount=0;
   if (( ++printcount & RATEMASK ) == 0) {
      printf("%d: %d %d %d %d \r",
             data->framecounter,
             data->cliffSense[0],
             data->cliffSense[1],
             data->cliffSense[2],
             data->cliffSense[3]);
      fflush(stdout);

   }
}

void fill_outgoing_frame(struct HeadToBody* data)
{
   data->framecounter++;
}

ssize_t robot_io(spine_ctx_t spine, bool drain)
{
  if (!drain)
    usleep(1000);

  uint8_t buffer[2048];
  int fd = spine_get_fd(spine);
  ssize_t r = 0;
  do {
    ssize_t r = read(fd, buffer, sizeof(buffer));
    if (r > 0) {
      r = spine_receive_data(spine, (const void*)buffer, r);
    } else if (r < 0) {
      if (errno == EAGAIN) {
        r = 0;
      }
    }
  } while (drain && r != 0);
  return r;
}

int spine_test_setup()
{
  spine_init(&gSpine);
  struct spine_params params = {
    .devicename = SPINE_TTY,
    .baudrate = SPINE_BAUD
  };
  int errCode = spine_open(&gSpine, params);
  if (errCode != 0) return errCode;

  return 0;
}


bool s_test_initialized = false;
size_t s_read_count = 0;

int spine_test_loop_once() {

  struct HeadToBody headData = {0};  //-we own this one.

  headData.framecounter = 0xDEADBEA7;

  spine_set_mode(&gSpine, RobotMode_RUN);

  uint8_t frame_buffer[SPINE_B2H_FRAME_LEN];

  printf("Starting loop\n");
  LOGD("Starting loop");

  while (1) {
    ssize_t r = spine_parse_frame(&gSpine, &frame_buffer, sizeof(frame_buffer), NULL);

    if (r < 0) {
      continue;
    } else if (r > 0) {
      const struct SpineMessageHeader* hdr = (const struct SpineMessageHeader*)frame_buffer;
      if (hdr->payload_type == PAYLOAD_ACK) {
        s_test_initialized = true;
      } else if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
        s_test_initialized = true;
        const struct spine_frame_b2h* frame = (const struct spine_frame_b2h*)frame_buffer;
        handle_incoming_frame(frame);
        spine_write_h2b_frame(&gSpine, &headData);
        robot_io(&gSpine, true);
      }
    } else {
      // r == 0 (waiting)
      if (s_read_count > 50) {
        if (!s_test_initialized && s_read_count > 100) {
          spine_set_mode(&gSpine, RobotMode_RUN);
        } else {
          spine_write_h2b_frame(&gSpine, &headData);
        }
        s_read_count = 0;
      }

    }

    robot_io(&gSpine, false);
    s_read_count++;
  }

  return 0;
}

#endif // DEBUG_SPINE_TEST
