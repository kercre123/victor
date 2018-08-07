/**
 * File: aiInfoAnalysisProcCalculateInterestingRegions
 *
 * Author: Raul
 * Created: 09/13/16
 *
 * Description: Process to analyze memory map and generate information about the interesting regions.
 *
 * Copyright: Anki, Inc. 2016
 *
 **/
#ifndef __Cozmo_Basestation_AIInfoAnalysisProcCalculateInterestingRegions_H__
#define __Cozmo_Basestation_AIInfoAnalysisProcCalculateInterestingRegions_H__

namespace Anki {
namespace Vector {

class AIInformationAnalyzer;
class Robot;

// calculates interesting regions in the memory map
void AIInfoAnalysisProcCalculateInterestingRegions(AIInformationAnalyzer& analyzer, Robot& robot);

} // namespace Vector
} // namespace Anki

#endif //
