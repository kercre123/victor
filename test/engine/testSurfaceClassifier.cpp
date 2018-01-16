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
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/overheadEdge.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/vision/drivingSurfaceClassifier.h"
#include "engine/vision/groundPlaneClassifier.h"
#include "util/fileUtils/fileUtils.h"

#include "gtest/gtest.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>

#define DISPLAY_IMAGES false

extern Anki::Cozmo::CozmoContext *cozmoContext;

void visualizeClassifierOnImages(const std::string& pathToImages, const Anki::Cozmo::DrivingSurfaceClassifier& clf,
                                const Anki::Cozmo::FeaturesExtractor& extractor= Anki::Cozmo::SinglePixelFeaturesExtraction()) {

  std::vector<std::string> imageFiles;
  if (Anki::Util::FileUtils::DirectoryExists(pathToImages)) {
    imageFiles = Anki::Util::FileUtils::FilesInDirectory(pathToImages, true, "jpg");
  }
  else {
    imageFiles.push_back(pathToImages);
  }
  uint imageno = 0;

  for (const auto& filename : imageFiles) {
    std::cout<<"Classifying image "<<++imageno<<" over "<<imageFiles.size()<<" total"<<std::endl;
    Anki::Vision::ImageRGB image;
    image.Load(filename);
    if (image.IsEmpty()) {
      std::cerr<<"Error while loading image: "<<filename<<std::endl;
      continue;
    }

    Anki::Vision::Image mask(image.GetNumRows(), image.GetNumCols());
    ClassifyImage(clf, extractor, image, mask);

    cv::namedWindow("Mask", cv::WINDOW_AUTOSIZE);
    cv::imshow("Mask", mask.get_CvMat_());
    cv::namedWindow("Image", cv::WINDOW_AUTOSIZE);
    // back to BGR before visualization
    cv::cvtColor(image.get_CvMat_(), image.get_CvMat_(), cv::COLOR_RGB2BGR);
    cv::imshow("Image", image.get_CvMat_());
    cv::waitKey(0);

  }
}

// Head looking down is -0.40142554f
Anki::Cozmo::VisionPoseData populateVisionPoseData(float angle) {

  Anki::Pose3d::AllowUnownedParents(true);

  Anki::Cozmo::Robot robot(0, cozmoContext);
  Anki::Cozmo::VisionComponent& component = robot.GetVisionComponent();
  component.SetIsSynchronous(true);

  // Don't really need a valid camera calibration, so just pass a dummy one in
  // to make vision system happy. All that matters is the image dimensions be correct.
  const std::array<f32, 8> distortionCoeffs = {{-0.03822904514363595, -0.2964213946476391, -0.00181089972406104,
                                                 0.001866070303033584, 0.1803429725181202,
                                                 0, 0, 0}};
  auto calib = std::make_shared<Anki::Vision::CameraCalibration>(360,
                                                           640,
                                                           364.7223064012286,
                                                           366.1693698832141,
                                                           310.6264440545544,
                                                           196.6729350209868,
                                                           0,
                                                           distortionCoeffs);
  component.SetCameraCalibration(calib);

  Anki::Matrix_3x3f groundPlaneHomography;
  const bool groundPlaneVisible = component.LookupGroundPlaneHomography(angle,
                                                                        groundPlaneHomography);

  Anki::Cozmo::VisionPoseData poseData;
  poseData.groundPlaneVisible = groundPlaneVisible;
  poseData.groundPlaneHomography = groundPlaneHomography;

  return poseData;
}

// This corresponds to head looking down at -0.40142554f radians
Anki::Cozmo::VisionPoseData populateVisionPoseData() {

  Anki::Cozmo::VisionPoseData poseData;
  poseData.groundPlaneVisible = true;
  Anki::Matrix_3x3f H{284.671, -297.243, 5046.35,
                      24.9759, 4.61332e-05, 12049.2,
                      0.890988, 5.96046e-08, 15.7945};
  poseData.groundPlaneHomography = H;

  return poseData;
};

/*
 * Trains the classifier from the files and performs accuracy checks
 */
void checkClassifier(Anki::Cozmo::DrivingSurfaceClassifier& clf,
                     const std::string& positivePath,
                     const std::string& negativePath,
                     float& resultErrorTrainingSet,
                     float maxTotalError = 0.12,
                     float maxPositivesError = 0.12) {

  bool result = clf.TrainFromFiles(positivePath.c_str(), negativePath.c_str());
  ASSERT_TRUE(result);

  cv::Mat trainingSamples, trainingLabels;
  clf.GetTrainingData(trainingSamples, trainingLabels);

  const float realNumberOfPositives = float(cv::sum(trainingLabels)[0]);

  // Error on whole training set
  {
    std::vector<std::vector<Anki::Cozmo::DrivingSurfaceClassifier::FeatureType>> pixels;
    Anki::Cozmo::convertToVector<float>(trainingSamples, pixels);

    // calculate error
    std::vector<uchar> responses = clf.PredictClass(pixels);
    cv::Mat responsesMat(responses);

    resultErrorTrainingSet = Anki::calculateError(responsesMat, trainingLabels);
    EXPECT_PRED_FORMAT2(::testing::FloatLE, resultErrorTrainingSet, maxTotalError);
    std::cout<<"Error on whole training set: "<<resultErrorTrainingSet<<std::endl;
  }

  // Error on positive elements
  {

    cv::Mat positiveY;
    std::vector<std::vector<Anki::Cozmo::DrivingSurfaceClassifier::FeatureType>> pixels;
    pixels.reserve(trainingSamples.rows); // probably won't be using that much

    // The local friendly lambda. There's a lot going on here just because trainingLabels is a cv::Mat_ with a template
    // type. Getting the type out of the Mat_ is not easy though, hence the dance with decltype
    auto fillPixels = [&trainingSamples = static_cast<const cv::Mat&>(trainingSamples)]
        (cv::Mat& positiveY,
         std::vector<std::vector<Anki::Cozmo::DrivingSurfaceClassifier::FeatureType>>& pixels,
         const auto& trainingLabels) {

      using T = decltype(trainingLabels(0)); //either float or int
      auto elementY = trainingLabels.template begin(); // probably don't need the .template here, but it looks cool :)
      for (uint rowNumber = 0; elementY != trainingLabels.template end(); rowNumber++, elementY++) {
        const T value = *elementY;
        if ( value != 0) { // if it's positive class, add the sample to pixels
          std::vector<Anki::Cozmo::DrivingSurfaceClassifier::FeatureType> rowPixels;
          Anki::Cozmo::convertToVector<float>(trainingSamples.row(rowNumber), rowPixels);
          pixels.push_back(std::move(rowPixels));
          positiveY.push_back(value);
        }
      }
    };


    if (trainingLabels.type() == cv::DataType<int>::type) {
      fillPixels(positiveY, pixels, cv::Mat_<int>(trainingLabels));
    }
    else if (trainingLabels.type() == cv::DataType<float>::type) {
      fillPixels(positiveY, pixels, cv::Mat_<float>(trainingLabels));
    }
    else {
      ASSERT_TRUE(false) << ("Unknown type for trainingLabels");
    }

    ASSERT_EQ(positiveY.rows, realNumberOfPositives);

    // calculate error

    std::vector<uchar> responses = clf.PredictClass(pixels);
    cv::Mat responsesMat(responses);

    const float error = Anki::calculateError(responsesMat, positiveY);
    EXPECT_PRED_FORMAT2(::testing::FloatLE, error, maxPositivesError);
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

  std::vector<std::vector<Anki::Cozmo::DrivingSurfaceClassifier::FeatureType>> pixels;
  Anki::Cozmo::convertToVector<float>(trainingSamples, pixels);

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

  float error = std::numeric_limits<float>::max();
  // There will be a high error on the whole training set, but low on the positives only
  checkClassifier(clf, positivePath, negativePath, error, 0.46, 0.12 );

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

  float error = std::numeric_limits<float>::max();
  checkClassifier(clf, positivePath, negativePath, error);

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

  std::vector<std::vector<Anki::Cozmo::DrivingSurfaceClassifier::FeatureType>> pixels;
  Anki::Cozmo::convertToVector<float>(trainingSamples, pixels);

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

  float error = std::numeric_limits<float>::max();
  checkClassifier(clf, positivePath, negativePath, error);

  if (DISPLAY_IMAGES) {
    visualizeClassifierOnImages("/Users/lorenzori/tmp/overhead_data/real/run8/images", clf);
  }
}

/*
 * Test decision tree classifier serialization and deserialization
 */
TEST(SurfaceClassifier, DTClassifier_TestSerialization) {

  Json::Value config;
  // populate the Json file
  {
    config["MaxDepth"] = 10;
    config["MinSampleCount"] = 10;
    config["TruncatePrunedTree"] = true;
    config["Use1SERule"] = true;
    config["PositiveWeight"] = 1.0f;
  }

  Anki::Cozmo::DTDrivingSurfaceClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/RealImagesDesk");

//  const std::string path = "/Users/lorenzori/tmp/images_training/real_images/selective_annotation";

  PRINT_NAMED_INFO("TestSurfaceClassifier.TrainingFromFile.PrintPath",
                   "Path to training data is %s", path.c_str());

  const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
  const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

  float error = std::numeric_limits<float>::max();
  checkClassifier(clf, positivePath, negativePath, error, 0.14, 0.1);

  // serialize the classifier
  const std::string serializeFileName = Anki::Util::FileUtils::FullFilePath({path, "deskClassifier.json"});
  bool result = clf.Serialize(serializeFileName.c_str());
  ASSERT_TRUE(result);

  // deserialize the classifier
  Anki::Cozmo::DTDrivingSurfaceClassifier clf2(serializeFileName, cozmoContext);
  float error2 = std::numeric_limits<float>::max();
  checkClassifier(clf2, positivePath, negativePath, error2, 0.14, 0.1);
  ASSERT_EQ(error, error2);

  if (DISPLAY_IMAGES) {
    visualizeClassifierOnImages("/Users/lorenzori/tmp/images_training/real_images/single_shots_desk/desk", clf);
  }
}

TEST(SurfaceClassifier, GroundPlaneClassifier_TestNoise) {

  Json::Value root;
  Json::Value& config = root["GroundPlaneClassifier"];
  {
    config["MaxDepth"] = 10;
    config["MinSampleCount"] = 10;
    config["TruncatePrunedTree"] = true;
    config["Use1SERule"] = true;
    config["PositiveWeight"] = 3.0f;
    config["OnTheFlyTrain"] = true;
    config["FileOrDirName"] = "test/overheadMap/RealImagesDesk";
  }

  Anki::Cozmo::GroundPlaneClassifier clf(root, cozmoContext);

  Anki::Cozmo::DebugImageList <Anki::Vision::ImageRGB> debugImageList;
  std::list<Anki::Cozmo::OverheadEdgeFrame> outEdges;

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/RealImagesDesk");
  {
    const std::string imagepath = Anki::Util::FileUtils::FullFilePath({path, "00000305.jpg"});
    PRINT_CH_INFO("VisionSystem", "Test.GroundPlaneClassifier.LoadImage", "Loading image %s", imagepath.c_str());

    Anki::Vision::ImageRGB testImg;
    ASSERT_EQ(testImg.Load(imagepath), Anki::RESULT_OK);
    const Anki::Cozmo::VisionPoseData poseData = populateVisionPoseData(Anki::Util::DegToRad(-15));

    Anki::Result result = clf.Update(testImg, poseData, debugImageList, outEdges);
    ASSERT_EQ(result, Anki::RESULT_OK);

    if (DISPLAY_IMAGES) {
      for (auto& name_image : debugImageList) {
        std::string& name = name_image.first;
        Anki::Vision::ImageRGB& image = name_image.second;
        cv::cvtColor(image.get_CvMat_(), image.get_CvMat_(), cv::COLOR_RGB2BGR);
        cv::namedWindow(name, cv::WINDOW_AUTOSIZE);

        cv::imshow(name, image.get_CvMat_());
      }
      visualizeClassifierOnImages(imagepath, clf.GetClassifier());
    }
  }

  cv::waitKey(0);

}