/*******************************************************************************************************************************
 *
 *  BaseStationEventDefinitions.cpp
 *  BaseStation
 *
 *  Created by Jarrod Hatfield on 1/17/14.
 *  Copyright (c) 2014 Anki. All rights reserved.
 *
 *******************************************************************************************************************************/

#include "anki/cozmo/basestation/events/BaseStationEventDefinitions.h"
#include "anki/cozmo/basestation/events/BaseStationEvent.h"

namespace Anki {
  namespace Cozmo {

//==============================================================================================================================
// These macros expand into the definitions of our required functions for each event defined in BaseStationEventDefinitions.def
#define EVENT_BEGIN( name, args... ) \
  void BSE_##name::Register( IBaseStationEventListener* observer ) \
  { \
    BaseStationEventDispatcher::Instance().RegisterEventListener( BSETYPE_##name, observer ); \
  } \
  void BSE_##name::Unregister( IBaseStationEventListener* observer ) \
  { \
    BaseStationEventDispatcher::Instance().UnregisterEventListener( BSETYPE_##name, observer ); \
  } \
  void BSE_##name::RaiseEvent( args ) \
  { \
    BSE_##name* event = new BSE_##name();
  
#define EVENT_ARG( type, name ) \
    event->name##_ = name;
  
#define EVENT_END( name ) \
    BaseStationEventDispatcher::Instance().EventRaised( event, BSED_IMMEDIATE ); \
  }

#include "anki/cozmo/basestation/events/BaseStationEventDefinitions.def"

#undef EVENT_END
#undef EVENT_ARG
#undef EVENT_BEGIN

  } // namespace Cozmo
} // namespace Anki