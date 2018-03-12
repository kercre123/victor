#ifndef __Anki_Vision_ObjectDetector_Caffe2_H__
#define __Anki_Vision_ObjectDetector_Caffe2_H__

#include "json/json.h"

#include "caffe2/core/predictor.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <list>
#include <memory>
#include <sstream>
#include <string>

// In lieu of using full Anki::Util
#include "helpers.h"

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

  void GetClassification(const caffe2::Predictor::TensorVector& output_vec, 
                         TimeStamp_t timestamp, 
                         std::list<DetectedObject>& objects);

/*
  void GetDetectedObjects(const std::vector<tensorflow::Tensor>& output_tensors, TimeStamp_t timestamp,
                          std::list<DetectedObject>& objects);
*/
  struct 
  {
    std::string   graph; // = "tensorflow/examples/label_image/data/inception_v3_2016_08_28_frozen.pb";
    std::string   architecture; // "classification" or "detection"
    std::string   labels; // = "tensorflow/examples/label_image/data/imagenet_slim_labels.txt";
    
    int32_t       input_width; // = 299;
    int32_t       input_height; // = 299;
    uint8_t       input_mean_R; // = 0;
    uint8_t       input_mean_G; // = 0;
    uint8_t       input_mean_B; // = 0;
    int32_t       input_std; // = 255;
    
    float         min_score; // in [0,1]
    
    bool          verbose;

    bool          memoryMapGraph;

  } _params;

  std::string                          _inputLayerName;
  std::vector<std::string>             _outputLayerNames;
  bool                                 _useFloatInput = false;

  caffe2::unique_ptr<caffe2::Predictor>      _predictor;
  std::vector<std::string>                   _labels;
  
}; // class ObjectDetector

#endif /* __Anki_Vision_ObjectDetector_TensorFlow_H__ */
