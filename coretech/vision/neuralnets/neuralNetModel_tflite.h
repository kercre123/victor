/**
 * File: neuralNetModel_tflite.h
 *
 * Author: Andrew Stein
 * Date:   12/5/2017
 *
 * Description: Implementation of NeuralNetModel class which wraps TensorFlow Lite.
 *
 * Copyright: Anki, Inc. 2017
 **/

// NOTE: this wrapper completely compiles out if we're using a different model (e.g. TensorFlow)
#ifdef VIC_NEURALNETS_USE_TFLITE

#ifndef __Anki_Vision_NeuralNetModel_TFLite_H__
#define __Anki_Vision_NeuralNetModel_TFLite_H__

#include "coretech/vision/neuralnets/neuralNetModel_interface.h"

#include <list>

// Forward declaration
namespace tflite
{
  class FlatBufferModel;
  class Interpreter;
}

namespace Anki {
namespace Vision {

class NeuralNetModel : public INeuralNetModel
{
public:
  
  NeuralNetModel();
  ~NeuralNetModel();

  // ObjectDetector expects LoadModel and Run to exist
  Result LoadModel(const std::string& modelPath, const Json::Value& config);
  Result Detect(cv::Mat& img, const TimeStamp_t t, std::list<Vision::SalientPoint>& salientPoints);
  
private:
  
  void ScaleImage(cv::Mat& img);
  
  std::unique_ptr<tflite::FlatBufferModel> _model;
  std::unique_ptr<tflite::Interpreter>     _interpreter;

}; // class NeuralNetModel

} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_NeuralNetModel_TFLite_H__ */

#endif // #if USE_TENSORFLOW_LITE
