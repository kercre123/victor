
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
  
  ~INeuralNetModel();
  
  // Load the model/labels files specified in the config and set up assocated parameters
  Result LoadModel(const std::string& modelPath, const Json::Value& config)
  {
    return RESULT_FAIL;
  }
  
  // Run forward inference on the given image/timestamp and return any SalientPoints found
  Result Detect(cv::Mat& img, const TimeStamp_t t, std::list<Vision::SalientPoint>& salientPoints)
  {
    return RESULT_FAIL;
  }
  
protected:
  
  // Base model not meant to be directly instantiated
  INeuralNetModel() {}
  
  // Helper to read simple text labels files (one label per line)
  static Result ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out);
  
  NeuralNetParams                           _params;
  std::vector<std::string>                  _labels;
  
};

} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_NeuralNetModel_Interface_H__ */
