/**
 * File: memoryMapData.h
 *
 * Author: Raul
 * Date:   08/02/2016
 *
 * Description: Base for data structs that will be held in every node depending on their content type.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_MEMORY_MAP_DATA_H
#define ANKI_COZMO_MEMORY_MAP_DATA_H

#include "engine/navMap/memoryMap/memoryMapTypes.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// NavMemoryMapQuadData
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MemoryMapData
{
public:
  const MemoryMapTypes::EContentType type;

  // base_of_five_defaults - destructor
  MemoryMapData(MemoryMapTypes::EContentType type, TimeStamp_t time);
  MemoryMapData(const MemoryMapData&) = default;
  MemoryMapData(MemoryMapData&&) = default;
  MemoryMapData& operator=(const MemoryMapData&) = default;
  MemoryMapData& operator=(MemoryMapData&&) = default;
  ~MemoryMapData() {}
  
  // create a copy of self (of appropriate subclass) and return it
  virtual MemoryMapData* Clone() const { return new MemoryMapData(*this); };
  
  // compare to MemoryMapData and return bool if the data stored is the same
  virtual bool Equals(const MemoryMapData* other) const { return (type == other->type); }
  
  void SetLastObservedTime(TimeStamp_t t) {lastObserved_ms = t;}
  void SetFirstObservedTime(TimeStamp_t t) {firstObserved_ms = t;}
  TimeStamp_t GetLastObservedTime()  const {return lastObserved_ms;}
  TimeStamp_t GetFirstObservedTime() const {return firstObserved_ms;}
  
private:
  TimeStamp_t firstObserved_ms;
  TimeStamp_t lastObserved_ms;
};

inline MemoryMapData::MemoryMapData(MemoryMapTypes::EContentType type, TimeStamp_t time)
  : type(type)
  , firstObserved_ms(time)
  , lastObserved_ms(time) {}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <class T>
T* MemoryMapDataCast(MemoryMapData* ptr)
{
  DEV_ASSERT( ptr, "MemoryMapDataCast.NullQuadData" );
  DEV_ASSERT( ptr->type == T().type, "MemoryMapDataCast.UnexpectedQuadData" );
  DEV_ASSERT( dynamic_cast<T*>(ptr), "MemoryMapDataCast.BadQuadDataDynCast" );
  return static_cast<T*>(ptr);
}

template <class T>
const T* MemoryMapDataCast(const MemoryMapData* ptr)
{
  assert( ptr->type == T().type );
  assert( dynamic_cast<const T*>(ptr) );
  return static_cast<const T*>(ptr);
}
  
} // namespace
} // namespace

#endif //
