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
#include "engine/aiComponent/aiInformationAnalysis/aiInformationAnalysisProcessTypes.h"

#include "util/logging/logging.h"

namespace Anki {
namespace Vector {
namespace AIInformationAnalysis {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
EProcess EProcessFromString(const char* inString)
{
  if ( inString == std::string("CalculateInterestingRegions") )
  {
    return EProcess::CalculateInterestingRegions;
  }
  
  DEV_ASSERT_MSG(false, "AIInformationAnalysis.EProcessFromString.Fail", "'%s' is not a valid EProcess", inString);
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

  DEV_ASSERT_MSG(false, "AIInformationAnalysis.EProcessFromString.Fail", "'%d' is not a valid EProcess", (int)process);
  return "ERROR";
}

} // namespace
} // namespace
} // namespace
