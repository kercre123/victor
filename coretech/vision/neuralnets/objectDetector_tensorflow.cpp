/**
 * File: objectDetector_tensorflow.cpp
 *
 * Author: Andrew Stein
 * Date:   5/17/2018
 *
 * Description: <See header>
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/common/shared/types.h"
#include "coretech/vision/neuralnets/objectDetector_tensorflow.h"

#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/graph_def_util.h"
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

#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"
#include "util/logging/logging.h"

#include <cmath>
#include <fstream>

namespace Anki {

#define LOG_CHANNEL "NeuralNets"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// TODO: Could use JsonTools:: instead of most of these...

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

static inline void SetFromConfigHelper(const Json::Value& json, std::vector<std::string>& values)
{
  if(json.isArray()) {
    for(const auto& value : json) {
      values.push_back(value.asString());
    }
  } 
  else {
    values.push_back(json.asString());
  }
}

// static inline void SetFromConfigHelper(const Json::Value& json, uint8_t& value) {
//   value = json.asUInt();
// }

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::ObjectDetector() 
: _params{} 
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::~ObjectDetector()
{
  LOG_INFO("ObjectDetector.Destructor", "");
  if(_session)
  {
    tensorflow::Status sessionCloseStatus = _session->Close();
    if (!sessionCloseStatus.ok() ) 
    {
      PRINT_NAMED_WARNING("ObjectDetector.Destructor.CloseSessionFailed", "Status: %s", 
                          sessionCloseStatus.ToString().c_str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::LoadModel(const std::string& modelPath, const Json::Value& config)
{
  const Result result = SetParamsFromConfig(config);
  if(RESULT_OK != result) 
  {
    PRINT_NAMED_ERROR("ObjectDetector.LoadModel.SetParamsFromConfigFailed", "");
    return result;
  }
  
  const std::string graphFileName = Util::FileUtils::FullFilePath({modelPath, _params.graphFile});
  
  if (!Util::FileUtils::FileExists(graphFileName))
                  
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.GraphFileDoesNotExist", "%s",
                      graphFileName.c_str());
    return RESULT_FAIL;
  }

  if(_params.verbose)
  {
    LOG_INFO("ObjectDetector.LoadModel.FoundGraphFile",
             "%s", graphFileName.c_str());
  }

  tensorflow::GraphDef graphDef;
  
  tensorflow::Session* sessionPtr = nullptr;

  if(_params.memoryMapGraph)
  {
    // Using memory-mapped graphs needs more testing/work, but this is a start. (VIC-3141)
    
    // See also: https://www.tensorflow.org/mobile/optimizing

    // Note that this is a class member because it needs to persist as long as we
    // use the graph referring to it
    _memmappedEnv.reset(new tensorflow::MemmappedEnv(tensorflow::Env::Default()));

    tensorflow::Status mmapStatus = _memmappedEnv->InitializeFromFile(graphFileName);
    
    tensorflow::Status loadGraphStatus = ReadBinaryProto(_memmappedEnv.get(),
        tensorflow::MemmappedFileSystem::kMemmappedPackageDefaultGraphDef,
        &graphDef);

    if (!loadGraphStatus.ok())
    {
      PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.MemoryMapBinaryProtoFailed",
                        "Status: %s", loadGraphStatus.ToString().c_str());
      return RESULT_FAIL;
    }

    LOG_INFO("ObjectDetector.LoadModel.MemMappedModelLoadSuccess", "%s", graphFileName.c_str());

    tensorflow::SessionOptions options;
    options.config.mutable_graph_options()
        ->mutable_optimizer_options()
        ->set_opt_level(::tensorflow::OptimizerOptions::L0);
    options.env = _memmappedEnv.get();

    tensorflow::Status session_status = tensorflow::NewSession(options, &sessionPtr);

    if (!session_status.ok())
    {
      PRINT_NAMED_ERROR("ObjectDetector.LoadModel.NewMemoryMappedSessionFailed", 
                        "Status: %s", session_status.ToString().c_str());
      return RESULT_FAIL;
    }

    _session.reset(sessionPtr);
  }
  else
  {
    tensorflow::Status loadGraphStatus = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), 
                                                                       graphFileName, &graphDef);
    if (!loadGraphStatus.ok())
    {
      PRINT_NAMED_ERROR("ObjectDetector.LoadModel.ReadBinaryProtoFailed",
                        "Status: %s", loadGraphStatus.ToString().c_str());
      return RESULT_FAIL;
    }

    LOG_INFO("ObjectDetector.LoadModel.ReadBinaryProtoSuccess", "%s", graphFileName.c_str());

    sessionPtr = tensorflow::NewSession(tensorflow::SessionOptions());
  } 

  if(_session)
  {
    tensorflow::Status sessionCloseStatus = _session->Close();
    if (!sessionCloseStatus.ok() ) 
    {
      PRINT_NAMED_WARNING("ObjectDetector.LoadModel.CloseSessionFailed", "Status: %s", 
                          sessionCloseStatus.ToString().c_str());
    }
  }
  _session.reset(sessionPtr);

  tensorflow::Status sessionCreateStatus = _session->Create(graphDef);

  if (!sessionCreateStatus.ok())
  {
    PRINT_NAMED_ERROR("ObjectDetector.LoadModel.CreateSessionFailed",
                      "Status: %s", sessionCreateStatus.ToString().c_str());
    return RESULT_FAIL;
  }

  LOG_INFO("ObjectDetector.LoadModel.SessionCreated", "");

  if (_params.verbose)
  {
    //const std::string graph_str = tensorflow::SummarizeGraphDef(graphDef);
    //std::cout << graph_str << std::endl;
    
    // Print some weights from each layer as a sanity check
    int node_count = graphDef.node_size();
    for (int i = 0; i < node_count; i++)
    {
      const auto n = graphDef.node(i);
      LOG_INFO("ObjectDetector.LoadModel.Summary", "Layer %d - Name: %s, Op: %s", i, n.name().c_str(), n.op().c_str());
      if(n.op() == "Const")
      {
        tensorflow::Tensor t;
        if (!t.FromProto(n.attr().at("value").tensor())) {
          LOG_INFO("ObjectDetector.LoadModel.SummaryFail", "Failed to create Tensor from proto");
          continue;
        }

        LOG_INFO("ObjectDetector.LoadModel.Summary", "%s", t.DebugString().c_str());
      }
      else if(n.op() == "Conv2D")
      {
        const auto& filterNodeName = n.input(1);
        LOG_INFO("ObjectDetector.LoadModel.Summary", "Filter input from Conv2D node: %s", filterNodeName.c_str());
      }
    }
  }

  const std::string labelsFileName = Util::FileUtils::FullFilePath({modelPath, _params.labelsFile});
  Result readLabelsResult = ReadLabelsFile(labelsFileName, _labels);
  if (RESULT_OK == readLabelsResult)
  {
    LOG_INFO("ObjectDetector.LoadModel.ReadLabelFileSuccess", "%s", labelsFileName.c_str());
  }
  return readLabelsResult;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::SetParamsFromConfig(const Json::Value& config)
{
# define GetFromConfig(keyName) \
  if(!config.isMember(QUOTE(keyName))) \
  { \
    PRINT_NAMED_ERROR("ObjectDetector.SetParamsFromConfig.MissingConfig", QUOTE(keyName)); \
    return RESULT_FAIL; \
  } \
  else \
  { \
    SetFromConfigHelper(config[QUOTE(keyName)], _params.keyName); \
  }
  
  GetFromConfig(verbose);
  GetFromConfig(labelsFile);
  GetFromConfig(minScore);  
  GetFromConfig(graphFile);
  GetFromConfig(inputHeight);
  GetFromConfig(inputWidth);
  GetFromConfig(architecture);
  GetFromConfig(memoryMapGraph);

  if("ssd_mobilenet" == _params.architecture)
  {
    _params.inputLayerName = "image_tensor";
    _params.outputLayerNames = {"detection_scores", "detection_classes", "detection_boxes", "num_detections"};
    _params.useFloatInput = false;
    _params.outputType = OutputType::AnchorBoxes;

    if(config.isMember("outputType")) {
      PRINT_NAMED_WARNING("ObjectDetector.SetParamsFromConfig.IgnoringOutputType", 
                          "Ignoring outputType and using 'AnchorBoxes' because architecture='ssd_mobilenet' was specified");
    }
  }
  else if(("mobilenet" == _params.architecture) || ("mobilenet_v1" == _params.architecture))
  { 
    _params.inputLayerName = "input";
    _params.outputLayerNames = {"MobilenetV1/Predictions/Softmax"};
    _params.useFloatInput = true;
    _params.outputType = OutputType::Classification;

    if(config.isMember("outputType")) {
      PRINT_NAMED_WARNING("ObjectDetector.SetParamsFromConfig.IgnoringOutputType", 
                          "Ignoring outputType and using 'Classification' because architecture='mobilenet' was specified");
    }
  }
  else if("custom" == _params.architecture)
  {
    GetFromConfig(inputLayerName);
    GetFromConfig(outputLayerNames);
    GetFromConfig(useFloatInput);
    
    const Result result = SetOutputTypeFromConfig(config);
    if(RESULT_OK != result) {
      // SetOutputTypeFromConfig will print an error, just return
      // up the chain
      return result;
    }
    
    if(config.isMember("useGrayscale"))
    {
      SetFromConfigHelper(config["useGrayscale"], _params.useGrayscale);
    }
  }
  else
  {
    PRINT_NAMED_ERROR("ObjectDetector.SetParamsFromConfig.UnrecognizedArchitecture", "%s", 
                      _params.architecture.c_str());
    return RESULT_FAIL;
  }

  if(_params.verbose)
  {
    std::string outputNames;
    for(auto const& outputLayerName : _params.outputLayerNames)
    {
      outputNames += outputLayerName + " ";
    }    

    LOG_INFO("ObjectDetector.SetParamsFromConfig.Summary", "Arch: %s, %s Input: %s, Outputs: %s", 
             _params.architecture.c_str(), (_params.useGrayscale ? "Grayscale" : "Color"),
             _params.inputLayerName.c_str(), outputNames.c_str());
  }

  if(_params.useFloatInput)
  {
    // NOTE: Only used when processing in floating point
    GetFromConfig(inputShift);
    GetFromConfig(inputScale);
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::SetOutputTypeFromConfig(const Json::Value& config)
{
  // Convert outputType to enum and validate number of outputs
  if(!config.isMember("outputType")) {
    PRINT_NAMED_ERROR("ObjectDetector.SetOutputTypeFromConfig.MissingOutputType", 
                      "Custom architecture requires outputType to be specified");
    return RESULT_FAIL;
  }

  std::string outputTypeStr;
  SetFromConfigHelper(config["outputType"], outputTypeStr);

  struct OutputTypeEntry {
    OutputType type;
    int        numOutputs;
  };
  const std::map<std::string, OutputTypeEntry> kOutputTypeMap{
    {"classification",        {OutputType::Classification,     1} }, 
    {"binary_localization",   {OutputType::BinaryLocalization, 1} },
    {"anchor_boxes",          {OutputType::AnchorBoxes,          4} },
  };

  auto iter = kOutputTypeMap.find(outputTypeStr);
  if(iter == kOutputTypeMap.end()) {
    std::string validKeys;
    for(auto const& entry : kOutputTypeMap) {
      validKeys += entry.first;
      validKeys += " ";
    }
    PRINT_NAMED_ERROR("ObjectDetector.SetOutputTypeFromConfig.BadOutputType", "Valid types: %s", validKeys.c_str());
    return RESULT_FAIL;
  }
  else {

    if(_params.outputLayerNames.size() != iter->second.numOutputs)
    {
      PRINT_NAMED_ERROR("ObjectDetector.SetOutputTypeFromConfig.WrongNumberOfOutputs", 
                        "OutputType %s requires %d outputs (%d provided)", 
                        iter->first.c_str(), iter->second.numOutputs, (int)_params.outputLayerNames.size());
      return RESULT_FAIL;
    }
    _params.outputType = iter->second.type;

    if(OutputType::BinaryLocalization == _params.outputType)
    {
      GetFromConfig(numGridRows);
      GetFromConfig(numGridCols);
    }
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out)
{
  std::ifstream file(fileName);
  if (!file)
  {
    PRINT_NAMED_ERROR("ObjectDetector.ReadLabelsFile.LabelsFileNotFound", "%s", fileName.c_str());
    return RESULT_FAIL;
  }
  
  labels_out.clear();
  std::string line;
  while (std::getline(file, line)) {
    labels_out.push_back(line);
  }
  
  LOG_INFO("ObjectDetector.ReadLabelsFile.Success", "Read %d labels", (int)labels_out.size());

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectDetector::GetClassification(const tensorflow::Tensor& outputTensor, TimeStamp_t timestamp, 
                                       std::list<DetectedObject>& objects)
{
  const float* outputData = outputTensor.tensor<float, 2>().data();
        
  float maxScore = _params.minScore;
  int labelIndex = -1;
  for(int i=0; i<_labels.size(); ++i)
  {
    if(outputData[i] > maxScore)
    {
      maxScore = outputData[i];
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
      LOG_INFO("ObjectDetector.GetClassification.ObjectFound", "Name: %s, Score: %f", object.name.c_str(), object.score);
    }

    objects.push_back(std::move(object));
  }
  else if(_params.verbose)
  {
    LOG_INFO("ObjectDetector.GetClassification.NoObjects", "MinScore: %f", _params.minScore);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectDetector::GetLocalizedBinaryClassification(const tensorflow::Tensor& outputTensor, TimeStamp_t timestamp, 
                                                      std::list<DetectedObject>& objects)
{
  // Create a detection box for each grid cell that is above threshold

  // This raw (Eigen) tensor data appears to be _column_ major (i.e. "not row major"). Ensure that remains true.
  DEV_ASSERT( !(outputTensor.tensor<float, 2>().Options & Eigen::RowMajor), 
             "ObjectDetector.GetLocalizedBinaryClassification.OutputNotRowMajor");

  const float* outputData = outputTensor.tensor<float, 2>().data();

  // Box size in normalized image coordiantes
  const float boxWidth  = 1.f / (float)_params.numGridCols;
  const float boxHeight = 1.f / (float)_params.numGridRows;

  int outputIndex = 0;
  for(int xBox=0; xBox < _params.numGridCols; ++xBox)
  {
    for(int yBox=0; yBox < _params.numGridRows; ++yBox)
    {
      const float score = outputData[outputIndex++];
      if(score > _params.minScore)
      {
        DetectedObject box{
          .timestamp = timestamp, 
          .score = score,
          .name = "",
          .xmin = boxWidth  * (float)xBox,
          .ymin = boxHeight * (float)yBox, 
          .xmax = boxWidth  * (float)(xBox+1),
          .ymax = boxHeight * (float)(yBox+1),
        };

        objects.push_back(std::move(box));
      }
    }
  }
} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectDetector::GetDetectedObjects(const std::vector<tensorflow::Tensor>& outputRensors, TimeStamp_t timestamp,
                                        std::list<DetectedObject>& objects)
{
  DEV_ASSERT(outputRensors.size() == 4, "ObjectDetector.GetDetectedObjects.WrongNumOutputs");

  const int numDetections = (int)outputRensors[3].tensor<float,1>().data()[0];

  if(_params.verbose)
  {
    LOG_INFO("ObjectDetector.GetDetectedObjects.NumDetections", "%d raw detections", numDetections);
  }

  if(numDetections > 0)
  {
    const float* scores  = outputRensors[0].tensor<float, 2>().data();
    const float* classes = outputRensors[1].tensor<float, 2>().data();
    
    auto const& boxesTensor = outputRensors[2].tensor<float, 3>();
    
    const float* boxes = boxesTensor.data();
    
    for(int i=0; i<numDetections; ++i)
    {
      if(scores[i] >= _params.minScore)
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
      std::string objectsStr;
      for(auto const& object : objects) {
        objectsStr += object.name + " ";
      }

      LOG_INFO("ObjectDetector.GetDetectedObjects.ReturningObjects", 
               "Returning %d objects with score above %f: %s", 
               (int)objects.size(), _params.minScore, objectsStr.c_str());
    }
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Detect(cv::Mat& img, const TimeStamp_t t, std::list<DetectedObject>& objects)
{
  tensorflow::Tensor image_tensor;

  if(_params.useGrayscale)
  {
    cv::cvtColor(img, img, CV_BGR2GRAY);
  }

  const char* typeStr = (_params.useFloatInput ? "FLOAT" : "UINT8");

  if(_params.verbose)
  {
    LOG_INFO("ObjectDetector.Detect.Resizing", "From [%dx%dx%d] image to [%dx%dx%d] %s tensor", 
             img.cols, img.rows, img.channels(), 
             _params.inputWidth, _params.inputHeight, (_params.useGrayscale ? 1 : 3), 
             typeStr);
  }

  const auto kResizeMethod = CV_INTER_LINEAR;

  if(_params.useFloatInput)
  {
    // Resize uint8 image data, and *then* convert smaller image to float below
    // TODO: Resize and convert directly into the tensor
    if(img.rows != _params.inputHeight || img.cols != _params.inputWidth)
    {
      cv::resize(img, img, cv::Size(_params.inputWidth,_params.inputHeight), 0, 0, kResizeMethod);
    } 
    else if(_params.verbose)
    {
      LOG_INFO("ObjectDetector.Detect.SkipResize", "Skipping actual resize: image already correct size");
    }
    DEV_ASSERT(img.isContinuous(), "ObjectDetector.Detect.ImageNotContinuous");

    image_tensor = tensorflow::Tensor(tensorflow::DT_FLOAT, {
      1, _params.inputHeight, _params.inputWidth, img.channels()
    });

    // Scale/shift resized image directly into the tensor data    
    const auto cvType = (img.channels() == 1 ? CV_32FC1 : CV_32FC3);
    
    cv::Mat cvTensor(_params.inputHeight, _params.inputWidth, cvType,
                     image_tensor.tensor<float, 4>().data());

    img.convertTo(cvTensor, cvType, 1.f/_params.inputScale, _params.inputShift);
  
  }
  else 
  {
    image_tensor = tensorflow::Tensor(tensorflow::DT_UINT8, {
      1, _params.inputHeight, _params.inputWidth, img.channels()
    });

    // Resize uint8 input image directly into the uint8 tensor data    
    cv::Mat cvTensor(_params.inputHeight, _params.inputWidth, 
                     (img.channels() == 1 ? CV_8UC1 : CV_8UC3),
                     image_tensor.tensor<uint8_t, 4>().data());

    cv::resize(img, cvTensor, cv::Size(_params.inputWidth, _params.inputHeight), 0, 0, kResizeMethod);
  }
    
  if(_params.verbose)
  {
    LOG_INFO("ObjectDetector.Detect.RunningSession", "Input=[%dx%dx%d], %s, %d output(s)", 
             img.cols, img.rows, img.channels(), typeStr, (int)_params.outputLayerNames.size());
  }

  std::vector<tensorflow::Tensor> outputRensors;
  tensorflow::Status run_status = _session->Run({{_params.inputLayerName, image_tensor}}, _params.outputLayerNames, {}, &outputRensors);

  if (!run_status.ok()) {
    PRINT_NAMED_ERROR("ObjectDetector.Detect.DetectionSessionRunFail", "%s", run_status.ToString().c_str());
    return RESULT_FAIL;
  }

  switch(_params.outputType)
  {
    case OutputType::Classification:
    {
      GetClassification(outputRensors[0], t, objects);
      break;
    }
    case OutputType::BinaryLocalization:
    {
      GetLocalizedBinaryClassification(outputRensors[0], t, objects);
      break;
    }
    case OutputType::AnchorBoxes:
    {
      GetDetectedObjects(outputRensors, t, objects);  
      break;
    }
  }

  if(_params.verbose)
  {
    LOG_INFO("ObjectDetector.Detect.SessionComplete", "");
  }

  return RESULT_OK;
}

} // namespace Anki
