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

#ifndef __Anki_NeuralNets_NeuralNetModel_Interface_H__
#define __Anki_NeuralNets_NeuralNetModel_Interface_H__

#include "coretech/common/shared/array2d.h"
#include "coretech/common/shared/types.h"
#include "coretech/neuralnets/neuralNetParams.h"
#include "coretech/vision/engine/image.h"

#include "clad/types/salientPointTypes.h"
#include "json/json.h"

#include <list>
#include <vector>

namespace Anki {  
namespace NeuralNets {
    
class INeuralNetModel
{
public:

  virtual ~INeuralNetModel();
  
  bool IsVerbose() const { return _params.verbose; }
  
  // Subclasses are expected to overload the LoadModel and Detect methods below.
  // Note that we are not using virtual abstract methods here because we have no need for polymorphism.
  // There will only ever be one "type" of NeuralNetModel present compiled into the system.
  // There is no implementation of these methods in the .cpp file for this class as they should never
  // be referenced, only overridden by subclasses.
  
  // Load the model/labels files specified in the config and set up assocated parameters
  Result LoadModel(const std::string& modelPath, const Json::Value& config);
  
  // Run forward inference on the given image and return any SalientPoints found
  // Note that the input imge could be modified (e.g. resized in place)
  virtual Result Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints) = 0;
  
  const std::string& GetName() const { return _name; }
  
protected:
  
  INeuralNetModel();
  
  // Called by LoadModel, to be implemented by all derived classes
  virtual Result LoadModelInternal(const std::string& modelPath, const Json::Value& config) = 0;
  
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
                                   const float scale, const int zero_point,
                                   std::list<Vision::SalientPoint>& salientPoints);

  template<typename T>
  void ResponseMapOutputHelper(const T* outputData, TimeStamp_t timestamp,
                               const int numberOfChannels,
                               std::list<Vision::SalientPoint>& salientPoints);

  NeuralNetParams                           _params;
  std::vector<std::string>                  _labels;
  
  // For OutputType::BinaryLocalization
  Vision::Image                             _detectionGrid;
  Array2d<int32_t>                          _labelsGrid;

private:

  void SaveResponseMaps(const std::vector<cv::Mat>& channels, const int numberOfChannels,
                        const TimeStamp_t timestamp);

  std::string _name;
  std::string _cachePath;
  
  class SlidingWindow;
  std::map<int,std::unique_ptr<SlidingWindow>> _slidingScoreWindows;
    
};

} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_NeuralNets_NeuralNetModel_Interface_H__ */
