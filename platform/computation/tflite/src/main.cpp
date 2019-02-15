#include <tensorflow/contrib/lite/kernels/register.h>
#include <tensorflow/contrib/lite/model.h>
#include <tensorflow/contrib/lite/profiling/profiler.h>

#include <fstream>
#include <iostream>
#include <string>
#include <cstdio>
#include <iomanip>

#include "accelerator.h"
#include "custom_layer_gpu.h"

namespace {

// A TFLite error reporter that uses our error logging system
struct TFLiteLogReporter : public tflite::ErrorReporter {
  int Report(const char* format, va_list args) override
  {
    size_t size = std::snprintf(nullptr, 0,  format, args) + 1;
    std::unique_ptr<char[]> buf(new char[size]);
    snprintf(buf.get(), size, format, args);
    std::string message(buf.get());
    std::cerr<<message<<std::endl;
    return 0;
  }
};

TFLiteLogReporter gLogReporter;

void usage()
{
  std::cerr<<"application <model>"<<std::endl;
}

} /* anonymous namespace */

int main(int argc, char** argv)
{

  if (argc != 2){
    usage();
    return 0;
  }

  Accelerator::instance(); // just initialize nthe GPU stuff

  tflite::ops::builtin::BuiltinOpResolver resolver;
  resolver.AddCustom("Hello", Register_HELLO_GPU());

  std::string path(argv[1]);

  std::unique_ptr<tflite::FlatBufferModel> model;
  std::unique_ptr<tflite::Interpreter>   interpreter;

  model = tflite::FlatBufferModel::BuildFromFile(path.c_str(), &gLogReporter);
  tflite::InterpreterBuilder(*model, resolver)(&interpreter);
  if (!interpreter)
  {
    std::cerr<<"Failed to build interpreter"<<std::endl;
    return -1;
  }
  interpreter->SetNumThreads(1);

  if (interpreter->AllocateTensors() != kTfLiteOk)
  {
    std::cerr<<"FailedToAllocateTensors"<<std::endl;
    return -1;
  }

  int batches = 1;
  int height = 3;
  int width = 3;
  int channels = 1;
  size_t size = batches*height*width*channels;

  // Input A
  {
    const int inputIndex = interpreter->inputs()[0];
    float* inputData = interpreter->typed_tensor<float>(inputIndex);
    for (size_t index = 0; index < size; ++index){
      inputData[index] = index;
    }
  }

  // Input B
  {
    const int inputIndex = interpreter->inputs()[1];
    float* inputData = interpreter->typed_tensor<float>(inputIndex);
    for (size_t index = 0; index < size; ++index){
      inputData[index] = index+1;
    }
  }

  // Invoke the graph
  const auto invokeResult = interpreter->Invoke();
  if (kTfLiteOk != invokeResult) {
    std::cerr<<"TFLiteModel.Detect.FailedToInvoke"<<std::endl;
    return -1;
  }

  for (int i = 0; i < 100; i++){
    const auto invokeResult = interpreter->Invoke();
    if (kTfLiteOk != invokeResult) {
      std::cerr<<"TFLiteModel.Detect.FailedToInvoke"<<std::endl;
      return -1;
    }
  }

  // Read the output
  const float* outputData = interpreter->typed_output_tensor<float>(0);
  for (size_t index = 0; index < size; ++index){
    std::cerr<<"Output["<<index<<"]: "<<outputData[index]<<std::endl;
  }

  Accelerator::instance().profile();

  return 0;
}