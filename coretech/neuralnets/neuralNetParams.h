#ifndef __Anki_NeuralNets_NeuralNetParams_H__
#define __Anki_NeuralNets_NeuralNetParams_H__

#include "coretech/common/shared/types.h"

#include "json/json.h"

namespace Anki {
namespace NeuralNets {

struct NeuralNetParams
{
  // Specified in config, used to determine how to interpret the output of the network, and
  // thus which helper above to call (string to use in JSON config shown for each)
  enum class OutputType {
    Classification,     // "classification"
    BinaryLocalization, // "binary_localization"
    AnchorBoxes,        // "anchor_boxes"
    Segmentation,       // "segmentation"
  };
  
  std::string               graphFile;
  std::string               labelsFile;
  std::string               architecture;
  
  int32_t                   inputWidth = 128;
  int32_t                   inputHeight = 128;
  
  // When input to graph is float, data is first divided by scale and then shifted
  // I.e.:  float_input = data / scale + shift
  float                     inputShift = 0.f;
  float                     inputScale = 255.f;
  
  float                     minScore = 0.5f; // in [0,1]
  
  // Used by "custom" architectures
  std::string               inputLayerName;
  std::vector<std::string>  outputLayerNames;
  OutputType                outputType;
  bool                      useFloatInput = false;
  bool                      useGrayscale = false;
  
  // For "binary_localization" output_type, the localization grid resolution
  // (ignored otherwise)
  int32_t                   numGridRows = 6;
  int32_t                   numGridCols = 6;
  
  bool                      verbose = false;
  
  // Number of times to run benchmarking and report to logs, if this value
  // is zero benchmarking will not be run
  int32_t                   benchmarkRuns = 0;
  
  // TODO: Not fully supported yet (VIC-3141)
  bool                      memoryMapGraph = false;
  
  std::string               visualizationDirectory = ""; 

  // Populate from Json config
  Result SetFromConfig(const Json::Value& config);
  
private:
  
  // Helper to set output_type enum
  // Must be called after input/output_layer_names have been set
  Result SetOutputTypeFromConfig(const Json::Value& config);
  
}; // struct NeuralNetParams
  
} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_NeuralNets_NeuralNetParams_H__ */

