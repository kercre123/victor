
#include <chrono>

#include <unistd.h>


#include "../spine/spine_hal.h"
#include "anki/cozmo/robot/hal.h"

namespace Anki {
namespace Cozmo {
namespace RobotIO {


  struct {
    fd_set fds;
    int spineFd;
    int maxFd;


  } gIO;

static  uint64_t steady_clock_now(void) {
    return std::chrono::steady_clock::now().time_since_epoch().count();
  }




  void BuildFDSet(void) {
    FD_ZERO(&gIO.fds);
    gIO.spineFd = spine_fd();
    FD_SET(gIO.spineFd, &gIO.fds);
    gIO.maxFd = gIO.spineFd;

  }


  uint64_t gBase;
  void Init(void) {
    gBase = steady_clock_now();
    BuildFDSet();
  }


  void SendOutgoingData(void)
  {
//    hal_serial_send(
  }


  void ReadIncomingData(void)
  {
    int result;
    fd_set fds;
    //FD_COPY(&gIO.fds, &fds);
    //
    memcpy(&fds, &gIO.fds, sizeof(fds));
    int nfds = select(gIO.maxFd+1, &fds, NULL, NULL, NULL);
    switch (nfds)
    {
      case -1:
        LOGE("Select error %d\n", errno);
        break;
      case 0:
        break;
      default:
        if (FD_ISSET(gIO.spineFd, &fds))
        {
          uint8_t test_buffer[SPINE_MAX_BYTES];
          result = read(gIO.spineFd, test_buffer, SPINE_MAX_BYTES);
          if (result < 0) {
            if (errno == EAGAIN) { //nonblocking no-data
              result = 0; //not an error
            }
          }
          if (result>0) {
            #ifdef LOG_RCV_TIMING
            uint64_t now = steady_clock_now();
            LOGI("%d bytes %ju", result, now-gBase);
            gBase=now;
#endif
            spine_receive_bytes(test_buffer, result);
          }
        }
        break;
    }
  }



  void Step(void) {
    SendOutgoingData();
    usleep(1);
    ReadIncomingData();
  }

}
}
}
