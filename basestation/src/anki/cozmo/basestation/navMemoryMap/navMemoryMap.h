/**
 * File: navMemoryMap.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: Map of the space navigated by the robot with some memory features (like decay = forget).
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_NAV_MEMORY_MAP_H
#define ANKI_COZMO_NAV_MEMORY_MAP_H

#include "anki/cozmo/basestation/navMemoryMap/navMemoryMapInterface.h"
#include "navMeshQuadTree.h"

namespace Anki {
namespace Cozmo {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NavMemoryMap : public INavMemoryMap
{
public:

  using EContentType = NavMemoryMapTypes::EContentType;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Construction/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  NavMemoryMap();
  virtual ~NavMemoryMap() {}  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // From INavMemoryMap
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // add a quad that with the specified content type
  virtual void AddQuad(const Quad2f& quad, EContentType type) override;
  
  // retrieve the borders currently found in the map between the given types. This query is not const
  // so that the memory map can calculate and cache values upon being requested, rather than when
  // the map is modified. Function is expected to clear the vector before returning the new borders
  virtual void CalculateBorders(EContentType innerType, EContentType outerType, BorderVector& outBorders) override;

  // Draw the memory map
  virtual void Draw() const override;
  
private:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // underlaying nav mesh representation
  NavMeshQuadTree _navMesh;
  
}; // class
  
} // namespace
} // namespace

#endif // ANKI_COZMO_NAV_MEMORY_MAP_H
