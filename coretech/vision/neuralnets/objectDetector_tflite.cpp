#include "objectDetector_tflite.h"
#include "helpers.h"

#include "tensorflow/contrib/lite/kernels/register.h"
#include "tensorflow/contrib/lite/model.h"
#include "tensorflow/contrib/lite/string_util.h"
#include "tensorflow/contrib/lite/tools/mutable_op_resolver.h"
#include "tensorflow/contrib/lite/interpreter.h"
#include "tensorflow/contrib/lite/builtin_op_data.h"

#include <cmath>
#include <fstream>
#include <queue>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::ObjectDetector() 
: _params{} 
{
  _params.num_threads = 1;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::~ObjectDetector()
{
  std::cout << "Destructing ObjectDetector" << std::endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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
  GetFromConfig(min_score);  
  GetFromConfig(graph);
  GetFromConfig(input_height);
  GetFromConfig(input_width);
  GetFromConfig(use_grayscale);

  // NOTE: Only used when processing in floating point
  GetFromConfig(input_shift);
  GetFromConfig(input_scale);
  
  const std::string graph_file_name = FullFilePath(modelPath, _params.graph);
  
  if (!FileExists(graph_file_name))
                  
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.GraphFileDoesNotExist", "%s",
                      graph_file_name.c_str());
    return RESULT_FAIL;
  }

  if(_params.verbose)
  {
    std::cout << "Found graph file: " << graph_file_name << std::endl;
  }

  if(modelPath.empty())
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.ReloadModelFromParams.EmptyModelPath", "");
    return RESULT_FAIL;
  }

  const std::string graphFileName = FullFilePath(modelPath,_params.graph);
  
  _model = tflite::FlatBufferModel::BuildFromFile(graphFileName.c_str());
  
  if (!_model)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.FailedToMMapModel", "%s",
                      graphFileName.c_str());
    return RESULT_FAIL;
  }
  
  std::cout << "ObjectDetector.Model.LoadModel.Success: Loaded " << graphFileName << std::endl;
  
  _model->error_reporter();
  
  std::cout << "ObjectDetector.Model.LoadModel.ResolvedReporter" << std::endl;
  
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
  
  if(config.isMember("num_threads"))
  {
    SetFromConfigHelper(config["num_threads"], _params.num_threads);
  }

  if (_params.num_threads != -1)
  {
    _interpreter->SetNumThreads(_params.num_threads);
  }
  
  const int input = _interpreter->inputs()[0];
  std::vector<int> sizes = {1, _params.input_height, _params.input_width, (_params.use_grayscale ? 1 : 3)};
  _interpreter->ResizeInputTensor(input, sizes);
  
  if (_interpreter->AllocateTensors() != kTfLiteOk)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.FailedToAllocateTensors", "");
    return RESULT_FAIL;
  }

  if (_params.verbose) 
  {
    std::cout << "tensors size: " << _interpreter->tensors_size() << "\n";
    std::cout << "nodes size: " << _interpreter->nodes_size() << "\n";
    std::cout << "inputs: " << _interpreter->inputs().size() << "\n";
    std::cout << "input(0) name: " << _interpreter->GetInputName(0) << "\n";

    int t_size = _interpreter->tensors_size();
    for (int i = 0; i < t_size; i++) {
      if (_interpreter->tensor(i)->name)
        std::cout << i << ": " << _interpreter->tensor(i)->name << ", "
                  << _interpreter->tensor(i)->bytes << ", "
                  << _interpreter->tensor(i)->type << ", "
                  << _interpreter->tensor(i)->params.scale << ", "
                  << _interpreter->tensor(i)->params.zero_point << "\n";
    }

    TfLiteIntArray* dims = _interpreter->tensor(input)->dims;
    int wanted_height = dims->data[1];
    int wanted_width = dims->data[2];
    int wanted_channels = dims->data[3];

    const char* typeStr = "UNKNOWN";
    switch (_interpreter->tensor(input)->type) 
    {
      case kTfLiteFloat32:
        typeStr = "FLOAT";
        break;
      case kTfLiteUInt8:
        typeStr = "UINT8";
        break;
      default:
        break; // Leave "UNKNOWN"
    }

    std::cout << "expected input: " << wanted_width << "x" << wanted_height << "x" << wanted_channels << 
      " (type: " << typeStr << ")" << std::endl;
  }

  const std::string labels_file_name = FullFilePath(modelPath, _params.labels);
  Result readLabelsResult = ReadLabelsFile(labels_file_name, _labels);
  if (RESULT_OK == readLabelsResult)
  {
    std::cout << "ObjectDetector.Model.LoadGraph.ReadLabelFileSuccess " << labels_file_name.c_str() << std::endl;
  }
  return readLabelsResult;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

// Inline helper for templated GetClassification below, to handle uint8/float scores
template<typename T> static inline float GetScoreScale(); 
template<> inline float GetScoreScale<uint8_t>() { return 255.f; }
template<> inline float GetScoreScale<float>()   { return 1.0f; }

template<typename T>
void ObjectDetector::GetClassification(TimeStamp_t timestamp, std::list<DetectedObject>& objects)
{
  objects.clear();
  
  const T* output = _interpreter->typed_output_tensor<T>(0);
  
  if(nullptr == output)
  {
    PRINT_NAMED_ERROR("ObjectDetector.GetClassification.NullOutput", "");
    return;
  }
  
  const float scoreScale = GetScoreScale<T>();

  T maxScore = (T)(scoreScale * _params.min_score);
  int labelIndex = -1;
  std::cout << "Scores: ";
  for(int i=0; i<_labels.size(); ++i)
  {
    std::cout << (float)(output[i]) << " ";
    if(output[i] > maxScore)
    {
      maxScore = output[i];
      labelIndex = i;
    }
  }
  std::cout << std::endl;

  if(labelIndex >= 0)
  {    
    DetectedObject object{
      .timestamp = timestamp,
      .score = (float)maxScore / scoreScale,
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectDetector::GetDetectedObjects(TimeStamp_t timestamp, std::list<DetectedObject>& objects)
{
  objects.clear();
  
  // TODO: Resurrect this for TF Lite
  /*
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
  */
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template <class T>
void resize(T* out, uint8_t* in, int image_height, int image_width,
            int image_channels, int wanted_height, int wanted_width,
            int wanted_channels, bool input_floating, float input_mean, float input_std) 
{
  int number_of_pixels = image_height * image_width * image_channels;
  std::unique_ptr<tflite::Interpreter> interpreter(new tflite::Interpreter);

  int base_index = 0;

  // two inputs: input and new_sizes
  interpreter->AddTensors(2, &base_index);
  // one output
  interpreter->AddTensors(1, &base_index);
  // set input and output tensors
  interpreter->SetInputs({0, 1});
  interpreter->SetOutputs({2});

  // set parameters of tensors
  TfLiteQuantizationParams quant;
  interpreter->SetTensorParametersReadWrite(
      0, kTfLiteFloat32, "input",
      {1, image_height, image_width, image_channels}, quant);
  interpreter->SetTensorParametersReadWrite(1, kTfLiteInt32, "new_size", {2},
                                            quant);
  interpreter->SetTensorParametersReadWrite(
      2, kTfLiteFloat32, "output",
      {1, wanted_height, wanted_width, wanted_channels}, quant);

  tflite::ops::builtin::BuiltinOpResolver resolver;
  TfLiteRegistration* resize_op =
      resolver.FindOp(tflite::BuiltinOperator_RESIZE_BILINEAR);
  auto* params = reinterpret_cast<TfLiteResizeBilinearParams*>(
      malloc(sizeof(TfLiteResizeBilinearParams)));
  //params->align_corners = false;
  interpreter->AddNodeWithParameters({0, 1}, {2}, nullptr, 0, params, resize_op,
                                     nullptr);

  interpreter->AllocateTensors();

  // fill input image
  // in[] are integers, cannot do memcpy() directly
  auto input = interpreter->typed_tensor<float>(0);
  for (int i = 0; i < number_of_pixels; i++) {
    input[i] = in[i];
  }

  // fill new_sizes
  interpreter->typed_tensor<int>(1)[0] = wanted_height;
  interpreter->typed_tensor<int>(1)[1] = wanted_width;

  interpreter->Invoke();

  auto output = interpreter->typed_tensor<float>(2);
  auto output_number_of_pixels =
      wanted_height * wanted_height * wanted_channels;

  for (int i = 0; i < output_number_of_pixels; i++) {
    if (input_floating)
      out[i] = (output[i] - input_mean) / input_std;
    else
      out[i] = (uint8_t)output[i];
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Detect(cv::Mat& img, std::list<DetectedObject>& objects)
{
  if(_params.use_grayscale)
  {
    cv::cvtColor(img, img, CV_BGR2GRAY);
  }
  
  const auto kResizeMethod = CV_INTER_NN;
  const char* typeStr = "UNKNOWN";
  const int inputIndex = _interpreter->inputs()[0];
  switch(_interpreter->tensor(inputIndex)->type)
  {
    case kTfLiteFloat32:
    {
      typeStr = "FLOAT";
      
      assert(img.isContinuous());
      resize<float>(_interpreter->typed_tensor<float>(inputIndex), img.ptr<uint8_t>(0), img.rows,
                    img.cols, img.channels(), _params.input_height, _params.input_width,
                    img.channels(), true, _params.input_shift, _params.input_scale);

/*
      // TODO: Resize and convert directly into the interpreter's input tensor data
      if(img.rows != _params.input_height || img.cols != _params.input_width)
      {
        cv::resize(img, img, cv::Size(_params.input_width,_params.input_height), 0, 0, kResizeMethod);
      }
      else if(_params.verbose)
      {
        std::cout << "Skipping actual resize: image already correct size" << std::endl;
      }
      assert(img.isContinuous());

      img.convertTo(img, CV_32FC(img.channels()), 1.f/_params.input_scale, _params.input_shift);
      memcpy(_interpreter->typed_tensor<float>(inputIndex), img.data, img.rows*img.cols*img.channels()*sizeof(float));
*/
      // float* tensor_data = image_tensor.tensor<float,4>().data();
      // for(int i=0; i<_params.input_height; ++i)
      // {
      //   const int scaled_row = i/
      //   const uint8_t* img_i = img.ptr<uint8_t>(i);

      //   for(int j=0; j<_params.input_width*img.channels(); ++j)
      //   {
      //     tensor_data[j] = (float)(img_i[j]) / 255.f;
      //     ++tensor_data;
      //   }
      // }
      break;
    }
      
    case kTfLiteUInt8:
    {
      typeStr = "UINT8";
      
      assert(img.isContinuous());
      resize<uint8_t>(_interpreter->typed_tensor<uint8_t>(inputIndex), img.ptr<uint8_t>(0), img.rows,
                    img.cols, img.channels(), _params.input_height, _params.input_width,
                    img.channels(), false, 0, 0);

      /*
      // Resize input image directly into the interpreter's input tensor data
      cv::Mat cvTensor(_params.input_height, _params.input_width, CV_8UC(img.channels()),
                       _interpreter->typed_tensor<uint8_t>(inputIndex));

      cv::resize(img, cvTensor, cv::Size(_params.input_width,_params.input_height), 0, 0, kResizeMethod);

      //cvTensor.convertTo(cvTensor, CV_8UC(img.channels()), 1.f/_params.input_scale, _params.input_shift);
      */

      break;
    }
      
    default:
      PRINT_NAMED_ERROR("ObjectDetector.Detect.UnsupportedInputType",
                        "Type: %d", _interpreter->tensor(inputIndex)->type);
      return RESULT_FAIL;
  }
  
  if(_params.verbose)
  {
    std::cout << "Resized " << img.cols << "x" << img.rows << "x" << img.channels() << " image into " <<
    _params.input_width << "x" << _params.input_height << " " << typeStr << " tensor" << std::endl;
  }
  
  if(_params.verbose)
  {
    std::cout << "Invoking with (" << img.cols << "x" << img.rows << "x" << img.channels() << "), " 
      << typeStr << " input" << std::endl;
  }

  assert(_interpreter != nullptr);
  const auto invokeResult = _interpreter->Invoke();

  if (kTfLiteOk != invokeResult)
  {
    PRINT_NAMED_ERROR("ObjectDetector.Detect.FailedToInvoke", "");
    return RESULT_FAIL;
  }
  
  if(_params.verbose)
  {
    std::cout << "Invoke completed" << std::endl;
  }

  // TODO: Get timestamp somehow
  const TimeStamp_t t = 0;

  if(_interpreter->outputs().size() == 1)
  {
    const int outputIndex = _interpreter->outputs()[0];
    switch (_interpreter->tensor(outputIndex)->type)
    {
      case kTfLiteFloat32:
        GetClassification<float>(t, objects);
        break;
      case kTfLiteUInt8:
        GetClassification<uint8_t>(t, objects);
        break;
      default:
        PRINT_NAMED_ERROR("ObjectDetector.Detect.UnsupportedOutputType",
                          "Type: %d", _interpreter->tensor(outputIndex)->type);
        return RESULT_FAIL;
    }
  }
  else
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.Run.DetectionModeNotSupported", "");
    //GetDetectedObjects(output_tensors, t, objects);  
    return RESULT_FAIL;
  }

  if(_params.verbose)
  {
    std::cout << "Session complete" << std::endl;
  }

  return RESULT_OK;
}
