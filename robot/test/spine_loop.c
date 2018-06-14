#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "../spine/platform/platform.h"
#include "schema/messages.h"

#define RATEMASK (16-1) //(64-1)

#include <spine_crc.h>

#include "spine.h"

struct spine_ctx gSpine;

void handle_incoming_frame(const struct BodyToHead* data)
{
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

int main(int argc, const char* argv[]) {

  spine_init(&gSpine);
  struct spine_params params = {
    .devicename = SPINE_TTY,
    .baudrate = SPINE_BAUD
  };
  int errCode = spine_open(&gSpine, params);
  if (errCode != 0) return errCode;

  struct HeadToBody headData = {0};  //-we own this one.
  struct BodyToHead *bodyData;

  headData.framecounter = 0xDEADBEA7;

  spine_set_mode(&gSpine, RobotMode_RUN);


  printf("Starting loop\n");
  LOGD("Starting loop");

  uint8_t frame_buffer[SPINE_B2H_FRAME_LEN];
  bool initialized = false;
  size_t read_count = 0;

  while (1) {
    struct SpineMessageHeader header;
    ssize_t r = spine_parse_frame(&gSpine, &frame_buffer, sizeof(frame_buffer), NULL);

    if (r < 0) {
      continue;
    } else if (r > 0) {
      const struct SpineMessageHeader* hdr = (const struct SpineMessageHeader*)frame_buffer;
      if (hdr->payload_type == PAYLOAD_ACK) {
        initialized = true;
      } else if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
        initialized = true;
        const struct spine_frame_b2h* frame = (const struct spine_frame_b2h*)frame_buffer;
        handle_incoming_frame(&frame->payload);
        spine_write_h2b_frame(&gSpine, &headData);
        robot_io(&gSpine, true);
      }
    } else {
      // r == 0 (waiting)
      if (read_count > 50) {
        if (!initialized && read_count > 100) {
          spine_set_mode(&gSpine, RobotMode_RUN);
        } else {
          spine_write_h2b_frame(&gSpine, &headData);
        }
        read_count = 0;
      }

    }

    robot_io(&gSpine, false);
    read_count++;
  }

  return 0;
}
