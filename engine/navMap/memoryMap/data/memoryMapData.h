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
  MemoryMapData(MemoryMapTypes::EContentType type, TimeStamp_t time) : MemoryMapData(type, time, false) {}
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
  
  // Wrappers for data casting operations  
  template <class T>
  static std::shared_ptr<T> MemoryMapDataCast(std::shared_ptr<MemoryMapData> ptr);
  
  template <class T>
  static std::shared_ptr<const T> MemoryMapDataCast(std::shared_ptr<const MemoryMapData> ptr);
  
protected:
  MemoryMapData(MemoryMapTypes::EContentType type, TimeStamp_t time, bool calledFromDerived);

private:
  TimeStamp_t firstObserved_ms;
  TimeStamp_t lastObserved_ms;
  
  // sneaky way of explicitly accessing protected constructors of derived classes for template casting operation
  template <class T>
  struct Concrete: T {
    Concrete(): T() {}
  };
};

inline MemoryMapData::MemoryMapData(MemoryMapTypes::EContentType type, TimeStamp_t time, bool expectsAdditionalData)
  : type(type)
  , firstObserved_ms(time)
  , lastObserved_ms(time) 
{
  // need to make sure we dont ever create a MemoryMapData without providing all information required.
  // This locks us from creating something like MemoryMapData_ObservableObject without the ID, for instance.
  DEV_ASSERT(ExpectsAdditionalData(type) == expectsAdditionalData, "MemoryMapData.ImproperConstructorCalled");
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <class T>
inline std::shared_ptr<T> MemoryMapData::MemoryMapDataCast(std::shared_ptr<MemoryMapData> ptr)
{
  DEV_ASSERT( ptr, "MemoryMapDataCast.NullQuadData" );
  DEV_ASSERT( ptr->type == Concrete<T>().type, "MemoryMapDataCast.UnexpectedQuadData" );
  DEV_ASSERT( std::dynamic_pointer_cast<T>(ptr), "MemoryMapDataCast.BadQuadDataDynCast" );
  return std::static_pointer_cast<T>(ptr);
}

template <class T>
inline std::shared_ptr<const T> MemoryMapData::MemoryMapDataCast(std::shared_ptr<const MemoryMapData> ptr)
{
  assert( ptr->type == Concrete<T>().type );
  assert( std::dynamic_pointer_cast<const T>(ptr) );
  return std::static_pointer_cast<const T>(ptr);
}
  
} // namespace
} // namespace

#endif //
