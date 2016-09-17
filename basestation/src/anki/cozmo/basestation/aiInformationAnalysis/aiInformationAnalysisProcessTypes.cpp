/**
 * File: AIInformationAnalysisProcessTypes
 *
 * Author: Raul
 * Created: 09/13/16
 *
 * Description: Enumerations and utils for the processes registered in the AIInformationAnalyzer.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#include "AIInformationAnalysisProcessTypes.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {
namespace AIInformationAnalysis {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EProcess EProcessFromString(const char* inString)
{
  if ( inString == std::string("CalculateInterestingRegions") )
  {
    return EProcess::CalculateInterestingRegions;
  }
  
  ASSERT_NAMED_EVENT(false, "AIInformationAnalysis.EProcessFromString.Fail", "'%s' is not a valid EProcess", inString);
  return EProcess::Invalid;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
const char* StringFromEProcess(EProcess process)
{
  switch(process)
  {
    case EProcess::Invalid: { return "Invalid"; }
    case EProcess::CalculateInterestingRegions: { return "CalculateInterestingRegions"; }
  }

  ASSERT_NAMED_EVENT(false, "AIInformationAnalysis.EProcessFromString.Fail", "'%d' is not a valid EProcess", (int)process);
  return "ERROR";
}

} // namespace
} // namespace
} // namespace
