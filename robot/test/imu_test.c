#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <assert.h>

#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "platform/platform.h"
#include "anki/cozmo/robot/spi_imu.h"


#ifdef CONSOLE_DEBUG_PRINTF
#define imu_debug(fmt, args...) printf(fmt, ##args)
#else
#define imu_debug(fmt, args...) (LOGD( fmt, ##args))
#endif

void imu_close(void);

uint64_t steady_clock_now(void) {
   struct timespec time;
   clock_gettime(CLOCK_REALTIME,&time);
   return time.tv_sec * (uint64_t)1e9 + time.tv_nsec;
}


int main(void)   //int argc, char** argv) {
{


  const char* err = imu_open();
  // Setup
  if (err) {
    imu_debug("%s\n", err);
    return -1;
  }
  imu_init();

  uint64_t now = steady_clock_now();
  uint64_t then = now;

  while (1) {
    struct IMURawData data;
    uint64_t target = now + TICKS_5MS;
    imu_manage(&data);
    uint64_t elapsed = (steady_clock_now()-now)/1000;

    printf("%09lld: %9d <%4d %4d %4d>  [%4d %4d %4d]\r", elapsed, data.timestamp,
           data.gyro[0], data.gyro[1], data.gyro[2],
           data.acc[0], data.acc[1], data.acc[2]);
    then = now;
    do {
      now = steady_clock_now();
    }
    while (now < target);
  }

  imu_close();

  return 0;
}
