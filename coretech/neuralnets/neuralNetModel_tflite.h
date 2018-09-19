/**
 * File: neuralNetModel_tflite.h
 *
 * Author: Andrew Stein
 * Date:   12/5/2017
 *
 * Description: Implementation of NeuralNetModel class which wraps TensorFlow Lite.
 *
 *              NOTE: this implementation (.cpp) of this wrapper completely compiles out if
 *              not using the TFLite platform for neural nets.
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Anki_NeuralNets_NeuralNetModel_TFLite_H__
#define __Anki_NeuralNets_NeuralNetModel_TFLite_H__

#include "coretech/neuralnets/neuralNetModel_interface.h"

#include <list>

// Forward declaration
namespace tflite
{
  class FlatBufferModel;
  class Interpreter;
}

namespace Anki {
namespace NeuralNets {

class NeuralNetModel : public INeuralNetModel
{
public:
  
  explicit NeuralNetModel(const std::string& cachePath);
  ~NeuralNetModel();

  // ObjectDetector expects LoadModel and Run to exist
  Result LoadModel(const std::string& modelPath, const Json::Value& config);
  Result Detect(Vision::ImageRGB& img, std::list<Vision::SalientPoint>& salientPoints);
  
private:
  
  void ScaleImage(Vision::ImageRGB& img);
  
  std::unique_ptr<tflite::FlatBufferModel> _model;
  std::unique_ptr<tflite::Interpreter>     _interpreter;

}; // class NeuralNetModel

} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_Vision_NeuralNetModel_TFLite_H__ */

