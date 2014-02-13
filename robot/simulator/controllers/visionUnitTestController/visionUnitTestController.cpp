/*
 * File:          cozmo_c_controller.c
 * Date:
 * Description:   
 * Author:        
 * Modifications: 
 */


#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <array>
#include <fstream>
#include <webots/Supervisor.hpp>

#include "json/json.h"

#include "anki/common/types.h"
#include "anki/common/robot/matlabInterface.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/cozmo/basestation/messages.h"

#define USE_MATLAB_DETECTION 1

/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  const int TIME_STEP = 5;
  
  if(argc < 8) {
    fprintf(stderr, "Not enough controllerArgs to specify a single robot pose.\n");
    return -1;
  }
  else if( ((argc-1) % 7) != 0 ) {
    fprintf(stderr, "Robot poses should be specified in groups of 7 values (Xaxis,Yaxis,Zaxis,Angle,Tx,Ty,Tz).\n");
    return -1;
  }
  
  const int numPoses = (argc-1)/7;
  int rotIndex = 1;
  int transIndex = 5;
  
#if USE_MATLAB_DETECTION
  // Create a Matlab engine and initialize the path
  Anki::Embedded::Matlab matlab(false);
  matlab.EvalStringEcho("run(fullfile('..', '..', '..', '..', 'matlab', 'initCozmoPath'));");
#endif
  
  webots::Supervisor webotRobot_;
  
  // Motors
  webots::Motor* headMotor_  = webotRobot_.getMotor("motor_head_pitch");
  webots::Motor* liftMotor_  = webotRobot_.getMotor("lift_motor");
  webots::Motor* liftMotor2_ = webotRobot_.getMotor("lift_motor2");
  
  // Enable position measurements on head and lift
  headMotor_->enablePosition(TIME_STEP);
  liftMotor_->enablePosition(TIME_STEP);
  liftMotor2_->enablePosition(TIME_STEP);

  // Position the head
  headMotor_->setPosition(-DEG_TO_RAD(10));
  
  // Lower the lift out of the way
  liftMotor_->setPosition(-0.275);
  liftMotor2_->setPosition(0.275);

  // Camera
  webots::Camera* headCam_ = webotRobot_.getCamera("cam_head");
  headCam_->enable(TIME_STEP);
  
  // Grab the robot node and its rotation/translation fields so we can
  // manually move it around to the specified poses
  const std::string robotName(webotRobot_.getName());
  webots::Node* robotNode   = webotRobot_.getFromDef(robotName);
  if(robotNode == NULL) {
    fprintf(stderr, "Could not robot node with DEF '%s'.\n", robotName.c_str());
    return -1;
  }
  webots::Field* transField = robotNode->getField("translation");
  webots::Field* rotField   = robotNode->getField("rotation");
  
  webotRobot_.step(TIME_STEP);
  
  for(int i_pose=0; i_pose<numPoses; ++i_pose, rotIndex+=7, transIndex+=7)
  {
   
    // Move to next pose
    const double translation[3] = {
      atof(argv[transIndex]),
      atof(argv[transIndex+1]),
      atof(argv[transIndex+2])
    };
    
    const double rotation[4] = {
      atof(argv[rotIndex]),
      atof(argv[rotIndex+1]),
      atof(argv[rotIndex+2]),
      atof(argv[rotIndex+3])
    };
    
    fprintf(stdout, "Moving robot '%s' to (%.3f,%.3f,%.3f), "
            "%.1fdeg @ (%.3f,%.3f,%.3f)\n", robotName.c_str(),
            translation[0], translation[1], translation[2],
            rotation[3]*180./M_PI, rotation[0], rotation[1], rotation[2]);
    
    rotField->setSFRotation(rotation);
    transField->setSFVec3f(translation);

    // Take a step to move the robot and get a new image from the camera
    webotRobot_.step(TIME_STEP);
    
    std::string imgFilename("visionUnitTest");
    imgFilename += std::to_string(i_pose);
    imgFilename += ".png";
    headCam_->saveImage(imgFilename, 100);
    
    std::vector<Anki::Cozmo::MessageVisionMarker> markers;
    
#if USE_MATLAB_DETECTION
    // Process the image with Matlab to detect the vision markers
    matlab.EvalStringEcho("markers = simpleDetector('%s'); "
                          "numMarkers = length(markers);", imgFilename.c_str());

    const int numMarkers = static_cast<int>(*matlab.Get<double>("numMarkers"));
    fprintf(stdout, "Detected %d markers at pose %d.\n", numMarkers, i_pose);
    
    for(int i_marker=0; i_marker<numMarkers; ++i_marker) {
      Anki::Cozmo::MessageVisionMarker msg;
      matlab.EvalStringEcho("marker = markers{%d}; "
                            "corners = marker.corners; "
                            "byteArray = marker.byteArray; ", i_marker+1);

      const double* x_corners = mxGetPr(matlab.GetArray("corners"));
      const double* y_corners = x_corners + 4;
      
      msg.x_imgUpperLeft = x_corners[0];
      msg.y_imgUpperLeft = y_corners[0];
      
      msg.x_imgLowerLeft = x_corners[1];
      msg.y_imgLowerLeft = y_corners[1];
      
      msg.x_imgUpperRight = x_corners[2];
      msg.y_imgUpperRight = y_corners[2];
      
      msg.x_imgLowerRight = x_corners[3];
      msg.y_imgLowerRight = y_corners[3];
      
      const u8* code = reinterpret_cast<const u8*>(mxGetData(matlab.GetArray("byteArray")));
      
      std::copy(code, code + VISION_MARKER_CODE_LENGTH, msg.code.begin());
      
      markers.emplace_back(msg);
      
    } // for each marker
#else
    // TODO: process with embedded vision instead of needing Matlab
    fprintf(stderr, "Non-matlab detection not implemented yet.\n");
    return -1;
#endif
    
    // Save each marker
    std::string jsonFilename("VisionMarkers_Pose");
    jsonFilename += std::to_string(i_pose);
    jsonFilename += ".json";
    
    std::ofstream jsonFile(jsonFilename, std::ofstream::out);

    Json::Value root;
    root["ImageFile"]  = imgFilename;
    root["NumMarkers"] = numMarkers;
    
    for(auto & marker : markers) {
      Json::Value jsonMarker = marker.CreateJson();
      
      fprintf(stdout, "Creating JSON for marker to %s with corners (%.1f,%.1f), (%.1f,%.1f), "
              "(%.1f,%.1f), (%.1f,%.1f), with code = [",
              jsonFilename.c_str(),
              marker.x_imgUpperLeft,  marker.y_imgUpperLeft,
              marker.x_imgLowerLeft,  marker.y_imgLowerLeft,
              marker.x_imgUpperRight, marker.y_imgUpperRight,
              marker.x_imgLowerRight, marker.y_imgLowerRight);
      for(int i=0; i<VISION_MARKER_CODE_LENGTH; ++i) {
        fprintf(stdout, "%d ", marker.code[i]);
      }
      fprintf(stdout, "\b]\n");
      
      root["VisionMarkers"].append(jsonMarker);
      
    } // for each marker
    
    fprintf(stdout, "Writing JSON to file %s.\n", jsonFilename.c_str());
    jsonFile << root.toStyledString();
    jsonFile.close();
    
  } // for each pose
 
  // TODO: Stop / quit simulation?
  
  return 0;
}
