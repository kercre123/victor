#include "custom_layer_gpu.h"

#include <tensorflow/contrib/lite/kernels/register.h>
#include <tensorflow/contrib/lite/kernels/kernel_util.h>
#include <tensorflow/contrib/lite/model.h>
#include <tensorflow/contrib/lite/string_util.h>

#include "accelerator.h"

#include <iostream>

TfLiteStatus Hello_GPU_Prepare(TfLiteContext* context, TfLiteNode* node) {
  // std::cerr<<"Hello_GPU_Prepare"<<std::endl;
  TF_LITE_ENSURE_EQ(context, tflite::NumInputs(node), 2);
  TF_LITE_ENSURE_EQ(context, tflite::NumOutputs(node), 1);

  const TfLiteTensor* input = tflite::GetInput(context, node, 0);
  TfLiteTensor* output = tflite::GetOutput(context, node, 0);

  int num_dims = tflite::NumDimensions(input);

  TfLiteIntArray* output_size = TfLiteIntArrayCreate(num_dims);
  for (int i=0; i<num_dims; ++i) {
    output_size->data[i] = input->dims->data[i];
  }

  return context->ResizeTensor(context, output, output_size);
}

TfLiteStatus Hello_GPU_Eval(TfLiteContext* context, TfLiteNode* node) {
  // std::cerr<<"Hello_GPU_Eval"<<std::endl;
  const TfLiteTensor* input_arg1 = tflite::GetInput(context, node, 0);
  const TfLiteTensor* input_arg2 = tflite::GetInput(context, node, 1);
  TfLiteTensor* output = tflite::GetOutput(context, node,0);

  int num_dims = tflite::NumDimensions(input_arg1);
  std::vector<size_t> dims(num_dims);
  for (int i = 0; i < num_dims; ++i){
    dims[i] = input_arg1->dims->data[i];
  }

  Accelerator::instance().hello(input_arg1->data.f, input_arg2->data.f, output->data.f, dims);
  return kTfLiteOk;
}

TfLiteRegistration* Register_HELLO_GPU() {
  // std::cerr<<"Register_HELLO_GPU"<<std::endl;
  static TfLiteRegistration r = {nullptr, nullptr, Hello_GPU_Prepare, Hello_GPU_Eval};
  return &r;
}