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

#include "coretech/common/engine/math/logisticRegression.h"
#include "coretech/common/engine/utils/data/dataPlatform.h"
#include "engine/components/visionComponent.h"
#include "engine/cozmoContext.h"
#include "engine/overheadEdge.h"
#include "engine/robot.h"
#include "engine/robotDataLoader.h"
#include "engine/vision/rawPixelsClassifier.h"
#include "engine/vision/groundPlaneClassifier.h"
#include "util/fileUtils/fileUtils.h"

#include "gtest/gtest.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <typeinfo>

#define DISPLAY_IMAGES false
#define WAIT_KEY (-1) // with <0 no image will be shown, but all the computations to classify it will be done

extern Anki::Cozmo::CozmoContext *cozmoContext;

void VisualizeClassifierOnImages(const std::string& pathToImages, const Anki::Cozmo::RawPixelsClassifier& clf,
                                 const Anki::Cozmo::IFeaturesExtractor& extractor = Anki::Cozmo::SinglePixelFeaturesExtraction())
{

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
    if (WAIT_KEY >=0) {
      cv::imshow("Image", image.get_CvMat_());
      cv::waitKey(WAIT_KEY);
    }
  }
}

// Head looking down is -0.40142554f
Anki::Cozmo::VisionPoseData PopulateVisionPoseData(float angle)
{

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
Anki::Cozmo::VisionPoseData PopulateVisionPoseData()
{

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
void checkClassifier(Anki::Cozmo::RawPixelsClassifier& clf,
                     const std::string& positivePath,
                     const std::string& negativePath,
                     float& resultErrorTrainingSet,
                     float maxTotalError = 0.12,
                     float maxPositivesError = 0.12)
{

  if ( (! positivePath.empty()) && (!negativePath.empty())) {
    std::cout << "Training the classifier..." << std::endl;
    bool result = clf.TrainFromFiles(positivePath.c_str(), negativePath.c_str());
    std::cout << "Training done!" << std::endl;
    ASSERT_TRUE(result);
  }

  cv::Mat trainingSamples, trainingLabels;
  clf.GetTrainingData(trainingSamples, trainingLabels);
  ASSERT_FALSE(trainingSamples.empty());
  ASSERT_FALSE(trainingLabels.empty());

  const float realNumberOfPositives = float(cv::sum(trainingLabels)[0]);

  // Error on whole training set
  if (maxTotalError > 0)
  {
    std::cout<<"Testing the whole training set"<<std::endl;
    std::vector<std::vector<Anki::Cozmo::RawPixelsClassifier::FeatureType>> pixels;
    Anki::Cozmo::CVMatToVector<float>(trainingSamples, pixels);

    // calculate error
    std::vector<uchar> responses = clf.PredictClass(pixels);
    cv::Mat responsesMat(responses);

    resultErrorTrainingSet = Anki::calculateError(responsesMat, trainingLabels);
    EXPECT_PRED_FORMAT2(::testing::FloatLE, resultErrorTrainingSet, maxTotalError);
    std::cout<<"Error on whole training set: "<<resultErrorTrainingSet<<std::endl;
  }

  // Error on positive elements
  if (maxPositivesError > 0)
  {
    std::cout<<"Testing the positive class in the training set"<<std::endl;
    cv::Mat positiveY;
    std::vector<std::vector<Anki::Cozmo::RawPixelsClassifier::FeatureType>> pixels;
    pixels.reserve(trainingSamples.rows); // probably won't be using that much

    // The local friendly lambda. There's a lot going on here just because trainingLabels is a cv::Mat_ with a template
    // type. Getting the type out of the Mat_ is not easy though, hence the dance with decltype
    auto fillPixels = [&trainingSamples = static_cast<const cv::Mat&>(trainingSamples)]
        (cv::Mat& positiveY,
         std::vector<std::vector<Anki::Cozmo::RawPixelsClassifier::FeatureType>>& pixels,
         const auto& trainingLabels) {

      using T = decltype(trainingLabels(0)); //either float or int
      auto elementY = trainingLabels.template begin(); // probably don't need the .template here, but it looks cool :)
      for (uint rowNumber = 0; elementY != trainingLabels.template end(); rowNumber++, elementY++) {
        const T value = *elementY;
        if ( value != 0) { // if it's positive class, add the sample to pixels
          std::vector<Anki::Cozmo::RawPixelsClassifier::FeatureType> rowPixels;
          Anki::Cozmo::CVMatToVector<float>(trainingSamples.row(rowNumber), rowPixels);
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
TEST(SurfaceClassifier, LRClassifier_TestWholeErrorNoWeighting)
{
  Json::Value config;
  // populate the Json file
  {
    config["NumClusters"] = 5;
    config["TrainingAlpha"] = 1.0;
    config["NumIterations"] = 10000;
    config["PositiveClassWeight"] = 1.0;
    config["RegularizationType"] = "Disable";
  }

  Anki::Cozmo::LRRawPixelsClassifier clf(config, cozmoContext);

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

  std::vector<std::vector<Anki::Cozmo::RawPixelsClassifier::FeatureType>> pixels;
  Anki::Cozmo::CVMatToVector<float>(trainingSamples, pixels);

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

  Anki::Cozmo::LRRawPixelsClassifier clf(config, cozmoContext);

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
TEST(SurfaceClassifier, LRClassifier_TestManualLabels)
{
  Json::Value config;
  // populate the Json file
  {
    config["NumClusters"] = 7;
    config["TrainingAlpha"] = 0.5;
    config["NumIterations"] = 10000;
    config["PositiveClassWeight"] = 1.5;
    config["RegularizationType"] = "Disable";
  }

  Anki::Cozmo::LRRawPixelsClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/manual_labels");

  PRINT_NAMED_INFO("TestSurfaceClassifier.TrainingFromFile.PrintPath",
                   "Path to training data is %s", path.c_str());

  const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
  const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

  float error = std::numeric_limits<float>::max();
  checkClassifier(clf, positivePath, negativePath, error);

  if (DISPLAY_IMAGES) {
    VisualizeClassifierOnImages("/Users/lorenzori/tmp/overhead_data/real/run8/images", clf);
  }

}

/*
 * Test the threshold classifier
 */
TEST(SurfaceClassifier, THClassifier_TestRealRun)
{
  Json::Value config;
  // populate the Json file
  {
    config["NumClusters"] = 7;
    config["MedianMultiplier"] = 2.0;
  }

  Anki::Cozmo::THRawPixelsClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/run7/positivePixels.txt");
  bool result = clf.TrainFromFile(path.c_str());
  ASSERT_TRUE(result);

  // Test that a fraction of all the pixels in the dataset are positive
  cv::Mat trainingSamples, trainingLabels;
  clf.GetTrainingData(trainingSamples, trainingLabels);

  std::vector<std::vector<Anki::Cozmo::RawPixelsClassifier::FeatureType>> pixels;
  Anki::Cozmo::CVMatToVector<float>(trainingSamples, pixels);

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
    VisualizeClassifierOnImages("/Users/lorenzori/tmp/overhead_data/real/run8/images", clf);
  }
}

/*
 * Test the decision trees classifier on manually labeled data
 */
TEST(SurfaceClassifier, DTClassifier_TestManualLabels)
{
  Json::Value config;
  // populate the Json file
  {
    config["MaxDepth"] = 10;
    config["MinSampleCount"] = 2;
    config["TruncatePrunedTree"] = false;
    config["Use1SERule"] = false;
  }

  Anki::Cozmo::DTRawPixelsClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/manual_labels");

  PRINT_NAMED_INFO("TestSurfaceClassifier.TrainingFromFile.PrintPath",
                   "Path to training data is %s", path.c_str());

  const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
  const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

  float error = std::numeric_limits<float>::max();
  checkClassifier(clf, positivePath, negativePath, error);

  if (DISPLAY_IMAGES) {
    VisualizeClassifierOnImages("/Users/lorenzori/tmp/overhead_data/real/run8/images", clf);
  }
}

/*
 * Test decision tree classifier serialization and deserialization
 */
TEST(SurfaceClassifier, DTClassifier_TestSerialization)
{

  Json::Value config;
  // populate the Json file
  {
    config["MaxDepth"] = 10;
    config["MinSampleCount"] = 10;
    config["TruncatePrunedTree"] = true;
    config["Use1SERule"] = true;
    config["PositiveWeight"] = 3.0f;
  }

  Anki::Cozmo::DTRawPixelsClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/realImagesDesk");

  PRINT_NAMED_INFO("TestSurfaceClassifier.TrainingFromFile.PrintPath",
                   "Path to training data is %s", path.c_str());

  const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
  const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

  float error = std::numeric_limits<float>::max();
  checkClassifier(clf, positivePath, negativePath, error, 0.14, 0.1);

  // serialize the classifier
  const std::string serializeFileName = Anki::Util::FileUtils::FullFilePath({path, "deskClassifier.yaml"});
  bool result = clf.Serialize(serializeFileName.c_str());
  ASSERT_TRUE(result);

  // deserialize the classifier
  Anki::Cozmo::DTRawPixelsClassifier clf2(cozmoContext);
  ASSERT_TRUE(clf2.DeSerialize(serializeFileName.c_str()));
  {
    cv::Mat trainingSamples, trainingLabels;
    clf.GetTrainingData(trainingSamples, trainingLabels);
    clf2.SetTrainingData(trainingSamples, trainingLabels);
  }
  float error2 = std::numeric_limits<float>::max();
  checkClassifier(clf2, "", "", error2, 0.14, 0.1);
  ASSERT_EQ(error, error2);

  if (DISPLAY_IMAGES) {
    VisualizeClassifierOnImages("/Users/lorenzori/tmp/images_training/real_images/single_shots_desk/desk", clf2);
  }
}

TEST(SurfaceClassifier, DTClassifier_TestMeanData)
{

  Json::Value root;
  Json::Value& config = root["GroundPlaneClassifier"];
  {
    config["MaxDepth"] = 7;
    config["MinSampleCount"] = 10;
    config["TruncatePrunedTree"] = true;
    config["Use1SERule"] = true;
    config["PositiveWeight"] = 1.0f;
    config["OnTheFlyTrain"] = true;
    config["FileOrDirName"] = "test/overheadMap/realImagesDesk";
  }

  Anki::Cozmo::DTRawPixelsClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/selectiveAnnotation");

  PRINT_NAMED_INFO("TestSurfaceClassifier.TrainingFromFile.PrintPath",
                   "Path to training data is %s", path.c_str());

  const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
  const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

  float error = std::numeric_limits<float>::max();
  checkClassifier(clf, positivePath, negativePath, error, 0.06, 0.03);

  // serialize the classifier
  const std::string serializeFileName = Anki::Util::FileUtils::FullFilePath({path, "deskClassifier.yaml"});
  bool result = clf.Serialize(serializeFileName.c_str());
  ASSERT_TRUE(result);

  if (DISPLAY_IMAGES) {
    const Anki::Cozmo::MeanFeaturesExtractor extractor = Anki::Cozmo::MeanFeaturesExtractor(1);
    VisualizeClassifierOnImages("/Users/lorenzori/tmp/images_training/real_images/single_shots_desk/desk",
                                clf, extractor);
  }
}

TEST(SurfaceClassifier, GroundClassifier_NoiseRemoval)
{

  Json::Value config;
  {
    config["MaxDepth"] = 7;
    config["MinSampleCount"] = 10;
    config["TruncatePrunedTree"] = true;
    config["Use1SERule"] = true;
    config["PositiveWeight"] = 1.0f;
    config["OnTheFlyTrain"] = false;
    config["FileOrDirName"] = "config/engine/vision/groundClassifier/deskClassifier.yaml";
  }

  Anki::Cozmo::GroundPlaneClassifier groundPlaneClassifier(config, cozmoContext);

  Anki::Cozmo::DebugImageList <Anki::Vision::ImageRGB> debugImageList;
  std::list<Anki::Cozmo::OverheadEdgeFrame> outEdges;
  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/realImagesDesk");

//  const std::string path = "/Users/lorenzori/tmp/images_training/real_images/selective_annotation";

  std::vector<std::string> imageFiles = Anki::Util::FileUtils::FilesInDirectory(path, true, "jpg");
  const Anki::Cozmo::VisionPoseData poseData = PopulateVisionPoseData(Anki::Util::DegToRad(-30));
  Anki::Cozmo::MeanFeaturesExtractor extractor(1);

  for (const auto& imagepath : imageFiles)
  {
    PRINT_CH_INFO("VisionSystem", "Test.GroundPlaneClassifier.LoadImage", "Loading image %s", imagepath.c_str());

    Anki::Vision::ImageRGB testImg;
    ASSERT_EQ(testImg.Load(imagepath), Anki::RESULT_OK);

    Anki::Result result = groundPlaneClassifier.Update(testImg, poseData, debugImageList, outEdges);
    ASSERT_EQ(result, Anki::RESULT_OK);

    if (DISPLAY_IMAGES) {
      for (auto& name_image : debugImageList) {
        std::string& name = name_image.first;
        Anki::Vision::ImageRGB& image = name_image.second;
        cv::cvtColor(image.get_CvMat_(), image.get_CvMat_(), cv::COLOR_RGB2BGR);
        cv::namedWindow(name, cv::WINDOW_AUTOSIZE);

        cv::imshow(name, image.get_CvMat_());
      }
      VisualizeClassifierOnImages(imagepath, groundPlaneClassifier.GetClassifier(), extractor);
    }
  }
}

/*
 * Test decision tree classifier on reflective maze
 */
TEST(SurfaceClassifier, DTClassifier_TestReflectiveDesk)
{

  Json::Value config;
  // populate the Json file
  {
    config["MaxDepth"] = 10;
    config["MinSampleCount"] = 10;
    config["TruncatePrunedTree"] = true;
    config["Use1SERule"] = true;
    config["PositiveWeight"] = 10.0f;
  }

  Anki::Cozmo::DTRawPixelsClassifier clf(config, cozmoContext);

  const std::string path = cozmoContext->GetDataPlatform()->pathToResource(Anki::Util::Data::Scope::Resources,
                                                                           "test/overheadMap/reflectiveDesk");

  PRINT_NAMED_INFO("TestSurfaceClassifier.TrainingFromFile.PrintPath",
                   "Path to training data is %s", path.c_str());

  const std::string positivePath = Anki::Util::FileUtils::FullFilePath({path, "positivePixels.txt"});
  const std::string negativePath = Anki::Util::FileUtils::FullFilePath({path, "negativePixels.txt"});

  float error = std::numeric_limits<float>::max();
  checkClassifier(clf, positivePath, negativePath, error, 0.25, 0.01);

  // serialize the classifier
  const std::string serializeFileName = Anki::Util::FileUtils::FullFilePath({path, "reflectiveMazeClassifier.yaml"});
  bool result = clf.Serialize(serializeFileName.c_str());
  ASSERT_TRUE(result);

  // deserialize the classifier
  Anki::Cozmo::DTRawPixelsClassifier clf2(cozmoContext);
  ASSERT_TRUE(clf2.DeSerialize(serializeFileName.c_str()));
  {
    cv::Mat trainingSamples, trainingLabels;
    clf.GetTrainingData(trainingSamples, trainingLabels);
    clf2.SetTrainingData(trainingSamples, trainingLabels);
  }
  float error2 = std::numeric_limits<float>::max();
  checkClassifier(clf2, "", "", error2, 0.25, 0.01);
  ASSERT_EQ(error, error2);

  if (DISPLAY_IMAGES) {
    auto extractor = Anki::Cozmo::MeanFeaturesExtractor(3);
    VisualizeClassifierOnImages("/Users/lorenzori/tmp/images_training/real_images/reflectiveMaze", clf2, extractor);
  }
}
