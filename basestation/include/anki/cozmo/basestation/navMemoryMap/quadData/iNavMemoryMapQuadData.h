/**
 * File: INavMemoryMapQuadData.h
 *
 * Author: Raul
 * Date:   08/02/2016
 *
 * Description: Base for data structs that will be held in every quad of content depending on their content type.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef ANKI_COZMO_NAV_MEMORY_MAP_QUAD_DATA_H
#define ANKI_COZMO_NAV_MEMORY_MAP_QUAD_DATA_H

#include "../navMemoryMapTypes.h"
#include "util/logging/logging.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// INavMemoryMapQuadData
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct INavMemoryMapQuadData
{
  const NavMemoryMapTypes::EContentType type;

  // base_of_five_defaults - destructor
  INavMemoryMapQuadData(const INavMemoryMapQuadData&) = default;
  INavMemoryMapQuadData(INavMemoryMapQuadData&&) = default;
  INavMemoryMapQuadData& operator=(const INavMemoryMapQuadData&) = default;
  INavMemoryMapQuadData& operator=(INavMemoryMapQuadData&&) = default;
  virtual ~INavMemoryMapQuadData() {}
  
  // create a copy of self (of appropriate subclass) and return it
  virtual INavMemoryMapQuadData* Clone() const = 0;
  
  // compare to INavMemoryMapQuadData and return bool if the data stored is the same
  virtual bool Equals(const INavMemoryMapQuadData* other) const = 0;
  
protected:
  INavMemoryMapQuadData(NavMemoryMapTypes::EContentType t) : type(t) {}
};

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Helper functions
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

template <class T>
T* INavMemoryMapQuadDataCast(INavMemoryMapQuadData* ptr)
{
  DEV_ASSERT( ptr, "INavMemoryMapQuadDataCast.NullQuadData" );
  DEV_ASSERT( ptr->type == T().type, "INavMemoryMapQuadDataCast.UnexpectedQuadData" );
  DEV_ASSERT( dynamic_cast<T*>(ptr), "INavMemoryMapQuadDataCast.BadQuadDataDynCast" );
  return static_cast<T*>(ptr);
}

template <class T>
const T* INavMemoryMapQuadDataCast(const INavMemoryMapQuadData* ptr)
{
  assert( ptr->type == T().type );
  assert( dynamic_cast<const T*>(ptr) );
  return static_cast<const T*>(ptr);
}
  
} // namespace
} // namespace

#endif //
