/*
 * File:          world_comms_controller.cpp
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */

#include "CozmoWorldComms.h"

int main(int argc, char **argv)
{
  CozmoWorldComms comms;
  comms.Init();
  comms.run();

  return 0;
}
