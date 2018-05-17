#ifndef __Anki_Vision_ObjectDetector_TensorFlow_H__
#define __Anki_Vision_ObjectDetector_TensorFlow_H__

#include "coretech/common/shared/types.h"
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

class ObjectDetector
{
public:
  ObjectDetector();

  ~ObjectDetector();

  Result LoadModel(const std::string& modelPath, const Json::Value& config);

  struct DetectedObject
  {
    TimeStamp_t     timestamp;
    float           score;
    std::string     name;
    float           xmin, ymin, xmax, ymax;
  };

  Result Detect(cv::Mat& img, const TimeStamp_t t, std::list<DetectedObject>& objects);

  bool IsVerbose() const { return _params.verbose; }

private:

  static Result ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out);

  void GetClassification(const tensorflow::Tensor& output_tensor, TimeStamp_t timestamp, 
                         std::list<DetectedObject>& objects);

  void GetLocalizedBinaryClassification(const tensorflow::Tensor& output_tensor, TimeStamp_t timestamp, 
                                        std::list<DetectedObject>& objects);

  void GetDetectedObjects(const std::vector<tensorflow::Tensor>& output_tensors, TimeStamp_t timestamp,
                          std::list<DetectedObject>& objects);

  enum class OutputType {
    Classification,
    BinaryLocalization,
    SSD_Boxes,
  };

  struct 
  {
    std::string               graph_file; 
    std::string               labels_file;
    std::string               architecture;
    
    int32_t                   input_width = 128;
    int32_t                   input_height = 128;
    
    // When input to graph is float, data is first divided by scale and then shifted
    // I.e.:  float_input = data / scale + shift
    float                     input_shift = 0.f;
    float                     input_scale = 255.f;
    
    float                     min_score = 0.5f; // in [0,1]
    
    // Used by "custom" architectures
    std::string               input_layer_name;
    std::vector<std::string>  output_layer_names;
    OutputType                output_type;
    bool                      use_float_input = false;
    bool                      use_grayscale = false;

    // For "binary_localization" output_type, the localization grid resolution
    // (ignored otherwise)
    int32_t                   num_grid_rows = 6;
    int32_t                   num_grid_cols = 6;

    bool                      verbose = false;

    bool                      memory_map_graph = false;

  } _params;

  // Populate _params struct from Json config
  Result SetParamsFromConfig(const Json::Value& config);

  // Helper to set _params.output_type enum
  // Must be called after input/output_layer_names have been set
  Result SetOutputTypeFromConfig(const Json::Value& config);

  std::unique_ptr<tensorflow::Session>      _session;
  std::unique_ptr<tensorflow::MemmappedEnv> _memmapped_env;
  std::unique_ptr<tensorflow::Session>      _imageReadSession;
  std::vector<std::string>                  _labels;
  
}; // class ObjectDetector

} // namespace Anki

#endif /* __Anki_Vision_ObjectDetector_TensorFlow_H__ */
