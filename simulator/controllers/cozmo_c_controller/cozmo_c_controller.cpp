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
#include <string.h>
#include "cozmoBot.h"
#include "engine.h"
#include "hal/hal.h"
#include "app/localization.h"
#include "app/pathFollower.h"
#include "keyboardController.h"
#include "cozmoConfig.h"


/*
 * You may want to add macros here.
 */

// If enabled, robot is controlled manually through keyboard.
//#define ENABLE_KEYBOARD_CONTROL



extern CozmoBot gCozmoBot;



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



/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  gCozmoBot.Init();

  InitMotors();
  PathFollower::InitPathFollower();
  Localization::InitLocalization();
  // TODO: Init more things?

#ifdef ENABLE_KEYBOARD_CONTROL
  EnableKeyboardController();
#endif

  //MatlabTest();

  gCozmoBot.run();
  return 0;
  
  
  
  
  
    
 
  
  /* Enter your cleanup code here */
  //DisableKeyboardController();
  //wb_camera_disable(cam_head);
  //wb_camera_disable(cam_lift);
  //wb_camera_disable(cam_down);
  
  /* This is necessary to cleanup webots resources */
  //wb_robot_cleanup();
  
  return 0;
}
