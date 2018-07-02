/**
 * File: objectDetector_tensorflow.h
 *
 * Author: Andrew Stein
 * Date:   5/17/2018
 *
 * Description: Implementation of NeuralNetModel class which wraps TensorFlow's API in order
 *              to be used by the vic-neuralnets standalone forward inference process.
 *
 *              Note this deliberately avoids use of Anki:Vision Image classes and instead uses
 *              raw cv::Mat data structures in order to reduce dependencies and chances of 
 *              conflict. This could probably be relaxed eventually.
 *
 * Copyright: Anki, Inc. 2018
 **/

// NOTE: this wrapper completely compiles out if we're using a different model (e.g. TFLite)
#ifdef VIC_NEURALNETS_USE_TENSORFLOW

#ifndef __Anki_Vision_NeuralNetModel_TensorFlow_H__
#define __Anki_Vision_NeuralNetModel_TensorFlow_H__

#include "coretech/common/shared/types.h"
#include "coretech/vision/neuralnets/neuralNetModel_interface.h"

#include "json/json.h"

#include "opencv2/core/core.hpp"

#include <list>
#include <memory>
#include <sstream>
#include <string>

// Forward declarations:
namespace tensorflow {
  class MemmappedEnv;
  class Session;
  class Tensor;
}

namespace Anki {
namespace Vision {
  
class NeuralNetModel : public INeuralNetModel
{
public:

  NeuralNetModel();

  ~NeuralNetModel();

  // Load the model/labels files specified in the config and set up assocated parameters
  Result LoadModel(const std::string& modelPath, const Json::Value& config);

  // Run forward inference on the given image/timestamp and return any SalientPoints found
  Result Detect(cv::Mat& img, const TimeStamp_t t, std::list<Vision::SalientPoint>& salientPoints);

private:

  // Helper to return a set of localization boxes from a grid, assuming a binary classifcation 
  // (e.g. person / no-person in a 6x6 grid). Grid size is specified in JSON config
  void GetLocalizedBinaryClassification(const tensorflow::Tensor& outputTensor, TimeStamp_t timestamp, 
                                        std::list<Vision::SalientPoint>& salientPoints);

  // Helper to interpret four outputs as SSD boxes (num detections, scores, classes, and boxes)
  void GetDetectedObjects(const std::vector<tensorflow::Tensor>& outputTensors, TimeStamp_t timestamp,
                          std::list<Vision::SalientPoint>& salientPoints);

  // Thin wrapper to tensorflow run to allow us to turn on benchmarking to logs
  Result Run(tensorflow::Tensor image_tensor, std::vector<tensorflow::Tensor>& outputTensors);

  std::unique_ptr<tensorflow::Session>      _session;
  std::unique_ptr<tensorflow::MemmappedEnv> _memmappedEnv;
  
  // For OutputType::BinaryLocalization
  cv::Mat_<uint8_t>                         _detectionGrid;
  cv::Mat_<int32_t>                         _labelsGrid;
  
}; // class NeuralNetModel

} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_NeuralNetModel_TensorFlow_H__ */

#endif /* VIC_NEURALNETS_USE_TENSORFLOW */
