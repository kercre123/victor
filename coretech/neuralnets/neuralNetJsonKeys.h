/**
 * File: neuralNetJsonKeys.h
 *
 * Author: Andrew Stein
 * Date:   10/25/2018
 *
 * Description: Commonly used JsonKeys for interacting with Json-configured neural nets.
 *              Avoids hard-coded strings littering code everywhere.
 *
 * Copyright: Anki, Inc. 2018
 **/

#ifndef __Anki_CoreTech_NeuralNets_JsonKeys_H__
#define __Anki_CoreTech_NeuralNets_JsonKeys_H__

namespace Anki {
namespace NeuralNets {

namespace JsonKeys {
  
  extern const char* const NeuralNets;
  extern const char* const Models;
  
  extern const char* const NetworkName;
  extern const char* const GraphFile;
  extern const char* const InputHeight;
  extern const char* const InputWidth;
  extern const char* const ModelType;
  extern const char* const PollingPeriod;
  extern const char* const TimeoutDuration;
  extern const char* const VisualizationDir;
  
  // Model types:
  extern const char* const OffboardModelType;
  extern const char* const TFLiteModelType;
  
}
  
} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_CoreTech_NeuralNets_JsonKeys_H__ */
