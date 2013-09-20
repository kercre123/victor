#include "app/mainExecution.h"
#include "app/wheelController.h"
#include "app/steeringController.h"
#include "app/vehicleSpeedController.h"
#include "hal/timers.h"
#include "hal/encoders.h"
#include "cozmoConfig.h"
#include "cozmoTypes.h"
#include <stdio.h>
 
     
void CozmoMainExecution(void) 
{  
  s16 fidx = 0;
  


  // Figure out what fidx should be according to pathFollower
  // ...


  // TESTING
  static u32 startDriveTime_us = 1000000;
  static BOOL driving = FALSE;
  if (!driving && getMicroCounter() > startDriveTime_us) {
    SetUserCommandedAcceleration(ONE_OVER_CONTROL_DT + 1);  // This can't be smaller than 1/CONTROL_DT!  
    SetUserCommandedDesiredVehicleSpeed(30);
    //printf("Speed: %d mm/s\n", GetUserCommandedDesiredVehicleSpeed() );
    driving = TRUE;
  }
  fidx = 2;

  //printf("\n");
  
  // Speed controller
  ManageVehicleSpeedController();


  // Steering controller
  ManageSteeringController(fidx);

  
  s16 wspeedlPWM = 0; s16 wspeedrPWM = 0;
  //Run one iteration of the encoder speed filter
  EncoderSpeedFilterIteration();
  // Wheel controller
  ManageWheelSpeedController(&wspeedlPWM, &wspeedrPWM);

}