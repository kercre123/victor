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
#include <sys/socket.h>
#include <sys/un.h>

namespace Anki {
namespace Cozmo {

namespace FaultCode {

static const char* kFaultCodeSocketName = "/run/error_code";

// Enum of fault codes, range 800 - 999
// Higher numbers take precedence when displaying
enum : uint16_t {
  NONE = 0,

  WIFI = 800,
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
// assuming animation process is running
static int DisplayFaultCode(uint16_t code)
{
  struct sockaddr_un name;
  size_t size;

  // Create a local socket
  int sock = socket(PF_LOCAL, SOCK_DGRAM, 0);
  if(sock < 0)
  {
    printf("DisplayFaultCode: Failed to create socket %d\n", errno);
    return errno;
  }

  name.sun_family = AF_LOCAL;
  strncpy(name.sun_path, kFaultCodeSocketName, sizeof(name.sun_path));
  name.sun_path[sizeof(name.sun_path) - 1] = '\0';
  size = (offsetof(struct sockaddr_un, sun_path) + strlen(name.sun_path));

  // Connect to the named fault code socket
  int rc = connect(sock, (struct sockaddr*)&name, size);
  if(rc < 0)
  {
    printf("DisplayFaultCode: Connect failed %d\n", errno);
    return errno;
  }

  // Write the fault code to the socket
  ssize_t numBytes = write(sock, &code, sizeof(code));
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
  
  rc = close(sock);
  if(rc < 0)
  {
    printf("DisplayFaultCode: Failed to close socket %d\n", errno);
    return errno;
  }
  
  return 0;
}

}
}
}

#endif

