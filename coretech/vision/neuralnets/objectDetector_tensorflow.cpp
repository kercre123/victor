#include "objectDetector_tensorflow.h"
#include "helpers.h"

#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/logging.h" 
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"
#include "tensorflow/core/util/memmapped_file_system.h"

#include <cmath>
#include <fstream>

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
  GetFromConfig(architecture);
  GetFromConfig(memoryMapGraph);
  
  if("ssd_mobilenet" == _params.architecture)
  {
    _inputLayerName = "image_tensor";
    _outputLayerNames = {"detection_scores", "detection_classes", "detection_boxes", "num_detections"};
    _useFloatInput = false;
  }
  else if(("mobilenet" == _params.architecture) || ("mobilenet_v1" == _params.architecture))
  { 
    _inputLayerName = "input";
    _outputLayerNames = {"MobilenetV1/Predictions/Softmax"};
    _useFloatInput = true;
  }
  else if("custom" == _params.architecture)
  {
    if(!config.isMember("input") || !config.isMember("output") || !config.isMember("use_float_input"))
    {
      PRINT_NAMED_ERROR("ObjectDetector.LoadModel.MissingInputOutput", 
                        "Custom architecture requires input/output names and use_float_input to be specified");
      return RESULT_FAIL;
    }
    SetFromConfigHelper(config["input"], _inputLayerName);
    const auto& outputs = config["output"];
    if(outputs.isArray()) {
      for(const auto& output : outputs) {
        _outputLayerNames.push_back(output.asString());
      }
    } 
    else {
      _outputLayerNames.push_back(outputs.asString());
    }
    SetFromConfigHelper(config["use_float_input"], _useFloatInput);

    if(config.isMember("use_grayscale"))
    {
      SetFromConfigHelper(config["use_grayscale"], _useGrayscale);
    }
  }
  else
  {
    PRINT_NAMED_ERROR("ObjectDetector.LoadModel.UnrecognizedArchitecture", "%s", 
                      _params.architecture.c_str());
    return RESULT_FAIL;
  }

  if(_params.verbose)
  {
    std::cout << "Arch: " << _params.architecture << 
      (_useGrayscale ? ", Grayscale " : ", Color ") << 
      "Input: " << _inputLayerName << ", Outputs: ";

    for(auto const& outputLayerName : _outputLayerNames)
    {
      std::cout << outputLayerName << " ";
    }
    std::cout << std::endl;
  }

  if(_useFloatInput)
  {
    // NOTE: Only used when processing in floating point
    GetFromConfig(input_shift);
    GetFromConfig(input_scale);
  }
  
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

  tensorflow::GraphDef graph_def;
  
  if(_params.memoryMapGraph)
  {
    // See also: https://www.tensorflow.org/mobile/optimizing

    // Note that this is a class member because it needs to persist as long as we
    // use the graph referring to it
    _memmapped_env.reset(new tensorflow::MemmappedEnv(tensorflow::Env::Default()));

    tensorflow::Status mmap_status = _memmapped_env->InitializeFromFile(graph_file_name);
    
    tensorflow::Status load_graph_status = ReadBinaryProto(_memmapped_env.get(),
        tensorflow::MemmappedFileSystem::kMemmappedPackageDefaultGraphDef,
        &graph_def);

    if (!load_graph_status.ok())
    {
      PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.MemoryMapBinaryProtoFailed",
                        "Status: %s", load_graph_status.ToString().c_str());
      return RESULT_FAIL;
    }

    std::cout << "ObjectDetector.Model.LoadGraph.MemMappedModelLoadSuccess " << graph_file_name << std::endl;

    tensorflow::SessionOptions options;
    options.config.mutable_graph_options()
        ->mutable_optimizer_options()
        ->set_opt_level(::tensorflow::OptimizerOptions::L0);
    options.env = _memmapped_env.get();

    tensorflow::Session* session_pointer = nullptr;
    tensorflow::Status session_status = tensorflow::NewSession(options, &session_pointer);

    if (!session_status.ok())
    {
      PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.NewMemoryMappedSessionFailed", 
                        "Status: %s", session_status.ToString().c_str());
      return RESULT_FAIL;
    }

    _session.reset(session_pointer);
  }
  else
  {
    tensorflow::Status load_graph_status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), 
                                                                       graph_file_name, &graph_def);
    if (!load_graph_status.ok())
    {
      PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.ReadBinaryProtoFailed",
                        "Status: %s", load_graph_status.ToString().c_str());
      return RESULT_FAIL;
    }

    std::cout << "ObjectDetector.Model.LoadGraph.ModelLoadSuccess " << graph_file_name << std::endl;

    _session.reset(tensorflow::NewSession(tensorflow::SessionOptions()));
  } 

  tensorflow::Status session_create_status = _session->Create(graph_def);

  if (!session_create_status.ok())
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.CreateSessionFailed",
                      "Status: %s", session_create_status.ToString().c_str());
    return RESULT_FAIL;
  }

  std::cout << "ObjectDetector.Model.LoadGraph.SessionCreated" << std::endl;

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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
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

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Detect(cv::Mat& img, std::list<DetectedObject>& objects)
{
  tensorflow::Tensor image_tensor;

  if(_useGrayscale)
  {
    cv::cvtColor(img, img, CV_BGR2GRAY);
  }

  const char* typeStr = (_useFloatInput ? "FLOAT" : "UINT8");

  if(_params.verbose)
  {
    std::cout << "Resizing " << img.cols << "x" << img.rows << "x" << img.channels() << " image into " << 
      _params.input_width << "x" << _params.input_height << " " << typeStr << " tensor" << std::endl;
  }

  if(_useFloatInput)
  {
    // TODO: Resize and convert directly into the tensor
    if(img.rows != _params.input_height || img.cols != _params.input_width)
    {
      cv::resize(img, img, cv::Size(_params.input_width,_params.input_height), 0, 0, CV_INTER_AREA);
    } 
    else if(_params.verbose)
    {
      std::cout << "Skipping actual resize: image already correct size" << std::endl;
    }
    assert(img.isContinuous());

    image_tensor = tensorflow::Tensor(tensorflow::DT_FLOAT, {
      1, _params.input_height, _params.input_width, img.channels()
    });

    img.convertTo(img, (img.channels() == 1 ? CV_32FC1 : CV_32FC3), 1.f/_params.input_scale, _params.input_shift);
    memcpy(image_tensor.tensor<float, 4>().data(), img.data, img.rows*img.cols*img.channels()*sizeof(float));

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
  
  }
  else 
  {
    image_tensor = tensorflow::Tensor(tensorflow::DT_UINT8, {
      1, _params.input_height, _params.input_width, img.channels()
    });

    // Resize input image directly into the tensor data    
    cv::Mat cvTensor(_params.input_height, _params.input_width, 
                     (img.channels() == 1 ? CV_8UC1 : CV_8UC3),
                     image_tensor.tensor<uint8_t, 4>().data());

    cv::resize(img, cvTensor, cv::Size(_params.input_width,_params.input_height), 0, 0, CV_INTER_AREA);
  }
    
  if(_params.verbose)
  {
    std::cout << "Running session: Input=(" << img.cols << "x" << img.rows << "x" << img.channels() << "), " << 
      typeStr << ", " << _outputLayerNames.size() << " outputs(s)" << std::endl;
  }

  std::vector<tensorflow::Tensor> output_tensors;
  tensorflow::Status run_status = _session->Run({{_inputLayerName, image_tensor}}, _outputLayerNames, {}, &output_tensors);

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
