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
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/vision/neuralnets/neuralNetModel_tensorflow.h"

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
#include "tensorflow/core/util/stat_summarizer.h"

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
NeuralNetModel::NeuralNetModel()
: _params{} 
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NeuralNetModel::~NeuralNetModel()
{
  LOG_INFO("NeuralNetModel.Destructor", "");
  if(_session)
  {
    tensorflow::Status sessionCloseStatus = _session->Close();
    if (!sessionCloseStatus.ok() ) 
    {
      PRINT_NAMED_WARNING("NeuralNetModel.Destructor.CloseSessionFailed", "Status: %s",
                          sessionCloseStatus.ToString().c_str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetModel::LoadModel(const std::string& modelPath, const Json::Value& config)
{
  const Result result = SetParamsFromConfig(config);
  if(RESULT_OK != result) 
  {
    PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.SetParamsFromConfigFailed", "");
    return result;
  }
  
  const std::string graphFileName = Util::FileUtils::FullFilePath({modelPath, _params.graphFile});
  
  if (!Util::FileUtils::FileExists(graphFileName))
                  
  {
    PRINT_NAMED_ERROR("NeuralNetModel.Model.LoadGraph.GraphFileDoesNotExist", "%s",
                      graphFileName.c_str());
    return RESULT_FAIL;
  }

  if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.LoadModel.FoundGraphFile",
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
      PRINT_NAMED_ERROR("NeuralNetModel.Model.LoadGraph.MemoryMapBinaryProtoFailed",
                        "Status: %s", loadGraphStatus.ToString().c_str());
      return RESULT_FAIL;
    }

    LOG_INFO("NeuralNetModel.LoadModel.MemMappedModelLoadSuccess", "%s", graphFileName.c_str());

    tensorflow::SessionOptions options;
    options.config.mutable_graph_options()
        ->mutable_optimizer_options()
        ->set_opt_level(::tensorflow::OptimizerOptions::L0);
    options.env = _memmappedEnv.get();

    tensorflow::Status session_status = tensorflow::NewSession(options, &sessionPtr);

    if (!session_status.ok())
    {
      PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.NewMemoryMappedSessionFailed",
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
      PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.ReadBinaryProtoFailed",
                        "Status: %s", loadGraphStatus.ToString().c_str());
      return RESULT_FAIL;
    }

    LOG_INFO("NeuralNetModel.LoadModel.ReadBinaryProtoSuccess", "%s", graphFileName.c_str());

    sessionPtr = tensorflow::NewSession(tensorflow::SessionOptions());
  } 

  if(_session)
  {
    tensorflow::Status sessionCloseStatus = _session->Close();
    if (!sessionCloseStatus.ok() ) 
    {
      PRINT_NAMED_WARNING("NeuralNetModel.LoadModel.CloseSessionFailed", "Status: %s",
                          sessionCloseStatus.ToString().c_str());
    }
  }
  _session.reset(sessionPtr);

  tensorflow::Status sessionCreateStatus = _session->Create(graphDef);

  if (!sessionCreateStatus.ok())
  {
    PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.CreateSessionFailed",
                      "Status: %s", sessionCreateStatus.ToString().c_str());
    return RESULT_FAIL;
  }

  LOG_INFO("NeuralNetModel.LoadModel.SessionCreated", "");

  if (_params.verbose)
  {
    //const std::string graph_str = tensorflow::SummarizeGraphDef(graphDef);
    //std::cout << graph_str << std::endl;
    
    // Print some weights from each layer as a sanity check
    int node_count = graphDef.node_size();
    for (int i = 0; i < node_count; i++)
    {
      const auto n = graphDef.node(i);
      LOG_INFO("NeuralNetModel.LoadModel.Summary", "Layer %d - Name: %s, Op: %s", i, n.name().c_str(), n.op().c_str());
      if(n.op() == "Const")
      {
        tensorflow::Tensor t;
        if (!t.FromProto(n.attr().at("value").tensor())) {
          LOG_INFO("NeuralNetModel.LoadModel.SummaryFail", "Failed to create Tensor from proto");
          continue;
        }

        LOG_INFO("NeuralNetModel.LoadModel.Summary", "%s", t.DebugString().c_str());
      }
      else if(n.op() == "Conv2D")
      {
        const auto& filterNodeName = n.input(1);
        LOG_INFO("NeuralNetModel.LoadModel.Summary", "Filter input from Conv2D node: %s", filterNodeName.c_str());
      }
    }
  }

  const std::string labelsFileName = Util::FileUtils::FullFilePath({modelPath, _params.labelsFile});
  Result readLabelsResult = ReadLabelsFile(labelsFileName, _labels);
  if (RESULT_OK == readLabelsResult)
  {
    LOG_INFO("NeuralNetModel.LoadModel.ReadLabelFileSuccess", "%s", labelsFileName.c_str());
  }
  return readLabelsResult;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetModel::SetParamsFromConfig(const Json::Value& config)
{
# define GetFromConfig(keyName) \
  if(!config.isMember(QUOTE(keyName))) \
  { \
    PRINT_NAMED_ERROR("NeuralNetModel.SetParamsFromConfig.MissingConfig", QUOTE(keyName)); \
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
  GetFromConfig(benchmarkRuns);

  if("ssd_mobilenet" == _params.architecture)
  {
    _params.inputLayerName = "image_tensor";
    _params.outputLayerNames = {"detection_scores", "detection_classes", "detection_boxes", "num_detections"};
    _params.useFloatInput = false;
    _params.outputType = OutputType::AnchorBoxes;

    if(config.isMember("outputType")) {
      PRINT_NAMED_WARNING("NeuralNetModel.SetParamsFromConfig.IgnoringOutputType",
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
      PRINT_NAMED_WARNING("NeuralNetModel.SetParamsFromConfig.IgnoringOutputType",
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
    PRINT_NAMED_ERROR("NeuralNetModel.SetParamsFromConfig.UnrecognizedArchitecture", "%s",
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

    LOG_INFO("NeuralNetModel.SetParamsFromConfig.Summary", "Arch: %s, %s Input: %s, Outputs: %s",
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
Result NeuralNetModel::SetOutputTypeFromConfig(const Json::Value& config)
{
  // Convert outputType to enum and validate number of outputs
  if(!config.isMember("outputType")) {
    PRINT_NAMED_ERROR("NeuralNetModel.SetOutputTypeFromConfig.MissingOutputType",
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
    {"anchor_boxes",          {OutputType::AnchorBoxes,        4} },
    {"segmentation",          {OutputType::Segmentation,       1} },
  };

  auto iter = kOutputTypeMap.find(outputTypeStr);
  if(iter == kOutputTypeMap.end()) {
    std::string validKeys;
    for(auto const& entry : kOutputTypeMap) {
      validKeys += entry.first;
      validKeys += " ";
    }
    PRINT_NAMED_ERROR("NeuralNetModel.SetOutputTypeFromConfig.BadOutputType", "Valid types: %s", validKeys.c_str());
    return RESULT_FAIL;
  }
  else {

    if(_params.outputLayerNames.size() != iter->second.numOutputs)
    {
      PRINT_NAMED_ERROR("NeuralNetModel.SetOutputTypeFromConfig.WrongNumberOfOutputs",
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
Result NeuralNetModel::ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out)
{
  std::ifstream file(fileName);
  if (!file)
  {
    PRINT_NAMED_ERROR("NeuralNetModel.ReadLabelsFile.LabelsFileNotFound", "%s", fileName.c_str());
    return RESULT_FAIL;
  }
  
  labels_out.clear();
  std::string line;
  while (std::getline(file, line)) {
    labels_out.push_back(line);
  }
  
  LOG_INFO("NeuralNetModel.ReadLabelsFile.Success", "Read %d labels", (int)labels_out.size());

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NeuralNetModel::GetClassification(const tensorflow::Tensor& outputTensor, TimeStamp_t timestamp,
                                       std::list<Vision::SalientPoint>& salientPoints)
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
  
  const Rectangle<int32_t> imgRect(0.f,0.f,1.f,1.f);
  const Poly2i imgPoly(imgRect);
  
  if(labelIndex >= 0)
  {    
    Vision::SalientPoint salientPoint(timestamp, 0.5f, 0.5f, maxScore, 1.f,
                                      Vision::SalientPointType::Object,
                                      (labelIndex < _labels.size() ? _labels.at((size_t)labelIndex) : "<UNKNOWN>"),
                                      imgPoly.ToCladPoint2dVector());
    
    if(_params.verbose)
    {
      LOG_INFO("NeuralNetModel.GetClassification.ObjectFound", "Name: %s, Score: %f", salientPoint.description.c_str(), salientPoint.score);
    }

    salientPoints.push_back(std::move(salientPoint));
  }
  else if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.GetClassification.NoObjects", "MinScore: %f", _params.minScore);
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NeuralNetModel::GetLocalizedBinaryClassification(const tensorflow::Tensor& outputTensor, TimeStamp_t timestamp,
                                                      std::list<Vision::SalientPoint>& salientPoints)
{
  // Create a detection box for each grid cell that is above threshold

  // This raw (Eigen) tensor data appears to be _column_ major (i.e. "not row major"). Ensure that remains true.
  DEV_ASSERT( !(outputTensor.tensor<float, 2>().Options & Eigen::RowMajor), 
             "NeuralNetModel.GetLocalizedBinaryClassification.OutputNotRowMajor");

  const float* outputData = outputTensor.tensor<float, 2>().data();

  _detectionGrid.create(_params.numGridRows, _params.numGridCols);
  
  bool anyDetections = false;
  for(int i=0; i<_detectionGrid.rows; ++i)
  {
    uint8_t* detectionGrid_i = _detectionGrid.ptr(i);
    for(int j=0; j<_detectionGrid.cols; ++j)
    {
      // Compute the column-major index to get data from the output tensor
      const int outputIndex = j*_params.numGridRows + i;
      const float score = outputData[outputIndex];
      
      // Create binary detection image
      if(score > _params.minScore) {
        anyDetections = true;
        detectionGrid_i[j] = static_cast<uint8_t>(255.f * score);
      }
      else {
        detectionGrid_i[j] = 0;
      }
      
    }
  }
  
  if(anyDetections)
  {
    // Someday if we ever link vic-neuralnets against coretech vision, we could use our own connected
    // components API, but for now, rely directly on OpenCV. Because we want to get average score, 
    // we'll do our own "stats" computation below instead of using connectedComponentsWithStats()
    const s32 count = cv::connectedComponents(_detectionGrid, _labelsGrid);
    DEV_ASSERT((_detectionGrid.rows == _labelsGrid.rows) && (_detectionGrid.cols == _labelsGrid.cols),
               "NeuralNetModel.GetLocalizedBinaryClassification.MismatchedLabelsGridSize");
    
    if(_params.verbose)
    {
      LOG_INFO("NeuralNetModel.GetLocalizedBinaryClassification.FoundConnectedComponents",
               "NumComponents: %d", count);
    }
    
    // Compute stats for each connected component
    struct Stat {
      int32_t area; // NOTE: area will be number of grid squares, not area of bounding box shape
      int32_t scoreSum;
      Point2f centroid;
      int xmin, xmax, ymin, ymax;
    };
    std::vector<Stat> stats(count, {0,0,{0.f,0.f},_detectionGrid.cols, -1, _detectionGrid.rows, -1});
    
    for(int i=0; i<_detectionGrid.rows; ++i)
    {
      const uint8_t* detectionGrid_i = _detectionGrid.ptr<uint8_t>(i);
      const int32_t* labelsGrid_i    = _labelsGrid.ptr<int32_t>(i);
      for(int j=0; j<_detectionGrid.cols; ++j)
      {
        const int32_t label = labelsGrid_i[j];
        if(label > 0) // zero is background (not part of any connected component)
        {
          DEV_ASSERT(label < count, "NeuralNetModel.GetLocalizedBinaryClassification.BadLabel");
          const int32_t score = detectionGrid_i[j];
          Stat& stat = stats[label];
          stat.scoreSum += score;
          stat.area++;
          
          stat.centroid.x() += j;
          stat.centroid.y() += i;
          
          stat.xmin = std::min(stat.xmin, j);
          stat.xmax = std::max(stat.xmax, j);
          stat.ymin = std::min(stat.ymin, i);
          stat.ymax = std::max(stat.ymax, i);
        }
      }
    }
    
    // Create a SalientPoint to return for each connected component (skipping background component 0)
    const float widthScale  = 1.f / static_cast<float>(_detectionGrid.cols);
    const float heightScale = 1.f / static_cast<float>(_detectionGrid.rows);
    for(s32 iComp=1; iComp < count; ++iComp)
    {
      Stat& stat = stats[iComp];
      
      const float area = static_cast<float>(stat.area);
      const float avgScore = (1.f/255.f) * ((float)stat.scoreSum / area);
      
      // Centroid currently contains a sum. Divide by area to get actual centroid.
      stat.centroid *= 1.f/area;
      
      // Convert centroid to normalized coordinates
      stat.centroid.x() = Util::Clamp(stat.centroid.x()*widthScale,  0.f, 1.f);
      stat.centroid.y() = Util::Clamp(stat.centroid.y()*heightScale, 0.f, 1.f);
      
      // Use the single label (this is supposed to be a binary classifier after all) and
      // convert it to a SalientPointType
      DEV_ASSERT(_labels.size()==1, "ObjectDetector.GetLocalizedBinaryClassification.NotBinary");
      Vision::SalientPointType type = Vision::SalientPointType::Unknown;
      const bool success = SalientPointTypeFromString(_labels[0], type);
      DEV_ASSERT(success, "ObjectDetector.GetLocalizedBinaryClassification.NoSalientPointTypeForLabel");

      // Use the bounding box as the shape (in normalized coordinates)
      // TODO: Create a more precise polygon that traces the shape of the connected component (cv::findContours?)
      const float xmin = (static_cast<float>(stat.xmin)-0.5f) * widthScale;
      const float ymin = (static_cast<float>(stat.ymin)-0.5f) * heightScale;
      const float xmax = (static_cast<float>(stat.xmax)+0.5f) * widthScale;
      const float ymax = (static_cast<float>(stat.ymax)+0.5f) * heightScale;
      const Poly2f shape( Rectangle<float>(xmin, ymin, xmax-xmin, ymax-ymin) );
      
      Vision::SalientPoint salientPoint(timestamp,
                                        stat.centroid.x(),
                                        stat.centroid.y(),
                                        avgScore,
                                        area * (widthScale*heightScale), // convert to area fraction
                                        type,
                                        EnumToString(type),
                                        shape.ToCladPoint2dVector());
      
      if(_params.verbose)
      {
        LOG_INFO("NeuralNetModel.GetLocalizedBinaryClassification.SalientPoint",
                 "%d: %s score:%.2f area:%.2f [%s %s %s %s]",
                 iComp, stat.centroid.ToString().c_str(), avgScore, area,
                 shape[0].ToString().c_str(),
                 shape[1].ToString().c_str(),
                 shape[2].ToString().c_str(),
                 shape[3].ToString().c_str());
      }
      
      salientPoints.push_back(std::move(salientPoint));
    }
  }

} 

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NeuralNetModel::GetDetectedObjects(const std::vector<tensorflow::Tensor>& outputRensors, TimeStamp_t timestamp,
                                        std::list<Vision::SalientPoint>& salientPoints)
{
  DEV_ASSERT(outputRensors.size() == 4, "NeuralNetModel.GetDetectedObjects.WrongNumOutputs");

  const int numDetections = (int)outputRensors[3].tensor<float,1>().data()[0];

  if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.GetDetectedObjects.NumDetections", "%d raw detections", numDetections);
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
        const float xmin = box[0];
        const float ymin = box[1];
        const float xmax = box[2];
        const float ymax = box[3];
        
        const size_t labelIndex = (size_t)(classes[i]);

        const Rectangle<int32_t> bbox(xmin, ymin, xmax-xmin, ymax-ymin);
        const Poly2i poly(bbox);
        
        Vision::SalientPoint salientPoint(timestamp,
                                          (float)(xmin+xmax) * 0.5f,
                                          (float)(ymin+ymax) * 0.5f,
                                          scores[i],
                                          bbox.Area(),
                                          Vision::SalientPointType::Object,
                                          (labelIndex < _labels.size() ? _labels[labelIndex] : "<UNKNOWN>"),
                                          poly.ToCladPoint2dVector());
        
        salientPoints.emplace_back(std::move(salientPoint));
      }
    }

    if(_params.verbose)
    {
      std::string salientPointsStr;
      for(auto const& salientPoint : salientPoints) {
        salientPointsStr += salientPoint.description + " ";
      }

      LOG_INFO("NeuralNetModel.GetDetectedObjects.ReturningObjects",
               "Returning %d salient points with score above %f: %s",
               (int)salientPoints.size(), _params.minScore, salientPointsStr.c_str());
    }
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetModel::Detect(cv::Mat& img, const TimeStamp_t t, std::list<Vision::SalientPoint>& salientPoints)
{
  tensorflow::Tensor imageTensor;

  if(_params.useGrayscale)
  {
    cv::cvtColor(img, img, CV_BGR2GRAY);
  }

  const char* typeStr = (_params.useFloatInput ? "FLOAT" : "UINT8");

  if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.Detect.Resizing", "From [%dx%dx%d] image to [%dx%dx%d] %s tensor",
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
      LOG_INFO("NeuralNetModel.Detect.SkipResize", "Skipping actual resize: image already correct size");
    }
    DEV_ASSERT(img.isContinuous(), "NeuralNetModel.Detect.ImageNotContinuous");

    imageTensor = tensorflow::Tensor(tensorflow::DT_FLOAT, {
      1, _params.inputHeight, _params.inputWidth, img.channels()
    });

    // Scale/shift resized image directly into the tensor data    
    const auto cvType = (img.channels() == 1 ? CV_32FC1 : CV_32FC3);
    
    cv::Mat cvTensor(_params.inputHeight, _params.inputWidth, cvType,
                     imageTensor.tensor<float, 4>().data());

    img.convertTo(cvTensor, cvType, 1.f/_params.inputScale, _params.inputShift);
  
  }
  else 
  {
    imageTensor = tensorflow::Tensor(tensorflow::DT_UINT8, {
      1, _params.inputHeight, _params.inputWidth, img.channels()
    });

    // Resize uint8 input image directly into the uint8 tensor data    
    cv::Mat cvTensor(_params.inputHeight, _params.inputWidth, 
                     (img.channels() == 1 ? CV_8UC1 : CV_8UC3),
                     imageTensor.tensor<uint8_t, 4>().data());

    cv::resize(img, cvTensor, cv::Size(_params.inputWidth, _params.inputHeight), 0, 0, kResizeMethod);
  }
    
  if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.Detect.RunningSession", "Input=[%dx%dx%d], %s, %d output(s)",
             img.cols, img.rows, img.channels(), typeStr, (int)_params.outputLayerNames.size());
  }

  std::vector<tensorflow::Tensor> outputTensors;
  if (RESULT_FAIL == Run(imageTensor, outputTensors))
  {
    return RESULT_FAIL;
  }

  switch(_params.outputType)
  {
    case OutputType::Classification:
    {
      GetClassification(outputTensors[0], t, salientPoints);
      break;
    }
    case OutputType::BinaryLocalization:
    {
      GetLocalizedBinaryClassification(outputTensors[0], t, salientPoints);
      break;
    }
    case OutputType::AnchorBoxes:
    {
      GetDetectedObjects(outputTensors, t, salientPoints);  
      break;
    }
    case OutputType::Segmentation:
    {
      // TODO (robert) need to convert the segmenation response map
      // into salient points
      break;
    }
  }

  if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.Detect.SessionComplete", "");
  }

  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Run(tensorflow::Tensor imageTensor,
                                       std::vector<tensorflow::Tensor>& outputTensors)
{
  tensorflow::Status runStatus;
  if (0 == _params.benchmarkRuns)
  {
    runStatus = _session->Run({{_params.inputLayerName, imageTensor}},
                              _params.outputLayerNames, {}, &outputTensors);
  }
  else
  {
    std::unique_ptr<tensorflow::StatSummarizer> stats;
    tensorflow::StatSummarizerOptions stats_options;
    stats_options.show_run_order = true;
    stats_options.run_order_limit = 0;
    stats_options.show_time = true;
    stats_options.time_limit = 10;
    stats_options.show_memory = true;
    stats_options.memory_limit = 10;
    stats_options.show_type = true;
    stats_options.show_summary = true;
    stats.reset(new tensorflow::StatSummarizer(stats_options));

    tensorflow::RunOptions run_options;
    if (nullptr != stats)
    {
      run_options.set_trace_level(tensorflow::RunOptions::FULL_TRACE);
    }

    tensorflow::RunMetadata run_metadata;
    for (uint32_t i = 0; i < _params.benchmarkRuns; ++i)
    {
      runStatus = _session->Run(run_options, {{_params.inputLayerName, imageTensor}},
                                _params.outputLayerNames, {}, &outputTensors, &run_metadata);
      if (!runStatus.ok())
      {
        break;
      }
      if (nullptr != stats)
      {
        assert(run_metadata.has_step_stats());
        const tensorflow::StepStats& step_stats = run_metadata.step_stats();
        stats->ProcessStepStats(step_stats);
      }
    }
    // Print all the stats to the logs
    // Note: right now all the stats are accummlated so for an average,
    // division by benchmarkRuns (which shows up as count in the tensorflow
    // stat summary) is neccesary.
    stats->PrintStepStats();
  }

  if (!runStatus.ok())
  {
    PRINT_NAMED_ERROR("ObjectDetector.Detect.Run.DetectionSessionRunFail", "%s", runStatus.ToString().c_str());
    return RESULT_FAIL;
  }
  else
  {
    return RESULT_OK;
  }
}

} // namespace Anki
