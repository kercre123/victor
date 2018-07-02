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


#if VIC_NEURALNETS_USE_TFLITE

#if defined(VIC_NEURALNETS_USE_TENSORFLOW)
#error VIC_NEURALNETS_USE_TFLITE should not be defined when using TensorFlow Lite
#endif

#include "anki/vision/basestation/objectDetector.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/profiler.h"

#include "anki/common/basestation/array2d_impl.h"
#include "anki/common/basestation/jsonTools.h"

#include "util/console/consoleInterface.h"
#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

#include <fstream>
#include <list>
#include <queue>

#include "tensorflow/contrib/lite/kernels/register.h"
#include "tensorflow/contrib/lite/model.h"
#include "tensorflow/contrib/lite/string_util.h"
#include "tensorflow/contrib/lite/tools/mutable_op_resolver.h"

namespace Anki {
namespace Vision {

static const char * const kLogChannelName = "VisionSystem";

namespace {  
#define CONSOLE_GROUP "Vision.ObjectDetector"

CONSOLE_VAR(s32, kObjectDetector_MobileNetSize,       CONSOLE_GROUP, 128);
CONSOLE_VAR(f32, kObjectDetector_MobileNetComplexity, CONSOLE_GROUP, 0.5f);

#undef CONSOLE_GROUP
}

class ObjectDetector::Model
{
public:
  
  Model(Profiler& profiler);

  // ObjectDetector expects LoadModel and Run to exist
  Result LoadModel(const std::string& modelPath, const Json::Value& config);
  Result Run(const ImageRGB& img, std::list<DetectedObject>& objects);
  
private:
  
  Result ReloadModelFromParams();
  void ScaleImage(const Vision::ImageRGB& img);

  // Member variables:
  struct {
    
    std::string graph; // = "tensorflow/examples/label_image/data/inception_v3_2016_08_28_frozen.pb";
    std::string labels; // = "tensorflow/examples/label_image/data/imagenet_slim_labels.txt";
    std::string mode;
    s32    input_width; // = 299;
    s32    input_height; // = 299;
    f32    input_mean_R; // = 0;
    f32    input_mean_G; // = 0;
    f32    input_mean_B; // = 0;
    f32    input_std; // = 255;
    
    s32    top_K; // = 1;
    f32    min_score; // in [0,1]
    
  } _params;
  
  std::string _modelPath;
  s32         _mobileNetSize;
  f32         _mobileNetComplexity;

  static const int num_threads; 
  static const std::string input_layer_type;

  std::unique_ptr<tflite::FlatBufferModel> _model;
  std::unique_ptr<tflite::Interpreter>     _interpreter;
  std::vector<std::string>  _labels;
  bool                      _isDetectionMode;
  
  Profiler& _profiler;

}; // class ObjectDetector::Model

// TODO: Parameter in config?
const int ObjectDetector::Model::num_threads = 1; 
const std::string ObjectDetector::Model::input_layer_type = "float";

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
ObjectDetector::Model::Model(Profiler& profiler) 
: _mobileNetSize(kObjectDetector_MobileNetSize)
, _mobileNetComplexity(kObjectDetector_MobileNetComplexity)
, _profiler(profiler) 
{ 

  PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.TFLite", "Using NEON:%s",
                tflite::FlatBufferModel::IsUsingNEON() ? "Y" : "N");

} 
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Returns the top N confidence values over threshold in the provided vector,
// sorted by confidence in descending order.
static void GetTopN(const float* prediction, const size_t prediction_size, const int num_results,
                    const float threshold, std::vector<std::pair<float, int> >* top_results)
{
  // Will contain top N results in ascending order.
  std::priority_queue<std::pair<float, int>, std::vector<std::pair<float, int> >,
  std::greater<std::pair<float, int> > >
  top_result_pq;
  
  const long count = prediction_size;
  for (int i = 0; i < count; ++i) {
    const float value = prediction[i];
    
    // Only add it if it beats the threshold and has a chance at being in
    // the top N.
    if (value < threshold) {
      continue;
    }
    
    top_result_pq.push(std::pair<float, int>(value, i));
    
    // If at capacity, kick the smallest value out.
    if (top_result_pq.size() > num_results) {
      top_result_pq.pop();
    }
  }
  
  // Copy to output vector and reverse into descending order.
  while (!top_result_pq.empty()) {
    top_results->push_back(top_result_pq.top());
    top_result_pq.pop();
  }
  std::reverse(top_results->begin(), top_results->end());
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::LoadModel(const std::string& modelPath, const Json::Value& config)
{
  ANKI_CPU_PROFILE("ObjectDetector.LoadModel");
  
  _modelPath = modelPath;

# define GetFromConfig(keyName) \
  if(false == JsonTools::GetValueOptional(config, QUOTE(keyName), _params.keyName)) \
  { \
    PRINT_NAMED_ERROR("ObjectDetector.Init.MissingConfig", QUOTE(keyName)); \
    return RESULT_FAIL; \
  }
  
  GetFromConfig(graph);
  GetFromConfig(input_height);
  GetFromConfig(input_width);
  GetFromConfig(input_mean_R);
  GetFromConfig(input_mean_G);
  GetFromConfig(input_mean_B);
  GetFromConfig(input_std);
  GetFromConfig(labels);
  GetFromConfig(top_K);
  
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
  
  Result loadModelResult = ReloadModelFromParams();

  if(RESULT_OK != loadModelResult)
  {
    return loadModelResult;
  }

  // Read the label list
  {
    const std::string labelsFileName = Util::FileUtils::FullFilePath({modelPath, _params.labels});
    std::ifstream file(labelsFileName);
    if (!file)
    {
      PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.LabelsFileNotFound",
                        "%s", labelsFileName.c_str());
      return RESULT_FAIL;
    }
    
    _labels.clear();
    std::string line;
    while (std::getline(file, line)) {
      _labels.push_back(line);
    }
    file.close();
  }

  return RESULT_OK;
}

Result ObjectDetector::Model::ReloadModelFromParams()
{
  DEV_ASSERT(!_modelPath.empty(), "ObjectDetector.Model.ReloadModelFromParams.EmptyModelPath");

  std::vector<int> sizes = {1, _params.input_height, _params.input_width, 3};
  
  const std::string graphFileName = Util::FileUtils::FullFilePath({_modelPath,_params.graph});
  
  _model = tflite::FlatBufferModel::BuildFromFile(graphFileName.c_str());
  
  if (!_model)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.FailedToMMapModel", "%s",
                      graphFileName.c_str());
    return RESULT_FAIL;
  }
  
  PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadModel.Success", "Loaded: %s",
                graphFileName.c_str());
  
  _model->error_reporter();
  
  PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadModel.ResolvedReporter", "");
  
#ifdef TFLITE_CUSTOM_OPS_HEADER
  tflite::MutableOpResolver resolver;
  RegisterSelectedOps(&resolver);
#else
  tflite::ops::builtin::BuiltinOpResolver resolver;
#endif
  
  tflite::InterpreterBuilder(*_model, resolver)(&_interpreter);
  if (!_interpreter)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.FailedToConstructInterpreter", "");
    return RESULT_FAIL;
  }
  
  if (num_threads != -1)
  {
    _interpreter->SetNumThreads(num_threads);
  }
  
  const int input = _interpreter->inputs()[0];
  
  if (input_layer_type != "string")
  {
    _interpreter->ResizeInputTensor(input, sizes);
  }
  
  if (_interpreter->AllocateTensors() != kTfLiteOk)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.FailedToAllocateTensors", "");
    return RESULT_FAIL;
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectDetector::Model::ScaleImage(const Vision::ImageRGB& img)
{
  DEV_ASSERT(_interpreter != nullptr, "ObjectDetector.Model.ScaleImage.NullInterpreter");

  auto ticToc = _profiler.TicToc("ScaleImage");

  const int wanted_width    = _params.input_width;
  const int wanted_height   = _params.input_height;  
  const f32 invStd = 1.f / _params.input_std;
 
  const int image_width    = img.GetNumCols();
  const int image_height   = img.GetNumRows();
  const int input = _interpreter->inputs()[0];
 
  float* out = _interpreter->typed_tensor<float>(input);

  // Wrap a PixelRGB<f32> header around the output so we can use it as if it is float RGB pixels
  Array2d<PixelRGB_<f32>> outImage(wanted_height, wanted_width, 
                                   reinterpret_cast<PixelRGB_<f32>*>(out));

  for (int y = 0; y < wanted_height; ++y)
  {
    const int scaled_y = (y * image_height) / wanted_height;
    const PixelRGB* img_row = img.GetRow(scaled_y);
    PixelRGB_<f32>* out_row = outImage.GetRow(scaled_y);
    
    for (int x = 0; x < wanted_width; ++x)
    {
      const int scaled_x = (x * image_width) / wanted_width;
      const PixelRGB& in_pixel = img_row[scaled_x];
      
      PixelRGB_<f32>& out_pixel = out_row[scaled_x];
      out_pixel[0] = ((float)in_pixel.r() - _params.input_mean_R) * invStd;
      out_pixel[1] = ((float)in_pixel.g() - _params.input_mean_G) * invStd;
      out_pixel[2] = ((float)in_pixel.b() - _params.input_mean_B) * invStd;
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::Run(const ImageRGB& img, std::list<DetectedObject>& objects)
{
  if(_mobileNetComplexity != kObjectDetector_MobileNetComplexity ||
     _mobileNetSize != kObjectDetector_MobileNetSize)
  {
    auto ticToc = _profiler.TicToc("ReloadModel");

    _mobileNetComplexity  = kObjectDetector_MobileNetComplexity;
    _mobileNetSize = kObjectDetector_MobileNetSize;

    std::string complexityStr("1.0");
    if(_mobileNetComplexity == 0.75f) {
      complexityStr = "0.75";
    } else if(_mobileNetComplexity == 0.5f) {
      complexityStr = "0.50";
    } else if(_mobileNetComplexity == 0.25f) {
      complexityStr = "0.25";
    }

    _params.graph = "mobilenet_" + complexityStr + "_" + std::to_string(_mobileNetSize) + "_flower_photos_float.lite";
    _params.input_height = _mobileNetSize;
    _params.input_width  = _mobileNetSize;    

    Result reloadResult = ReloadModelFromParams();
    if(RESULT_OK != reloadResult)
    {
      PRINT_NAMED_ERROR("ObjectDetector.Model.Run.ReloadFailed", 
                        "Failed to reload with size:%d complexity:%.2f", 
                        _mobileNetSize, _mobileNetComplexity);
      return RESULT_FAIL;
    }
  }

  // Scale image, subtract mean, divide by standard deviation and store in the interpreter's input tensor
  ScaleImage(img);
  
  _profiler.Tic("ForwardInference");
  const auto invokeResult = _interpreter->Invoke();
  _profiler.Toc("ForwardInference");
  if (kTfLiteOk != invokeResult)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.Run.FailedToInvoke", "");
    return RESULT_FAIL;
  }
  
  float* output = _interpreter->typed_output_tensor<float>(0);
  const size_t output_size = _labels.size();
  
  if(_isDetectionMode)
  {
    DEV_ASSERT(false, "ObjectDetector.Model.Run.DetectionModeNotSupported");
    // TODO: Create a DetectedObject and put it in the returned objects list
    return RESULT_FAIL;
  }
  else // Whole-image classification
  {
    std::vector<std::pair<float, int> > top_results;
    GetTopN(output, output_size, _params.top_K, _params.min_score, &top_results);
    
    std::stringstream ss;
    ss.precision(3);
    for (const auto& result : top_results)
    {
      if(result.second < _labels.size())
      {
        objects.emplace_back(DetectedObject{
          .timestamp = img.GetTimestamp(),
          .score     = result.first,
          .name      = _labels.at((size_t)result.second),
          .rect      = Rectangle<s32>(0,0,img.GetNumCols(),img.GetNumRows())
        });
      }
      else
      {
        PRINT_NAMED_WARNING("ObjectDetector.Model.Run.BadResultIndex", "%d >= %zu", result.second, _labels.size());
      }
    }
    
  }
  
  return RESULT_OK;
}
  
} // namespace Vision
} // namespace Anki

#endif // #if USE_TENSORFLOW_LITE
