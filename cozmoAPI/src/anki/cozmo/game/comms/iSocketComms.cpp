/**
 * File: iSocketComms
 *
 * Author: Mark Wesley
 * Created: 05/14/16
 *
 * Description: Interface for any socket-based communications from e.g. Game/SDK to Engine
 *
 * Copyright: Anki, Inc. 2016
 *
 **/



#include "anki/cozmo/game/comms/iSocketComms.h"


namespace Anki {
namespace Cozmo {


ISocketComms::ISocketComms(UiConnectionType connectionType)
  : _pingCounter(0)
{
}


ISocketComms::~ISocketComms()
{
}

  
} // namespace Cozmo
} // namespace Anki

