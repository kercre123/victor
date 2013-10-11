*
 * File:         inverted_pendulum.c
 * Date:         September 11th, 2006
 * Description:  A controller using the PID technic to control an inverted
 *               pendulum.
 * Author:       Simon Blanchoud
 * Modifications:
 *
 * Copyright (c) 2008 Cyberbotics - www.cyberbotics.com
 */

#include <webots/robot.h>
#include <webots/motor.h>
#include <webots/position_sensor.h>
#include <math.h>
#include <stdlib.h>

#define MAX_FORCE 40
#define CENTERING_ANGLE 0.7
#define CENTERING_FORCE 4
#define TIME_STEP 64

static WbDeviceTag horizontal_motor, hip;
static double previous_position, differential, integral_sum;

static void initialize()
{
    wb_robot_init();
    horizontal_motor = wb_robot_get_device("horizontal_motor");
    hip = wb_robot_get_device("hipmotor");
    //wb_motor_enable_position(horizontal_motor, TIME_STEP);
    //wb_position_sensor_enable(hip, TIME_STEP);
    
    wb_motor_set_position(hip, 0);

    previous_position = wb_position_sensor_get_value(hip);
    differential = 0;
    integral_sum = 0;
}


static void run()
{
  static float pos = 0;
  static bool inc = true;
  
  if (inc == true)
  {
    pos += 0.02;
    
    if (pos > 3.14159/4)
    {
      inc = false;
    }
  }else{
  
    pos -= 0.02;
    
    if (pos < -3.14159/4)
    {
      inc = true;
    }
  }
  
  
  wb_motor_set_position(hip, pos);
}

int main()
{
    initialize();
    while (1) {
      wb_robot_step(TIME_STEP);
      run();
    }
    return 0;
}
