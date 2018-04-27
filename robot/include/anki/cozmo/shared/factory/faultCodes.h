/**
* File: faultCodes.h
*
* Author: Al Chaussee
* Date:   4/3/2018
*
* Description: Hardware and software fault codes
*              Higher codes take precedence over lower codes
*              DO NOT MODIFY
*
* Copyright: Anki, Inc. 2018
**/

#ifndef FAULT_CODES_H
#define FAULT_CODES_H

#include <inttypes.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <errno.h>

namespace Anki {
namespace Cozmo {

namespace FaultCode {

static const char* kFaultCodeFifoName = "/run/error_code";

// Enum of fault codes, range 800 - 999
// Higher numbers take precedence when displaying
enum : uint16_t {
  NONE = 0,

  // Should always be 800 as the fault code
  // display image for this fault is hardcoded into
  // the animfail program
  NO_ANIM_PROCESS = 800,
    
  WIFI,
  NO_SWITCHBOARD,
  SYSTEMD,
    
  NO_ENGINE_PROCESS,

  CLIFF_FR,
  CLIFF_FL,
  CLIFF_BR,
  CLIFF_BL,
  TOF,
  TOUCH_SENSOR,
  MIC_FR,
  MIC_FL,
  MIC_BR,
  MIC_BL,
  NO_BODY,

  NO_ROBOT_PROCESS,
    
  CAMERA_FAILURE, // Needs to be higher than NO_BODY

  COUNT = 1000
};

// Displays a fault code to the face
// Will queue fault codes until animation process is able to read
// from the fifo
static int DisplayFaultCode(uint16_t code)
{
  // If the fifo doesn't exist create it
  if(access(FaultCode::kFaultCodeFifoName, F_OK) == -1)
  {
    int res = mkfifo(FaultCode::kFaultCodeFifoName, S_IRUSR | S_IWUSR);
    if(res < 0)
    {
      printf("DisplayFaultCode: mkfifo failed %d", errno);
      return errno;
    }
  }
 
  int fifo = open(FaultCode::kFaultCodeFifoName, O_WRONLY);
  if(fifo < 0)
  {
    printf("DisplayFaultCode: Failed to open fifo %d\n", errno);
    return errno;
  }
  
  // Write the fault code to the socket
  ssize_t numBytes = write(fifo, &code, sizeof(code));
  if(numBytes != sizeof(code))
  {
    printf("DisplayFaultCode: Expected to write %u bytes but only wrote %u\n",
	   sizeof(code),
	   numBytes);

    if(numBytes == 0)
    {
      printf("DisplayFaultCode: Write failed %d\n", errno);
      return errno;
    }
  }
  
  int rc = close(fifo);
  if(rc < 0)
  {
    printf("DisplayFaultCode: Failed to close fifo %d\n", errno);
    return errno;
  }
  
  return 0;
}

}
}
}

#endif

