/**
 * File: testVisionSystem.cpp
 *
 * Author:  Andrew Stein
 * Created: 08/08/2016
 *
 * Description: Unit/regression tests for Cozmo's VisionSystem
 *
 * Copyright: Anki, Inc. 2016
 *
 * --gtest_filter=VisionSystem*
 **/


#include "anki/common/basestation/jsonTools.h"
#include "anki/common/basestation/utils/data/dataPlatform.h"
#include "anki/common/types.h"
#include "anki/common/basestation/math/rotation.h"

#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/visionSystem.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/vision/basestation/imageCache.h"

#include "util/logging/logging.h"
#include "util/fileUtils/fileUtils.h"

#include "gtest/gtest.h"
#include "json/json.h"

#include "opencv2/calib3d/calib3d.hpp"

#include "anki/common/basestation/colorRGBA.h"

extern Anki::Cozmo::CozmoContext* cozmoContext;

TEST(VisionSystem, MarkerDetectionCameraCalibrationTests)
{
  /*
   
   _
   | |
   - _
   | |
   - _
   | |
   - ...
   
   
   ^
   R
   
   ^ +y
   |
   -> +x
   +z out
   
   Marker corners are defined relative to center of bottom left cube in target in this orientation (before rotations
   are applied to get cubes to their actual positions)
   FrontFace is the marker that is facing the robot in this orientation. !FrontMarker is the left marker, that will be
   visible when rotation are applied (rotate 45degree on Z and then -30 degree in Y in this origin)
   */

  

    const f32 halfMarkerSize_mm = 15.f / 2.f;
    const f32 halfTargetFace_mm = 20.f / 2.f;
    const Anki::Quad3f originsFrontFace({
      {-halfMarkerSize_mm, -halfTargetFace_mm,  halfMarkerSize_mm},
      {-halfMarkerSize_mm, -halfTargetFace_mm, -halfMarkerSize_mm},
      { halfMarkerSize_mm, -halfTargetFace_mm,  halfMarkerSize_mm},
      { halfMarkerSize_mm, -halfTargetFace_mm, -halfMarkerSize_mm}
    });
    
    const Anki::Quad3f originsLeftFace({
      {-halfTargetFace_mm,  halfMarkerSize_mm,  halfMarkerSize_mm},
      {-halfTargetFace_mm,  halfMarkerSize_mm, -halfMarkerSize_mm},
      {-halfTargetFace_mm, -halfMarkerSize_mm,  halfMarkerSize_mm},
      {-halfTargetFace_mm, -halfMarkerSize_mm, -halfMarkerSize_mm}
    });
    
    auto GetCoordsForFace = [&originsLeftFace, &originsFrontFace](bool isFrontFace,
                                                                  int numCubesRightOfOrigin,
                                                                  int numCubesAwayRobotFromOrigin,
                                                                  int numCubesAboveOrigin)
    {
      
      Anki::Quad3f whichFace = (isFrontFace ? originsFrontFace : originsLeftFace);
      
      Anki::Pose3d p;
      p.SetTranslation({20.f * numCubesRightOfOrigin,
        20.f * numCubesAwayRobotFromOrigin,
        20.f * numCubesAboveOrigin});
      
      p.ApplyTo(whichFace, whichFace);
      return whichFace;
    };
    
    std::map<Anki::Vision::MarkerType, Anki::Quad3f> _markerTo3dCoords;

    // Bottom row of cubes
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_RIGHT] = GetCoordsForFace(true, 0, 0, 0);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_LEFT]   = GetCoordsForFace(false, 1, -1, 0);
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_FRONT]  = GetCoordsForFace(true, 1, -1, 0);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_TOP]    = GetCoordsForFace(false, 2, -2, 0);
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_BACK]   = GetCoordsForFace(true, 2, -2, 0);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_TOP]    = GetCoordsForFace(false, 3, -3, 0);
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_RIGHT]  = GetCoordsForFace(true, 3, -3, 0);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_LEFT]   = GetCoordsForFace(false, 4, -4, 0);
    
    // Second row of cubes
    _markerTo3dCoords[Anki::Vision::MARKER_ARROW]             = GetCoordsForFace(true, 0, 1, 1);
    
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_2HEXAGONS]     = GetCoordsForFace(true, 1, 0, 1);
    
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_5DIAMONDS]     = GetCoordsForFace(false, 2, -1, 1);
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_4DIAMONDS]     = GetCoordsForFace(true, 2, -1, 1);
    
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_3DIAMONDS]     = GetCoordsForFace(false, 3, -2, 1);
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_2DIAMONDS]     = GetCoordsForFace(true, 3, -2, 1);
    
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_5CIRCLES]      = GetCoordsForFace(false, 4, -3, 1);
    
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_3CIRCLES]      = GetCoordsForFace(false, 5, -4, 1);
    
    // Third row of cubes
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_4HEXAGONS]     = GetCoordsForFace(true, 0, 2, 2);
    
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_2CIRCLES]      = GetCoordsForFace(true, 1, 1, 2);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_FRONT]  = GetCoordsForFace(false, 2, 0, 2);
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_TOP]    = GetCoordsForFace(true, 2, 0, 2);
    
    _markerTo3dCoords[Anki::Vision::MARKER_STAR5]             = GetCoordsForFace(false, 3, -1, 2);
    _markerTo3dCoords[Anki::Vision::MARKER_BULLSEYE2]         = GetCoordsForFace(true, 3, -1, 2);
    
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_5TRIANGLES]    = GetCoordsForFace(false, 4, -2, 2);
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_4TRIANGLES]    = GetCoordsForFace(true, 4, -2, 2);
    
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_3TRIANGLES]    = GetCoordsForFace(false, 5, -3, 2);
    
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_5HEXAGONS]     = GetCoordsForFace(false, 6, -4, 2);
    
    // Fourth row of cubes
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_4CIRCLES]      = GetCoordsForFace(true, 0, 3, 3);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_BACK]   = GetCoordsForFace(true, 1, 2, 3);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_RIGHT]  = GetCoordsForFace(true, 2, 1, 3);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_LEFT]   = GetCoordsForFace(false, 3, 0, 3);
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_FRONT]  = GetCoordsForFace(true, 3, 0, 3);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_BOTTOM] = GetCoordsForFace(false, 4, -1, 3);
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_BACK]   = GetCoordsForFace(true, 4, -1, 3);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_TOP]    = GetCoordsForFace(false, 5, -2, 3);
    
    _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_BOTTOM] = GetCoordsForFace(false, 6, -3, 3);
    
    _markerTo3dCoords[Anki::Vision::MARKER_SDK_2TRIANGLES]    = GetCoordsForFace(false, 7, -4, 3);
  
  Anki::Cozmo::VisionSystem* visionSystem = new Anki::Cozmo::VisionSystem(cozmoContext);
  cozmoContext->GetDataLoader()->LoadRobotConfigs();
  Anki::Result result = visionSystem->Init(cozmoContext->GetDataLoader()->GetRobotVisionConfig());
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  //fx: 507.872742, fy: 507.872742, cx: 639.500000 cy: 359.500000
  //  const f32 kRadialDistCoeff1     = -0.02f;
  //  const f32 kRadialDistCoeff2     = -0.01f;
  //  const f32 kRadialDistCoeff3     = -0.005f;
  //  const f32 kTangentialDistCoeff1 = 0.005f;
  //  const f32 kTangentialDistCoeff2 = 0.0025f;
  
  Anki::Vision::CameraCalibration calib(360,640,290,290,320,180,0.f);
//    Anki::Vision::CameraCalibration calib(720,1280,507.8f,507.8f,639.5f,359.5f,0.f);
  result = visionSystem->UpdateCameraCalibration(calib);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  // Turn on _only_ marker detection
  result = visionSystem->SetNextMode(Anki::Cozmo::VisionMode::Idle, true);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  result = visionSystem->SetNextMode(Anki::Cozmo::VisionMode::DetectingMarkers, true);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  // Make sure we run on every frame
  result = visionSystem->PushNextModeSchedule(Anki::Cozmo::AllVisionModesSchedule({{Anki::Cozmo::VisionMode::DetectingMarkers, Anki::Cozmo::VisionModeSchedule(1)}}));
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  Anki::Vision::ImageCache imageCache;
  
  Anki::Vision::ImageRGB img;
  
  result = img.Load("/Users/alchaussee/Desktop/images/000000000000.png");
//  result = img.Load("/Users/alchaussee/Desktop/images_26435_2.jpg");
  
  
//    result = img.Load("/Users/alchaussee/Desktop/Camera Calibration/images_14900_0.jpg");
  //  result = img.Load("/Users/alchaussee/Desktop/images_20265_0.jpg"); //really warped
  //  result = img.Load("/Users/alchaussee/Desktop/images_34975_1.jpg"); //warped and back
  //  result = img.Load("/Users/alchaussee/Desktop/images_15440_1.jpg"); //warped
  //  result = img.Load("/Users/alchaussee/Desktop/images_12905_1.jpg"); // Unwarped
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  imageCache.Reset(img);
  
  Anki::Cozmo::VisionPoseData robotState; // not needed just to detect markers
  result = visionSystem->Update(robotState, imageCache);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  Anki::Cozmo::VisionProcessingResult processingResult;
  bool resultAvailable = visionSystem->CheckMailbox(processingResult);
  EXPECT_TRUE(resultAvailable);
  
  std::vector<cv::Vec2f> imgPts;
  std::vector<cv::Vec3f> worldPts;
  std::set<Anki::Vision::Marker::Code> codes;
  for(const auto& marker : processingResult.observedMarkers)
  {
    PRINT_NAMED_WARNING("", "%s", marker.GetCodeName());
    ASSERT_TRUE(codes.count(marker.GetCode()) == 0);
    codes.insert(marker.GetCode());
    const auto& iter = _markerTo3dCoords.find(static_cast<Anki::Vision::MarkerType>(marker.GetCode()));
    
    if(iter != _markerTo3dCoords.end())
    {
      const auto& corners = marker.GetImageCorners();
      
      imgPts.push_back({corners.GetTopLeft().x(),
        corners.GetTopLeft().y()});
      worldPts.push_back({iter->second.GetTopLeft().x(),
        iter->second.GetTopLeft().y(),
        iter->second.GetTopLeft().z()});
      
      imgPts.push_back({corners.GetTopRight().x(), corners.GetTopRight().y()});
      worldPts.push_back({iter->second.GetTopRight().x(), iter->second.GetTopRight().y(), iter->second.GetTopRight().z()});
      
      imgPts.push_back({corners.GetBottomLeft().x(), corners.GetBottomLeft().y()});
      worldPts.push_back({iter->second.GetBottomLeft().x(), iter->second.GetBottomLeft().y(), iter->second.GetBottomLeft().z()});
      
      imgPts.push_back({corners.GetBottomRight().x(), corners.GetBottomRight().y()});
      worldPts.push_back({iter->second.GetBottomRight().x(), iter->second.GetBottomRight().y(), iter->second.GetBottomRight().z()});
    }
  }
  
  
  for(int i =0; i < imgPts.size(); ++i)
  {
    auto p = imgPts[i];
    img.DrawFilledCircle({p[0], p[1]}, Anki::NamedColors::RED, 2);
    
    auto w = worldPts[i];
    std::stringstream ss;
    ss << w[0] << " " << w[1] << " " << w[2];
    img.DrawText({p[0], p[1]}, ss.str(), Anki::NamedColors::RED, 0.3);
  }
  
  img.Display("");
  cv::waitKey();
  
//  cv::Mat_<double> D = cv::Mat::zeros(8, 1, CV_64F);
  std::vector<cv::Mat_<double>> R2;
  R2.push_back(cv::Mat::zeros(3, 3, CV_64F));
  std::vector<cv::Mat_<double>> T2;
  T2.push_back(cv::Mat::zeros(3,3,CV_64F));
  
  //  (fx: 507.872742, fy: 507.872742, cx: 639.500000 cy: 359.500000)
  cv::Mat_<double> K2 = (cv::Mat_<double>(3,3) <<
                         362, 0, 303,
                         0, 364, 196,
                         0, 0, 1);
  
  cv::Mat_<double> D = (cv::Mat_<double>(8,1) << -0.1, -0.1, 0.00005, -0.0001, 0.05, 0, 0, 0);
  
//    cv::Mat_<double> K2 = (cv::Mat_<double>(3,3) <<
//                           507, 0, 639,
//                           0, 507, 359,
//                           0, 0, 1);
  
  std::vector<std::vector<cv::Vec2f>> imgPts2;
  std::vector<std::vector<cv::Vec3f>> worldPts2;
  imgPts2.push_back(imgPts);
  worldPts2.push_back(worldPts);
  f32 rms = cv::calibrateCamera(worldPts2, imgPts2, cv::Size(img.GetNumCols(), img.GetNumRows()),
                                K2, D, R2, T2,
                                cv::CALIB_USE_INTRINSIC_GUESS);
  
  std::stringstream ss;
  ss << K2 << std::endl;
  PRINT_NAMED_WARNING("K", "%s", ss.str().c_str());
  
  std::stringstream ss1;
  ss1 << D << std::endl;
  PRINT_NAMED_WARNING("D", "%s", ss1.str().c_str());
  
  PRINT_NAMED_WARNING("", "%f", rms);
  
  Anki::Vision::ImageRGB i;
  i.Load("/Users/alchaussee/Desktop/images/0.png");
  Anki::Vision::ImageRGB imgUndistorted(360,640);
  cv::undistort(i.get_CvMat_(), imgUndistorted.get_CvMat_(), K2, D);
//  imgUndistorted.Display("");
  imgUndistorted.Save("/Users/alchaussee/Desktop/t1.jpg");
//  cv::waitKey();
}

TEST(VisionSystem, MarkerDetectionCameraCalibrationTestsBox)
{
  /*
  
   _
  | |
   - _
    | |
     - _
      | |
       - ...
   
   
    ^
    R
  
  ^ +y
  |
   -> +x
   +z out
  
   Marker corners are defined relative to center of bottom left cube in target in this orientation (before rotations
   are applied to get cubes to their actual positions)
   FrontFace is the marker that is facing the robot in this orientation. !FrontMarker is the left marker, that will be
   visible when rotation are applied (rotate 45degree on Z and then -30 degree in Y in this origin)
  */

  const Anki::Quad3f originsFrontFace({
    {-7.5, -15, 7.5},
    {-7.5, -15, -7.5},
    {7.5, -15, 7.5},
    {7.5, -15, -7.5}
    });
  
  const Anki::Quad3f originsLeftFace({
    {-15, 7.5, 7.5},
    {-15, 7.5, -7.5},
    {-15, -7.5, 7.5},
    {-15, -7.5, -7.5}
    });
  
  const Anki::Quad3f originsBottomFace({
    {-7.5, -7.5 , -15},
    {-7.5, 7.5 , -15},
    {7.5, -7.5 , -15},
    {7.5, 7.5 , -15},
  });
  
  auto GetCoordsForFace = [&originsLeftFace, &originsFrontFace, &originsBottomFace](bool isFrontFace,
                                 int numCubesRightOfOrigin,
                                 int numCubesAwayRobotFromOrigin,
                                 int numCubesAboveOrigin,
                                 bool isBottomFace = false)
  {
    
    Anki::Quad3f whichFace = (isFrontFace ? originsFrontFace : originsLeftFace);
    whichFace = (isBottomFace ? originsBottomFace : whichFace);

    Anki::Pose3d p;
    p.SetTranslation({30.f * numCubesRightOfOrigin,
                      30.f * numCubesAwayRobotFromOrigin,
                      30.f * numCubesAboveOrigin});
    
    p.ApplyTo(whichFace, whichFace);
    return whichFace;
  };
  
  std::map<Anki::Vision::MarkerType, Anki::Quad3f> _markerTo3dCoords;
  
  // Bottom row of cubes
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_LEFT] = GetCoordsForFace(true, 0, 0, 0);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_RIGHT] = GetCoordsForFace(true, 1, 0, 0);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_TOP] = GetCoordsForFace(true, 2, 0, 0);
  
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_3CIRCLES] = GetCoordsForFace(true, -1, 0, 1);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_TOP] = GetCoordsForFace(true, 0, 0, 1);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_BACK] = GetCoordsForFace(true, 1, 0, 1);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEK_BOTTOM] = GetCoordsForFace(true, 2, 0, 1);
  
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_2CIRCLES] = GetCoordsForFace(true, -1, 0, 2);
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_2DIAMONDS] = GetCoordsForFace(true, 0, 0, 2);
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_2HEXAGONS] = GetCoordsForFace(true, 1, 0, 2);
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_2TRIANGLES] = GetCoordsForFace(true, 2, 0, 2);
  
  // Left face
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_BOTTOM] = GetCoordsForFace(false, 3, -1, 0);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_FRONT] = GetCoordsForFace(false, 3, -2, 0);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_LEFT] = GetCoordsForFace(false, 3, -3, 0);
  
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_LEFT] = GetCoordsForFace(false, 3, -1, 1);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_RIGHT] = GetCoordsForFace(false, 3, -2, 1);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_TOP] = GetCoordsForFace(false, 3, -3, 1);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEJ_BACK] = GetCoordsForFace(false, 3, -4, 1);
  
  _markerTo3dCoords[Anki::Vision::MARKER_ARROW] = GetCoordsForFace(false, 3, -1, 2);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_BACK] = GetCoordsForFace(false, 3, -2, 2);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_BOTTOM] = GetCoordsForFace(false, 3, -3, 2);
  _markerTo3dCoords[Anki::Vision::MARKER_LIGHTCUBEI_FRONT] = GetCoordsForFace(false, 3, -4, 2);
  
//  // Bottom of top face
  _markerTo3dCoords[Anki::Vision::MARKER_BULLSEYE2] = GetCoordsForFace(false, -1, -1, 3, true);
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_5TRIANGLES] = GetCoordsForFace(false, 0, -1, 3, true);
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_4TRIANGLES] = GetCoordsForFace(false, 1, -1, 3, true);
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_5HEXAGONS] =  GetCoordsForFace(false, 2, -1, 3, true);
  
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_4DIAMONDS] = GetCoordsForFace(false, 0, -2, 3, true);
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_4CIRCLES] =          GetCoordsForFace(false, 1, -2, 3, true);
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_4HEXAGONS] =      GetCoordsForFace(false, 2, -2, 3, true);

  _markerTo3dCoords[Anki::Vision::MARKER_SDK_3HEXAGONS] = GetCoordsForFace(false, 1, -3, 3, true);
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_3TRIANGLES] =  GetCoordsForFace(false, 2, -3, 3, true);
  
  _markerTo3dCoords[Anki::Vision::MARKER_SDK_3DIAMONDS] = GetCoordsForFace(false, 2, -4, 3, true);
  
  Anki::Cozmo::VisionSystem* visionSystem = new Anki::Cozmo::VisionSystem(cozmoContext);
  cozmoContext->GetDataLoader()->LoadRobotConfigs();
  Anki::Result result = visionSystem->Init(cozmoContext->GetDataLoader()->GetRobotVisionConfig());
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  //fx: 507.872742, fy: 507.872742, cx: 639.500000 cy: 359.500000
//  const f32 kRadialDistCoeff1     = -0.02f;
//  const f32 kRadialDistCoeff2     = -0.01f;
//  const f32 kRadialDistCoeff3     = -0.005f;
//  const f32 kTangentialDistCoeff1 = 0.005f;
//  const f32 kTangentialDistCoeff2 = 0.0025f;

  Anki::Vision::CameraCalibration calib(240,320,290,290,160,120,0.f);
//  Anki::Vision::CameraCalibration calib(720,1280,507.8f,507.8f,639.5f,359.5f,0.f);
  result = visionSystem->UpdateCameraCalibration(calib);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  // Turn on _only_ marker detection
  result = visionSystem->SetNextMode(Anki::Cozmo::VisionMode::Idle, true);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  result = visionSystem->SetNextMode(Anki::Cozmo::VisionMode::DetectingMarkers, true);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  // Make sure we run on every frame
  result = visionSystem->PushNextModeSchedule(Anki::Cozmo::AllVisionModesSchedule({{Anki::Cozmo::VisionMode::DetectingMarkers, Anki::Cozmo::VisionModeSchedule(1)}}));
  ASSERT_EQ(Anki::Result::RESULT_OK, result);

  Anki::Vision::ImageCache imageCache;
  
  Anki::Vision::ImageRGB img;
  
//  result = img.Load("/Users/alchaussee/Desktop/images_4410_0.jpg");
  result = img.Load("/Users/alchaussee/Desktop/images_31635_0.jpg");
  
  
//  result = img.Load("/Users/alchaussee/Desktop/images_5135_0.jpg");
//  result = img.Load("/Users/alchaussee/Desktop/images_20265_0.jpg"); //really warped
//  result = img.Load("/Users/alchaussee/Desktop/images_34975_1.jpg"); //warped and back
//  result = img.Load("/Users/alchaussee/Desktop/images_15440_1.jpg"); //warped
//  result = img.Load("/Users/alchaussee/Desktop/images_12905_1.jpg"); // Unwarped
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  imageCache.Reset(img);
  
  Anki::Cozmo::VisionPoseData robotState; // not needed just to detect markers
  result = visionSystem->Update(robotState, imageCache);
  ASSERT_EQ(Anki::Result::RESULT_OK, result);
  
  Anki::Cozmo::VisionProcessingResult processingResult;
  bool resultAvailable = visionSystem->CheckMailbox(processingResult);
  EXPECT_TRUE(resultAvailable);
  
  std::vector<cv::Vec2f> imgPts;
  std::vector<cv::Vec3f> worldPts;
  std::set<Anki::Vision::Marker::Code> codes;
  for(const auto& marker : processingResult.observedMarkers)
  {
    PRINT_NAMED_WARNING("", "%s", marker.GetCodeName());
    ASSERT_TRUE(codes.count(marker.GetCode()) == 0);
    codes.insert(marker.GetCode());
    const auto& iter = _markerTo3dCoords.find(static_cast<Anki::Vision::MarkerType>(marker.GetCode()));
    
    if(iter != _markerTo3dCoords.end())
    {
      const auto& corners = marker.GetImageCorners();
    
      imgPts.push_back({corners.GetTopLeft().x(),
                        corners.GetTopLeft().y()});
      worldPts.push_back({iter->second.GetTopLeft().x(),
                          iter->second.GetTopLeft().y(),
                          iter->second.GetTopLeft().z()});
      
      imgPts.push_back({corners.GetTopRight().x(), corners.GetTopRight().y()});
      worldPts.push_back({iter->second.GetTopRight().x(), iter->second.GetTopRight().y(), iter->second.GetTopRight().z()});
      
      imgPts.push_back({corners.GetBottomLeft().x(), corners.GetBottomLeft().y()});
      worldPts.push_back({iter->second.GetBottomLeft().x(), iter->second.GetBottomLeft().y(), iter->second.GetBottomLeft().z()});
      
      imgPts.push_back({corners.GetBottomRight().x(), corners.GetBottomRight().y()});
      worldPts.push_back({iter->second.GetBottomRight().x(), iter->second.GetBottomRight().y(), iter->second.GetBottomRight().z()});
    }
  }
  
  
  for(int i =0; i < imgPts.size(); ++i)
  {
    auto p = imgPts[i];
    img.DrawFilledCircle({p[0], p[1]}, Anki::NamedColors::RED, 2);
    
    auto w = worldPts[i];
    std::stringstream ss;
    ss << w[0] << " " << w[1] << " " << w[2];
    img.DrawText({p[0], p[1]}, ss.str(), Anki::NamedColors::RED, 0.3);
  }
  
  img.Display("");
  cv::waitKey();
  
  cv::Mat_<double> D = cv::Mat::zeros(8, 1, CV_64F);
  std::vector<cv::Mat_<double>> R2;
  R2.push_back(cv::Mat::zeros(3, 3, CV_64F));
  std::vector<cv::Mat_<double>> T2;
  T2.push_back(cv::Mat::zeros(3,3,CV_64F));
  
//  (fx: 507.872742, fy: 507.872742, cx: 639.500000 cy: 359.500000)
  cv::Mat_<double> K2 = (cv::Mat_<double>(3,3) <<
                          290, 0, 160,
                          0, 290, 120,
                          0, 0, 1);
  
//  cv::Mat_<double> K2 = (cv::Mat_<double>(3,3) <<
//                         507, 0, 639,
//                         0, 507, 359,
//                         0, 0, 1);
  
  std::vector<std::vector<cv::Vec2f>> imgPts2;
  std::vector<std::vector<cv::Vec3f>> worldPts2;
  imgPts2.push_back(imgPts);
  worldPts2.push_back(worldPts);
  cv::calibrateCamera(worldPts2, imgPts2, cv::Size(img.GetNumCols(), img.GetNumRows()),
                      K2, D, R2, T2,
                      cv::CALIB_USE_INTRINSIC_GUESS);
  
    std::stringstream ss;
    ss << K2 << std::endl;
    PRINT_NAMED_WARNING("K", "%s", ss.str().c_str());
  
  std::stringstream ss1;
  ss1 << D << std::endl;
  PRINT_NAMED_WARNING("D", "%s", ss1.str().c_str());
}

TEST(VisionSystem, MarkerDetectionTests)
{
# define DISABLED        0
# define ENABLED         1
# define ENABLE_AND_SAVE 2
  
# define DEBUG_DISPLAY DISABLED
  
  using namespace Anki;
  
  // Construct a vision system
  Cozmo::VisionSystem visionSystem(cozmoContext);
  cozmoContext->GetDataLoader()->LoadRobotConfigs();
  Result result = visionSystem.Init(cozmoContext->GetDataLoader()->GetRobotVisionConfig());
  ASSERT_EQ(RESULT_OK, result);
  
  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  Vision::CameraCalibration calib(240,320,290.f,290.f,160.f,120.f,0.f);
  result = visionSystem.UpdateCameraCalibration(calib);
  ASSERT_EQ(RESULT_OK, result);
  
  // Turn on _only_ marker detection
  result = visionSystem.SetNextMode(Cozmo::VisionMode::Idle, true);
  ASSERT_EQ(RESULT_OK, result);
  
  result = visionSystem.SetNextMode(Cozmo::VisionMode::DetectingMarkers, true);
  ASSERT_EQ(RESULT_OK, result);
  
  // Make sure we run on every frame
  result = visionSystem.PushNextModeSchedule(Cozmo::AllVisionModesSchedule({{Cozmo::VisionMode::DetectingMarkers, Cozmo::VisionModeSchedule(1)}}));
  ASSERT_EQ(RESULT_OK, result);
  
  // Grab all the test images from "resources/test/lowLightMarkerDetectionTests"
  const std::string testImageDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                   "test/markerDetectionTests");
  
  struct TestDefinition
  {
    std::string subDir;
    f32         expectedFailureRate;
    std::function<bool(size_t numMarkers)> didSucceedFcn;
  };
  
  const std::vector<TestDefinition> testDefinitions = {
    
    TestDefinition{
      .subDir = "BacklitStack",
      .expectedFailureRate = 0.f,
      .didSucceedFcn = [](size_t numMarkers) -> bool
      {
        return numMarkers >= 2;
      }
    },
    
    TestDefinition{
      .subDir = "LowLight",
      .expectedFailureRate = .02f,
      .didSucceedFcn = [](size_t numMarkers) -> bool
      {
        return numMarkers > 0;
      }
    },
    
    TestDefinition{
      .subDir = "NoMarkers",
      .expectedFailureRate = 0.f,
      .didSucceedFcn = [](size_t numMarkers) -> bool
      {
        return numMarkers == 0;
      }
    },
    
  };
  

  Vision::ImageCache imageCache;
  
  for(auto & testDefinition : testDefinitions)
  {
    const std::string& subDir = testDefinition.subDir;
    
    const std::vector<std::string> testFiles = Util::FileUtils::FilesInDirectory(Util::FileUtils::FullFilePath({testImageDir, subDir}), false, ".jpg");
    
    u32 numFailures = 0;
    for(auto & filename : testFiles)
    {
      Vision::ImageRGB img;
      result = img.Load(Util::FileUtils::FullFilePath({testImageDir, subDir, filename}));
      ASSERT_EQ(RESULT_OK, result);
      
      imageCache.Reset(img);
      
      Cozmo::VisionPoseData robotState; // not needed just to detect markers
      result = visionSystem.Update(robotState, imageCache);
      ASSERT_EQ(RESULT_OK, result);
      
      Cozmo::VisionProcessingResult processingResult;
      bool resultAvailable = visionSystem.CheckMailbox(processingResult);
      EXPECT_TRUE(resultAvailable);
      
      // For now, the measure of "success" for an image is if we detected at least
      // one marker in it. We are not checking whether the marker's type or position
      // is correct.
      if(!testDefinition.didSucceedFcn(processingResult.observedMarkers.size()))
      {
        ++numFailures;
      }
      
      if(DEBUG_DISPLAY != DISABLED)
      {
        for(auto const& debugImg : processingResult.debugImageRGBs)
        {
          debugImg.second.Display(debugImg.first.c_str());
        }
        
        Vision::ImageRGB dispImg(img);
        for(auto const& marker : processingResult.observedMarkers)
        {
          dispImg.DrawQuad(marker.GetImageCorners(), NamedColors::RED, 2);
          dispImg.DrawLine(marker.GetImageCorners().GetTopLeft(),
                           marker.GetImageCorners().GetTopRight(),
                           NamedColors::GREEN, 3);
          std::string markerName(Vision::MarkerTypeStrings[marker.GetCode()]);
          markerName = markerName.substr(strlen("MARKER_"), std::string::npos);
          
          // Use leftmost corner to anchor text
          Point2f textPoint(std::numeric_limits<f32>::max(), 0.f);
          for(auto const& corner : marker.GetImageCorners())
          {
            if(corner.x() < textPoint.x())
            {
              textPoint = corner;
            }
          }
          dispImg.DrawText(textPoint, markerName, NamedColors::YELLOW, 0.5f, true);
        }
        auto meanVal = cv::mean(img.get_CvMat_());
        dispImg.DrawText(Point2f(1,9), "mean: " +  std::to_string((u8)meanVal[0]), NamedColors::RED, 0.4f, true);
        dispImg.Display(filename.c_str(), 0);
        
        if(DEBUG_DISPLAY == ENABLE_AND_SAVE)
        {
          dispImg.Save("temp/markerDetectionTests/" + filename + ".png");
        }
      }
      
    }
    
    const f32 failureRate = (f32) numFailures / (f32) testFiles.size();
    
    PRINT_NAMED_INFO("VisionSystem.MarkerDetectionTests",
                     "%s: %.1f%% failures (%u of %zu)",
                     subDir.c_str(), 100.f*failureRate, numFailures, testFiles.size());
    
    // Note that we're not expecting perfection here
    EXPECT_LE(failureRate, testDefinition.expectedFailureRate);
  }
  
# undef DEBUG_DISPLAY
# undef ENABLED
# undef DISABLED
# undef ENABLE_AND_SAVE
  
} // MarkerDetectionTests


// Makes sure image quality matches the subdirectory name for all images in
// test/imageQualityTests
TEST(VisionSystem, ImageQuality)
{
  using namespace Anki;
  
  // Construct a vision system
  Cozmo::VisionSystem visionSystem(cozmoContext);

  // Make sure we are running every frame so we don't skip test images!
  Json::Value config = cozmoContext->GetDataLoader()->GetRobotVisionConfig();
  
  Result result = visionSystem.Init(config);
  ASSERT_EQ(RESULT_OK, result);
  
  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  Vision::CameraCalibration calib(240,320,290.f,290.f,160.f,120.f,0.f);
  result = visionSystem.UpdateCameraCalibration(calib);
  ASSERT_EQ(RESULT_OK, result);
  
  // Turn on _only_ image quality check
  result = visionSystem.SetNextMode(Cozmo::VisionMode::Idle, true);
  ASSERT_EQ(RESULT_OK, result);
  
  result = visionSystem.SetNextMode(Cozmo::VisionMode::CheckingQuality, true);
  ASSERT_EQ(RESULT_OK, result);
  
  result = visionSystem.PushNextModeSchedule(Cozmo::AllVisionModesSchedule({{Cozmo::VisionMode::CheckingQuality, Cozmo::VisionModeSchedule(1)}}));
  ASSERT_EQ(RESULT_OK, result);
  
  const std::string testImageDir = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                   "test/imageQualityTests");
  
  const std::vector<std::string> testSubDirs = {
    "Good", "TooBright", "TooDark"
  };
  
  Vision::ImageCache imageCache;
  
  // Fake the exposure parameters so that we are always against the extremes in order
  // to trigger TooDark and TooBright
  const Cozmo::VisionSystem::GammaCurve gammaCurve{};
  result = visionSystem.SetCameraExposureParams(1, 1, 1, 2.f, 2.f, 2.f, gammaCurve);
  ASSERT_EQ(RESULT_OK, result);
  
  for(auto & subDir : testSubDirs)
  {
    const std::vector<std::string> testFiles = Util::FileUtils::FilesInDirectory(Util::FileUtils::FullFilePath({testImageDir, subDir}), false, ".jpg");
    
    for(auto & filename : testFiles)
    {
      Vision::ImageRGB img;
      result = img.Load(Util::FileUtils::FullFilePath({testImageDir, subDir, filename}));
      ASSERT_EQ(RESULT_OK, result);
      
      imageCache.Reset(img);
      
      Cozmo::VisionPoseData robotState; // not needed for image quality check
      result = visionSystem.Update(robotState, imageCache);
      ASSERT_EQ(RESULT_OK, result);
      
      Cozmo::VisionProcessingResult processingResult;
      bool resultAvailable = visionSystem.CheckMailbox(processingResult);
      EXPECT_TRUE(resultAvailable);
      
      // Make sure the detected quality is as expected for this subdir
      EXPECT_EQ(subDir, EnumToString(processingResult.imageQuality));
      
      PRINT_NAMED_INFO("VisionSystem.ImageQuality", "%s = %s",
                       filename.c_str(), EnumToString(processingResult.imageQuality));
      
#     define DISPLAY_IMAGES 0
      if(DISPLAY_IMAGES)
      {
        for(auto const& debugImg : processingResult.debugImageRGBs)
        {
          debugImg.second.Display(debugImg.first.c_str());
        }
        img.Display("TestImage", 0);
      }
    }
  }
  
} // ImageQuality


