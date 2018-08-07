/**
* File: timerUtilityDevFunctions.h
*
* Author: Kevin M. Karol
* Created: 3/15/18
*
* Description: Dev functions related to the timer utility.
* Implementations are split across the coordinator behavior and the timer utility
* component within the AI system. Functions declared here so they're exposed to each other
* and provide a centeral reference point for Devs working with this system
*
* Copyright: Anki, Inc. 2018
*
**/

#ifndef __Cozmo_Basestation_BehaviorSystem_TimerUtilityDevFunctions_H__
#define __Cozmo_Basestation_BehaviorSystem_TimerUtilityDevFunctions_H__

namespace Anki {
namespace Vector {

void AdvanceAnticBySeconds(int seconds);
void AdvanceTimerBySeconds(int seconds);

} // namespace Vector
} // namespace Anki

#endif // __Cozmo_Basestation_BehaviorSystem_TimerUtilityDevFunctions_H__
