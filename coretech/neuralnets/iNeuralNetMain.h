/**
 * File: iNeuralNetMain.h
 *
 * Author: Andrew Stein
 * Date:   9/14/2018
 *
 * Description: Interface class to create a standalone process to run forward inference using a neural network.
 *
 *              Currently uses the file system as a poor man's IPC to communicate with the "messenger"
 *              NeuralNetRunner::Model implementation in vic-engine's Vision System. See also VIC-2686.
 *
 * Copyright: Anki, Inc. 2018
 **/


#ifndef __Anki_NeuralNets_INeuralNetMain_H__
#define __Anki_NeuralNets_INeuralNetMain_H__

#include "coretech/common/shared/types.h"

#include <list>
#include <string>

namespace cv {
  class Mat;
}

namespace Json {
  class Value;
}

namespace Anki {
  
namespace Util {
  class ILoggerProvider;
}
  
namespace Vision {
  struct SalientPoint;
}
  
namespace NeuralNets {
  
class NeuralNetModel;
 
class INeuralNetMain
{
public:
  
  // One-time setup that must be called before calling Run()
  // If imageFileToProcess is empty, the cachePath is polled for "neuralNetImage.png" to process.
  // Otherwise the given image is processed on Run(), and will complete immediately.
  Result Init(const std::string& configFilename,
              const std::string& modelPath,
              const std::string& cachePath,
              const std::string& imageFileToProcess = "");
  
  // Wait for and process images until ShouldShutdown() returns true (is blocking)
  Result Run();
  
  virtual ~INeuralNetMain();
  
protected:
  
  INeuralNetMain();
  
  //
  // Derived classes should implement these simple methods
  //
  
  // Return true if something has happened to tell the class to stop running
  virtual bool ShouldShutdown() = 0;
  
  // Return a logger to use
  virtual Util::ILoggerProvider* GetLoggerProvider() = 0;
  
  // Override if any cleanup is needed after Run() stops
  virtual void DerivedCleanup() {}
  
  // Return the time between polling for a new image available for processing
  virtual int GetPollPeriod_ms(const Json::Value& config) const = 0;
  
  // Define what happens at the end of each loop of Run() (e.g. a wait, check for shutdown, etc)
  virtual void Step(int pollPeriod_ms) = 0;
  
private:
  
  void CleanupAndExit(Result result);
  
  static void GetImage(const std::string& imageFilename,
                       const std::string timestampFilename,
                       cv::Mat& img,
                       TimeStamp_t& timestamp);
  
  static void ConvertSalientPointsToJson(const std::list<Vision::SalientPoint>& salientPoints,
                                         const bool isVerbose, Json::Value& detectionResults);
  
  static bool WriteResults(const std::string jsonFilename, const Json::Value& detectionResults);
  
  std::unique_ptr<NeuralNetModel> _neuralNet;
  
  std::string _imageFilename;
  std::string _timestampFilename;
  std::string _jsonFilename;
  
  int  _pollPeriod_ms = 10;
  bool _imageFileprovided = false;
  bool _isInitialized = false;
  
};

} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_NeuralNets_INeuralNetMain_H__ */

