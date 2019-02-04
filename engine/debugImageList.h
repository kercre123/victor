/**
 * File: debugImageList.h
 *
 * Author: Andrew Stein
 * Date:   2017-04-16
 *
 * Description:   Container for passing around debugging images paired with their debug name.
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Anki_Cozmo_Basestation_DebugImageList_H__
#define __Anki_Cozmo_Basestation_DebugImageList_H__

#include <list>
#include <string>

namespace Anki {
namespace Vector {

  template<class ImageType>
  using DebugImageList = std::list<std::pair<std::string, ImageType>>;

} // namespace Vector
} // namespace Anki

#endif /* __Anki_Cozmo_Basestation_DebugImageList_H__ */
