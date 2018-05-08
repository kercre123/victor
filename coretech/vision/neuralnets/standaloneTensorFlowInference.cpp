/**
 * File: standaloneTensorFlowInference.cpp
 *
 * Author: Andrew Stein
 * Date:   1/29/2018
 *
 * Description: Standalone process to run forward inference through a TensorFlow model.
 *              Currently uses the file system as a poor man's IPC.
 *
 * Copyright: Anki, Inc. 2018
 **/

#if defined(TENSORFLOW)
#  include "objectDetector_tensorflow.h"
#elif defined(CAFFE2)
#  include "objectDetector_caffe2.h"
#elif defined(OPENCV_DNN)
#  include "objectDetector_opencvdnn.h"
#elif defined(TFLITE)
#  include "objectDetector_tflite.h"
#else 
#  error TENSORFLOW or CAFFE2 or OPENCVDNN or TFLITE must be defined
#endif

#include <atomic>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <thread>

std::atomic<bool> quit(false);
void got_signal(int)
{
  quit.store(true);
}

#define PRINT_TIMING 1

using ClockType = std::chrono::high_resolution_clock;
using TimePoint = std::chrono::time_point<ClockType>;

inline TimePoint Tic() {
  return ClockType::now();  
}

void Toc(TimePoint startTime, const std::string& name) {
  if(PRINT_TIMING) {
    const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(ClockType::now() - startTime);
    std::cout << "StandaloneTensorFlow." << name << " took " << duration.count() << "ms" << std::endl;
  }
}

cv::Mat read_bmp(const std::string& input_bmp_name); // defined below, after main()

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
int main(int argc, char **argv)
{
  // For calling destructor when process is killed
  // https://stackoverflow.com/questions/4250013/is-destructor-called-if-sigint-or-sigstp-issued
  struct sigaction sa;
  memset( &sa, 0, sizeof(sa) );
  sa.sa_handler = got_signal;
  sigfillset(&sa.sa_mask);
  sigaction(SIGINT,&sa,NULL);

  if(argc < 4)
  {
    std::cout << "Usage: " << argv[0] << " <configFile>.json modelPath cachePath <imageFile>" << std::endl;
    return -1;
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
      PRINT_NAMED_ERROR(argv[0], "Could not read config file: %s", configFilename.c_str());
      return -1;
    }

    if(!config.isMember("ObjectDetector"))
    {
      PRINT_NAMED_ERROR(argv[0], "Config file missing 'ObjectDetector' field");
      return -1;
    }

    config = config["ObjectDetector"];
  }

  const bool imageFileProvided = (argc > 4);

  const std::string imageFilename = (imageFileProvided ? 
                                     argv[4] :
                                     FullFilePath(cachePath, "objectDetectionImage.png")); 

  const std::string timestampFilename = FullFilePath(cachePath, "timestamp.txt");

  std::cout << (imageFileProvided ? "Loading given image: " : "Polling for images at: ") << 
    imageFilename << std::endl;

  const int kPollFrequency_ms = 10;

  // Initialize the detector
  ObjectDetector detector;
  auto startTime = Tic();
  Result initResult = detector.LoadModel(modelPath, config);
  Toc(startTime, "LoadModel");
  if(RESULT_OK != initResult)
  {
    PRINT_NAMED_ERROR(argv[0], "Failed to load model from path: %s", modelPath.c_str());
    return -1;
  }
  
  std::cout << "Detector initialized, waiting for images" << std::endl;

  while(true)
  {
    // Is there an image file available in the cache?
    const bool isImageAvailable = FileExists(imageFilename);

    if(isImageAvailable)
    {
      if(detector.IsVerbose())
      {
        std::cout << "Found image: " << imageFilename << std::endl;
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
      TimeStamp_t timestamp=0;
      if(FileExists(timestampFilename))
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

      Json::Value detectionResults;
      
      // Convert the results to JSON
      {
        if(!objects.empty())
        {
          std::cout << "Detected " << objects.size() << " objects: ";
        }

        Json::Value& objectsJSON = detectionResults["objects"];
        for(auto const& object : objects)
        {
          std::cout << object.name << "[" << (int)round(100.f*object.score) << "] ";
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
          std::cout << std::endl;
        }        
      }

      // Write out the Json
      startTime = Tic();
      {
        const std::string jsonFilename = FullFilePath(cachePath, "objectDetectionResults.json");
        if(detector.IsVerbose())
        {
          std::cout << "Writing results to JSON: " << jsonFilename << std::endl;
        }

        Json::StyledStreamWriter writer;
        std::fstream fs;
        fs.open(jsonFilename, std::ios::out);
        if (!fs.is_open()) {
          std::cerr << "Failed to open output file: " << jsonFilename << std::endl;
          return -1;
        }
        writer.write(fs, detectionResults);
        fs.close();
      }
      Toc(startTime, "WriteJSON");

      
      if(imageFileProvided)
      {
        break;
      }
      else
      {
        // Remove the image file we were working with
        if(detector.IsVerbose())
        {
          std::cout << "Deleting image file: " << imageFilename << std::endl;
        }
        remove(imageFilename.c_str());
      }
    }
    else if(imageFileProvided)
    {
      std::cerr << imageFilename << " does not exist!" << std::endl;
      break;
    }
    else 
    {
      if(detector.IsVerbose())
      {
        const int kVerbosePrintFreq_ms = 1000;
        static int count = 0;
        if(count++ * kPollFrequency_ms >= kVerbosePrintFreq_ms)
        {
          std::cout << "Waiting for image..." << std::endl;
          count = 0;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(kPollFrequency_ms));  
    }

    if(quit.load()) 
    {
      std::cout << std::endl << "Terminating " << argv[0] << std::endl;
      break;
    }
  }

  return 0;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
cv::Mat read_bmp(const std::string& input_bmp_name) 
{
  int begin, end;

  std::ifstream file(input_bmp_name, std::ios::in | std::ios::binary);
  if (!file) {
    std::cerr << "Input file " << input_bmp_name << " not found" << std::endl;
    exit(-1);
  }

  begin = file.tellg();
  file.seekg(0, std::ios::end);
  end = file.tellg();
  size_t len = end - begin;

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
          std::cerr << "Unexpected number of channels: " << channels << std::endl;
          return img;
      }
    }
  }

  return img;
}
