/**
 * File: memoryMap.h
 *
 * Author: Raul
 * Date:   12/09/2015
 *
 * Description: QuadTree map of the space navigated by the robot with some memory features (like decay = forget).
 *
 * Copyright: Anki, Inc. 2015
 **/

#ifndef ANKI_COZMO_MEMORY_MAP_H
#define ANKI_COZMO_MEMORY_MAP_H

#include "engine/navMap/iNavMap.h"
#include "engine/navMap/quadTree/quadTree.h"

#include <shared_mutex>

namespace Anki {
namespace Cozmo {
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
class MemoryMap : public INavMap
{
public:

  using EContentType     = MemoryMapTypes::EContentType;
  using FullContentArray = MemoryMapTypes::FullContentArray;
  using MemoryMapRegion  = MemoryMapTypes::MemoryMapRegion;

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Construction/Destruction
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  MemoryMap();
  virtual ~MemoryMap() {}

  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // From INavMemoryMap
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -  
  
  // add data to the memory map defined by poly
  virtual bool Insert(const Poly2f& poly, const MemoryMapData& data) override { return Insert(MemoryMapRegion( FastPolygon(poly) ), data); }
  virtual bool Insert(const MemoryMapRegion& r, const MemoryMapData& data) override;
  virtual bool Insert(const MemoryMapRegion& r, NodeTransformFunction transform) override;
  
  // merge the given map into this map by applying to the other's information the given transform
  // although this methods allows merging any INavMemoryMap into any INavMemoryMap, subclasses are not
  // expected to provide support for merging other subclasses, but only other instances from the same
  // subclass
  virtual bool Merge(const INavMap* other, const Pose3d& transform) override;
  
  // change the content type from typeToReplace into newData if there's a border from any of the neighborsToFillFrom towards typeToReplace
  virtual bool FillBorder(EContentType typeToReplace, const FullContentArray& neighborsToFillFrom, const MemoryMapDataPtr& newData) override;
  
  // fills inner regions satisfying innerPred( inner node ) && outerPred(neighboring node), converting
  // the inner region to the given data
  bool FillBorder(const NodePredicate& innerPred, const NodePredicate& outerPred, const MemoryMapDataPtr& data) override;
  
  // attempt to apply a transformation function to all nodes in the tree
  virtual bool TransformContent(NodeTransformFunction transform) override;
  
  // attempt to apply a transformation function to any node intersecting the poly
  virtual bool TransformContent(const MemoryMapRegion& poly, NodeTransformFunction transform) override;

  // populate a list of all data that matches the predicate
  virtual void FindContentIf(NodePredicate pred, MemoryMapDataConstList& output) const override;
  
  // populate a list of all data that matches the predicate inside poly
  virtual void FindContentIf(const Poly2f& poly, NodePredicate pred, MemoryMapDataConstList& output) const override;
  virtual void FindContentIf(const MemoryMapRegion& poly, NodePredicate pred, MemoryMapDataConstList& output) const override;

  
  // return the size of the area currently explored
  virtual double GetExploredRegionAreaM2() const override;
  // return the size of the area currently flagged as interesting edges
  virtual double GetInterestingEdgeAreaM2() const override;
  
  // returns the precision of content data in the memory map. For example, if you add a point, and later query for it,
  // the region that the point generated to store the point could have an error of up to this length.
  virtual float GetContentPrecisionMM() const override;
  
  // check whether the given content types would have any borders at the moment. This method is expected to
  // be faster than CalculateBorders for the same innerType/outerType combination, since it only queries
  // whether a border exists, without requiring calculating all of them
  virtual bool HasBorders(EContentType innerType, const FullContentArray& outerTypes) const override;
  
  // retrieve the borders currently found in the map between the given types. This query is not const
  // so that the memory map can calculate and cache values upon being requested, rather than when
  // the map is modified. Function is expected to clear the vector before returning the new borders
  virtual void CalculateBorders(EContentType innerType, const FullContentArray& outerTypes, BorderRegionVector& outBorders) override;
  
  // checks if the given polygon collides with the given types (any quad with that type)
  virtual bool HasCollisionWithTypes(const FastPolygon& poly, const FullContentArray& types) const override;
  
  // evaluates f along any node that the region collides with. returns true if any call to NodePredicate returns true
  virtual bool AnyOf(const Poly2f& p, NodePredicate f)             const override;
  virtual bool AnyOf(const MemoryMapRegion& p, NodePredicate f) const override;

  // returns the accumulated area of cells that satisfy the predicate
  virtual float GetArea(const MemoryMapRegion& p, NodePredicate f) const override;

  // returns true if there are any nodes of the given type, false otherwise
  virtual bool HasContentType(EContentType type) const override;
  
  // Broadcast the memory map
  virtual void GetBroadcastInfo(MemoryMapTypes::MapBroadcastData& info) const override;

  // get the timestamp the QT was last measured (we can update the QT with a new timestamp even if the content
  // does not change)
  virtual TimeStamp_t GetLastChangedTimeStamp() const override {return _quadTree.GetData()->GetLastObservedTime();}

private:
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // Attributes
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  
  // underlaying data container
  QuadTree          _quadTree;
  mutable std::shared_timed_mutex _writeAccess;
  
}; // class
  
} // namespace
} // namespace

#endif // ANKI_COZMO_MEMORY_MAP_H
