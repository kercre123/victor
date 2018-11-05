/**
 * File: neuralNetFilenames.h
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

#ifndef __Anki_CoreTech_NeuralNets_Filenames_H__
#define __Anki_CoreTech_NeuralNets_Filenames_H__

namespace Anki {
namespace NeuralNets {

namespace Filenames {
  extern const char* const Timestamp;
  extern const char* const Image;
  extern const char* const Result;
}
  
} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_CoreTech_NeuralNets_Filenames_H__ */
