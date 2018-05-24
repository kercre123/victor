/**
* File: faultCodeMain.cpp
*
* Author: Al Chaussee
* Date:   4/5/2018
*
* Description: Displays the first argument as a uint16_t to the face
*              AnimationProcess needs to be running for this to work
*
* Copyright: Anki, Inc. 2018
**/

#include "faultCodes.h"
#include <stdlib.h>
#include <csignal>

void handle(int sig)
{
  exit(sig);
}

int main(int argc, char * argv[])
{
  signal(SIGALRM, handle);

  // Fork a child process and let the parent die
  // so as to not block the caller should DisplayFaultCode
  // block trying to open the named pipe to animation process
  pid_t child = fork();

  if(child == 0)
  {
    // Use an alarm signal to kill the child after a certain amount
    // of time. This in case animation process never starts and would
    // leave this process blocked forever trying to open the named pipe
    alarm(60);
      
    int rc = 1;
  
    if(argc > 1)
    {
      char* end;
      // Convert first argument from a string to a uint16_t
      uint16_t code = (uint16_t)strtoimax(argv[1], &end, 10);
      rc = Anki::Cozmo::FaultCode::DisplayFaultCode(code);
    }
    
    return rc;
  }
  
  return 0;
}
