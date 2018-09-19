/**
 * File: objectDetector_tensorflow.h
 *
 * Author: Andrew Stein
 * Date:   5/17/2018
 *
 * Description: Implementation of NeuralNetModel class which wraps TensorFlow's API in order
 *              to be used by the vic-neuralnets standalone forward inference process.
 *
 *              NOTE: this implementation (.cpp) of this wrapper completely compiles out if
 *              not using the TensorFlow platform for neural nets.
 *
 * Copyright: Anki, Inc. 2018
 **/

// NOTE: this wrapper completely compiles out if we're using a different model (e.g. TFLite)

#ifndef __Anki_Vision_NeuralNetModel_TensorFlow_H__
#define __Anki_Vision_NeuralNetModel_TensorFlow_H__

#include "coretech/common/shared/types.h"
#include "coretech/neuralnets/neuralNetModel_interface.h"

#include "json/json.h"

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
namespace NeuralNets {
  
class NeuralNetModel : public INeuralNetModel
{
public:

  explicit NeuralNetModel(const std::string& cachePath);

  ~NeuralNetModel();

  // Load the model/labels files specified in the config and set up assocated parameters
  Result LoadModel(const std::string& modelPath, const Json::Value& config);

  // Run forward inference on the given image/timestamp and return any SalientPoints found
  Result Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints);

private:

  // Helper to interpret four outputs as SSD boxes (num detections, scores, classes, and boxes)
  void GetDetectedObjects(const std::vector<tensorflow::Tensor>& outputTensors, TimeStamp_t timestamp,
                          std::list<Vision::SalientPoint>& salientPoints);

  // Thin wrapper to tensorflow run to allow us to turn on benchmarking to logs
  Result Run(tensorflow::Tensor image_tensor, std::vector<tensorflow::Tensor>& outputTensors);

  std::unique_ptr<tensorflow::Session>      _session;
  std::unique_ptr<tensorflow::MemmappedEnv> _memmappedEnv;
  
}; // class NeuralNetModel

} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_NeuralNetModel_TensorFlow_H__ */
