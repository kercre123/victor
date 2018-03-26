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

#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "coretech/vision/engine/camera.h"
#include "coretech/vision/engine/faceTracker.h"
#include "coretech/vision/engine/eyeContact.h"
#include "engine/cozmoContext.h"

#include <fstream>

extern Anki::Cozmo::CozmoContext* cozmoContext;

using namespace Anki::Vision;
using namespace Anki;

TEST(EyeContact, GazeEstimationInterface)
{
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

  const std::string pathToImages = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                                   "test/gazeEstimationTests");
  std::vector<std::string> imageFiles = 
    Anki::Util::FileUtils::FilesInDirectory(pathToImages, true, ".jpg");

  int frameNumber = 0;
  for (auto const& filename : imageFiles) {
    Vision::Image image;
    lastResult = image.Load(filename);

    // Do the gaze estimation
    std::list<TrackedFace> faces;
    std::list<UpdatedFaceID> updatedIDs;
    lastResult = faceTracker.Update(image, faces, updatedIDs);
    // We don't detect a face in the first frame (even though
    // there is one present) but should find one face in the
    // rest of the frames
    if (frameNumber > 0 ) {
      ASSERT_GE(faces.size(), 1);
      auto const& face = faces.back();

      auto const& gaze = face.GetGaze();
      ASSERT_LT(std::abs(gaze.leftRight_deg), 40);
      ASSERT_LT(std::abs(gaze.upDown_deg), 30);

      auto const& blink = face.GetBlinkAmount();
      ASSERT_LT(blink.blinkAmountLeft, 1.f);
      ASSERT_GT(blink.blinkAmountLeft, 0.f);
      ASSERT_LT(blink.blinkAmountRight, 1.f);
      ASSERT_GT(blink.blinkAmountRight, 0.f);
    }
    frameNumber += 1;
  } // imageFiles
}

TEST(EyeContact, EyeContactInterface)
{
  Json::Value gazeData;
  
  const std::string base = cozmoContext->GetDataPlatform()->pathToResource(Util::Data::Scope::Resources,
                                                                           "test/gazeEstimationTests");
  std::string inputPath = Util::FileUtils::FullFilePath({base, "gazePoints.json"});
  std::ifstream gazeDataFile(inputPath, std::ifstream::binary);
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
    ASSERT_FALSE(eyeContact.GetExpired(timeStamp));
    auto makingEyeContact = eyeContact.IsMakingEyeContact();

    if (frameNumber < 6) {
      ASSERT_FALSE(makingEyeContact);
    } else if (frameNumber >= 6 && frameNumber < 12) { 
      ASSERT_TRUE(makingEyeContact);
    } else {
      return;
    }
  }
}
