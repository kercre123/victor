/*
 * File:         overlay_display_supervisor.c
 * Date:         Sept 13, 2013
 * Description:  For displaying the path that the robot should and does take.
 *               Can be used for whatever other debug info desired.
 */

#include <webots/robot.h>
#include <webots/supervisor.h>
#include <webots/display.h>
#include <stdlib.h>

#define TIME_STEP  64

#define GROUND_X   1.0
#define GROUND_Z   1.0

#define PATH_TRACK_BLEED_RATE 1
#define LIGHT_GREY 0x505050
#define RED        0xBB2222
#define GREEN      0x22BB11
#define BLUE       0x2222BB

// main function
int main() {

  // init Webots stuff
  wb_robot_init();

  // First we get a handler to devices
  WbDeviceTag ground_display = wb_robot_get_device("overlay_display");
  
  // get the properties of the Display
  int width = wb_display_get_width(ground_display);
  int height = wb_display_get_height(ground_display);
  
  // prepare stuff to get the
  // DifferentialWheels(MYBOT).translation field
  WbNodeRef mybot = wb_supervisor_node_get_from_def("Cozmo");
  WbFieldRef translationField = wb_supervisor_node_get_field(mybot,"translation");
  const double *translation;
  
  WbNodeRef estPose = wb_supervisor_node_get_from_def("CozmoBotPose");
  WbFieldRef estTranslationField = wb_supervisor_node_get_field(estPose, "translation");
  const double *estTranslation;
  
  // paint the display's background
  //wb_display_set_color(ground_display,LIGHT_GREY);
  wb_display_fill_rectangle(ground_display,0,0,width,height);
  wb_display_set_color(ground_display,RED);
  wb_display_draw_line(ground_display,0,height/2,width-1,height/2);
  wb_display_draw_text(ground_display,"x",width-10,height/2-10);
  wb_display_set_color(ground_display,BLUE);
  wb_display_draw_line(ground_display,width/2,0,width/2,height-1);
  //wb_display_draw_text(ground_display,"z",width/2-10,height-10); // Actual z-axis
  wb_display_draw_text(ground_display,"y",width/2-10,0); // In Cozmo world, -ve z-axis is +ve y-axis. 
                                                                 // Can fix this later if we care, but all this stuff is encapsulated in CozmoBot
                                                                 // so it's not a big deal doing all the transforms there.

  // init image ref used to save into the image file
  WbImageRef to_store = NULL;
  
  // init a variable which counts the time steps
  int counter = 0;
  
  while(wb_robot_step(TIME_STEP)!=-1) {
  
    // Update the translation field
    translation = wb_supervisor_field_get_sf_vec3f(translationField);
    
    // Update the counter
    counter++;
    
    // display the robot's true position
    wb_display_set_opacity(ground_display,PATH_TRACK_BLEED_RATE);
    wb_display_set_color(ground_display,GREEN);
    wb_display_fill_oval(ground_display,
                         width*(translation[0]+GROUND_X/2)/GROUND_X,
                         height*(translation[2]+GROUND_Z/2)/GROUND_Z,
                         1, 1);
    
    // display the robot's estimated position
    wb_display_set_opacity(ground_display,0.5f);
    estTranslation = wb_supervisor_field_get_sf_vec3f(estTranslationField);
    wb_display_set_color(ground_display, RED);
    wb_display_draw_oval(ground_display,
                         width*(estTranslation[0] + GROUND_X/2)/GROUND_X,
                         height*(estTranslation[2]+GROUND_Z/2)/GROUND_Z,
                         1.5, 1.5);
    
    // Clear previous to_store
    if (to_store) {
      wb_display_image_delete(ground_display,to_store);
      to_store = NULL;
    }
    
    // Every 50 steps, store the resulted image into a file
    if (counter%50 == 0) {
      to_store = wb_display_image_copy(ground_display,0,0,width,height);
      wb_display_image_save(ground_display,to_store,"screenshot.png");
    }
  }

  wb_robot_cleanup();

  return 0;
}
