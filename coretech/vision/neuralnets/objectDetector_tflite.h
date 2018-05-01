#ifndef __Anki_Vision_ObjectDetector_TFLite_H__
#define __Anki_Vision_ObjectDetector_TFLite_H__

#include "json/json.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <list>
#include <memory>
#include <sstream>
#include <string>

// In lieu of using full Anki::Util
#include "helpers.h"

// Forward declarations:
namespace tflite {
  class FlatBufferModel;
  class Interpreter;
}

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

  Result Detect(cv::Mat& img, std::list<DetectedObject>& objects);

  bool IsVerbose() const { return _params.verbose; }

private:

  static Result ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out);

  template<typename T>
  void GetClassification(TimeStamp_t timestamp, std::list<DetectedObject>& objects);

  void GetDetectedObjects(TimeStamp_t timestamp, std::list<DetectedObject>& objects);

  struct 
  {
    std::string   graph; // = "tensorflow/examples/label_image/data/inception_v3_2016_08_28_frozen.pb";
    std::string   labels; // = "tensorflow/examples/label_image/data/imagenet_slim_labels.txt";
    
    int32_t       input_width; // = 299;
    int32_t       input_height; // = 299;
    bool          use_grayscale;

    // When input to graph is float, data is first divided by scale and then shifted
    // I.e.:  float_input = data / scale + shift
    // NOTE: Only used with floating point (non-quantized) models!
    float         input_shift; // = 0;
    float         input_scale; // = 255;
    
    float         min_score; // in [0,1]
    
    bool          verbose;

    int32_t       num_threads;

  } _params;

  std::unique_ptr<tflite::FlatBufferModel>  _model;
  std::unique_ptr<tflite::Interpreter>      _interpreter;
  std::vector<std::string>                  _labels;
  
}; // class ObjectDetector

#endif /* __Anki_Vision_ObjectDetector_TFLite_H__ */
