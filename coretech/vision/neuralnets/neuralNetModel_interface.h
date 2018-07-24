/**
 * File: neuralNetModel_interface.h
 *
 * Author: Andrew Stein
 * Date:   7/2/2018
 *
 * Description: Defines interface and shared helpers for various NeuralNetModel implementations.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_Vision_NeuralNetModel_Interface_H__
#define __Anki_Vision_NeuralNetModel_Interface_H__

#include "coretech/common/shared/types.h"
#include "coretech/vision/neuralnets/neuralNetParams.h"

#include "clad/types/salientPointTypes.h"
#include "json/json.h"

#include "opencv2/core/core.hpp"

#include <list>
#include <vector>

namespace Anki {
namespace Vision {
    
class INeuralNetModel
{
public:

  explicit INeuralNetModel(const std::string& cachePath);

  ~INeuralNetModel()
  {
    
  }
  
  bool IsVerbose() const { return _params.verbose; }
  
  // Subclasses are expected to overload the LoadModel and Detect methods below.
  // Note that we are not using virtual abstract methods here because we have no need for polymorphism.
  // There will only ever be one "type" of NeuralNetModel present compiled into the system.
  // There is no implementation of these methods in the .cpp file for this class as they should never
  // be referenced, only overridden by subclasses.
  
  // Load the model/labels files specified in the config and set up assocated parameters
  Result LoadModel(const std::string& modelPath, const Json::Value& config);
  
  // Run forward inference on the given image/timestamp and return any SalientPoints found
  Result Detect(cv::Mat& img, const TimeStamp_t t, std::list<Vision::SalientPoint>& salientPoints);
  
protected:
  
  // Base model not meant to be directly instantiated
  INeuralNetModel() = default;
  
  // Helper to read simple text labels files (one label per line)
  static Result ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out);
  
  // Helper to find the index of the single output with the highest score (assumed to correspond to the matching
  // label from the labels file) and add a single, centered, full-image SalientPoint to the given list
  // Implemented for float and uint8 types
  template<typename T>
  void ClassificationOutputHelper(const T* outputData, TimeStamp_t timestamp,
                                  std::list<Vision::SalientPoint>& salientPoints);
  
  // Helper to return a SalientPoint for each connected component of a grid of binary classifiers
  // (e.g. person / no-person in a 6x6 grid). Grid size is specified in JSON config.
  // Implemented for float and uint8 types
  template<typename T>
  void LocalizedBinaryOutputHelper(const T* outputData, TimeStamp_t timestamp,
                                   std::list<Vision::SalientPoint>& salientPoints);

  template<typename T>
  void ResponseMapOutputHelper(const T* outputData, TimeStamp_t timestamp,
                               const int numberOfChannels,
                               std::list<Vision::SalientPoint>& salientPoints);

  NeuralNetParams                           _params;
  std::vector<std::string>                  _labels;
  
  // For OutputType::BinaryLocalization
  cv::Mat_<uint8_t>                         _detectionGrid;
  cv::Mat_<int32_t>                         _labelsGrid;

private:

  void SaveResponseMaps(const std::vector<cv::Mat>& channels, const int numberOfChannels,
                        const TimeStamp_t timestamp);
  std::string _cachePath;
};

} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_NeuralNetModel_Interface_H__ */
