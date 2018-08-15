/**
 * File: aiInformationAnalysisProcessTypes
 *
 * Author: Raul
 * Created: 09/13/16
 *
 * Description: Enumerations and utils for the processes registered in the AIInformationAnalyzer.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_AIInformationAnalysisProcessTypes_H__
#define __Cozmo_Basestation_AIInformationAnalysisProcessTypes_H__

namespace Anki {
namespace Vector {
namespace AIInformationAnalysis {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// registered processes
enum class EProcess {
  Invalid,                    // not a valid process
  CalculateInterestingRegions // calculate regions of interesting borders in the memory map
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// helper to return EProcess from string
EProcess EProcessFromString(const char* inString);
const char* StringFromEProcess(EProcess process);

} // namespace AIInformationAnalysis
} // namespace Vector
} // namespace Anki

#endif //
