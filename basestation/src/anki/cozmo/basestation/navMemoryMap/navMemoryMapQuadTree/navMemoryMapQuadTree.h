/**
 * File: navMemoryMapQuadTree.h
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

#include "anki/cozmo/basestation/navMemoryMap/iNavMemoryMap.h"
#include "navMeshQuadTree.h"

namespace Anki {
namespace Cozmo {
  
class VizManager;

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class NavMemoryMapQuadTree : public INavMemoryMap
{
public:

  using EContentType = NavMemoryMapTypes::EContentType;
  using FullContentArray = NavMemoryMapTypes::FullContentArray;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Construction/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  NavMemoryMapQuadTree(VizManager* vizManager);
  virtual ~NavMemoryMapQuadTree() {}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // From INavMemoryMap
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // merge the given map into this map by applying to the other's information the given transform
  // although this methods allows merging any INavMemoryMap into any INavMemoryMap, subclasses are not
  // expected to provide support for merging other subclasses, but only other instances from the same
  // subclass
  virtual void Merge(const INavMemoryMap* other, const Pose3d& transform) override;
  
  // change the content type from typeToReplace into newTypeSet if there's a border from any of the typesToFillFrom towards typeToReplace
  virtual void FillBorderInternal(EContentType typeToReplace, const FullContentArray& neighborsToFillFrom, EContentType newTypeSet) override;
  
  // return the size of the area currently explored
  virtual double GetExploredRegionAreaM2() const override;
  
  // check whether the given content types would have any borders at the moment. This method is expected to
  // be faster than CalculateBorders for the same innerType/outerType combination, since it only queries
  // whether a border exists, without requiring calculating all of them
  virtual bool HasBorders(EContentType innerType, const FullContentArray& outerTypes) const override;
  
  // retrieve the borders currently found in the map between the given types. This query is not const
  // so that the memory map can calculate and cache values upon being requested, rather than when
  // the map is modified. Function is expected to clear the vector before returning the new borders
  virtual void CalculateBorders(EContentType innerType, const FullContentArray& outerTypes, BorderVector& outBorders) override;
  
  // checks if the given ray collides with the given type (any quad with that type)
  virtual bool HasCollisionRayWithTypes(const Point2f& rayFrom, const Point2f& rayTo, const FullContentArray& types) const override;
  
  // returns true if there are any nodes of the given type, false otherwise
  virtual bool HasContentType(EContentType type) const override;
  
  // Draw/stop drawing the memory map
  virtual void Draw(size_t mapIdxHint) const override;
  virtual void ClearDraw() const override;
  
protected:

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // From INavMemoryMap
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

  // add a quad with the specified content
  virtual void AddQuadInternal(const Quad2f& quad, EContentType type) override;
  virtual void AddQuadInternal(const Quad2f& quad, const INavMemoryMapQuadData& content) override;
  
  // add a line with the specified content
  virtual void AddLineInternal(const Point2f& from, const Point2f& to, EContentType type) override;
  virtual void AddLineInternal(const Point2f& from, const Point2f& to, const INavMemoryMapQuadData& content) override;
  
  // add a triangle with the specified content
  virtual void AddTriangleInternal(const Triangle2f& tri, EContentType type) override;
  virtual void AddTriangleInternal(const Triangle2f& tri, const INavMemoryMapQuadData& content) override;

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
