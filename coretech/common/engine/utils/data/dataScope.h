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

#ifndef _Anki_Common_Basestation_Utils_Data_DataScope_H__
#define _Anki_Common_Basestation_Utils_Data_DataScope_H__

namespace Anki {
namespace Util {
namespace Data {

enum class Scope {
  Persistent,
  Resources,
  Cache,
  CurrentGameLog
};


} // end namespace Data
} // end namespace Util
} // end namespace Anki




#endif //_Anki_Common_Basestation_Utils_Data_DataScope_H__
