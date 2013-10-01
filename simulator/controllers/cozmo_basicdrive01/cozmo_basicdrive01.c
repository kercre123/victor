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

//Names of the wheels used for steering
#define WHEEL_FL "wheel_fl"
#define WHEEL_FR "wheel_fr"
#define PITCH "motor_head_pitch"
#define LIFT "lift_motor"
#define LIFT2 "lift_motor2"
#define GOAL "goal_motor"

#define DRIVE_VELOCITY_SLOW 5.0f
#define TURN_VELOCITY_SLOW 1.0f
#define HIST 50
#define LIFT_CENTER -0.275
#define LIFT_UP 0.635
#define LIFT_UPUP 0.7

//Why do some of those not match ASCII codes?
//Numbers, spacebar etc. work, letters are different, why?
//a, z, s, x, Space
#define CKEY_LIFT_UP    65 
#define CKEY_LIFT_DOWN  90 
#define CKEY_HEAD_UP    83 
#define CKEY_HEAD_DOWN  88 
#define CKEY_UNLOCK     32 


//Reference to the steering wheels
WbDeviceTag wheels[2];
//The head pitch motor
WbDeviceTag head_pitch;
//The lift motor
WbDeviceTag lift;
WbDeviceTag lift2;
//Camera on the head
WbDeviceTag cam_head;
WbDeviceTag cam_lift;

WbDeviceTag con_lift;

//Count down after unlock until the next lock
int unlockhysteresis = 0;


void InitCozmo(void)
{
  wheels[0] = wb_robot_get_device(WHEEL_FL);
  wheels[1] = wb_robot_get_device(WHEEL_FR);
  head_pitch = wb_robot_get_device(PITCH);
  lift = wb_robot_get_device(LIFT);
  lift2 = wb_robot_get_device(LIFT2);

  
  //Set the motors to velocity mode  
  wb_motor_set_position(wheels[0], INFINITY);
  wb_motor_set_position(wheels[1], INFINITY);
  
  //Set teh head pitch to 0
  wb_motor_set_position(head_pitch, 0);
  wb_motor_set_position(lift, LIFT_CENTER);
  wb_motor_set_position(lift2, -LIFT_CENTER);
  
  
  //Enable the cameras
  cam_head = wb_robot_get_device("cam_head");
  wb_camera_enable(cam_head, TIME_STEP);
  cam_lift = wb_robot_get_device("cam_lift");
  wb_camera_enable(cam_lift, TIME_STEP);
  
  /* get a handler to the connector and the motor. */
  con_lift = wb_robot_get_device("connector");

  /* activate them. */
  wb_connector_enable_presence(con_lift, TIME_STEP);
  
}

void SetLiftAngle(float angle)
{
  wb_motor_set_position(lift, angle);
  wb_motor_set_position(lift2, -angle);
}


//Set an angular wheel velocity in rad/sec
void SetAngularWheelVelocity(float left, float right)
{
  // write actuators inputs
  wb_motor_set_velocity(wheels[0], -left);
  wb_motor_set_velocity(wheels[1], -right);
}

//Check the keyboard keys and issue robot commands
void ControlCozmo()
{
  int key = wb_robot_keyboard_get_key();
  
  static float pitch_angle = 0;
  static float lift_angle = LIFT_CENTER;
  static bool locked = false;
  
  //Should we lock to a block which is close to the connector?
  if (locked == false && wb_connector_get_presence(con_lift) == 1)
  {
    if (unlockhysteresis == 0)
    {
      wb_connector_lock(con_lift);
      locked = true;
      printf("LOCKED!\n");
    }else{
      unlockhysteresis--;
    }
  }
  
  switch (key)
  {
    case WB_ROBOT_KEYBOARD_UP:
    {  
      SetAngularWheelVelocity(DRIVE_VELOCITY_SLOW, DRIVE_VELOCITY_SLOW);
      break;
    }
    
    case WB_ROBOT_KEYBOARD_DOWN:
    {
      SetAngularWheelVelocity(-DRIVE_VELOCITY_SLOW, -DRIVE_VELOCITY_SLOW);
      break;
    }
    
    case WB_ROBOT_KEYBOARD_LEFT:
    {
      SetAngularWheelVelocity(-TURN_VELOCITY_SLOW, TURN_VELOCITY_SLOW);
      break;
    }
    
    case WB_ROBOT_KEYBOARD_RIGHT:
    {
      SetAngularWheelVelocity(TURN_VELOCITY_SLOW, -TURN_VELOCITY_SLOW);
      break;
    }
    
    case CKEY_HEAD_UP: //s-key: move head up
    {
      pitch_angle += 0.01f;
      wb_motor_set_position(head_pitch, pitch_angle);
      break;
    }
    
    case CKEY_HEAD_DOWN: //x-key: move head down
    {
      pitch_angle -= 0.01f;
      wb_motor_set_position(head_pitch, pitch_angle);
      break;
    }
    case CKEY_LIFT_UP: //a-key: move lift up
    {
      lift_angle += 0.02f;
      SetLiftAngle(lift_angle);
      break;
    }
    
    case CKEY_LIFT_DOWN: //z-key: move lift down
    {
      lift_angle -= 0.02f;
      SetLiftAngle(lift_angle);
      break;
    }
    case '1': //set lift to pickup position
    {
      lift_angle = LIFT_CENTER;
      SetLiftAngle(lift_angle);
      break;
    }
    case '2': //set lift to block +1 position
    {
      lift_angle = LIFT_UP;
      SetLiftAngle(lift_angle);
      break;
    }
    case '3': //set lift to highest position
    {
      lift_angle = LIFT_UPUP;
      SetLiftAngle(lift_angle);
      break;
    }
    
    case CKEY_UNLOCK: //spacebar-key: unlock
    {
      if (locked == true)
      {
        locked = false;
        unlockhysteresis = HIST;
        wb_connector_unlock(con_lift);
        printf("UNLOCKED!\n");
      }
      break;
    }
    
    
    default:
    {
      SetAngularWheelVelocity(0, 0);
    }
    
  }
  
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
  InitCozmo();
  
  printf("Drive: arrows\n");
  printf("Lift up/down: a/z\n");
  printf("Head up/down: s/x\n");
  printf("Undock: space\n");
  
  /*
   * You should declare here WbDeviceTag variables for storing
   * robot devices like this:
   *  WbDeviceTag my_sensor = wb_robot_get_device("my_sensor");
   *  WbDeviceTag my_actuator = wb_robot_get_device("my_actuator");
   */
  
  //Update Keyboard every 0.1 seconds
  wb_robot_keyboard_enable(TIME_STEP); 
  
  /* main loop
   * Perform simulation steps of TIME_STEP milliseconds
   * and leave the loop when the simulation is over
   */
  while (wb_robot_step(TIME_STEP) != -1) {
    
    /* 
     * Read the sensors :
     * Enter here functions to read sensor data, like:
     *  double val = wb_distance_sensor_get_value(my_sensor);
     */
    
    /* Process sensor data here */
    
    /*
     * Enter here functions to send actuator commands, like:
     * wb_differential_wheels_set_speed(100.0,100.0);
     */
     
     ControlCozmo();
     
  };
  
  /* Enter your cleanup code here */
  
  //Disable the keyboard
  wb_robot_keyboard_disable();
  wb_camera_disable(cam_head);
  wb_camera_disable(cam_lift);
  
  /* This is necessary to cleanup webots resources */
  wb_robot_cleanup();
  
  return 0;
}
