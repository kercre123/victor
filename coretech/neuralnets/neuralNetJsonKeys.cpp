/**
 * File: neuralNetJsonKeys.cpp
 *
 * Author: Andrew Stein
 * Date:   10/25/2018
 *
 * Description: Commonly used JsonKeys for interacting with Json-configured neural nets.
 *              Avoids hard-coded strings littering code everywhere.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/neuralnets/neuralNetJsonKeys.h"

namespace Anki {
namespace NeuralNets {

namespace JsonKeys {
  
  const char* const NeuralNets       = "NeuralNets";
  const char* const Models           = "Models";
  
  const char* const NetworkName      = "networkName";
  const char* const GraphFile        = "graphFile";
  const char* const InputHeight      = "inputHeight";
  const char* const InputWidth       = "inputWidth";
  const char* const ModelType        = "modelType";
  const char* const PollingPeriod    = "pollPeriod_ms";
  const char* const TimeoutDuration  = "timeoutDuration_sec";
  const char* const VisualizationDir = "visualizationDirectory";
  
  const char* const OffboardModelType = "offboard";
  const char* const TFLiteModelType   = "TFLite";
}
  
} // namespace NeuralNets
} // namespace Anki
