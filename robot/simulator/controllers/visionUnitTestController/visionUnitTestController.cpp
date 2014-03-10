/*
 * File:          visionUnitTestController.cpp
 * Author:        Andrew Stein
 * Date:          2/13/2014
 *
 * Description:   Webot controller to load vision test worlds and create JSON
 *                ground truth files for vision system unit tests.
 *
 * Modifications: 
 * 
 * Copyright 2014, Anki, Inc.
 */


#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <array>
#include <fstream>
#include <webots/Supervisor.hpp>

#include "json/json.h"

#include "anki/common/types.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/platformPathManager.h"

#include "anki/vision/basestation/camera.h"

#include "anki/cozmo/basestation/messages.h"


#define USE_MATLAB_DETECTION 1

#if USE_MATLAB_DETECTION
#include "anki/common/robot/matlabInterface.h"
#endif

using namespace Anki;

#ifdef SIMULATOR
// TODO: put this elsewhere?
Anki::Vision::CameraCalibration::CameraCalibration(const webots::Camera* camera)
{
  nrows  = static_cast<u16>(camera->getHeight());
  ncols  = static_cast<u16>(camera->getWidth());
  
  f32 width  = static_cast<f32>(ncols);
  f32 height = static_cast<f32>(nrows);
  f32 aspect = width/height;
  
  f32 fov_hor = camera->getFov();
  f32 fov_ver = fov_hor / aspect;
  
  f32 fy = height / (2.f * std::tan(0.5f*fov_ver));
  
  focalLength_x = fy;
  focalLength_y = fy;
  center.x()    = 0.5f*width;
  center.y()    = 0.5f*height;
  skew          = 0.f;
  
  // TODO: Add radial distortion coefficients
  
}
#endif


/*
 * This is the main program.
 * The arguments of the main function can be specified by the
 * "controllerArgs" field of the Robot node
 */
int main(int argc, char **argv)
{
  const int TIME_STEP = 5;
  
  // TODO: Add specification of head angle too
  
  const int NUM_POSE_VALS = 8;
  
  if(argc < (NUM_POSE_VALS+1)) {
    fprintf(stderr, "Not enough controllerArgs to specify a single robot pose.\n");
    return -1;
  }
  else if( ((argc-1) % NUM_POSE_VALS) != 0 ) {
    fprintf(stderr, "Robot poses should be specified in groups of 8 values (Xaxis,Yaxis,Zaxis,Angle,Tx,Ty,Tz,HeadAngle).\n");
    return -1;
  }
  
  const int numPoses = (argc-1)/NUM_POSE_VALS;
  int rotIndex = 1;
  int transIndex = 5;
  int headAngleIndex = 8;
  
#if USE_MATLAB_DETECTION
  // Create a Matlab engine and initialize the path
  Embedded::Matlab matlab(false);
  matlab.EvalStringEcho("run(fullfile('..', '..', '..', '..', 'matlab', 'initCozmoPath'));");
#endif
  
  webots::Supervisor webotRobot_;
  
  // Motors
  webots::Motor* headMotor_  = webotRobot_.getMotor("HeadMotor");
  webots::Motor* liftMotor_  = webotRobot_.getMotor("LiftMotor");
  webots::Motor* liftMotor2_ = webotRobot_.getMotor("LiftMotorFront");
  
  // Enable position measurements on head and lift
  headMotor_->enablePosition(TIME_STEP);
  liftMotor_->enablePosition(TIME_STEP);
  liftMotor2_->enablePosition(TIME_STEP);
 
  // Lower the lift out of the way
  liftMotor_->setPosition(0.f);
  liftMotor2_->setPosition(0.f);

  // Camera
  webots::Camera* headCam_ = webotRobot_.getCamera("HeadCamera");
  headCam_->enable(TIME_STEP);
  Vision::CameraCalibration calib(headCam_);
  Json::Value jsonCalib;
  calib.CreateJson(jsonCalib);
  
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
  
  
  Json::Value root;
  
  // Store the ground truth block poses and world name
  int numBlocks = 0;
  webots::Node* rootNode = webotRobot_.getRoot();
  webots::Field* children = rootNode->getField("children");
  const int numNodes = children->getCount();
  for(int i_node=0; i_node<numNodes; ++i_node) {
    webots::Node* child = children->getMFNode(i_node);
    
    webots::Field* nameField = child->getField("name");
    if(nameField != NULL && nameField->getSFString().compare(0,5,"Block") == 0)
    {
      ObjectType_t blockType = std::stoi(child->getField("type")->getSFString());
      if(blockType > 0)
      {
        Json::Value jsonBlock;
        jsonBlock["Type"] = blockType;
        
        jsonBlock["BlockName"] = child->getField("name")->getSFString();
        
        const double *blockTrans_m = child->getField("translation")->getSFVec3f();
        const double *blockRot   = child->getField("rotation")->getSFRotation();
        for(int i=0; i<3; ++i) {
          jsonBlock["BlockPose"]["Translation"].append(M_TO_MM(blockTrans_m[i]));
          jsonBlock["BlockPose"]["Axis"].append(blockRot[i]);
        }
        jsonBlock["BlockPose"]["Angle"] = blockRot[3];
        
        root["Blocks"].append(jsonBlock);
        numBlocks++;
      }
      else {
        fprintf(stdout, "Skipping unobserved (Type 0) block.\n");
      }
    } // if this is a block
    else if(child->getType() == webots::Node::WORLD_INFO) {
      root["WorldTitle"] = child->getField("title")->getSFString();
      
      std::string checkPoseStr = child->getField("info")->getMFString(0);
      if(checkPoseStr.back() == '0') {
        root["CheckRobotPose"] = false;
      }
      else if(checkPoseStr.back() == '1') {
        root["CheckRobotPose"] = true;
      }
      else {
        CORETECH_THROW("Unexpected character when looking for CheckRobotPose "
                       "setting in WorldInfo.\n");
      }
    }
    
  } // for each node
  root["NumBlocks"] = numBlocks;
  
  // Store the camera calibration
  root["CameraCalibration"] = jsonCalib;
  
  CORETECH_ASSERT(root.isMember("WorldTitle"));
  
  std::string outputPath = PlatformPathManager::getInstance()->PrependPath(PlatformPathManager::Test, "basestation/test/blockWorldTests/") + root["WorldTitle"].asString();
  
  for(int i_pose=0; i_pose<numPoses; ++i_pose,
      rotIndex+=NUM_POSE_VALS, transIndex+=NUM_POSE_VALS, headAngleIndex+=NUM_POSE_VALS)
  {
   
    // Move to next pose
    const double translation_m[3] = {
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

    const double headAngle = atof(argv[headAngleIndex]);
    headMotor_->setPosition(headAngle);
    
    rotField->setSFRotation(rotation);
    transField->setSFVec3f(translation_m);

    fprintf(stdout, "Moving robot '%s' to (%.3f,%.3f,%.3f), "
            "%.1fdeg @ (%.3f,%.3f,%.3f), with headAngle=%.1fdeg\n", robotName.c_str(),
            translation_m[0], translation_m[1], translation_m[2],
            RAD_TO_DEG(rotation[3]), rotation[0], rotation[1], rotation[2],
            RAD_TO_DEG(headAngle));
    
    // Step until the head and lift are in position
    const float TOL = DEG_TO_RAD(0.5f);
    float headErr, liftErr, liftErr2;
    do {
      webotRobot_.step(TIME_STEP);
      headErr  = fabs(headMotor_->getPosition()  - headMotor_->getTargetposition());
      liftErr  = fabs(liftMotor_->getPosition()  - liftMotor_->getTargetposition());
      liftErr2 = fabs(liftMotor2_->getPosition() - liftMotor2_->getTargetposition());
      //fprintf(stdout, "HeadErr = %.4f, LiftErr = %.4f, LiftErr2 = %.4f\n", headErr, liftErr, liftErr2);
    } while(headErr > TOL || liftErr > TOL || liftErr2 > TOL);
    //fprintf(stdout, "Head and lift in position. Continuing.\n");
    
    Json::Value currentPose;

    // Store the image from the current position
    std::string imgFilename = outputPath + std::to_string(i_pose) + ".png";
    headCam_->saveImage(imgFilename, 100);
    
    // Store the associated image file
    currentPose["ImageFile"]  = imgFilename;
    
    // Store the ground truth robot pose
    for(int i=0; i<3; ++i) {
      currentPose["RobotPose"]["Translation"].append(M_TO_MM(translation_m[i]));
      currentPose["RobotPose"]["Axis"].append(rotation[i]);
    }
    currentPose["RobotPose"]["Angle"]     = rotation[3];
    currentPose["RobotPose"]["HeadAngle"] = headAngle;
    
    std::vector<Cozmo::MessageVisionMarker> markers;

#if USE_MATLAB_DETECTION
    // Process the image with Matlab to detect the vision markers
    matlab.EvalStringEcho("img = imread('%s'); "
                          "img = separable_filter(img, gaussian_kernel(0.5)); "
                          "imwrite(img, '%s'); "
                          "markers = simpleDetector(img); "
                          "numMarkers = length(markers);",
                          imgFilename.c_str(), imgFilename.c_str());

    const int numMarkers = static_cast<int>(*matlab.Get<double>("numMarkers"));
    fprintf(stdout, "Detected %d markers at pose %d.\n", numMarkers, i_pose);
    
    for(int i_marker=0; i_marker<numMarkers; ++i_marker) {
      Cozmo::MessageVisionMarker msg;
      matlab.EvalStringEcho("marker = markers{%d}; "
                            "corners = marker.corners; "
                            "code = marker.codeID; ", i_marker+1);

      const double* x_corners = mxGetPr(matlab.GetArray("corners"));
      const double* y_corners = x_corners + 4;
      
      // Sutract one for Matlab indexing!
      msg.x_imgUpperLeft  = x_corners[0]-1.f;
      msg.y_imgUpperLeft  = y_corners[0]-1.f;
      
      msg.x_imgLowerLeft  = x_corners[1]-1.f;
      msg.y_imgLowerLeft  = y_corners[1]-1.f;
      
      msg.x_imgUpperRight = x_corners[2]-1.f;
      msg.y_imgUpperRight = y_corners[2]-1.f;
      
      msg.x_imgLowerRight = x_corners[3]-1.f;
      msg.y_imgLowerRight = y_corners[3]-1.f;
      
      msg.markerType = static_cast<u16>(mxGetScalar(matlab.GetArray("code")));
      /*
      mxArray* mxByteArray = matlab.GetArray("byteArray");
      CORETECH_ASSERT(mxGetNumberOfElements(mxByteArray) == VISION_MARKER_CODE_LENGTH);
      const u8* code = reinterpret_cast<const u8*>(mxGetData(mxByteArray));
      std::copy(code, code + VISION_MARKER_CODE_LENGTH, msg.code.begin());
      */
      
      markers.emplace_back(msg);
      
    } // for each marker
#else
    // TODO: process with embedded vision instead of needing Matlab
    fprintf(stderr, "Non-matlab detection not implemented yet.\n");
    return -1;
#endif
    
    // Store the VisionMarkers
    currentPose["NumMarkers"] = numMarkers;
    for(auto & marker : markers) {
      Json::Value jsonMarker = marker.CreateJson();
      
      fprintf(stdout, "Creating JSON for marker type %d with corners (%.1f,%.1f), (%.1f,%.1f), "
              "(%.1f,%.1f), (%.1f,%.1f)\n",
              marker.markerType,
              marker.x_imgUpperLeft,  marker.y_imgUpperLeft,
              marker.x_imgLowerLeft,  marker.y_imgLowerLeft,
              marker.x_imgUpperRight, marker.y_imgUpperRight,
              marker.x_imgLowerRight, marker.y_imgLowerRight);
      
      currentPose["VisionMarkers"].append(jsonMarker);
      
    } // for each marker
    
    root["Poses"].append(currentPose);
    
  } // for each pose
 
  // Actually write the Json to file
  
  std::string jsonFilename = outputPath + ".json";
  std::ofstream jsonFile(jsonFilename, std::ofstream::out);
  
  fprintf(stdout, "Writing JSON to file %s.\n", jsonFilename.c_str());
  jsonFile << root.toStyledString();
  jsonFile.close();
  
  webotRobot_.simulationQuit(EXIT_SUCCESS);
  
  return 0;
}
