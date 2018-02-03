/**
 * File: objectDetectorModel_tensorflow.cpp
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description: Implementation of ObjectDetector Model class which wraps TensorFlow.
 *
 * Copyright: Anki, Inc. 2017
 **/

#if 0

#include "coretech/vision/engine/objectDetector.h"
#include "coretech/vision/engine/objectDetectorModel_tensorflow.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/profiler.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

#include <list>

//#include "tensorflow/cc/ops/const_op.h"
//#include "tensorflow/contrib/image/kernels/image_ops.h"
//#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"

namespace Anki {
namespace Vision {

class ObjectDetector : TensorflowModel
{
public:

  Model(Profiler& profiler) : _profiler(profiler) { }
  
  // ObjectDetector expects LoadModel and Run to exist
  Result LoadModel(const std::string& modelPath, const Json::Value& config);
  Result Run(const ImageRGB& img, std::list<DetectedObject>& objects);
  
private:
  
  using Tensor = tensorflow::Tensor;
  using Status = tensorflow::Status;
  
  // If crop=true, the middle [input_width x input_height] is cropped out.
  // Otherwise, the entire image is scaled.
  // See also: https://github.com/tensorflow/tensorflow/issues/6955
  Status CreateTensorFromImage(const ImageRGB& img, std::vector<Tensor>* out_tensors);
  
  std::unique_ptr<tensorflow::Session> _session;
  
  Profiler _profiler;

}; // class ObjectDetector::Model
  
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::LoadModel(const std::string& modelPath, const Json::Value& config)
{
  ANKI_CPU_PROFILE("ObjectDetector.LoadModel");
  
# define GetFromConfig(keyName) \
  if(false == JsonTools::GetValueOptional(config, QUOTE(keyName), _params.keyName)) \
  { \
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.MissingConfig", QUOTE(keyName)); \
    return RESULT_FAIL; \
  }
  
  GetFromConfig(labels);
  GetFromConfig(top_K);
  GetFromConfig(min_score);
  
  if(!ANKI_VERIFY(Util::IsFltGEZero(_params.min_score) && Util::IsFltLE(_params.min_score, 1.f),
                  "ObjectDetector.Model.LoadModel.Badmin_score",
                  "%f not in range [0.0,1.0]", _params.min_score))
  {
    return RESULT_FAIL;
  }
  
  GetFromConfig(graph);
  GetFromConfig(mode);
  if(_params.mode == "detection")
  {
    _isDetectionMode = true;
  }
  else
  {
    if(_params.mode != "classification")
    {
      PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.UnknownMode",
                        "Expecting 'classification' or 'detection'. Got '%s'.",
                        _params.mode.c_str());
      return RESULT_FAIL;
    }
    _isDetectionMode = false;
  }
  
  GetFromConfig(input_layer);
  GetFromConfig(output_scores_layer);
  GetFromConfig(input_height);
  GetFromConfig(input_width);
  GetFromConfig(do_crop);
  
  if(_params.output_scores_layer.empty())
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.UnspecifiedScoresLayer", "");
    return RESULT_FAIL;
  }
  
  if(_isDetectionMode)
  {
    // These are only required/used in detection mode
    GetFromConfig(output_boxes_layer);
    GetFromConfig(output_classes_layer);
    GetFromConfig(output_num_detections_layer);
  }
  else
  {
    // Only required in classification mode
    GetFromConfig(input_mean_R);
    GetFromConfig(input_mean_G);
    GetFromConfig(input_mean_B);
    GetFromConfig(input_std);
  }
  
  const std::string graph_file_name = Util::FileUtils::FullFilePath({modelPath, _params.graph});
  
  if(!ANKI_VERIFY(Util::FileUtils::FileExists(graph_file_name),
                  "ObjectDetector.Model.LoadGraph.GraphFileDoesNotExist", "%s",
                  graph_file_name.c_str()))
  {
    return RESULT_FAIL;
  }
  
  tensorflow::GraphDef graph_def;
  Status load_graph_status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), graph_file_name, &graph_def);
  if (!load_graph_status.ok())
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.ReadBinaryProtoFailed",
                      "Status: %s", load_graph_status.ToString().c_str());
    return RESULT_FAIL;
  }
  
  PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadGraph.ModelLoadSuccess", "%s",
                graph_file_name.c_str());
  
  _session.reset(tensorflow::NewSession(tensorflow::SessionOptions()));
  Status session_create_status = _session->Create(graph_def);
  if (!session_create_status.ok())
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.CreateSessionFailed",
                      "Status: %s", session_create_status.ToString().c_str());
    return RESULT_FAIL;
  }
  
  const std::string labels_file_name = Util::FileUtils::FullFilePath({modelPath, _params.labels});
  Result readLabelsResult = ReadLabelsFile(labels_file_name);
  if(RESULT_OK == readLabelsResult)
  {
    PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadGraph.ReadLabelFileSuccess", "%s",
                  labels_file_name.c_str());
  }
  return readLabelsResult;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::Run(const ImageRGB& img, std::list<DetectedObject>& objects)
{
  Result result = RESULT_FAIL;
  
  ImageRGB imgResized;
  Point2i upperLeft(0,0);
  if(_params.input_width == 0 && _params.input_height == 0)
  {
    imgResized = img;
  }
  else
  {
    ResizeImage(img, _params.input_width, _params.input_height, _params.do_crop, imgResized, upperLeft);
  }
  
  //imgResized.Display("Resized", 0);
  
  std::vector<Tensor> image_tensors;
  Status read_tensor_status = CreateTensorFromImage(imgResized, &image_tensors);
  if (!read_tensor_status.ok()) {
    PRINT_NAMED_ERROR("ObjectDetector.Model.Run.CreateTensorFailed", "Status: %s",
                      read_tensor_status.ToString().c_str());
    return RESULT_FAIL;
  }
  
  const Tensor& image_tensor = image_tensors[0];
  std::vector<Tensor> outputs;
  
  if(_isDetectionMode)
  {
    {
      ANKI_CPU_PROFILE("RunModel");
      Status run_status = _session->Run({{_params.input_layer, image_tensor}},
                                        {_params.output_scores_layer,
                                          _params.output_classes_layer,
                                          _params.output_boxes_layer,
                                          _params.output_num_detections_layer}, {}, &outputs);
      if (!run_status.ok()) {
        PRINT_NAMED_ERROR("ObjectDetector.Model.Run.DetectionSessionRunFail",
                          "Status: %s", run_status.ToString().c_str());
        return RESULT_FAIL;
      }
    }
    
    const s32 numDetections = (s32)outputs[3].tensor<float,1>().data()[0];
    if(numDetections > 0)
    {
      const float* scores  = outputs[0].tensor<float, 2>().data();
      const float* classes = outputs[1].tensor<float, 2>().data();
      
      auto const& boxesTensor = outputs[2].tensor<float, 3>();
      
      DEV_ASSERT_MSG(boxesTensor.dimension(0) == 1 &&
                     boxesTensor.dimension(1) == numDetections &&
                     boxesTensor.dimension(2) == 4,
                     "ObjectDetector.Model.Run.UnexpectedOutputBoxesSize",
                     "%dx%dx%d instead of 1x%dx4",
                     (int)boxesTensor.dimension(0), (int)boxesTensor.dimension(1), (int)boxesTensor.dimension(2),
                     numDetections);
      
      const float* boxes = boxesTensor.data();
      
      for(s32 i=0; i<std::min(_params.top_K,numDetections); ++i)
      {
        if(scores[i] >= _params.min_score)
        {
          const float* box = boxes + (4*i);
          const s32 ymin = std::round(box[0] * (float)imgResized.GetNumRows()) + upperLeft.y();
          const s32 xmin = std::round(box[1] * (float)imgResized.GetNumCols()) + upperLeft.x();
          const s32 ymax = std::round(box[2] * (float)imgResized.GetNumRows());
          const s32 xmax = std::round(box[3] * (float)imgResized.GetNumCols());
          
          DEV_ASSERT_MSG(xmax > xmin, "ObjectDetector.Model.Run.InvalidDetectionBoxWidth",
                         "xmin=%d xmax=%d", xmin, xmax);
          DEV_ASSERT_MSG(ymax > ymin, "ObjectDetector.Model.Run.InvalidDetectionBoxHeight",
                         "ymin=%d ymax=%d", ymin, ymax);
          
          DetectedObject object{
            .timestamp = img.GetTimestamp(),
            .score     = scores[i],
            .name      = _labels[(size_t)classes[i]],
            .rect      = Rectangle<s32>(xmin, ymin, xmax-xmin, ymax-ymin),
          };
          
          PRINT_CH_DEBUG(kLogChannelName, "ObjectDetector.Model.Run.ObjectDetected",
                         "Name:%s Score:%.3f Box:[%d %d %d %d] t:%ums",
                         object.name.c_str(), object.score,
                         object.rect.GetX(), object.rect.GetY(), object.rect.GetWidth(), object.rect.GetHeight(),
                         object.timestamp);
          
          objects.push_back(std::move(object));
        }
      }
    }
    result = RESULT_OK;
  }
  else
  {
    // Actually run the image through the model.
    ANKI_CPU_PROFILE("RunModel");
    Status run_status = _session->Run({{_params.input_layer, image_tensor}},
                                      {_params.output_scores_layer}, {}, &outputs);
    
    if (!run_status.ok()) {
      PRINT_NAMED_ERROR("ObjectDetector.Model.Run.ClassificationSessionRunFail",
                        "Status: %s", run_status.ToString().c_str());
      return RESULT_FAIL;
    }
    
    const Tensor& output_scores = outputs[0];
    const float* output_data = output_scores.tensor<float, 2>().data();
    result = GetClassificationOutputs(img, output_data, objects);
  }
  
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
tensorflow::Status ObjectDetector::Model::CreateTensorFromImage(const cv::Mat& img, std::vector<Tensor>* out_tensors)
{


}

  
} // namespace Vision
} // namespace Anki

#endif

#include "json/json.h"

#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <sys/stat.h>

#include <cmath>
#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <thread>

#define QUOTE_HELPER(__ARG__) #__ARG__
#define QUOTE(__ARG__) QUOTE_HELPER(__ARG__)

// TODO: Use coretech shared types.h? Or replicate full enum here?
using Result = int;
const Result RESULT_OK = 0;
const Result RESULT_FAIL = 1;

using TimeStamp_t = uint32_t;

// Define local versions of FulLFilePath and FileExists to avoid needing to include 
// all of Anki::Util
static inline std::string FullFilePath(const std::string& path, const std::string& filename)
{
  return path + "/" + filename;
}

static inline bool FileExists(const std::string& filename)
{
  struct stat st;
  const int statResult = stat(filename.c_str(), &st);
  return (statResult == 0);
}

static void PRINT_NAMED_ERROR(const char* eventName, const char* format, ...)
{
  const size_t kMaxStringBufferSize = 1024;

  // parse string
  va_list args;
  char logString[kMaxStringBufferSize]{0};
  va_start(args, format);
  vsnprintf(logString, kMaxStringBufferSize, format, args);
  va_end(args);

  // TODO: log it
  //LogError(eventName, keyValues, logString);
}

class ObjectDetector
{
public:
  ObjectDetector() : _params{} 
  {

  }

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

  void GetClassification(const tensorflow::Tensor& output_tensor, TimeStamp_t timestamp, 
                         std::list<DetectedObject>& objects);

  void GetDetectedObjects(const std::vector<tensorflow::Tensor>& output_tensors, TimeStamp_t timestamp,
                          std::list<DetectedObject>& objects);

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
    
    int32_t       top_K; // = 1;
    float         min_score; // in [0,1]
    
    bool          verbose;
  } _params;

  std::string                          _input_layer;
  std::vector<std::string>             _output_layers;
  bool                                 _useFloatInput = false;

  std::unique_ptr<tensorflow::Session> _session;
  std::vector<std::string>             _labels;
  
}; // class ObjectDetector

static inline void SetFromConfigHelper(const Json::Value& json, int32_t& value) {
  value = json.asInt();
}

static inline void SetFromConfigHelper(const Json::Value& json, float& value) {
  value = json.asFloat();
}

static inline void SetFromConfigHelper(const Json::Value& json, bool& value) {
  value = json.asBool();
}

static inline void SetFromConfigHelper(const Json::Value& json, std::string& value) {
  value = json.asString();
}

static inline void SetFromConfigHelper(const Json::Value& json, uint8_t& value) {
  value = json.asUInt();
}

Result ObjectDetector::LoadModel(const std::string& modelPath, const Json::Value& config)
{
 
# define GetFromConfig(keyName) \
  if(!config.isMember(QUOTE(keyName))) \
  { \
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.MissingConfig", QUOTE(keyName)); \
    return RESULT_FAIL; \
  } \
  else \
  { \
    SetFromConfigHelper(config[QUOTE(keyName)], _params.keyName); \
  }

  GetFromConfig(verbose);
  GetFromConfig(labels);
  GetFromConfig(top_K);
  GetFromConfig(min_score);
  
  // if(!ANKI_VERIFY(Util::IsFltGEZero(_params.min_score) && Util::IsFltLE(_params.min_score, 1.f),
  //                 "ObjectDetector.Model.LoadModel.Badmin_score",
  //                 "%f not in range [0.0,1.0]", _params.min_score))
  // {
  //   return RESULT_FAIL;
  // }
  
  GetFromConfig(graph);
  GetFromConfig(input_height);
  GetFromConfig(input_width);
  GetFromConfig(architecture);

  if("ssd_mobilenet" == _params.architecture)
  {
    _input_layer = "image_tensor";
    _output_layers = {"detection_scores", "detection_classes", "detection_boxes", "num_detections"};
    _useFloatInput = false;
  }
  else if("mobilenet" == _params.architecture)
  { 
    _input_layer = "input";
    _output_layers = {"MobilenetV1/Predictions/Softmax"};
    _useFloatInput = true;
  }
  else
  {
    PRINT_NAMED_ERROR("ObjectDetector.LoadModel.UnrecognizedArchitecture", "%s", 
                      _params.architecture.c_str());
    return RESULT_FAIL;
  }

  if(_params.verbose)
  {
    std::cout << "Input: " << _input_layer << ", Outputs: ";
    for(auto const& output_layer : _output_layers)
    {
      std::cout << output_layer << " ";
    }
    std::cout << std::endl;
  }

  if ("mobilenet" == _params.architecture)
  {
    // Only required in classification mode
    GetFromConfig(input_mean_R);
    GetFromConfig(input_mean_G);
    GetFromConfig(input_mean_B);
    GetFromConfig(input_std);
  }
  
  const std::string graph_file_name = FullFilePath(modelPath, _params.graph);
  
  if (!FileExists(graph_file_name))
                  
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.GraphFileDoesNotExist", "%s",
                      graph_file_name.c_str());
    return RESULT_FAIL;
  }
  
  tensorflow::GraphDef graph_def;
  tensorflow::Status load_graph_status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), graph_file_name, &graph_def);
  if (!load_graph_status.ok())
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.ReadBinaryProtoFailed",
                      "Status: %s", load_graph_status.ToString().c_str());
    return RESULT_FAIL;
  }
  
  std::cout << "ObjectDetector.Model.LoadGraph.ModelLoadSuccess " << graph_file_name << std::endl;
  
  _session.reset(tensorflow::NewSession(tensorflow::SessionOptions()));
  tensorflow::Status session_create_status = _session->Create(graph_def);
  if (!session_create_status.ok())
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.CreateSessionFailed",
                      "Status: %s", session_create_status.ToString().c_str());
    return RESULT_FAIL;
  }
  
  const std::string labels_file_name = FullFilePath(modelPath, _params.labels);
  Result readLabelsResult = ReadLabelsFile(labels_file_name, _labels);
  if (RESULT_OK == readLabelsResult)
  {
    std::cout << "ObjectDetector.Model.LoadGraph.ReadLabelFileSuccess " << labels_file_name.c_str() << std::endl;
  }
  return readLabelsResult;
}

// returns true on success, false on failure
Result ObjectDetector::ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out)
{
  std::ifstream file(fileName);
  if (!file)
  {
    PRINT_NAMED_ERROR("ObjectDetector.ReadLabelsFile.LabelsFileNotFound: ", "%s", fileName.c_str());
    return RESULT_FAIL;
  }
  
  labels_out.clear();
  std::string line;
  while (std::getline(file, line)) {
    labels_out.push_back(line);
  }
  
  std::cout << "ReadLabelsFile read " << labels_out.size() << " labels" << std::endl;

  return RESULT_OK;
}

void ObjectDetector::GetClassification(const tensorflow::Tensor& output_tensor, TimeStamp_t timestamp, 
                                       std::list<DetectedObject>& objects)
{
  const float* output_data = output_tensor.tensor<float, 2>().data();
        
  float maxScore = _params.min_score;
  int labelIndex = -1;
  for(int i=0; i<_labels.size(); ++i)
  {
    if(output_data[i] > maxScore)
    {
      maxScore = output_data[i];
      labelIndex = i;
    }
  }
  
  if(labelIndex >= 0)
  {    
    DetectedObject object{
      .timestamp = timestamp,
      .score = maxScore,
      .name = (labelIndex < _labels.size() ? _labels.at((size_t)labelIndex) : "<UNKNOWN>"),
      .xmin = 0, .ymin = 0, .xmax = 1, .ymax = 1,
    };
    
    if(_params.verbose)
    {
      std::cout << "Found " << object.name << "[" << labelIndex << "] with score=" << object.score << std::endl;
    }

    objects.push_back(std::move(object));
  }
  else if(_params.verbose)
  {
    std::cout << "Nothing found above min_score=" << _params.min_score << std::endl;
  }
}

void ObjectDetector::GetDetectedObjects(const std::vector<tensorflow::Tensor>& output_tensors, TimeStamp_t timestamp,
                                        std::list<DetectedObject>& objects)
{
  assert(output_tensors.size() == 4);

  const int numDetections = (int)output_tensors[3].tensor<float,1>().data()[0];

  if(_params.verbose)
  {
    std::cout << "Got " << numDetections << " raw detections" << std::endl;
  }

  if(numDetections > 0)
  {
    const float* scores  = output_tensors[0].tensor<float, 2>().data();
    const float* classes = output_tensors[1].tensor<float, 2>().data();
    
    auto const& boxesTensor = output_tensors[2].tensor<float, 3>();
    
    const float* boxes = boxesTensor.data();
    
    for(int i=0; i<numDetections; ++i)
    {
      if(scores[i] >= _params.min_score)
      {
        const float* box = boxes + (4*i);
        
        const size_t labelIndex = (size_t)(classes[i]);

        objects.emplace_back(DetectedObject{
          .timestamp = 0, // TODO: Fill in timestamp!  img.GetTimestamp(),
          .score     = scores[i],
          .name      = (labelIndex < _labels.size() ? _labels[labelIndex] : "<UNKNOWN>"),
          .xmin = box[0], .ymin = box[1], .xmax = box[2], .ymax = box[3],
        });
      }
    }

    if(_params.verbose)
    {
      std::cout << "Returning " << objects.size() << " detections with score above " << _params.min_score << ":";
      for(auto const& object : objects)
      {
        std::cout << object.name << " ";
      }
      std::cout << std::endl;
    }
  } 
}

Result ObjectDetector::Detect(cv::Mat& img, std::list<DetectedObject>& objects)
{
  // TODO: specify size on command line or get from graph?
  cv::resize(img, img, cv::Size(224,224), 0, 0, CV_INTER_AREA);

  tensorflow::Tensor image_tensor;

  if(_useFloatInput)
  {
    if(_params.verbose)
    {
      std::cout << "Copying in FLOAT " << img.rows << "x" << img.cols << " image data" << std::endl;
    }

    image_tensor = tensorflow::Tensor(tensorflow::DT_FLOAT, {
      1, img.rows, img.cols, img.channels()
    });

    img.convertTo(img, CV_32FC3, 1.f/255.f);
    memcpy(image_tensor.tensor<float, 4>().data(), img.data, img.rows*img.cols*img.channels()*sizeof(float));

    // float* tensor_data = image_tensor.tensor<float,4>().data();
    // for(int i=0; i<img.rows; ++i)
    // {
    //   const uint8_t* img_i = img.ptr<uint8_t>(i);
    //   for(int j=0; j<img.cols*img.channels(); ++j)
    //   {
    //     tensor_data[j] = (float)(img_i[j]) / 255.f;
    //     ++tensor_data;
    //   }
    // }
  
  }
  else 
  {
    if(_params.verbose)
    {
      std::cout << "Copying in UINT8 " << img.rows << "x" << img.cols << " image data" << std::endl;
    }

    image_tensor = tensorflow::Tensor(tensorflow::DT_UINT8, {
      1, img.rows, img.cols, img.channels()
    });

    assert(img.isContinuous());
    
    // TODO: Avoid copying the data (just "point" the tensor at the image's data pointer?)
    memcpy(image_tensor.tensor<uint8_t, 4>().data(), img.data, img.rows*img.cols*img.channels());
  }
    
  if(_params.verbose)
  {
    std::cout << "Running session with input dtype=" << image_tensor.dtype() << " and " << _output_layers.size() << " outputs" << std::endl;
  }

  std::vector<tensorflow::Tensor> output_tensors;
  tensorflow::Status run_status = _session->Run({{_input_layer, image_tensor}}, _output_layers, {}, &output_tensors);

  if (!run_status.ok()) {
    PRINT_NAMED_ERROR("ObjectDetector.Model.Run.DetectionSessionRunFail", "%s", run_status.ToString().c_str());
    return RESULT_FAIL;
  }

  // TODO: Get timestamp somehow
  const TimeStamp_t t = 0;

  if(output_tensors.size() == 1)
  {
    GetClassification(output_tensors[0], t, objects);
  }
  else
  {
    GetDetectedObjects(output_tensors, t, objects);  
  }

  if(_params.verbose)
  {
    std::cout << "Session complete" << std::endl;
  }

  return RESULT_OK;
}

int main(int argc, char **argv)
{
  if(argc < 4)
  {
    //std::cout << "Usage: " << argv[0] << " modelPath cachePath min_score detectionMode={mobilenet,ssd_mobilenet} <verbose=0>" << std::endl;
    std::cout << "Usage: " << argv[0] << " configFile.json modelPath cachePath <verbose>" << std::endl;
    return -1;
  }
  
  const std::string configFilename(argv[1]);
  const std::string modelPath(argv[2]);
  const std::string cachePath(argv[3]);

  // Read config file
  Json::Value config;
  {
    Json::Reader reader;
    std::ifstream file(configFilename);
    const bool success = reader.parse(file, config);
    if(!success)
    {
      PRINT_NAMED_ERROR(argv[0], "Could not read config file: %s", configFilename.c_str());
      return -1;
    }
  }

  const std::string imageFilename = FullFilePath(cachePath, "objectDetectionImage.png");
  const int kPollFrequency_ms = 10;

  // Initialize the detector
  ObjectDetector detector;
  Result initResult = detector.LoadModel(modelPath, config);
  if(RESULT_OK != initResult)
  {
    PRINT_NAMED_ERROR(argv[0], "Failed to load model from path: %s", modelPath.c_str());
    return -1;
  }
  std::cout << "Loaded model, waiting for images" << std::endl;

  while(true)
  {
    // Is there an image file available in the cache?
    const bool isImageAvailable = FileExists(imageFilename);

    if(isImageAvailable)
    {
      if(detector.IsVerbose())
      {
        std::cout << "Found image" << std::endl;
      }

      // Get the image
      cv::Mat img = cv::imread(imageFilename);

      // Detect what's in it
      std::list<ObjectDetector::DetectedObject> objects;
      Result result = detector.Detect(img, objects);

      Json::Value detectionResults;
      
      // Convert the results to JSON
      {
        Json::Value& objectsJSON = detectionResults["objects"];
        for(auto const& object : objects)
        {
          Json::Value json;
          json["timestamp"] = object.timestamp;
          json["score"]     = object.score;
          json["name"]      = object.name;
          json["xmin"]      = object.xmin;
          json["ymin"]      = object.ymin;
          json["xmax"]      = object.xmax;
          json["ymax"]      = object.ymax;
          
          objectsJSON.append(json);
        }          
      }

      // Write out the Json
      {
        const std::string jsonFilename = FullFilePath(cachePath, "objectDetectionResults.json");
        if(detector.IsVerbose())
        {
          std::cout << "Writing results to JSON: " << jsonFilename << std::endl;
        }

        Json::StyledStreamWriter writer;
        std::fstream fs;
        fs.open(jsonFilename, std::ios::out);
        if (!fs.is_open()) {
          std::cerr << "Failed to open output file: " << jsonFilename << std::endl;
          return -1;
        }
        writer.write(fs, detectionResults);
        fs.close();
      }

      // Remove the image file we were working with
      if(detector.IsVerbose())
      {
        std::cout << "Deleting image file: " << imageFilename << std::endl;
      }
      remove(imageFilename.c_str());
    }
    else 
    {
      if(detector.IsVerbose())
      {
        const int kVerbosePrintFreq_ms = 1000;
        static int count = 0;
        if(count++ * kPollFrequency_ms >= kVerbosePrintFreq_ms)
        {
          std::cout << "Waiting for image..." << std::endl;
          count = 0;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(kPollFrequency_ms));  
    }
  }

  return 0;
}
