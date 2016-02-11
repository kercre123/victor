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
  
} // namespace
} // namespace

#endif //
