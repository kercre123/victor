/**
 * File: testGazePredictionTests.cpp
 *
 * Author: Robert Cosgriff
 * Created: 2/2/2018
 *
 * Description: Tests for gaze prediction, and eye contact
 *
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "util/helpers/includeGTest.h"
#include "util/fileUtils/fileUtils.h"

#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/cameraCalibration.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/faceTrackerImpl_okao.h"
#include "coretech/vision/engine/eyeContact.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <utility>
#include <stdlib.h>

using namespace Anki::Vision;
using namespace Anki;


TEST(EyeContact, GazeEstimationInterface){
  Camera camera(1);
  const std::vector<f32> distortionCoeffs = {{-0.07167206757206086,
                                              -0.2198782133395603,
                                              0.001435740245449692,
                                              0.001523365725052927,
                                              0.1341471670512819,
                                              0, 0, 0}};
  auto calib = std::make_shared<CameraCalibration>(360,640,
      362.8773099149878, 366.7347434532929, 302.2888225643724,
      200.012543449327, 0, distortionCoeffs);
  camera.SetCalibration(calib);

  Json::Value config;
  Json::Value faceDetection;
  faceDetection["DetectionMode"] = "video";
  config["FaceDetection"] = faceDetection;

  Json::Value faceRecognition;
  faceRecognition["RunMode"] = "synchronous";
  config["FaceRecognition"] = faceRecognition;

  Json::Value initialVisionModes;
  initialVisionModes["DetectingFaces"] = true;
  config["InitialVisionModes"] = initialVisionModes;
  config["FaceAlbum"] = "robot";

  const std::string path = "foo";
  FaceTracker faceTracker(camera, path, config);
  faceTracker.EnableGazeDetection(true);
  faceTracker.EnableBlinkDetection(true);

  Result lastResult = RESULT_OK;

  char* root_dir = getenv("ROOT_DIR");
  ASSERT_FALSE(root_dir == nullptr);
  std::string base = root_dir;
  std::string static_path = "/resources/test/gazeEstimationTests/clip1";
  std::string pathToImages = base + static_path;

  // TODO also should not need this output directory either
  std::vector<std::string> imageFiles = 
    Anki::Util::FileUtils::FilesInDirectory(pathToImages, true, ".jpg");

  uint imageNumber = 0;
  for (auto const& filename : imageFiles) {

    // TODO should really only be loading the the gray scale images
    Vision::ImageRGB img;
    lastResult = img.Load(filename.c_str());
    Vision::Image grayImage = img.ToGray();

    // Do the gaze estimation
    std::list<TrackedFace> faces;
    std::list<UpdatedFaceID> updatedIDs;
    lastResult = faceTracker.Update(grayImage, faces, updatedIDs);
    if (faces.size() >= 1) {
      auto face = faces.back();

      auto gaze = face.GetGaze();
      ASSERT_GT(gaze.leftRight_deg, -30);
      ASSERT_LT(gaze.leftRight_deg, 30);
      ASSERT_GT(gaze.upDown_deg,-20);
      ASSERT_LT(gaze.upDown_deg, 20);

      auto blink = face.GetBlinkAmount();
      ASSERT_LT(blink.blinkAmountLeft, 1.f);
      ASSERT_GT(blink.blinkAmountLeft, 0.f);
      ASSERT_LT(blink.blinkAmountRight, 1.f);
      ASSERT_GT(blink.blinkAmountRight, 0.f);

      ASSERT_EQ(imageNumber, 1);
      return;
    } // if face found

    imageNumber += 1;
  } // imageFiles
}

TEST(EyeContact, DISABLED_GazeEstimationAccuracy){

  Camera camera(1);
  const std::vector<f32> distortionCoeffs = {{-0.07167206757206086,
                                              -0.2198782133395603,
                                              0.001435740245449692,
                                              0.001523365725052927,
                                              0.1341471670512819,
                                              0, 0, 0}};
  auto calib = std::make_shared<CameraCalibration>(360,640,
      362.8773099149878, 366.7347434532929, 302.2888225643724,
      200.012543449327, 0, distortionCoeffs);
  camera.SetCalibration(calib);

  Json::Value config;
  Json::Value faceDetection;
  faceDetection["DetectionMode"] = "video";
  config["FaceDetection"] = faceDetection;

  Json::Value faceRecognition;
  faceRecognition["RunMode"] = "synchronous";
  config["FaceRecognition"] = faceRecognition;

  Json::Value initialVisionModes;
  initialVisionModes["DetectingFaces"] = true;
  config["InitialVisionModes"] = initialVisionModes;
  config["FaceAlbum"] = "robot";

  const std::string path = "foo";
  FaceTracker faceTracker(camera, path, config);
  faceTracker.EnableGazeDetection(true);
  faceTracker.EnableBlinkDetection(true);

  Result lastResult = RESULT_OK;

  std::vector<std::string> clips = {"clip1/", "clip2/", "clip3/", "clip4/"};

  char* root_dir = getenv("ROOT_DIR");
  ASSERT_FALSE(root_dir == nullptr);
  std::string base = root_dir;
  std::string static_path = "/resources/test/gazeEstimationTests/";
  std::string pathToImages = base + static_path;

  std::string outputDir("/tmp/outputDirGaze/");
  for (const auto& clip: clips) {
    // TODO also should not need this output directory either
    std::string outputClipDir = outputDir + clip;
    Anki::Util::FileUtils::CreateDirectory(outputClipDir, false, true);
    std::vector<std::string> imageFiles = 
      Anki::Util::FileUtils::FilesInDirectory(pathToImages + clip, 
      true, ".jpg");

    std::ofstream gazeOutput(outputClipDir + "gazePoints.txt", std::ifstream::out);

    uint imageNumber = 0;
    for (auto const& filename : imageFiles) {

      // TODO should really only be loading the the gray scale images
      Vision::ImageRGB img;
      lastResult = img.Load(filename.c_str());
      Vision::Image grayImage = img.ToGray();

      // Do the gaze estimation
      std::list<TrackedFace> faces;
      std::list<UpdatedFaceID> updatedIDs;
      lastResult = faceTracker.Update(grayImage, faces, updatedIDs);
      if (faces.size() >= 1) {
        auto face = faces.back();

        auto roll = RAD_TO_DEG(face.GetHeadRoll().ToFloat());
        auto yaw = RAD_TO_DEG(face.GetHeadYaw().ToFloat());
        auto pitch = RAD_TO_DEG(face.GetHeadPitch().ToFloat());
        auto gaze = face.GetGaze();
        auto blink = face.GetBlinkAmount();
        auto translation = face.GetHeadPose().GetTranslation();
        float sum = 0.f;
        for (int i = 0; i < 3; i++) {
          sum += translation[i]*translation[i];
        }
        sum = std::sqrt(sum);
        std::string rotationString = "roll : " + std::to_string(static_cast<int>(roll))
          + " pitch : " + std::to_string(static_cast<int>(pitch))
          + " yaw : " + std::to_string(static_cast<int>(yaw))
          + " distance : " + std::to_string(static_cast<int>(sum));
        std::string blinkString = " blink left %: "
          + std::to_string(static_cast<int>(100*blink.blinkAmountLeft))
          + " blink right % : "
          + std::to_string(static_cast<int>(100*blink.blinkAmountRight));

        gazeOutput << "imageNumber " << imageNumber <<
          " leftRight_deg " << gaze.leftRight_deg <<
          " upDown_deg " << gaze.upDown_deg <<
          " roll " << roll <<
          " pitch " << pitch <<
          " yaw " << yaw <<
          " distance " << sum << std::endl;
        // There isn't any reason for orange other than it stands out
        // against the green white balance we currently have
        ColorRGBA drawColor = NamedColors::ORANGE;

        // Put the rotation on the image
        img.DrawText(Point2f(10, 20), rotationString, drawColor, 0.5);
        img.DrawText(Point2f(10, 40), blinkString, drawColor, 0.5);

        uint maxGazeLeftRight = 30;
        uint maxGazeUpDown = 20;
        auto dot_x = (gaze.leftRight_deg + maxGazeLeftRight) / 
          (2 * maxGazeLeftRight) * (img.GetNumCols() - 1);
        auto dot_y = (-gaze.upDown_deg + maxGazeUpDown) / 
          (2 * maxGazeUpDown) * (img.GetNumRows() - 1);

        // Sometimes the library returns values outside of the
        // ranges provided in the specs; threshold to force
        // the output to be in those specs
        if (dot_x < 1) {
          dot_x = 1; 
        } else if (dot_x > (img.GetNumCols() - 1)) {
          dot_x = img.GetNumCols() - 1;
        }
        if (dot_y < 1) {
          dot_y = 1; 
        } else if (dot_y > (img.GetNumRows() - 1)) {
          dot_y = img.GetNumRows() - 1; 
        }

        Point2f gazeDot(dot_x, dot_y);
        img.DrawFilledCircle(gazeDot, drawColor, 5);

        // TODO there must be a better way to do this
        std::string unpaddedName = std::to_string(imageNumber);
        auto numberOfZeros = 3 - unpaddedName.size();
        std::string output_filename;
        if (numberOfZeros > 0) {
          output_filename = std::string(numberOfZeros, '0').append(unpaddedName);
        } else {
          output_filename = unpaddedName;
        }

        auto fullFilePath = outputDir + clip + output_filename + ".jpg";
        img.Save(fullFilePath);
      } // if face found

      imageNumber += 1;
    } // imageFiles
  }
}

TEST(EyeContact, EyeContactInterface) {

  Json::Value gazeData;
  char* root_dir = getenv("ROOT_DIR");
  ASSERT_FALSE(root_dir == nullptr);
  std::string base = root_dir;
  std::string static_path = "/resources/test/gazeEstimationTests/clip2/gazePoints.json";
  std::ifstream gazeDataFile(base + static_path, std::ifstream::binary);
  gazeDataFile >> gazeData;

  std::map <uint , std::pair<int, int>> gazePoints;
  for (Json::Value::iterator it = gazeData.begin();
        it != gazeData.end(); it++) {

    auto key = it.key().asString();
    uint frameNumber = static_cast<uint>(std::stoi(key));

    auto value = (*it);
    auto gazeX = value[0].asInt();
    auto gazeY = value[1].asInt();

    std::pair<int, int> gazePoint(gazeX, gazeY);
    gazePoints[frameNumber] = std::move(gazePoint);
  }

  std::string inputPath = std::string(getenv("ROOT_DIR")) +
    "/resources/test/gazeEstimationTests/";
  EyeContact eyeContact;
  TrackedFace face;
  TimeStamp_t timeStamp;

  for (const auto& kv: gazePoints) {

    auto frameNumber = kv.first;
    auto gazeX = kv.second.first;
    auto gazeY = kv.second.second;
    face.SetGaze(gazeX, gazeY);
    timeStamp = frameNumber;
    eyeContact.Update(face, timeStamp);

    auto makingEyeContact = eyeContact.GetEyeContact();
    if (frameNumber < 6) {
      ASSERT_FALSE(makingEyeContact);
    } else if (frameNumber >= 6 && frameNumber < 12) { 
      ASSERT_TRUE(makingEyeContact);
    } else {
      return;
    }
  }
}

TEST(EyeContact, DISABLED_EyeContactEvaluation) {
  std::vector<std::string> clips = {"clip1/", "clip2/", "clip3/", "clip4/"};
  char* root_dir = getenv("ROOT_DIR");
  ASSERT_FALSE(root_dir == nullptr);
  std::string base = root_dir;
  std::string inputPath = std::string(getenv("ROOT_DIR")) +
    "/resources/test/gazeEstimationTests/";
  std::string filename = "gazePoints.json";

  for (const auto& clip: clips) {
    Json::Value gazeData;
    std::ifstream gazeDataFile(inputPath + clip + filename, std::ifstream::binary);
    gazeDataFile >> gazeData;

    std::map <uint , std::pair<int, int>> gazePoints;
    for (Json::Value::iterator it = gazeData.begin();
          it != gazeData.end(); it++) {

      // TODO is this really the best way to do this, seems
      // a bit verbose?
      auto key = it.key().asString();
      uint frameNumber = static_cast<uint>(std::stoi(key));

      auto value = (*it);
      auto gazeX = value[0].asInt();
      auto gazeY = value[1].asInt();

      std::pair<int, int> gazePoint(gazeX, gazeY);
      gazePoints[frameNumber] = std::move(gazePoint);
    }

    EyeContact eyeContact;
    uint lastFrameNumber = 0;

    std::string inputPath = std::string(getenv("ROOT_DIR")) +
      "/resources/test/gazeEstimationTests/";
    std::string outputDir("/tmp/fixationOutputDir/");
    TrackedFace trackedFace;
    TimeStamp_t timeStamp;
    for (const auto& kv: gazePoints) {

      auto frameNumber = kv.first;
      auto gazeX = kv.second.first;
      auto gazeY = kv.second.second;
      timeStamp = frameNumber;
      trackedFace.SetGaze(gazeX, gazeY);

      // Make sure if we missed a detection we pass
      // that along to the fixation detector
      // TODO include head pose data, currently not in the file we read in
      eyeContact.Update(trackedFace, timeStamp);

      char buffer[4];
      std::snprintf(buffer, sizeof(buffer), "%03d", frameNumber);
      std::string filename = inputPath + clip + buffer + ".jpg";
      Vision::ImageRGB img;
      img.Load(filename.c_str());

      ColorRGBA orange = NamedColors::ORANGE;
      uint maxGazeLeftRight = 30;
      uint maxGazeUpDown = 20;
      std::vector<GazeData> history = eyeContact.GetHistory();
      int index = 0;
      for (const auto& gazeData: history) {
        auto dot_x = (static_cast<float>(gazeData.point[0]) + maxGazeLeftRight) / 
          (2 * maxGazeLeftRight) * (img.GetNumCols() - 1);
        auto dot_y = (static_cast<float>(-gazeData.point[1]) + maxGazeUpDown) / 
          (2 * maxGazeUpDown) * (img.GetNumRows() - 1);

        // Sometimes the library returns values outside of the
        // ranges provided in the specs; threshold to force
        // the output to be in those specs
        if (dot_x < 1) {
          dot_x = 1; 
        } else if (dot_x > (img.GetNumCols() - 1)) {
          dot_x = img.GetNumCols() - 1;
        }
        if (dot_y < 1) {
          dot_y = 1; 
        } else if (dot_y > (img.GetNumRows() - 1)) {
          dot_y = img.GetNumRows() - 1; 
        }
        Point2f gazeDot(dot_x, dot_y);
        if (gazeData.inlier) {
          img.DrawFilledCircle(gazeDot, orange, 5);
        } else {
          img.DrawFilledCircle(gazeDot, NamedColors::YELLOW, 5);
        }
        index += 1;
      }

      // TODO the naming here is terrible
      auto averageGaze = eyeContact.GetAverageGaze();
      ColorRGBA green = NamedColors::GREEN;
      auto fixation_dot_x = (averageGaze[0] + maxGazeLeftRight) / 
        (2 * maxGazeLeftRight) * (img.GetNumCols() - 1);
      auto fixation_dot_y = (-averageGaze[1] + maxGazeUpDown) / 
        (2 * maxGazeUpDown) * (img.GetNumRows() - 1);
      Point2f fixationDot(fixation_dot_x, fixation_dot_y);
      img.DrawFilledCircle(fixationDot, green, 6);

      auto makingEyeContact = eyeContact.GetEyeContact();
      Point2f center = Point2f(img.GetNumCols()/2, img.GetNumRows()/2);
      float radius = static_cast<float>(10.f)/(2*maxGazeUpDown) * img.GetNumRows();
      if (makingEyeContact) {
        img.DrawCircle(center, NamedColors::RED, radius);
      } else {
        img.DrawCircle(center, NamedColors::GREEN, radius);
      }

      auto outputFilename = outputDir + clip + buffer + ".jpg";
      img.Save(outputFilename);
      lastFrameNumber = frameNumber;
    }
  }
}
