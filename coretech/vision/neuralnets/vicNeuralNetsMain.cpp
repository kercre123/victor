/**
 * File: standaloneForwardInference.cpp
 *
 * Author: Andrew Stein
 * Date:   1/29/2018
 *
 * Description: Standalone process to run forward inference through a variety of neural network platforms,
 *              specified at compile time defines (e.g. TensorFlow, OpenCV DNN, or TF Lite).
 *
 *              Currently uses the file system as a poor man's IPC to communicate with the "standalone"
 *              ObjectDetector implementation in vic-engine's Vision System.
 *
 *              Can be used as a Webots controller when compiled with -DSIMULATOR
 *
 * Copyright: Anki, Inc. 2018
 **/

#if defined(VIC_NEURALNETS_USE_TENSORFLOW)
#  include "objectDetector_tensorflow.h"
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
#define PRINT_TIMING 1

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
// NOTE: this simplified Tic/Toc approach does not support nested calls. Toc just returns time since last Tic!
// TODO: Use coretech/util profilers 

using ClockType = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<ClockType>;

static inline TimePoint Tic() 
{
  return ClockType::now();  
}

static void Toc(TimePoint startTime, const std::string& name) 
{
  if(PRINT_TIMING) {
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(ClockType::now() - startTime);
    const std::string eventName("VicNeuralNets.Toc." + name);
    LOG_INFO(eventName.c_str(), "%dms", (int)duration.count());
  }
}

// Simple bmp reader for test images
cv::Mat read_bmp(const std::string& input_bmp_name); // implemented below, after main()

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
  Util::gLoggerProvider = logger.get();
# else
  const bool colorizeStderrOutput = false; // TODO: Get from Webots proto in simulation?
  auto logger = std::make_unique<Util::PrintfLoggerProvider>(Util::ILoggerProvider::LOG_LEVEL_DEBUG,
                                                               colorizeStderrOutput);  
# endif

  Util::gLoggerProvider = logger.get();

# ifdef SIMULATOR
  webots::Supervisor webotsSupervisor;
# endif

  Result result = RESULT_OK;

  if(argc < 4)
  {
    std::cout << "Usage: " << argv[0] << " <configFile>.json modelPath cachePath <imageFile>" << std::endl;
    CleanupAndExit(result);
  }
  
  const std::string configFilename(argv[1]);
  const std::string modelPath(argv[2]);
  const std::string cachePath(argv[3]);

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

    if(!config.isMember("ObjectDetector"))
    {
      PRINT_NAMED_ERROR("VicNeuralNets.Main.MissingObjectDetectorField", "Config file missing 'ObjectDetector' field");
      CleanupAndExit(RESULT_FAIL);
    }

    config = config["ObjectDetector"];

    if(!config.isMember("poll_period_ms")) 
    {
      PRINT_NAMED_ERROR("VicNeuralNets.Main.MissingPollPeriodField", "No 'poll_period_ms' specified in config file");
      CleanupAndExit(RESULT_FAIL);
    }
  }

  const bool imageFileProvided = (argc > 4);

  const std::string imageFilename = (imageFileProvided ? 
                                     argv[4] :
                                     Util::FileUtils::FullFilePath({cachePath, "objectDetectionImage.png"})); 

  const std::string timestampFilename = Util::FileUtils::FullFilePath({cachePath, "timestamp.txt"});

  LOG_INFO("VicNeuralNets.Main.ImageLoadMode", "%s: %s",
           (imageFileProvided ? "Loading given image" : "Polling for images at"),
           imageFilename.c_str());

  const int kPollPeriod_ms = config["poll_period_ms"].asInt();

  // Initialize the detector
  ObjectDetector detector;
  auto startTime = Tic();
  result = detector.LoadModel(modelPath, config);
  Toc(startTime, "LoadModel");
  if(RESULT_OK != result)
  {
    PRINT_NAMED_ERROR(argv[0], "Failed to load model from path: %s", modelPath.c_str());
    CleanupAndExit(result);
  }
  
  LOG_INFO("VicNeuralNets.Main.DetectorInitialized", "Waiting for images");

  // Loop until shutdown or error
  while (!gShutdown) 
  {
    // Is there an image file available in the cache?
    const bool isImageAvailable = Util::FileUtils::FileExists(imageFilename);

    if(isImageAvailable)
    {
      if(detector.IsVerbose())
      {
        LOG_INFO("VicNeuralNets.Main.FoundImage", "%s", imageFilename.c_str());
      }

      // Get the image
      auto startTime = Tic();
      cv::Mat img;
      
      const std::string ext = imageFilename.substr(imageFilename.size()-3,3);
      if(ext == "bmp")
      {
        img = read_bmp(imageFilename); // Converts to RGB internally
      } 
      else {
        img = cv::imread(imageFilename);
        cv::cvtColor(img, img, CV_BGR2RGB); // OpenCV loads BGR, TF expects RGB
      }

      if(img.empty())
      {
        PRINT_NAMED_ERROR("VicNeuralNets.Main.ImageReadFailed", "Empty image from %s", imageFilename.c_str());
        result = RESULT_FAIL;
        break;
      }

      TimeStamp_t timestamp=0;
      if(Util::FileUtils::FileExists(timestampFilename))
      {
        std::ifstream file(timestampFilename);
        std::string line;
        std::getline(file, line);
        timestamp = std::stoi(line);
      }
      Toc(startTime, "ImageRead");

      // Detect what's in it
      startTime = Tic();
      std::list<ObjectDetector::DetectedObject> objects;
      Result result = detector.Detect(img, timestamp, objects);
      Toc(startTime, "Detect");

      if(RESULT_OK != result)
      {
        PRINT_NAMED_ERROR(argv[0], "Detect failed!");
      }

      Json::Value detectionResults;
      
      // Convert the results to JSON
      {
        std::string objectsStr;
        
        Json::Value& objectsJSON = detectionResults["objects"];
        for(auto const& object : objects)
        {
          objectsStr += object.name + "[" + std::to_string((int)round(100.f*object.score)) + "] ";
          Json::Value json;
          json["timestamp"] = object.timestamp;
          json["score"]     = object.score;
          json["name"]      = object.name;
          json["xmin"]      = object.xmin;
          json["ymin"]      = object.ymin;
          json["xmax"]      = object.xmax;
          json["ymax"]      = object.ymax;
          
          objectsJSON.append(json);
        }  

        if(!objects.empty())
        {
          LOG_INFO("VicNeuralNets.Main.DetectedObjects", 
                   "Detected %zu objects: %s", objects.size(), objectsStr.c_str());
        }        
      }

      // Write out the Json
      startTime = Tic();
      {
        const std::string jsonFilename = Util::FileUtils::FullFilePath({cachePath, "objectDetectionResults.json"});
        if(detector.IsVerbose())
        {
          LOG_INFO("VicNeuralNets.Main.WritingResults", "%s", jsonFilename.c_str());
        }

        Json::StyledStreamWriter writer;
        std::fstream fs;
        fs.open(jsonFilename, std::ios::out);
        if (!fs.is_open()) {
          PRINT_NAMED_ERROR("VicNeuralNets.Main.OutputFileOpenFailed", "%s", jsonFilename.c_str());
          result = RESULT_FAIL;
          break;
        }
        writer.write(fs, detectionResults);
        fs.close();
      }
      Toc(startTime, "WriteJSON");

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
        if(detector.IsVerbose())
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
      if(detector.IsVerbose())
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
cv::Mat read_bmp(const std::string& input_bmp_name) 
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
