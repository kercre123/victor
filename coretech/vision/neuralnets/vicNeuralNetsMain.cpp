/**
 * File: standaloneForwardInference.cpp
 *
 * Author: Andrew Stein
 * Date:   1/29/2018
 *
 * Description: Standalone process to run forward inference using a variety of neural network platforms,
 *              specified via compile-time defines (e.g. TensorFlow, OpenCV DNN, or TF Lite).
 *
 *              Currently uses the file system as a poor man's IPC to communicate with the "messenger"
 *              NeuralNetRunner::Model implementation in vic-engine's Vision System. See also VIC-2686.
 *
 *              Can be used as a Webots controller when compiled with -DSIMULATOR
 *
 * Copyright: Anki, Inc. 2018
 **/

#if defined(VIC_NEURALNETS_USE_TENSORFLOW)
#  include "neuralNetModel_tensorflow.h"
#elif defined(VIC_NEURALNETS_USE_CAFFE2)
#  include "objectDetector_caffe2.h"
#elif defined(VIC_NEURALNETS_USE_OPENCV_DNN)
#  include "objectDetector_opencvdnn.h"
#elif defined(VIC_NEURALNETS_USE_TFLITE)
#  include "objectDetector_tflite.h"
#else 
#  error TENSORFLOW or CAFFE2 or OPENCVDNN or TFLITE must be defined
#endif

#include "coretech/common/shared/types.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#ifdef VICOS
#  include "util/logging/victorLogger.h"
#else
#  include "util/logging/printfLoggerProvider.h"
#  include "util/logging/multiFormattedLoggerProvider.h"
#endif

#ifdef SIMULATOR
#  include <webots/Supervisor.hpp>
#endif

#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <thread>
#include <unistd.h>

#define LOG_PROCNAME "vic-neuralnets"
#define LOG_CHANNEL "NeuralNets"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {
  bool gShutdown = false;
}

static void Shutdown(int signum)
{
  LOG_INFO("VicNeuralNets.Shutdown", "Shutdown on signal %d", signum);
  gShutdown = true;
}

static void CleanupAndExit(Anki::Result result)
{
  LOG_INFO("VicNeuralNets.CleanupAndExit", "result:%d", result);
  Anki::Util::gLoggerProvider = nullptr;
  // TODO: Add GoogleBreakpad
  // GoogleBreakpad::UnInstallGoogleBreakpad();
  sync();
  exit(result);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Lightweight scoped timing mechanism
// TODO: Use coretech/util profiling mechanism
class TicToc
{
  using ClockType = std::chrono::high_resolution_clock;
  using TimePoint = std::chrono::time_point<ClockType>;

  std::string _name;
  TimePoint   _startTime;
  static bool _enabled;
  
public:

  static void Enable(const bool enable) { _enabled = enable; }
  
  TicToc(const std::string& name)
  : _name(name) 
  , _startTime(ClockType::now())
  { 

  }

  ~TicToc()
  {
    if(_enabled)
    {
      const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(ClockType::now() - _startTime);
      const std::string eventName("VicNeuralNets.Toc." + _name);
      LOG_INFO(eventName.c_str(), "%dms", (int)duration.count());
    }
  }

};

bool TicToc::_enabled = false;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Define helpers for timing and reading / writing images and results (implemented below, after main())

static void GetImage(const std::string& imageFilename, const std::string timestampFilename,
                     cv::Mat& img, Anki::TimeStamp_t& timestamp);

static void ConvertSalientPointsToJson(const std::list<Anki::Vision::SalientPoint>& salientPoints,
                                       const bool isVerbose, Json::Value& detectionResults);

static bool WriteResults(const std::string jsonFilename, const Json::Value& detectionResults);

static cv::Mat ReadBMP(const std::string& input_bmp_name); 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char **argv)
{
  signal(SIGTERM, Shutdown);

  // TODO: Add GoogleBreakpad
  //static char const* filenamePrefix = "anim";
  //GoogleBreakpad::InstallGoogleBreakpad(filenamePrefix);

  using namespace Anki;

  // Create and set logger, depending on platform
# ifdef VICOS
  auto logger = std::make_unique<Util::VictorLogger>(LOG_PROCNAME);
# else
  const bool colorizeStderrOutput = false; // TODO: Get from Webots proto in simulation?
  auto logger = std::make_unique<Util::PrintfLoggerProvider>(Anki::Util::LOG_LEVEL_DEBUG,
                                                             colorizeStderrOutput);  
# endif

  Util::gLoggerProvider = logger.get();

  if(nullptr == Util::gLoggerProvider)
  {
    // Having no logger shouldn't kill us but probably isn't what we intended, so issue a warning
    PRINT_NAMED_WARNING("VicNeuralNets.Main.NullLogger", "");
  }
  
  Result result = RESULT_OK;

  if(argc < 4)
  {
    PRINT_NAMED_ERROR("VicNeuralNets.Main.UnexpectedArguments", "");
    std::cout << std::endl << "Usage: " << argv[0] << " <configFile>.json modelPath cachePath <imageFile>" << std::endl;
    std::cout << std::endl << " If no imageFile is provided, polls cachePath for neuralNetImage.png" << std::endl;
    CleanupAndExit(result);
  }
  
  const std::string configFilename(argv[1]);
  const std::string modelPath(argv[2]);
  const std::string cachePath(argv[3]);

  // Make sure the config file and model path are valid
  // NOTE: cachePath need not exist yet as it will be created by the NeuralNetRunner
  if(!Util::FileUtils::FileExists(configFilename))
  {
    PRINT_NAMED_ERROR("VicNeuralNets.Main.BadConfigFile", "ConfigFile:%s", configFilename.c_str());
    result = RESULT_FAIL;
  }
  if(!Util::FileUtils::DirectoryExists(modelPath))
  {
    PRINT_NAMED_ERROR("VicNeuralNets.Main.BadModelPath", "ModelPath:%s", modelPath.c_str());
    result = RESULT_FAIL;
  }
  if(RESULT_OK != result)
  {
    CleanupAndExit(result);
  }
  LOG_INFO("VicNeuralNets.Main.Starting", "Config:%s ModelPath:%s CachePath:%s",
           configFilename.c_str(), modelPath.c_str(), cachePath.c_str());
  
  // Read config file
  Json::Value config;
  {
    Json::Reader reader;
    std::ifstream file(configFilename);
    const bool success = reader.parse(file, config);
    if(!success)
    {
      PRINT_NAMED_ERROR("VicNeuralNets.Main.ReadConfigFailed", 
                        "Could not read config file: %s", configFilename.c_str());
      CleanupAndExit(RESULT_FAIL);
    }

    const char *  const kNeuralNetsKey = "NeuralNets";
    if(!config.isMember(kNeuralNetsKey))
    {
      PRINT_NAMED_ERROR("VicNeuralNets.Main.MissingConfigKey", "Config file missing '%s' field", kNeuralNetsKey);
      CleanupAndExit(RESULT_FAIL);
    }

    config = config[kNeuralNetsKey];

    if(!config.isMember("pollPeriod_ms"))
    {
      PRINT_NAMED_ERROR("VicNeuralNets.Main.MissingPollPeriodField", "No 'pollPeriod_ms' specified in config file");
      CleanupAndExit(RESULT_FAIL);
    }
  }

  const bool imageFileProvided = (argc > 4);

  const std::string imageFilename = (imageFileProvided ? 
                                     argv[4] :
                                     Util::FileUtils::FullFilePath({cachePath, "neuralNetImage.png"}));

  const std::string timestampFilename = Util::FileUtils::FullFilePath({cachePath, "timestamp.txt"});

  const std::string jsonFilename = Util::FileUtils::FullFilePath({cachePath, "neuralNetResults.json"});

  LOG_INFO("VicNeuralNets.Main.ImageLoadMode", "%s: %s",
           (imageFileProvided ? "Loading given image" : "Polling for images at"),
           imageFilename.c_str());

  
# ifdef SIMULATOR
  webots::Supervisor webotsSupervisor;
  const int kPollPeriod_ms  = webotsSupervisor.getSelf()->getField("pollingPeriod_ms")->getSFInt32();
# else 
  const int kPollPeriod_ms = config["pollPeriod_ms"].asInt();
# endif

  // Initialize the detector
  NeuralNetModel neuralNet;
  {
    auto ticToc = TicToc("LoadModel");
    result = neuralNet.LoadModel(modelPath, config);
  
    if(RESULT_OK != result)
    {
      PRINT_NAMED_ERROR(argv[0], "Failed to load model from path: %s", modelPath.c_str());
      CleanupAndExit(result);
    }
    
    TicToc::Enable(neuralNet.IsVerbose());
  }
  
  LOG_INFO("VicNeuralNets.Main.DetectorInitialized", "Waiting for images");

  // Loop until shutdown or error
  while (!gShutdown) 
  {
    // Is there an image file available in the cache?
    const bool isImageAvailable = Util::FileUtils::FileExists(imageFilename);

    if(isImageAvailable)
    {
      if(neuralNet.IsVerbose())
      {
        LOG_INFO("VicNeuralNets.Main.FoundImage", "%s", imageFilename.c_str());
      }

      // Get the image
      cv::Mat img;
      TimeStamp_t timestamp=0;
      {
        auto ticToc = TicToc("GetImage");
        GetImage(imageFilename, timestampFilename, img, timestamp);
        
        if(img.empty())
        {
          PRINT_NAMED_ERROR("VicNeuralNets.Main.ImageReadFailed", "Empty image from %s", imageFilename.c_str());
          result = RESULT_FAIL;
          break;
        }
      }

      // Detect what's in it
      std::list<Vision::SalientPoint> salientPoints;
      {
        auto ticToc = TicToc("Detect");
        result = neuralNet.Detect(img, timestamp, salientPoints);
      
        if(RESULT_OK != result)
        {
          PRINT_NAMED_ERROR("VicNeuralNets.Main.DetectFailed", "");
        }
      }

      // Convert the results to JSON
      Json::Value detectionResults;
      ConvertSalientPointsToJson(salientPoints, neuralNet.IsVerbose(), detectionResults);

      // Write out the Json
      {
        auto ticToc = TicToc("WriteJSON");
        if(neuralNet.IsVerbose())
        {
          LOG_INFO("VicNeuralNets.Main.WritingResults", "%s", jsonFilename.c_str());
        }

        const bool success = WriteResults(jsonFilename, detectionResults);
        if(!success)
        {
          result = RESULT_FAIL;
          break;
        }
      }

      if(imageFileProvided)
      {
        // If we loaded in image file specified on the command line, we are done 
        result = RESULT_OK;
        break;
      }
      else
      {
        // Remove the image file we were working with to signal that we're done with it 
        // and ready for a new image
        if(neuralNet.IsVerbose())
        {
          LOG_INFO("VicNeuralNets.Main.DeletingImageFile", "%s", imageFilename.c_str());
        }
        remove(imageFilename.c_str());
      }
    }
    else if(imageFileProvided)
    {
      PRINT_NAMED_ERROR("VicNeuralNets.Main.ImageFileDoesNotExist", "%s", imageFilename.c_str());
      break;
    }
    else 
    {
      if(neuralNet.IsVerbose())
      {
        const int kVerbosePrintFreq_ms = 1000;
        static int count = 0;
        if(count++ * kPollPeriod_ms >= kVerbosePrintFreq_ms)
        {
          LOG_INFO("VicNeuralNets.Main.WaitingForImage", "%s", imageFilename.c_str());
          count = 0;
        }
      }

#     ifdef SIMULATOR
      const int rc = webotsSupervisor.step(kPollPeriod_ms);
      if(rc == -1)
      {
        LOG_INFO("VicNeuralNets.Main.WebotsTerminating", "");
        break;
      }
#     else
      std::this_thread::sleep_for(std::chrono::milliseconds(kPollPeriod_ms));  
#     endif
    }
  }

  CleanupAndExit(result);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void GetImage(const std::string& imageFilename, const std::string timestampFilename,
              cv::Mat& img, Anki::TimeStamp_t& timestamp)
{
  const std::string ext = imageFilename.substr(imageFilename.size()-3,3);
  if(ext == "bmp")
  {
    img = ReadBMP(imageFilename); // Converts to RGB internally
  } 
  else {
    img = cv::imread(imageFilename);
    cv::cvtColor(img, img, CV_BGR2RGB); // OpenCV loads BGR, TF expects RGB
  }

  if(img.empty())
  {
    // Don't bother reading the timestamp file
    return;
  }

  if(Anki::Util::FileUtils::FileExists(timestampFilename))
  {
    std::ifstream file(timestampFilename);
    std::string line;
    std::getline(file, line);
    timestamp = uint(std::stol(line));
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ConvertSalientPointsToJson(const std::list<Anki::Vision::SalientPoint>& salientPoints,
                                const bool isVerbose,
                                Json::Value& detectionResults)
{
  std::string salientPointsStr;
  
  Json::Value& salientPointsJson = detectionResults["salientPoints"];
  for(auto const& salientPoint : salientPoints)
  {
    salientPointsStr += salientPoint.description + "[" + std::to_string((int)round(100.f*salientPoint.score)) + "] ";
    const Json::Value json = salientPoint.GetJSON();
    salientPointsJson.append(json);
  }  

  if(isVerbose && !salientPoints.empty())
  {
    LOG_INFO("VicNeuralNets.Main.ConvertSalientPointsToJson", 
             "Detected %zu objects: %s", salientPoints.size(), salientPointsStr.c_str());
  }    
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool WriteResults(const std::string jsonFilename, const Json::Value& detectionResults)
{
  // Write to a temporary file, then move into place once the write is complete (poor man's "lock")
  const std::string tempFilename = jsonFilename + ".lock";
  Json::StyledStreamWriter writer;
  std::fstream fs;
  fs.open(tempFilename, std::ios::out);
  if (!fs.is_open()) {
    PRINT_NAMED_ERROR("VicNeuralNets.Main.OutputFileOpenFailed", "%s", jsonFilename.c_str());
    return false;
  }
  writer.write(fs, detectionResults);
  fs.close();
  
  const bool success = (rename(tempFilename.c_str(), jsonFilename.c_str()) == 0);
  if (!success)
  {
    PRINT_NAMED_ERROR("VicNeuralNets.WriteResults.RenameFail",
                      "%s -> %s", tempFilename.c_str(), jsonFilename.c_str());
    return false;
  }
  
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cv::Mat ReadBMP(const std::string& input_bmp_name) 
{
  std::ifstream file(input_bmp_name, std::ios::in | std::ios::binary);
  if (!file) {
    PRINT_NAMED_ERROR("ReadBMP.FileNotFound", "%s", input_bmp_name.c_str());
    return cv::Mat();
  }

  const auto begin = file.tellg();
  file.seekg(0, std::ios::end);
  const auto end = file.tellg();
  assert(end >= begin);
  const size_t len = (size_t) (end - begin);

  // Decode the bmp header
  const uint8_t* img_bytes = new uint8_t[len];
  file.seekg(0, std::ios::beg);
  file.read((char*)img_bytes, len);
  const int32_t header_size = *(reinterpret_cast<const int32_t*>(img_bytes + 10));
  const int32_t width = *(reinterpret_cast<const int32_t*>(img_bytes + 18));
  const int32_t height = *(reinterpret_cast<const int32_t*>(img_bytes + 22));
  const int32_t bpp = *(reinterpret_cast<const int32_t*>(img_bytes + 28));
  const int32_t channels = bpp / 8;

  // there may be padding bytes when the width is not a multiple of 4 bytes
  // 8 * channels == bits per pixel
  const int row_size = (8 * channels * width + 31) / 32 * 4;

  // if height is negative, data layout is top down
  // otherwise, it's bottom up
  const bool top_down = (height < 0);
  const int32_t absHeight = abs(height);

  // Decode image, allocating tensor once the image size is known
  cv::Mat img(height, width, CV_8UC(channels));
  uint8_t* output = img.data;

  const uint8_t* input = &img_bytes[header_size];

  for (int i = 0; i < absHeight; i++) 
  {
    int src_pos;
    int dst_pos;

    for (int j = 0; j < width; j++) 
    {
      if (!top_down) {
        src_pos = ((absHeight - 1 - i) * row_size) + j * channels;
      } else {
        src_pos = i * row_size + j * channels;
      }

      dst_pos = (i * width + j) * channels;

      switch (channels) {
        case 1:
          output[dst_pos] = input[src_pos];
          break;
        case 3:
          // BGR -> RGB
          output[dst_pos] = input[src_pos + 2];
          output[dst_pos + 1] = input[src_pos + 1];
          output[dst_pos + 2] = input[src_pos];
          break;
        case 4:
          // BGRA -> RGBA
          output[dst_pos] = input[src_pos + 2];
          output[dst_pos + 1] = input[src_pos + 1];
          output[dst_pos + 2] = input[src_pos];
          output[dst_pos + 3] = input[src_pos + 3];
          break;
        default:
          PRINT_NAMED_ERROR("ReadBMP.UnexpectedNumChannels", "%d", channels);
          return cv::Mat();
      }
    }
  }

  return img;
}
