#include "app/mainExecution.h"
#include "app/wheelController.h"
#include <stdio.h>
 
     
void CozmoMainExecution(void) 
{
  
  
  s16 fidx = 0;
  
  
  
  // Wheel controller
  ManageWheelSpeedController(fidx);

}