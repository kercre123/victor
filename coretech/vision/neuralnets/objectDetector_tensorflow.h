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

  struct 
  {
    std::string   graph; // = "tensorflow/examples/label_image/data/inception_v3_2016_08_28_frozen.pb";
    std::string   labels; // = "tensorflow/examples/label_image/data/imagenet_slim_labels.txt";
    std::string   architecture;
    
    int32_t       input_width; // = 299;
    int32_t       input_height; // = 299;

    // When input to graph is float, data is first divided by scale and then shifted
    // I.e.:  float_input = data / scale + shift
    float         input_shift; // = 0;
    float         input_scale; // = 255;
    
    float         min_score; // in [0,1]
    
    bool          verbose;

    bool          memory_map_graph;

  } _params;

  std::string                          _inputLayerName;
  std::vector<std::string>             _outputLayerNames;
  bool                                 _useFloatInput = false;
  bool                                 _useGrayscale = false;
  
  enum class OutputType {
    Classification,
    BinaryLocalization,
    SSD_Boxes,
  };

  OutputType _outputType;

  std::unique_ptr<tensorflow::Session>      _session;
  std::unique_ptr<tensorflow::MemmappedEnv> _memmapped_env;
  std::unique_ptr<tensorflow::Session>      _imageReadSession;
  std::vector<std::string>                  _labels;
  
}; // class ObjectDetector

} // namespace Anki

#endif /* __Anki_Vision_ObjectDetector_TensorFlow_H__ */
