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
protected:
  using MemoryMapDataPtr      = MemoryMapTypes::MemoryMapDataPtr;
  using MemoryMapDataConstPtr = MemoryMapTypes::MemoryMapDataConstPtr;
  using EContentType          = MemoryMapTypes::EContentType;

public:
  const EContentType type;

  // base_of_five_defaults - destructor
  MemoryMapData(EContentType type = EContentType::Unknown, TimeStamp_t time = 0.f) : MemoryMapData(type, time, false) {}
  MemoryMapData(const MemoryMapData&) = default;
  MemoryMapData(MemoryMapData&&) = default;
  virtual ~MemoryMapData() = default;
  
  // create a copy of self (of appropriate subclass) and return it
  virtual MemoryMapDataPtr Clone() const { return MemoryMapDataPtr(*this); };
  
  // compare to MemoryMapData and return bool if the data stored is the same
  virtual bool Equals(const MemoryMapData* other) const { return (type == other->type); }
  
  void SetLastObservedTime(TimeStamp_t t) {lastObserved_ms = t;}
  void SetFirstObservedTime(TimeStamp_t t) {firstObserved_ms = t;}
  TimeStamp_t GetLastObservedTime()  const {return lastObserved_ms;}
  TimeStamp_t GetFirstObservedTime() const {return firstObserved_ms;}
  
  // returns true if this node can be replaced by the given content type
  bool CanOverrideSelfWithContent(EContentType newContentType) const;
  
  // Wrappers for data casting operations  
  template <class T>
  static MemoryMapDataWrapper<T> MemoryMapDataCast(MemoryMapDataPtr ptr);
  
  template <class T>
  static MemoryMapDataWrapper<const T> MemoryMapDataCast(MemoryMapDataConstPtr ptr);
  
protected:
  MemoryMapData(MemoryMapTypes::EContentType type, TimeStamp_t time, bool calledFromDerived);
  
  static bool HandlesType(MemoryMapTypes::EContentType otherType) {
    return (otherType != MemoryMapTypes::EContentType::ObstacleProx) &&
           (otherType != MemoryMapTypes::EContentType::Cliff) &&
           (otherType != MemoryMapTypes::EContentType::ObstacleObservable);
  }

private:
  TimeStamp_t firstObserved_ms;
  TimeStamp_t lastObserved_ms;
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
inline MemoryMapDataWrapper<T> MemoryMapData::MemoryMapDataCast(MemoryMapDataPtr ptr)
{
  DEV_ASSERT( ptr.GetSharedPtr(), "MemoryMapDataCast.NullQuadData" );
  DEV_ASSERT( T::HandlesType( ptr->type ), "MemoryMapDataCast.UnexpectedQuadData" );
  DEV_ASSERT( std::dynamic_pointer_cast<T>(ptr.GetSharedPtr()), "MemoryMapDataCast.BadQuadDataDynCast" );
  return MemoryMapDataWrapper<T>(std::static_pointer_cast<T>(ptr.GetSharedPtr()));
}

template <class T>
inline MemoryMapDataWrapper<const T> MemoryMapData::MemoryMapDataCast(MemoryMapDataConstPtr ptr)
{
  assert( T::HandlesType( ptr->type ) );
  assert( std::dynamic_pointer_cast<const T>(ptr.GetSharedPtr()) );
  return MemoryMapDataWrapper<const T>(std::static_pointer_cast<const T>(ptr.GetSharedPtr()));
}
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
inline bool MemoryMapData::CanOverrideSelfWithContent(EContentType newContentType) const
{
  EContentType dataType = type;
  if ( newContentType == EContentType::Cliff )
  {
    // Cliff can override any other
    return true;
  }
  else if ( dataType == EContentType::Cliff )
  {
    // Cliff can only be overridden by a full ClearOfCliff (the cliff is gone)
    const bool isTotalClear = (newContentType == EContentType::ClearOfCliff);
    return isTotalClear;
  }
  else if ( newContentType == EContentType::ClearOfObstacle )
  {
    // ClearOfObstacle current comes from vision or prox sensor having a direct line of sight
    // to some object, so it can't clear negative space obstacles (IE cliffs). Additionally,
    // ClearOfCliff is currently a superset of Clear of Obstacle, so trust ClearOfCliff flags.
    const bool isTotalClear = ( dataType != EContentType::Cliff ) &&
                              ( dataType != EContentType::ClearOfCliff );
    return isTotalClear;
  }
  else if ( newContentType == EContentType::InterestingEdge )
  {
    // InterestingEdge can only override basic node types, because it would cause data loss otherwise. For example,
    // we don't want to override a recognized marked cube or a cliff with their own border
    if ( ( dataType == EContentType::ObstacleObservable   ) ||
         ( dataType == EContentType::ObstacleCharger      ) ||
         ( dataType == EContentType::ObstacleUnrecognized ) ||
         ( dataType == EContentType::Cliff                ) ||
         ( dataType == EContentType::NotInterestingEdge   ) )
    {
      return false;
    }
  }
  else if ( newContentType == EContentType::ObstacleProx )
  {
    if ( ( dataType == EContentType::ObstacleObservable   ) ||
         ( dataType == EContentType::ObstacleCharger      ) ||
         ( dataType == EContentType::Cliff                ) )
    {
      return false;
    }
  }
  else if ( newContentType == EContentType::NotInterestingEdge )
  {
    // NotInterestingEdge can only override interesting edges
    if ( dataType != EContentType::InterestingEdge ) {
      return false;
    }
  }
  else if ( newContentType == EContentType::ObstacleChargerRemoved )
  {
    // ObstacleChargerRemoved can only remove ObstacleCharger
    if ( dataType != EContentType::ObstacleCharger ) {
      return false;
    }
  }
  
  return true;
}

} // namespace
} // namespace

#endif //
