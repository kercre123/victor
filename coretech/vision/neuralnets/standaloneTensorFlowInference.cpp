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


#include "objectDetector_tensorflow.h"

#include <atomic>
#include <chrono>
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
    std::cout << "Usage: " << argv[0] << " <configFile>.json modelPath cachePath" << std::endl;
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
  }

  const std::string imageFilename = FullFilePath(cachePath, "objectDetectionImage.png");
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
        std::cout << "Found image" << std::endl;
      }

      // Get the image
      auto startTime = Tic();
      cv::Mat img = cv::imread(imageFilename);
      Toc(startTime, "ImageRead");

      // Detect what's in it
      startTime = Tic();
      std::list<ObjectDetector::DetectedObject> objects;
      Result result = detector.Detect(img, objects);
      Toc(startTime, "Detect");

      Json::Value detectionResults;
      
      // Convert the results to JSON
      {
        Json::Value& objectsJSON = detectionResults["objects"];
        for(auto const& object : objects)
        {
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

      // Remove the image file we were working with
      if(detector.IsVerbose())
      {
        std::cout << "Deleting image file: " << imageFilename << std::endl;
      }
      remove(imageFilename.c_str());
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
