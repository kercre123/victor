/**
 * File: testSurfaceClassifier.cpp
 *
 * Author: Lorenzo Riano
 * Created: 11/8/17
 *
 * Description: Unit test for DrivingSurfaceClassifier
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "anki/common/basestation/math/logisticRegression.h"
#include "basestation/utils/data/dataPlatform.h"
#include "engine/cozmoContext.h"
#include "engine/robotDataLoader.h"
#include "engine/vision/drivingSurfaceClassifier.h"
#include "util/fileUtils/fileUtils.h"

#include "gtest/gtest.h"

extern Anki::Cozmo::CozmoContext *cozmoContext;

/*
 * Tests that training works with a specific error when not using class weighting
 */
TEST(SurfaceClassifier, TestWholeErrorNoWeighting) {
  Json::Value config;
  // populate the Json file
  {
    config["NumClusters"] = 5;
    config["TrainingAlpha"] = 1.0;
    config["NumIterations"] = 10000;
    config["PositiveClassWeight"] = 1.0;
    config["RegularizationType"] = "Disable";
  }

  Anki::Cozmo::DrivingSurfaceClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/run7");

  PRINT_NAMED_INFO("TestSurfaceClassifier.TrainingFromFile.PrintPath",
                   "Path to training data is %s", path.c_str());

  const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
  const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

  bool result = clf.TrainFromFiles(positivePath.c_str(), negativePath.c_str());
  ASSERT_TRUE(result);

  cv::Mat trainingSamples, trainingLabels;
  clf.GetTrainingData(trainingSamples, trainingLabels);


  // building a std::vector<Vision::PixelRGB>
  std::vector<Anki::Vision::PixelRGB> pixels;
  {
    pixels.reserve(trainingSamples.rows);
    EXPECT_EQ(trainingSamples.type(), CV_32FC1);
    const cv::Vec3f* data = trainingSamples.ptr<cv::Vec3f>(0);
    for (int i = 0; i < trainingSamples.rows; ++i) {
      const cv::Vec3f& vec = data[i];
      pixels.emplace_back(vec[0], vec[1], vec[2]);
    }
  }

  // calculate error
  std::vector<uchar> responses = clf.PredictClass(pixels);
  cv::Mat responsesMat(responses);

  const float error = Anki::calculateError(responsesMat, trainingLabels);

  // Expect the error to be below 0.42 for the overall dataset
  EXPECT_PRED_FORMAT2(::testing::FloatLE, error, 0.42);

}

/*
 * Test that training works better with a higher weight for the positive class.
 */
TEST(SurfaceClassifier, TestPositiveClassOnlyWithWeights)
{

  Json::Value config;
  // populate the Json file
  {
    config["NumClusters"] = 5;
    config["TrainingAlpha"] = 1.0;
    config["NumIterations"] = 10000;
    config["PositiveClassWeight"] = 2.0;
    config["RegularizationType"] = "Disable";
  }

  Anki::Cozmo::DrivingSurfaceClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/run7");

  PRINT_NAMED_INFO("TestSurfaceClassifier.TrainingFromFile.PrintPath",
                   "Path to training data is %s", path.c_str());

  const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
  const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

  bool result = clf.TrainFromFiles(positivePath.c_str(), negativePath.c_str());
  ASSERT_TRUE(result);

  cv::Mat trainingSamples, trainingLabels;
  clf.GetTrainingData(trainingSamples, trainingLabels);

  const float realNumberOfPositives = float(cv::sum(trainingLabels)[0]);

  // Test the positive class only
  ASSERT_EQ(trainingSamples.type(), CV_32FC1);
  ASSERT_EQ(trainingLabels.type(), CV_32FC1);

  cv::Mat positiveY;
  std::vector<Anki::Vision::PixelRGB> pixels;
  pixels.reserve(trainingSamples.rows); // probably won't be using that much

  auto elementX = trainingSamples.begin<cv::Vec3f>();
  auto elementY = trainingLabels.begin<float>();
  for (; elementX != trainingSamples.end<cv::Vec3f>() && elementY != trainingLabels.end<float>();
         elementX += 3, elementY++) {
    const float value = *elementY;
    if ( value != 0) {
      const cv::Vec3f& vec = *elementX;
      pixels.emplace_back(uchar(vec[0]), uchar(vec[1]), uchar(vec[2]));
      positiveY.push_back(value);
    }
  }
  ASSERT_EQ(positiveY.rows, realNumberOfPositives);

  // calculate error
  std::vector<uchar> responses = clf.PredictClass(pixels);
  cv::Mat responsesMat(responses);

  const float error = Anki::calculateError(responsesMat, positiveY);

  // Expect the error to be below 0.12 for the positive elements
  EXPECT_PRED_FORMAT2(::testing::FloatLE, error, 0.12);

}