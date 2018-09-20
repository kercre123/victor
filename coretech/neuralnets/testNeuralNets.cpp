#if defined(ANKI_NEURALNETS_USE_TENSORFLOW)
#  include "neuralNetModel_tensorflow.h"
#elif defined(ANKI_NEURALNETS_USE_CAFFE2)
#  include "objectDetector_caffe2.h"
#elif defined(ANKI_NEURALNETS_USE_OPENCV_DNN)
#  include "objectDetector_opencvdnn.h"
#elif defined(ANKI_NEURALNETS_USE_TFLITE)
#  include "neuralNetModel_tflite.h"
#else
#  error One of ANKI_NEURALNETS_USE_{TENSORFLOW | CAFFE2 | OPENCVDNN | TFLITE} must be defined
#endif

#include "coretech/vision/engine/image.h"

#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"

#include "gtest/gtest.h"
#include "json/json.h"

#include <fstream>

namespace JsonKeys
{
  const char * const NeuralNets = "NeuralNets";
  const char * const GraphFile  = "graphFile";
}
namespace TestPaths
{
#ifndef TEST_DATA_PATH
# error TEST_DATA_PATH should be defined for this target!
#endif
  const std::string TestDataPathStr(QUOTE(TEST_DATA_PATH));
  const std::string TestRoot  = TestDataPathStr + "/test";
  const std::string ModelPath = TestDataPathStr + "/test/models";
  const std::string CachePath = TestDataPathStr + "/test/models/cache";
  const std::string ImagePath = TestDataPathStr + "/test/images";
}

// This test is meant to avoid checking in a vision_config.json or code changes that break the ability to load
// the neural net model(s) successfully.
GTEST_TEST(NeuralNets, InitFromConfigAndLoadModel)
{
  using namespace Anki;
  
  const std::string configFilename = Util::FileUtils::FullFilePath({TestPaths::TestRoot, "test_config.json"});
  ASSERT_TRUE(Util::FileUtils::FileExists(configFilename));
  
  // Read config file
  Json::Value config;
  {
    Json::Reader reader;
    std::ifstream file(configFilename);
    const bool success = reader.parse(file, config);
    ASSERT_TRUE(success);
  }
  
  ASSERT_TRUE(config.isMember(JsonKeys::NeuralNets));
  const Json::Value& neuralNetConfig = config[JsonKeys::NeuralNets];
  
  ASSERT_TRUE(neuralNetConfig.isMember(JsonKeys::GraphFile));
  const std::string modelFileName = neuralNetConfig[JsonKeys::GraphFile].asString();
  
  NeuralNets::NeuralNetModel neuralNet(TestPaths::CachePath);
  
  const Result loadResult = neuralNet.LoadModel(TestPaths::ModelPath, neuralNetConfig);
  ASSERT_EQ(RESULT_OK, loadResult);
}

// Make sure we can run stock MobileNet on the standard Grace Hopper image
GTEST_TEST(NeuralNets, MobileNet)
{
  using namespace Anki;
  
  // Read config file
  Json::Value config;

# if defined(ANKI_NEURALNETS_USE_TENSORFLOW)
  config[JsonKeys::GraphFile] = "mobilenet_v1_1.0_224_frozen.pb";
  config["useFloatInput"] = true;
# elif defined(ANKI_NEURALNETS_USE_TFLITE)
  config[JsonKeys::GraphFile] = "mobilenet_v1_1.0_224_quant.tflite";
  config["useFloatInput"] = false;
# endif
  
  config["labelsFile"] = "mobilenet_labels.txt";
  config["architecture"] = "custom";
  config["inputWidth"] = 224;
  config["inputHeight"] = 224;
  config["inputShift"] = -1;
  config["inputScale"] = 127.5;
  config["memoryMapGraph"] = false;
  config["minScore"] = 0.1f;
  config["inputLayerName"] = "input";
  config["outputLayerNames"] = "MobilenetV1/Predictions/Reshape_1";
  config["outputType"] = "classification";
  config["verbose"] = true;
  config["benchmarkRuns"] = 0;
  
  NeuralNets::NeuralNetModel neuralNet(TestPaths::CachePath);
  const Result loadResult = neuralNet.LoadModel(TestPaths::ModelPath, config);
  ASSERT_EQ(RESULT_OK, loadResult);
  
  struct Test {
    std::string imageFile;
    std::string expectedLabel;
  };
  
  const std::vector<Test> tests{
    Test{.imageFile = "grace_hopper.jpg", .expectedLabel = "653:military uniform"},
    Test{.imageFile = "cat.jpg",          .expectedLabel = "283:tiger cat"},
  };
  
  TimeStamp_t timestamp = 0;
  for(auto const& test : tests)
  {
    const std::string testImageFile = Util::FileUtils::FullFilePath({TestPaths::ImagePath, test.imageFile});
    
    Vision::ImageRGB img;
    const Result imgLoadResult = img.Load(testImageFile);
    ASSERT_EQ(RESULT_OK, imgLoadResult);
    img.SetTimestamp(timestamp);
    
    std::list<Vision::SalientPoint> salientPoints;
    const Result detectResult = neuralNet.Detect(img, salientPoints);
    ASSERT_EQ(RESULT_OK, detectResult);
    
    for(auto const& salientPoint : salientPoints)
    {
      printf("Found %s in image %s\n", salientPoint.description.c_str(), testImageFile.c_str());
    }
    EXPECT_EQ(1, salientPoints.size());
    if(!salientPoints.empty())
    {
      EXPECT_EQ(test.expectedLabel, salientPoints.front().description);
      EXPECT_EQ(timestamp, salientPoints.front().timestamp);
    }
    
    timestamp += 10;
  }
  
}

// Make sure we can detect a person in a few test images
GTEST_TEST(NeuralNets, PersonDetection)
{
  enum class DebugViz {
    Disable,
    ShowAll,
    ShowFailures,
  };
  
  const DebugViz kDebugViz = DebugViz::Disable;
  
  using namespace Anki;
  
  // This is a little gross because it reaches out to engine's resources...
  // The alternative is to duplicate the config and model within coretech/neuralnets.
  const std::string vectorResourcePath = Util::FileUtils::FullFilePath({
    TestPaths::ModelPath, "..", "..", "..", "..", "resources", "config", "engine"
  });
  ASSERT_TRUE(Util::FileUtils::DirectoryExists(vectorResourcePath));
  
  const std::string configFilename = Util::FileUtils::FullFilePath({vectorResourcePath, "vision_config.json"});
  ASSERT_TRUE(Util::FileUtils::FileExists(configFilename));
  
  const std::string vectorModelPath = Util::FileUtils::FullFilePath({vectorResourcePath, "vision", "dnn_models"});
  ASSERT_TRUE(Util::FileUtils::DirectoryExists(vectorModelPath));
  
  // Read config file
  Json::Value config;
  {
    Json::Reader reader;
    std::ifstream file(configFilename);
    const bool success = reader.parse(file, config);
    ASSERT_TRUE(success);
  }
  
  ASSERT_TRUE(config.isMember(JsonKeys::NeuralNets));
  const Json::Value& neuralNetConfig = config[JsonKeys::NeuralNets];
  
  NeuralNets::NeuralNetModel neuralNet(TestPaths::CachePath);
  const Result loadResult = neuralNet.LoadModel(vectorModelPath, neuralNetConfig);
  ASSERT_EQ(RESULT_OK, loadResult);
  
  const std::string peopleFilePath = Util::FileUtils::FullFilePath({TestPaths::ImagePath, "people"});
  const auto files = Util::FileUtils::FilesInDirectory(peopleFilePath, true, {".jpg", ".png"});
  
  ASSERT_FALSE(files.empty());
  
  Vision::ImageRGB img;
  TimeStamp_t timestamp = 0;
  for(auto const& file : files)
  {
    const Result imgLoadResult = img.Load(file);
    ASSERT_EQ(RESULT_OK, imgLoadResult);
    img.SetTimestamp(timestamp);
    
    std::list<Vision::SalientPoint> salientPoints;
    const Result detectResult = neuralNet.Detect(img, salientPoints);
    ASSERT_EQ(RESULT_OK, detectResult);
    
    EXPECT_FALSE(salientPoints.empty());
    for(auto const& salientPoint : salientPoints)
    {
      ASSERT_EQ(timestamp, salientPoint.timestamp);
      ASSERT_EQ(Vision::SalientPointType::Person, salientPoint.salientType);
      // TODO: Compare actual centroid/shape to ground truth (may be annoying to keep up to date as network changes)
    }
    
    if(DebugViz::Disable != kDebugViz)
    {
      Vision::ImageRGB dispImg;
      img.CopyTo(dispImg);
      
      const bool isFailure = salientPoints.empty();
      
      if(isFailure || (DebugViz::ShowAll == kDebugViz))
      {
        for(auto const& salientPoint : salientPoints)
        {
          for(int i=0; i<salientPoint.shape.size(); ++i)
          {
            const int iNext = (i == (salientPoint.shape.size()-1) ? 0 : i+1);
            const Point2f pt1(salientPoint.shape[i].x * (f32)img.GetNumCols(),
                              salientPoint.shape[i].y * (f32)img.GetNumRows());
            const Point2f pt2(salientPoint.shape[iNext].x * (f32)img.GetNumCols(),
                              salientPoint.shape[iNext].y * (f32)img.GetNumRows());
            
            img.DrawLine(pt1, pt2, NamedColors::GREEN);
          }
        }
        dispImg.Display("PersonDetection", 0);
      }
    }
    
    timestamp += 10;
  }
}


int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  
  int rc = 0;
  {
    Anki::Util::PrintfLoggerProvider printfLoggerProvider;
    printfLoggerProvider.SetMinLogLevel(Anki::Util::LOG_LEVEL_DEBUG);
    Anki::Util::gLoggerProvider = &printfLoggerProvider;
    rc = RUN_ALL_TESTS();
    Anki::Util::gLoggerProvider = nullptr;
  }
  return rc;
}

