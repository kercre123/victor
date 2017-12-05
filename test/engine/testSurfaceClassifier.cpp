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

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>

#define DISPLAY_IMAGES false

extern Anki::Cozmo::CozmoContext *cozmoContext;

void visualizeClassifierOnImages(const char *pathToImages, Anki::Cozmo::DrivingSurfaceClassifier& clf) {

  std::vector<std::string> imageFiles = Anki::Util::FileUtils::FilesInDirectory(pathToImages, true);
  uint imageno = 0;

  for (const auto& filename : imageFiles) {
    std::cout<<"Classifying image "<<++imageno<<" over "<<imageFiles.size()<<" total"<<std::endl;
    const cv::Mat image = cv::imread(filename, cv::IMREAD_COLOR);
    if (image.empty()) {
      std::cerr<<"Error while loading image: "<<filename<<std::endl;
      continue;
    }

    // create a mask of pixels
    cv::Mat mask(image.rows, image.cols, CV_8UC3);
    auto imageIterator = image.begin<cv::Vec3b>();
    auto maskIterator = mask.begin<cv::Vec3b>();
    // iterate over all the pixels in image, if classifier says 1 then set mask to red
    while (imageIterator != image.end<cv::Vec3b>()) {
      const cv::Vec3b vec = *imageIterator;
      const Anki::Vision::PixelRGB pixel(vec[2], vec[1], vec[0]);
      const uchar label = clf.PredictClass(pixel);
      if (label) {
        *maskIterator = cv::Vec3b(0,0,255); // Red
      }
      else {
        *maskIterator = cv::Vec3b(0,0,0); // Black
      }
      ++maskIterator;
      ++imageIterator;

    }
    const float alpha = 0.5;
    const cv::Mat toDisplay = alpha * image + (1.0 - alpha) * mask;

    cv::namedWindow("Mask", cv::WINDOW_AUTOSIZE);
    cv::imshow("Mask", mask);
    cv::namedWindow("Image", cv::WINDOW_AUTOSIZE);
    cv::imshow("Image", image);
    cv::waitKey(10);

//    {
//      const std::string saveFilename = "/Users/lorenzori/tmp/results_images/mask_" + std::to_string(imageno) + ".png";
//      cv::imwrite(saveFilename, mask);
//    }
//    {
//      const std::string saveFilename = "/Users/lorenzori/tmp/results_images/image_" + std::to_string(imageno) + ".png";
//      cv::imwrite(saveFilename, image);
//    }

  }
}

/*
 * Trains the classifier from the files and performs accuracy checks
 */
void checkClassifier(Anki::Cozmo::DrivingSurfaceClassifier& clf,
                     const std::string& positivePath,
                     const std::string& negativePath) {
  bool result = clf.TrainFromFiles(positivePath.c_str(), negativePath.c_str());
  ASSERT_TRUE(result);

  cv::Mat trainingSamples, trainingLabels;
  clf.GetTrainingData(trainingSamples, trainingLabels);

  const float realNumberOfPositives = float(cv::sum(trainingLabels)[0]);

  // Error on whole training set
  {
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
    EXPECT_PRED_FORMAT2(::testing::FloatLE, error, 0.12);
    std::cout<<"Error on whole training set: "<<error<<std::endl;
  }

  // Error on positive elements
  {
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
    EXPECT_PRED_FORMAT2(::testing::FloatLE, error, 0.12);
    std::cout<<"Error on the positive elements: "<<error<<std::endl;
  }

}

/*
 * Tests that training works with a specific error when not using class weighting
 */
TEST(SurfaceClassifier, LRClassifier_TestWholeErrorNoWeighting) {
  Json::Value config;
  // populate the Json file
  {
    config["NumClusters"] = 5;
    config["TrainingAlpha"] = 1.0;
    config["NumIterations"] = 10000;
    config["PositiveClassWeight"] = 1.0;
    config["RegularizationType"] = "Disable";
  }

  Anki::Cozmo::LRDrivingSurfaceClassifier clf(config, cozmoContext);

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
TEST(SurfaceClassifier, LRClassifier_TestPositiveClassOnlyWithWeights)
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

  Anki::Cozmo::LRDrivingSurfaceClassifier clf(config, cozmoContext);

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

/*
 * Test the Logistic Regression classifier on manually labeled data
 */
TEST(SurfaceClassifier, LRClassifier_TestManualLabels) {
  Json::Value config;
  // populate the Json file
  {
    config["NumClusters"] = 7;
    config["TrainingAlpha"] = 0.5;
    config["NumIterations"] = 10000;
    config["PositiveClassWeight"] = 1.5;
    config["RegularizationType"] = "Disable";
  }

  Anki::Cozmo::LRDrivingSurfaceClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/manual_labels");

  PRINT_NAMED_INFO("TestSurfaceClassifier.TrainingFromFile.PrintPath",
                   "Path to training data is %s", path.c_str());

  const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
  const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

  checkClassifier(clf, positivePath, negativePath);

  if (DISPLAY_IMAGES) {
    visualizeClassifierOnImages("/Users/lorenzori/tmp/overhead_data/real/run8/images", clf);
  }

}

/*
 * Test the threshold classifier
 */
TEST(SurfaceClassifier, THClassifier_TestRealRun) {
  Json::Value config;
  // populate the Json file
  {
    config["NumClusters"] = 7;
    config["MedianMultiplier"] = 2.0;
  }

  Anki::Cozmo::THDrivingSurfaceClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/run7/positivePixels.txt");
  bool result = clf.TrainFromFile(path.c_str());
  ASSERT_TRUE(result);

  // Test that a fraction of all the pixels in the dataset are positive
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

  std::vector<uchar> responses = clf.PredictClass(pixels);
  uint sum = 0;
  for (int j = 0; j < responses.size(); ++j) {
    if (responses[j]) {
      ++sum;
    }
  }
  float ratioPositives = float(sum) / float(responses.size());

  EXPECT_GT(ratioPositives, 0.7);

  if (DISPLAY_IMAGES) {
    visualizeClassifierOnImages("/Users/lorenzori/tmp/overhead_data/real/run8/images", clf);
  }
}

/*
 * Test the decision trees classifier on manually labeled data
 */
TEST(SurfaceClassifier, DTClassifier_TestManualLabels) {
  Json::Value config;
  // populate the Json file
  {
    config["MaxDepth"] = 10;
    config["MinSampleCount"] = 2;
    config["TruncatePrunedTree"] = false;
    config["Use1SERule"] = false;
  }

  Anki::Cozmo::DTDrivingSurfaceClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/manual_labels");

  PRINT_NAMED_INFO("TestSurfaceClassifier.TrainingFromFile.PrintPath",
                   "Path to training data is %s", path.c_str());

  const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
  const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

  checkClassifier(clf, positivePath, negativePath);

  if (DISPLAY_IMAGES) {
    visualizeClassifierOnImages("/Users/lorenzori/tmp/overhead_data/real/run8/images", clf);
  }
}