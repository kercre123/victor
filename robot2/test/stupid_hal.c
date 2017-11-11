#include <stdio.h>
#include <stdlib.h>

#include "../spine/platform/platform.h"
//#include "../spine/spine_protocol.h"
#include "schema/messages.h"
#include "../spine/spine_hal.h"
#include "core/clock.h"

#define RATEMASK (16-1) //(64-1)

#include <spine_crc.h>



void handle_incoming_frame(struct BodyToHead* data)
{
   static uint32_t lastfc = 0;
   static uint64_t start = 0;
   uint64_t now = steady_clock_now();
//   if (!start) start = steady_clock_now();
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
      printf("%jd  %d: %d %d %d %d \r",
             now-start,
             data->framecounter,
             data->cliffSense[0],
             data->cliffSense[1],
             data->cliffSense[2],
             data->cliffSense[3]);
      fflush(stdout);

   }
   start = now;
}

void fill_outgoing_frame(struct HeadToBody* data)
{
   data->framecounter++;
}


int selector(int fd) {
  int result;  fd_set fds;
  FD_ZERO(&fds);
  FD_SET(fd, &fds);

  if (select(fd+1, &fds, NULL, NULL, NULL))
  {
      uint8_t test_buffer[SPINE_MAX_BYTES];
      result = read(fd, test_buffer, SPINE_MAX_BYTES);
      if (result < 0) {
        if (errno == EAGAIN) { //nonblocking no-data
          result = 0; //not an error
        }
      }
      if (result>0) {
        spine_receive_bytes(test_buffer, result);
      }
    return result;
  }
  return 0;
}


void robot_io(void) {

   int spinefd = spine_fd();
   selector(spinefd);
   microwait(1000);
}



int main(int argc, const char* argv[]) {

   struct HeadToBody headData = {0};  //-we own this one.
   struct BodyToHead *bodyData;

   headData.framecounter = 0xDEADBEA7;

   int errCode = hal_init(SPINE_TTY, SPINE_BAUD);
   if (errCode != 0) return errCode;

   hal_set_mode(RobotMode_RUN);

   printf("Starting loop\n");
   LOGD("Starting loop");

   while (1) {
     robot_io(); //buffers all IO until good frame.
     const struct SpineMessageHeader* hdr = hal_get_frame(PAYLOAD_DATA_FRAME, 500);
     if (!hdr) {  //DEBUG CRASHER
       printf(".\n");
       //    hal_send_frame(PAYLOAD_DATA_FRAME, &headData, sizeof(headData));
     }
     else {
       if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
         bodyData = (struct BodyToHead*)(hdr+1);
         handle_incoming_frame(bodyData);
         fill_outgoing_frame(&headData);
         hal_send_frame(PAYLOAD_DATA_FRAME, &headData, sizeof(headData));
       }
     }
   }
   return 0;
}
