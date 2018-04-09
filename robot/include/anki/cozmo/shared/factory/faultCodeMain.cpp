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

int main(int argc, char * argv[])
{
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
