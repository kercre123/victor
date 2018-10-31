/**
 * File: neuralNetFilenames.cpp
 *
 * Author: Andrew Stein
 * Date:   10/25/2018
 *
 * Description: Commonly used filenames for interacting with neural nets running as a process.
 *              Not needed if we switch to real shared memory for IPC or put the neural nets
 *              back into the engine process.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/neuralnets/neuralNetFilenames.h"

namespace Anki {
namespace NeuralNets {

namespace Filenames {

  const char* const Timestamp = "timestamp.txt";
  const char* const Image     = "neuralNetImage.png";
  const char* const Result    = "neuralNetResults.json";
  
}
  
} // namespace NeuralNets
} // namespace Anki
