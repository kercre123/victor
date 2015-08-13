/**
* File: dataScope
*
* Author: damjan stulic
* Created: 8/5/15
*
* Description:
*
* Copyright: Anki, inc. 2015
*
*/

#ifndef _Anki_Cozmo_Basestation_Data_DataScope_H__
#define _Anki_Cozmo_Basestation_Data_DataScope_H__

namespace Anki {
namespace Cozmo {
namespace Data {

enum class Scope {
  Output,
  Resources,
  Cache,
  CurrentGameLog,
  External
};


} // end namespace Data
} // end namespace Cozmo
} // end namespace Anki




#endif //_Anki_Cozmo_Basestation_Data_DataScope_H__
