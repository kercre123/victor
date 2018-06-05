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

#ifndef __Anki_Vision_NeuralNetModel_TensorFlow_H__
#define __Anki_Vision_NeuralNetModel_TensorFlow_H__

#include "coretech/common/shared/types.h"

#include "clad/types/salientPointTypes.h"

#include "json/json.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"

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

class NeuralNetModel
{
public:

  NeuralNetModel();

  ~NeuralNetModel();

  // Load the model/labels files specified in the config and set up assocated parameters
  Result LoadModel(const std::string& modelPath, const Json::Value& config);

  // Run forward inference on the given image/timestamp and return any SalientPoints found
  Result Detect(cv::Mat& img, const TimeStamp_t t, std::list<Vision::SalientPoint>& salientPoints);

  bool IsVerbose() const { return _params.verbose; }

private:

  // Helper to read simple text labels files (one label per line)
  static Result ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out);

  // Helper to find the index of the a single output with the highest score, assumed to 
  // correspond to the matching label from the labels file
  void GetClassification(const tensorflow::Tensor& outputTensor, TimeStamp_t timestamp, 
                         std::list<Vision::SalientPoint>& salientPoints);

  // Helper to return a set of localization boxes from a grid, assuming a binary classifcation 
  // (e.g. person / no-person in a 6x6 grid). Grid size is specified in JSON config
  void GetLocalizedBinaryClassification(const tensorflow::Tensor& outputTensor, TimeStamp_t timestamp, 
                                        std::list<Vision::SalientPoint>& salientPoints);

  // Helper to interpret four outputs as SSD boxes (num detections, scores, classes, and boxes)
  void GetDetectedObjects(const std::vector<tensorflow::Tensor>& outputTensors, TimeStamp_t timestamp,
                          std::list<Vision::SalientPoint>& salientPoints);

  // Specified in config, used to determine how to interpret the output of the network, and
  // thus which helper above to call (string to use in JSON config shown for each)
  enum class OutputType {
    Classification,
    BinaryLocalization,
    AnchorBoxes,
  };

  struct 
  {
    std::string               graphFile; 
    std::string               labelsFile;
    std::string               architecture;
    
    int32_t                   inputWidth = 128;
    int32_t                   inputHeight = 128;
    
    // When input to graph is float, data is first divided by scale and then shifted
    // I.e.:  float_input = data / scale + shift
    float                     inputShift = 0.f;
    float                     inputScale = 255.f;
    
    float                     minScore = 0.5f; // in [0,1]
    
    // Used by "custom" architectures
    std::string               inputLayerName;
    std::vector<std::string>  outputLayerNames;
    OutputType                outputType;
    bool                      useFloatInput = false;
    bool                      useGrayscale = false;

    // For "binary_localization" output_type, the localization grid resolution
    // (ignored otherwise)
    int32_t                   numGridRows = 6;
    int32_t                   numGridCols = 6;

    bool                      verbose = false;

    // TODO: Not fully supported yet (VIC-3141)
    bool                      memoryMapGraph = false;

  } _params;

  // Populate _params struct from Json config
  Result SetParamsFromConfig(const Json::Value& config);

  // Helper to set _params.output_type enum
  // Must be called after input/output_layer_names have been set
  Result SetOutputTypeFromConfig(const Json::Value& config);

  std::unique_ptr<tensorflow::Session>      _session;
  std::unique_ptr<tensorflow::MemmappedEnv> _memmappedEnv;
  std::vector<std::string>                  _labels;
  
  // For OutputType::BinaryLocalization
  cv::Mat_<uint8_t>                         _detectionGrid;
  cv::Mat_<int32_t>                         _labelsGrid;
  
}; // class NeuralNetModel

} // namespace Anki

#endif /* __Anki_Vision_NeuralNetModel_TensorFlow_H__ */
