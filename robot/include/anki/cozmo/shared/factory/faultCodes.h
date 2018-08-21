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
#include <stdio.h>

namespace Anki {
namespace Vector {

namespace FaultCode {

static const char* kFaultCodeFifoName = "/run/error_code";

// Enum of fault codes, range 800 - 999
// Higher numbers take precedence when displaying
enum : uint16_t {
  NONE = 0,

  //Use higher numbers for head self-tests
  //display precedence over (external) body tests
  DISPLAY_FAILURE       = 990, //nobody will ever see this :(

  CAMERA_FAILURE        = 980,

  //WIFI                  = 970,
  WIFI_HW_FAILURE       = 970, //local wifi hw checks only

  IMU_FAILURE           = 960,

  //critical processes
  NO_GATEWAY            = 921,
  SYSTEMD               = 919,
  NO_ROBOT_COMMS        = 917,
  NO_ENGINE_COMMS       = 915,
  NO_SWITCHBOARD        = 913,
  AUDIO_FAILURE         = 911,
  STOP_BOOT_ANIM_FAILED = 909,
    
  //Body and external errors
  NO_BODY               = 899, //no response from syscon
  SPINE_SELECT_TIMEOUT  = 898,
    
  //Sensor Errors
  TOUCH_SENSOR          = 895,
  TOF                   = 894,
  CLIFF_BL              = 893,
  CLIFF_BR              = 892,
  CLIFF_FL              = 891,
  CLIFF_FR              = 890,

  //Mic Errors
  MIC_BL                = 873,
  MIC_BR                = 872,
  MIC_FL                = 871,
  MIC_FR                = 870,

  //Cloud Errors
  CLOUD_READ_ESN        = 852,
  CLOUD_TOKEN_STORE     = 851,
  CLOUD_CERT            = 850,

  //Camera config errors
  NO_CAMERA_CALIB       = 840,

  // DO NOT CHANGE any of the codes in this section
  // They are hardcoded into various system level programs
  DFU_FAILED            = 801,
  NO_ANIM_PROCESS       = 800,

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
    printf("DisplayFaultCode: Expected to write %zu bytes but only wrote %zd\n",
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

