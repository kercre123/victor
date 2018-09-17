#include "coretech/neuralnets/neuralNetParams.h"

#include "util/helpers/quoteMacro.h"
#include "util/logging/logging.h"

namespace Anki {
namespace NeuralNets {
  
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
Result NeuralNetParams::SetFromConfig(const Json::Value& config)
{
# define GetFromConfig(keyName) \
  if(!config.isMember(QUOTE(keyName))) \
  { \
    PRINT_NAMED_ERROR("NeuralNetParams.SetFromConfig.MissingConfig", QUOTE(keyName)); \
    return RESULT_FAIL; \
  } \
  else \
  { \
    SetFromConfigHelper(config[QUOTE(keyName)], keyName); \
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

  if (config.isMember("visualizationDirectory"))
  {
    GetFromConfig(visualizationDirectory);
  }
  
  if("ssd_mobilenet" == architecture)
  {
    inputLayerName = "image_tensor";
    outputLayerNames = {"detection_scores", "detection_classes", "detection_boxes", "num_detections"};
    useFloatInput = false;
    outputType = OutputType::AnchorBoxes;
    
    if(config.isMember("outputType")) {
      PRINT_NAMED_WARNING("NeuralNetParams.SetFromConfig.IgnoringOutputType",
                          "Ignoring outputType and using 'AnchorBoxes' because architecture='ssd_mobilenet' was specified");
    }
  }
  else if(("mobilenet" == architecture) || ("mobilenet_v1" == architecture))
  {
    inputLayerName = "input";
    outputLayerNames = {"MobilenetV1/Predictions/Softmax"};
    useFloatInput = true;
    outputType = OutputType::Classification;
    
    if(config.isMember("outputType")) {
      PRINT_NAMED_WARNING("NeuralNetParams.SetFromConfig.IgnoringOutputType",
                          "Ignoring outputType and using 'Classification' because architecture='mobilenet' was specified");
    }
  }
  else if("custom" == architecture)
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
      SetFromConfigHelper(config["useGrayscale"], useGrayscale);
    }
  }
  else
  {
    PRINT_NAMED_ERROR("NeuralNetParams.SetFromConfig.UnrecognizedArchitecture", "%s",
                      architecture.c_str());
    return RESULT_FAIL;
  }
  
  if(verbose)
  {
    std::string outputNames;
    for(auto const& outputLayerName : outputLayerNames)
    {
      outputNames += outputLayerName + " ";
    }
    
    LOG_INFO("NeuralNetParams.SetFromConfig.Summary", "Arch: %s, %s Input: %s, Outputs: %s",
             architecture.c_str(), (useGrayscale ? "Grayscale" : "Color"),
             inputLayerName.c_str(), outputNames.c_str());
  }
  
  if(useFloatInput)
  {
    // NOTE: Only used when processing in floating point
    GetFromConfig(inputShift);
    GetFromConfig(inputScale);
  }
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetParams::SetOutputTypeFromConfig(const Json::Value& config)
{
  // Convert outputType to enum and validate number of outputs
  if(!config.isMember("outputType")) {
    PRINT_NAMED_ERROR("NeuralNetParams.SetOutputTypeFromConfig.MissingOutputType",
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
    PRINT_NAMED_ERROR("NeuralNetParams.SetOutputTypeFromConfig.BadOutputType", "Valid types: %s", validKeys.c_str());
    return RESULT_FAIL;
  }
  else {
    
    if(outputLayerNames.size() != iter->second.numOutputs)
    {
      PRINT_NAMED_ERROR("NeuralNetParams.SetOutputTypeFromConfig.WrongNumberOfOutputs",
                        "OutputType %s requires %d outputs (%d provided)",
                        iter->first.c_str(), iter->second.numOutputs, (int)outputLayerNames.size());
      return RESULT_FAIL;
    }
    outputType = iter->second.type;
    
    if(OutputType::BinaryLocalization == outputType)
    {
      GetFromConfig(numGridRows);
      GetFromConfig(numGridCols);
    }
  }
  
  return RESULT_OK;
}

} // namespace NeuralNets
} // namespace Anki
