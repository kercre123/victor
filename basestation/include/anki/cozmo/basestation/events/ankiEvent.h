/**
 * File: ankiEvent.h
 *
 * Author: Lee Crippen
 * Created: 07/30/15
 *
 * Description: Events that contain a simple type and a templatized data parameter.
 *
 * Copyright: Anki, Inc. 2015
 *
 * COZMO_PUBLIC_HEADER
 **/

#ifndef ANKI_COZMO_EVENT_H
#define ANKI_COZMO_EVENT_H

#include "anki/cozmo/shared/cozmoTypes.h"


namespace Anki {
namespace Cozmo {

template <typename DataType>
class AnkiEvent
{
public:
  template <typename FwdType>
  AnkiEvent(u32 type, FwdType&& newData)
  : _myType(type)
  , _data( std::forward<FwdType>(newData) )
{ }
  
  u32 GetType() const { return _myType; }
  const DataType& GetData() const { return _data; }
  
protected:
  
  u32 _myType;
  DataType _data;
  
}; // class Event


} // namespace Cozmo
} // namespace Anki

#endif //  ANKI_COZMO_EVENT_H