/**
* File: faceInfoScreenTypes.h
*
* Author: Kevin Yoon
* Created: 03/02/2018
*
* Description: Types of Customer Support Info / Debug face screens
*
* Copyright: Anki, Inc. 2018
*
*/

#ifndef __AnimProcess_CozmoAnim_FaceDisplay_FaceInfoScreenTypes_H_
#define __AnimProcess_CozmoAnim_FaceDisplay_FaceInfoScreenTypes_H_

namespace Anki {
namespace Cozmo {
  
// The names of all the screens that are supported
enum class ScreenName {
  None = 0,
  FAC  = 1, // Needs to be after None

  Recovery,

  Pairing,
    
  Main,
  ClearUserData,
  ClearUserDataFail,
  SelfTest,
  Network,
  SensorInfo,
  IMUInfo,
  MotorInfo,
  Camera,
  MicInfo,
  MicDirectionClock,
  CustomText,
  
  Count
};

} // namespace Cozmo
} // namespace Anki

#endif // __AnimProcess_CozmoAnim_FaceDisplay_FaceInfoScreenTypes_H_
