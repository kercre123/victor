#if defined(ANKI_NEURALNETS_USE_TENSORFLOW)
#  include "neuralNetModel_tensorflow.h"
#elif defined(ANKI_NEURALNETS_USE_CAFFE2)
#  include "objectDetector_caffe2.h"
#elif defined(ANKI_NEURALNETS_USE_OPENCV_DNN)
#  include "objectDetector_opencvdnn.h"
#elif defined(ANKI_NEURALNETS_USE_TFLITE)
#  include "coretech/neuralnets/neuralNetModel_tflite.h"
#else
#  error One of ANKI_NEURALNETS_USE_{TENSORFLOW | CAFFE2 | OPENCVDNN | TFLITE} must be defined
#endif

#include "coretech/common/engine/jsonTools.h"
#include "coretech/common/shared/math/point.h"
#include "coretech/neuralnets/iNeuralNetMain.h"
#include "coretech/neuralnets/neuralNetFilenames.h"
#include "coretech/neuralnets/neuralNetJsonKeys.h"
#include "coretech/vision/engine/image_impl.h"

#include "util/fileUtils/fileUtils.h"
#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header
#include "util/helpers/quoteMacro.h"
#include "util/logging/logging.h"
#include "util/logging/printfLoggerProvider.h"

#include "json/json.h"

#include <fstream>

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
  
  ASSERT_TRUE(config.isMember(NeuralNets::JsonKeys::NeuralNets));
  const Json::Value& neuralNetConfig = config[NeuralNets::JsonKeys::NeuralNets];
  
  ASSERT_TRUE(neuralNetConfig.isMember(NeuralNets::JsonKeys::Models));
  const Json::Value& modelsConfig = neuralNetConfig[NeuralNets::JsonKeys::Models];

  NeuralNets::TFLiteModel neuralNet;

  ASSERT_TRUE(modelsConfig.isArray());
  for(const auto& modelConfig : modelsConfig)
  {
    const Result loadResult = neuralNet.LoadModel(TestPaths::ModelPath, modelConfig);
    ASSERT_EQ(RESULT_OK, loadResult);
  }
}

// Make sure we can run stock MobileNet on the standard Grace Hopper image
GTEST_TEST(NeuralNets, MobileNet)
{
  using namespace Anki;
  
  // Read config file
  Json::Value config;

# if defined(ANKI_NEURALNETS_USE_TENSORFLOW)
  config[NeuralNets::JsonKeys::GraphFile] = "mobilenet_v1_1.0_224_frozen.pb";
  config["useFloatInput"] = true;
# elif defined(ANKI_NEURALNETS_USE_TFLITE)
  config[NeuralNets::JsonKeys::GraphFile] = "mobilenet_v1_1.0_224_quant.tflite";
  config["useFloatInput"] = false;
# endif
  
  config[NeuralNets::JsonKeys::ModelType] = "TFLite";
  config[NeuralNets::JsonKeys::NetworkName] = "mobilenet";
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
  
  NeuralNets::TFLiteModel neuralNet;
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
  
  ASSERT_TRUE(config.isMember(NeuralNets::JsonKeys::NeuralNets));
  const Json::Value& neuralNetConfig = config[NeuralNets::JsonKeys::NeuralNets];
  
  NeuralNets::TFLiteModel neuralNet;
  
  ASSERT_TRUE(neuralNetConfig.isMember(NeuralNets::JsonKeys::Models));
  const Json::Value& modelsConfig = neuralNetConfig[NeuralNets::JsonKeys::Models];
  ASSERT_TRUE(modelsConfig.isArray());
  
  Result loadResult = RESULT_FAIL;
  for(const auto& modelConfig : modelsConfig)
  {
    std::string networkName;
    if(JsonTools::GetValueOptional(modelConfig, NeuralNets::JsonKeys::NetworkName, networkName))
    {
      if(networkName == "person_detector")
      {
        loadResult = neuralNet.LoadModel(vectorModelPath, modelConfig);
        break;
      }
    }
  }
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


GTEST_TEST(NeuralNets, MultipleModels)
{
  using namespace Anki;
  
  class TestMain : public NeuralNets::INeuralNetMain
  {
  public:
    TestMain() { }
    
    Result ReadResult(const std::string& resultFilename, std::vector<Vision::SalientPoint>& salientPoints)
    {
      Json::Reader reader;
      Json::Value detectionResult;
      std::ifstream file(resultFilename);
      const bool success = reader.parse(file, detectionResult);
      file.close();
      if(!success)
      {
        PRINT_NAMED_ERROR("NeuralNetRunner.Model.FailedToReadJSON", "%s", resultFilename.c_str());
        return RESULT_FAIL;
      }
      else
      {
        // Translate JSON into a SalientPoint and put it in the output
        const Json::Value& salientPointsJson = detectionResult["salientPoints"];
        if(salientPointsJson.isArray())
        {
          for(auto const& salientPointJson : salientPointsJson)
          {
            Vision::SalientPoint salientPoint;
            const bool success = salientPoint.SetFromJSON(salientPointJson);
            if(!success)
            {
              PRINT_NAMED_ERROR("NeuralNetRunner.Model.FailedToSetFromJSON", "");
              return RESULT_FAIL;
            }
            
            salientPoints.emplace_back(std::move(salientPoint));
          }
        }
      }
      
      Util::FileUtils::DeleteFile(resultFilename);
      return RESULT_OK;
    }
    
  protected:
    
    virtual bool ShouldShutdown() override
    {
      // NOTE: ShouldShutdown is superfluous because this test provides an input file, so
      //       Run() will always stop immediately after processing that one file.
      return false;
    }
    
    virtual Util::ILoggerProvider* GetLoggerProvider() override
    {
      return Anki::Util::gLoggerProvider;
    }
    
    virtual int GetPollPeriod_ms(const Json::Value& config) const override
    {
      assert(config.isMember(NeuralNets::JsonKeys::PollingPeriod));
      return config[NeuralNets::JsonKeys::PollingPeriod].asInt();
    }
    
    virtual void Step(int pollPeriod_ms) override
    {
      
    }
    
  private:
    std::unique_ptr<Util::PrintfLoggerProvider> _logger;
  };
  
  
  const std::string configFilename = Util::FileUtils::FullFilePath({TestPaths::TestRoot, "test_config.json"});
  ASSERT_TRUE(Util::FileUtils::FileExists(configFilename));
  
  TestMain testMain;
  
  const std::string testImageFile = Util::FileUtils::FullFilePath({TestPaths::ImagePath, "cat.jpg"});
  const Result initResult = testMain.Init(configFilename, TestPaths::ModelPath, TestPaths::CachePath, testImageFile);
  ASSERT_EQ(RESULT_OK, initResult);
  
  // Running on a test file like this should not delete it
  ASSERT_TRUE(Util::FileUtils::FileExists(testImageFile));
  const Result runResult = testMain.Run();
  ASSERT_EQ(RESULT_OK, runResult);
  
  std::vector<Vision::SalientPoint> salientPoints;
  
  const std::vector<std::string> resultFilenames =  {
    Util::FileUtils::FullFilePath({TestPaths::CachePath, "quantized", NeuralNets::Filenames::Result}),
    Util::FileUtils::FullFilePath({TestPaths::CachePath, "non-quantized", NeuralNets::Filenames::Result}),
  };
  
  for(const auto& resultFilename : resultFilenames)
  {
    ASSERT_TRUE(Util::FileUtils::FileExists(resultFilename));
    const Result readResult = testMain.ReadResult(resultFilename, salientPoints);
    ASSERT_EQ(RESULT_OK, readResult);
    ASSERT_FALSE(Util::FileUtils::FileExists(resultFilename));
  }
  
  ASSERT_EQ(2, salientPoints.size());
}

GTEST_TEST(NeuralNets, ClassificationConsensus)
{
  using namespace Anki;
 
  // Read config file
  Json::Value config;
  
# if defined(ANKI_NEURALNETS_USE_TENSORFLOW)
  config[NeuralNets::JsonKeys::GraphFile] = "mobilenet_v1_1.0_224_frozen.pb";
  config["useFloatInput"] = true;
# elif defined(ANKI_NEURALNETS_USE_TFLITE)
  config[NeuralNets::JsonKeys::GraphFile] = "mobilenet_v1_1.0_224_quant.tflite";
  config["useFloatInput"] = false;
# endif
  
  config[NeuralNets::JsonKeys::ModelType] = "TFLite";
  config[NeuralNets::JsonKeys::NetworkName] = "mobilenet";
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
  
  // Have to see the class in 3 of 5 frames to report it
  const int kNumFrames = 5;
  const int kMajority = 3;
  config["numFrames"] = kNumFrames;
  config["majority"] = kMajority;
  
  NeuralNets::TFLiteModel neuralNet;
  const Result loadResult = neuralNet.LoadModel(TestPaths::ModelPath, config);
  ASSERT_EQ(RESULT_OK, loadResult);
  
  struct Test {
    std::string imageFile;
    std::string expectedLabel;
    std::vector<bool> schedule;
    std::vector<bool> expectedDetections;
  };
  
  // "schedule" is true when the image with the expected label present is shown
  // "expectedDetections" is true when the classifier is expected to return
  //   a detection, given the NumFrames/Majority settings above
  const std::vector<Test> tests{
    Test{
      .imageFile = "grace_hopper.jpg",
      .expectedLabel = "653:military uniform",
      .schedule           = {true,  false, true,  false, true, false, false},
      .expectedDetections = {false, false, false, false, true, false, false},
    },
    Test{
      .imageFile = "cat.jpg",
      .expectedLabel = "283:tiger cat",
      .schedule           = {false, false, true,  true,  true, false, false},
      .expectedDetections = {false, false, false, false, true, true,  true},
    },
    Test{
      .imageFile = "cat.jpg",
      .expectedLabel = "283:tiger cat",
      .schedule           = {true,  true,  true, true, true, false, false, false},
      .expectedDetections = {false, false, true, true, true, true,  true,  false},
    },
  };
  
  TimeStamp_t timestamp = 0;
  for(auto const& test : tests)
  {
    const std::string testImageFile = Util::FileUtils::FullFilePath({TestPaths::ImagePath, test.imageFile});
    
    Vision::ImageRGB img;
    const Result imgLoadResult = img.Load(testImageFile);
    ASSERT_EQ(RESULT_OK, imgLoadResult);
    img.SetTimestamp(timestamp);
    
    Vision::ImageRGB nothing(img.GetNumRows(), img.GetNumCols());
    nothing.FillWith(0);
    
    int numTimesShown = 0;
    ASSERT_GE(test.schedule.size(), kNumFrames);
    ASSERT_EQ(test.schedule.size(), test.expectedDetections.size());
    for(int iCount=0; iCount<test.schedule.size(); ++iCount)
    {
      Vision::ImageRGB* imgToShow = (test.schedule[iCount] ? &img : &nothing);
      
      imgToShow->SetTimestamp(timestamp);
      
      if(test.schedule[iCount])
      {
        ++numTimesShown;
      }
      
      std::list<Vision::SalientPoint> salientPoints;
      const Result detectResult = neuralNet.Detect(*imgToShow, salientPoints);
      ASSERT_EQ(RESULT_OK, detectResult);
      
      if(test.expectedDetections[iCount])
      {
        EXPECT_EQ(1, salientPoints.size());
        if(!salientPoints.empty())
        {
          EXPECT_EQ(test.expectedLabel, salientPoints.front().description);
          EXPECT_EQ(timestamp, salientPoints.front().timestamp);
        }
      }
      else
      {
        EXPECT_TRUE(salientPoints.empty());
      }

      timestamp += 10;
    }
    
    // "Clear" the results by showing a bunch of "nothing" frames
    for(int i=0; i<kNumFrames; ++i)
    {
      std::list<Vision::SalientPoint> salientPoints;
      const Result detectResult = neuralNet.Detect(nothing, salientPoints);
      ASSERT_EQ(RESULT_OK, detectResult);
    }
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

