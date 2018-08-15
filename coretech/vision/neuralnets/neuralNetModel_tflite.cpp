/**
 * File: neuralNetModel_tflite.cpp
 *
 * Author: Andrew Stein
 * Date:   12/5/2017
 *
 * Description: Implementation of ObjectDetector Model class which wraps TensorFlow Lite.
 *
 * Copyright: Anki, Inc. 2017
 **/

// NOTE: this wrapper completely compiles out if we're using a different model (e.g. TensorFlow)
#ifdef VIC_NEURALNETS_USE_TFLITE

#include "coretech/vision/neuralnets/neuralNetModel_tflite.h"
#include <list>
#include <queue>

#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include "tensorflow/contrib/lite/kernels/register.h"
#include "tensorflow/contrib/lite/model.h"
#include "tensorflow/contrib/lite/string_util.h"

namespace Anki {
namespace Vision {

#define LOG_CHANNEL "NeuralNets"
  
// TODO: Make this a parameter in config?
constexpr int kNumThreads = 1;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
namespace {

// A TFLite error reporter that uses our error logging system
struct TFLiteLogReporter : public tflite::ErrorReporter {
  int Report(const char* format, va_list args) override
  {
    ::Anki::Util::sErrorV("NeuralNetModel.TFLiteErrorReporter", {}, format, args);
    return 0;
  }
};

TFLiteLogReporter gLogReporter;

} // anonymous namespace
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
NeuralNetModel::NeuralNetModel(const std::string& cachePath)
: INeuralNetModel(cachePath)
{ 

}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NeuralNetModel::~NeuralNetModel()
{
  LOG_INFO("NeuralNetModel.Destructor", "");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetModel::LoadModel(const std::string& modelPath, const Json::Value& config)
{
  const Result paramsResult = _params.SetFromConfig(config);
  if(RESULT_OK != paramsResult)
  {
    return paramsResult;
  }
   
  DEV_ASSERT(!modelPath.empty(), "NeuralNetModel.LoadModel.EmptyModelPath");
  
  std::vector<int> sizes = {1, _params.inputHeight, _params.inputWidth, 3};
  
  const std::string graphFileName = Util::FileUtils::FullFilePath({modelPath,_params.graphFile});
  
  if(!Util::FileUtils::FileExists(graphFileName))
  {
    PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.GraphFileDoesNotExist", "%s", graphFileName.c_str());
    return RESULT_FAIL;
  }
  
  _model = tflite::FlatBufferModel::BuildFromFile(graphFileName.c_str(), &gLogReporter);
  
  if (!_model)
  {
    PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.FailedToBuildFromFile", "%s", graphFileName.c_str());
    return RESULT_FAIL;
  }
  
  LOG_INFO("NeuralNetModel.LoadModel.Success", "Loaded: %s",
           graphFileName.c_str());
  
  //_model->error_reporter();
  //LOG_INFO("NeuralNetModel.LoadModel.ResolvedReporter", "");
  
#ifdef TFLITE_CUSTOM_OPS_HEADER
  tflite::MutableOpResolver resolver;
  RegisterSelectedOps(&resolver);
#else
  tflite::ops::builtin::BuiltinOpResolver resolver;
#endif
  
  tflite::InterpreterBuilder(*_model, resolver)(&_interpreter);
  if (!_interpreter)
  {
    PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.FailedToConstructInterpreter", "");
    return RESULT_FAIL;
  }
  
  if (kNumThreads != -1)
  {
    _interpreter->SetNumThreads(kNumThreads);
  }
  
  const int input = _interpreter->inputs()[0];
  _interpreter->ResizeInputTensor(input, sizes);
  
  if (_interpreter->AllocateTensors() != kTfLiteOk)
  {
    PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.FailedToAllocateTensors", "");
    return RESULT_FAIL;
  }

  // Read the label list
  const std::string labelsFileName = Util::FileUtils::FullFilePath({modelPath, _params.labelsFile});
  const Result readLabelsResult = ReadLabelsFile(labelsFileName, _labels);
  if(RESULT_OK != readLabelsResult)
  {
    return readLabelsResult;
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NeuralNetModel::ScaleImage(cv::Mat& img)
{
  DEV_ASSERT(_interpreter != nullptr, "NeuralNetModel.ScaleImage.NullInterpreter");

  const int inputIndex = _interpreter->inputs()[0];
  
  const auto kResizeMethod = CV_INTER_LINEAR;
  
  if(_params.useFloatInput)
  {
    float* scaledInputData = _interpreter->typed_tensor<float>(inputIndex);
    
    // Resize uint8 image data, and *then* convert smaller image to float below
    // TODO: Resize and convert directly into the scaledInputData (ideally using NEON?)
    if(img.rows != _params.inputHeight || img.cols != _params.inputWidth)
    {
      cv::resize(img, img, cv::Size(_params.inputWidth,_params.inputHeight), 0, 0, kResizeMethod);
    }
    else if(_params.verbose)
    {
      LOG_INFO("NeuralNetModel.Detect.SkipResize", "Skipping actual resize: image already correct size");
    }
    DEV_ASSERT(img.isContinuous(), "NeuralNetModel.Detect.ImageNotContinuous");
    
    // Scale/shift resized image directly into the tensor data
    const auto cvType = (img.channels() == 1 ? CV_32FC1 : CV_32FC3);
    
    cv::Mat cvTensor(_params.inputHeight, _params.inputWidth, cvType, scaledInputData);
    
    img.convertTo(cvTensor, cvType, 1.f/_params.inputScale, _params.inputShift);
  }
  else
  {
    uint8_t* scaledInputData = _interpreter->typed_tensor<uint8_t>(inputIndex);
    
    // Resize uint8 input image directly into the uint8 tensor data
    cv::Mat cvTensor(_params.inputHeight, _params.inputWidth,
                     (img.channels() == 1 ? CV_8UC1 : CV_8UC3),
                     scaledInputData);
    
    cv::resize(img, cvTensor, cv::Size(_params.inputWidth, _params.inputHeight), 0, 0, kResizeMethod);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetModel::Detect(cv::Mat& img, const TimeStamp_t t, std::list<Vision::SalientPoint>& salientPoints)
{
  // Scale image, subtract mean, divide by standard deviation and store in the interpreter's input tensor
  ScaleImage(img);

  if (_params.benchmarkRuns == 0){
    const auto invokeResult = _interpreter->Invoke();
    if (kTfLiteOk != invokeResult)
    {
      PRINT_NAMED_ERROR("NeuralNetModel.Run.FailedToInvoke", "");
      return RESULT_FAIL;
    }
  }
  else
  {
    tflite::profiling::Profiler profiler;
    
    _interpreter->SetProfiler(&profiler);

    profiler.StartProfiling();
    {
      tflite::profiling::ScopedProfile profile(&profiler, "benchmarkRuns");
      for (uint32_t i = 0; i < _params.benchmarkRuns; ++i)
      {
        const auto invokeResult = _interpreter->Invoke();
        if (kTfLiteOk != invokeResult) {
          PRINT_NAMED_ERROR("NeuralNetModel.Run.FailedToInvokeBenchmark", "");
          return RESULT_FAIL;
        }
      }
    }
    profiler.StopProfiling();
    // TODO: Upgrade to TF r1.10 in order to build profile_summarizer.cc for detailed timing
    auto profile_events = profiler.GetProfileEvents();
    for (auto const& e : profile_events) 
    {
      auto op_index = e->event_metadata;
      const auto node_and_registration =
          _interpreter->node_and_registration(op_index);
      const TfLiteRegistration registration = node_and_registration->second;
      LOG_INFO("Profiling", "Num Runs: %d, Avg: %f ms, Node: %u, OpCode: %i, %s \n",
              _params.benchmarkRuns, 
              (e->end_timestamp_us - e->begin_timestamp_us) / (1000.0 * _params.benchmarkRuns),
              op_index, registration.builtin_code,
              EnumNameBuiltinOperator(
                static_cast<tflite::BuiltinOperator>(registration.builtin_code)));
    }
  }
  
  switch(_params.outputType)
  {
    case NeuralNetParams::OutputType::Classification:
    {
      if(_params.useFloatInput)
      {
        const float* outputData = _interpreter->typed_output_tensor<float>(0);
        ClassificationOutputHelper(outputData, t, salientPoints);
      }
      else
      {
        const uint8_t* outputData = _interpreter->typed_output_tensor<uint8_t>(0);
        ClassificationOutputHelper(outputData, t, salientPoints);
      }
      
      break;
    }
      
    case NeuralNetParams::OutputType::BinaryLocalization:
    {
      if(_params.useFloatInput)
      {
        const float* outputData = _interpreter->typed_output_tensor<float>(0);
        LocalizedBinaryOutputHelper(outputData, t,  1.f, 0, salientPoints);
      }
      else
      {
        const uint8_t* outputData = _interpreter->typed_output_tensor<uint8_t>(0);
        const std::vector<int>& outputs_ = _interpreter->outputs();
        TfLiteTensor* outputTensor = _interpreter->tensor(outputs_[0]);
        const float scale = outputTensor->params.scale;
        const int zero_point = outputTensor->params.zero_point;
        LocalizedBinaryOutputHelper(outputData, t, scale, zero_point, salientPoints);
      }
      
      break;
    }
      
    case NeuralNetParams::OutputType::AnchorBoxes:
      //GetDetectedObjects(outputTensors, t, salientPoints);
    case NeuralNetParams::OutputType::Segmentation:
    {
      PRINT_NAMED_ERROR("NeuralNetModel.Detect.OutputTypeNotSupported", "TFLite model needs more output types implemented");
      return RESULT_FAIL;
    }
  }
  
  return RESULT_OK;
}
  
} // namespace Vision
} // namespace Anki

#endif // #if VIC_NEURALNETS_USE_TFLITE
