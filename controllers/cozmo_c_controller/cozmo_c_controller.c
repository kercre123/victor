/*
 * File:          cozmo_c_controller.c
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
#include <webots/camera.h>

#include <string.h>
//#include "engine.h"

#include "app/mainExecution.h"

#include "hal/motors.h"
#include "keyboardController.h"
#include "cozmoConfig.h"

/*
 * You may want to add macros here.
 */




//Camera on the head
WbDeviceTag cam_head;
WbDeviceTag cam_lift;
WbDeviceTag cam_down;



void InitCozmo(void)
{

  //Enable the cameras
  cam_head = wb_robot_get_device("cam_head");
  wb_camera_enable(cam_head, TIME_STEP);
  cam_lift = wb_robot_get_device("cam_lift");
  wb_camera_enable(cam_lift, TIME_STEP);
  cam_down = wb_robot_get_device("cam_down");
  wb_camera_enable(cam_down, TIME_STEP);

  
}



/*
#define  BUFSIZE 256
int MatlabTest(void)
{
  
  Engine *ep;
	mxArray *T = NULL, *result = NULL;
	char buffer[BUFSIZE+1];
	double time[10] = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0 };
  
  if (!(ep = engOpen(""))) {
		fprintf(stderr, "\nCan't start MATLAB engine\n");
		return EXIT_FAILURE;
	}
  

	T = mxCreateDoubleMatrix(1, 10, mxREAL);
	memcpy((void *)mxGetPr(T), (void *)time, sizeof(time));

	engPutVariable(ep, "T", T);
  
	
	engEvalString(ep, "D = .5.*(-9.8).*T.^2;");
  
	
	engEvalString(ep, "plot(T,D);");
	engEvalString(ep, "title('Position vs. Time for a falling object');");
	engEvalString(ep, "xlabel('Time (seconds)');");
	engEvalString(ep, "ylabel('Position (meters)');");
  
	
	printf("Hit return to continue\n\n");
	fgetc(stdin);
	
  printf("Done for Part I.\n");
	mxDestroyArray(T);
	engEvalString(ep, "close;");
  
  
  
  buffer[BUFSIZE] = '\0';
	engOutputBuffer(ep, buffer, BUFSIZE);
	while (result == NULL) {
    char str[BUFSIZE+1];
  
    printf("Enter a MATLAB command to evaluate.  This command should\n");
    printf("create a variable X.  This program will then determine\n");
    printf("what kind of variable you created.\n");
    printf("For example: X = 1:5\n");
    printf(">> ");
    
    fgets(str, BUFSIZE, stdin);
	  
    
    engEvalString(ep, str);
    
    
    printf("%s", buffer);
    
    
    printf("\nRetrieving X...\n");
    if ((result = engGetVariable(ep,"X")) == NULL)
      printf("Oops! You didn't create a variable X.\n\n");
    else {
      printf("X is class %s\t\n", mxGetClassName(result));
    }
	}
  

	printf("Done!\n");
	mxDestroyArray(result);
	engClose(ep);
  
  
  return 0;
}
*/


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
  
  
  
  
  //MatlabTest();
  
  
  // Initialize HAL
  InitMotors();
  
    
  EnableKeyboardController();
  
  
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
     
     RunKeyboardController();
     
     CozmoMainExecution();
     
  };
  
  /* Enter your cleanup code here */
  DisableKeyboardController();
  wb_camera_disable(cam_head);
  wb_camera_disable(cam_lift);
  wb_camera_disable(cam_down);
  
  /* This is necessary to cleanup webots resources */
  wb_robot_cleanup();
  
  return 0;
}
