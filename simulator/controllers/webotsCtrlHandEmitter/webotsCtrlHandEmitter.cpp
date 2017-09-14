/*
 * File:          webotsCtrlHandEmitter.cpp
 * Date:
 * Description:   Webots controller for simulated hand that triggers touch sensor
 * Author:        
 * Modifications: 
 */

#include <webots/Supervisor.hpp>
#include <webots/Emitter.hpp>
#include <cstdio>

// handle to Supervisor
webots::Supervisor hand_controller; 

/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  const int TIMESTEP = 5; //milliseconds

  if(hand_controller.step(TIMESTEP) == -1) {
    printf("ERROR (webotsCtrlHandEmitter): Failed to init hand");
    return -1;
  }

  webots::Emitter* emitter_ = hand_controller.getEmitter("emitter");

  bool res = hand_controller.step(TIMESTEP) != -1;
  while(res)
  {
    uint8_t buf[1] = {0};
    emitter_->send(buf,1);
    res = hand_controller.step(TIMESTEP) != -1;
  }
  
  return 0;
}
