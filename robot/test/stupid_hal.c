#include <stdio.h>
#include <stdlib.h>

#include "../spine/platform/platform.h"
//#include "../spine/spine_protocol.h"
#include "schema/messages.h"
#include "../spine/spine_hal.h"

#define RATEMASK (16-1) //(64-1)

#include <spine_crc.h>



void handle_incoming_frame(struct BodyToHead* data)
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

extern const struct SpineMessageHeader* hal_read_frame();

int main(int argc, const char* argv[]) {

   struct HeadToBody headData = {0};  //-we own this one.
   struct BodyToHead *bodyData;
   int framerate_us = 0;
   bool spammy = false;

   headData.framecounter = 0xDEADBEA7;

   if (argc > 1) {
     framerate_us = atoi(argv[1]);
     spammy = true;
     framerate_us = (framerate_us < HAL_SERIAL_POLL_INTERVAL_US) ? 0:
       framerate_us - HAL_SERIAL_POLL_INTERVAL_US;
     printf("Spamming Head2Body frames %d us apart\n", framerate_us+HAL_SERIAL_POLL_INTERVAL_US);
   }

   int errCode = hal_init(SPINE_TTY, SPINE_BAUD);
   if (errCode != 0) return errCode;

   hal_set_mode(RobotMode_RUN);

   printf("Starting loop\n");
   LOGD("Starting loop");
   while (1) {
     const struct SpineMessageHeader* hdr;
     fill_outgoing_frame(&headData);
     if (spammy)  {
       usleep(framerate_us);
       hdr = hal_read_frame();
     }
     else {
       hdr = hal_get_frame(PAYLOAD_DATA_FRAME, 10);
     }
     if (!hdr) {  //DEBUG CRASHER
       hal_send_frame(PAYLOAD_DATA_FRAME, &headData, sizeof(headData));
     }
     else
       if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
         bodyData = (struct BodyToHead*)(hdr+1);
         handle_incoming_frame(bodyData);
         hal_send_frame(PAYLOAD_DATA_FRAME, &headData, sizeof(headData));
       }
   }
   return 0;
}
