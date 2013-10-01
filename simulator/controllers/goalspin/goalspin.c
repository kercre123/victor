/*
 * File:          cozmo_basicdrive01.c
 * Date:          
 * Description:   
 * Author:        
 * Modifications: 
 */

/*
 * You may need to add include files like <webots/distance_sensor.h> or
 * <webots/differential_wheels.h>, etc.
 */
#include <stdio.h>
#include <ctype.h>
#include <webots/robot.h>
#include <webots/motor.h>
#include <webots/camera.h>
#include <webots/connector.h>

/*
 * You may want to add macros here.
 */
#define TIME_STEP 64
#define GOAL "goal_motor"

WbDeviceTag goal;

void InitGoal(void)
{
  goal = wb_robot_get_device(GOAL);

  wb_motor_set_position(goal, 0);
  
}

void updateGoal(void)
{
  static float angle = 0;
  angle += 0.05;
  wb_motor_set_position(goal, angle);
}


/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  /* necessary to initialize webots stuff */
  wb_robot_init();
  InitGoal();
  
  /* main loop
   * Perform simulation steps of TIME_STEP milliseconds
   * and leave the loop when the simulation is over
   */
  while (wb_robot_step(TIME_STEP) != -1) {
     updateGoal();
     
  };
  
  /* This is necessary to cleanup webots resources */
  wb_robot_cleanup();
  
  return 0;
}