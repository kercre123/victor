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
  
class VizManager;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NavMemoryMap : public INavMemoryMap
{
public:

  using EContentType = NavMemoryMapTypes::EContentType;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Construction/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  NavMemoryMap(VizManager* vizManager);
  virtual ~NavMemoryMap() {}  

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // From INavMemoryMap
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // add a quad that with the specified content type
  virtual void AddQuad(const Quad2f& quad, EContentType type) override;
  
  // check whether the given content types would have any borders at the moment. This method is expected to
  // be faster than CalculateBorders for the same innerType/outerType combination, since it only queries
  // whether a border exists, without requiring calculating all of them
  virtual bool HasBorders(EContentType innerType, EContentType outerType) const override;
  virtual bool HasBorders(EContentType innerType, std::set<EContentType> outerTypes) const override;
  
  // retrieve the borders currently found in the map between the given types. This query is not const
  // so that the memory map can calculate and cache values upon being requested, rather than when
  // the map is modified. Function is expected to clear the vector before returning the new borders
  virtual void CalculateBorders(EContentType innerType, EContentType outerType, BorderVector& outBorders) override;
  virtual void CalculateBorders(EContentType innerType, std::set<EContentType> outerTypes, BorderVector& outBorders) override;
  
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
