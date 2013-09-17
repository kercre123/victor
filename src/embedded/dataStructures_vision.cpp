#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    ConnectedComponentSegment::ConnectedComponentSegment()
      : xStart(-1), xEnd(-1), y(-1), id(0)
    {
    } // ConnectedComponentSegment::ConnectedComponentSegment()

    ConnectedComponentSegment::ConnectedComponentSegment(const s16 xStart, const s16 xEnd, const s16 y, const u16 id)
      : xStart(xStart), xEnd(xEnd), y(y), id(id)
    {
    } // ConnectedComponentSegment::ConnectedComponentSegment(const s16 xStart, const s16 xEnd, const s16 y, const u16 id)

    void ConnectedComponentSegment::Print() const
    {
      printf("[%d: (%d->%d, %d)] ", static_cast<s32>(id), static_cast<s32>(xStart), static_cast<s32>(xEnd), static_cast<s32>(y));
    } // void ConnectedComponentSegment::Print() const

    bool ConnectedComponentSegment::operator== (const ConnectedComponentSegment &component2) const
    {
      if((this->xStart == component2.xStart) &&
        (this->xEnd == component2.xEnd) &&
        (this->y == component2.y) &&
        (this->id == component2.id)) {
          return true;
      }

      return false;
    }

    FiducialMarker::FiducialMarker()
      : upperLeft(-1,-1), upperRight(-1,-1), lowerLeft(-1,-1), lowerRight(-1,-1), blockType(-1), faceType(-1)
    {
    } // FiducialMarker::FiducialMarker()

    FiducialMarker::FiducialMarker(const Point<s32> upperLeft, const Point<s32> upperRight, const Point<s32> lowerLeft, const Point<s32> lowerRight, const s16 blockType, const s16 faceType)
      : upperLeft(upperLeft), upperRight(upperRight), lowerLeft(lowerLeft), lowerRight(lowerRight), blockType(blockType), faceType(faceType)
    {
    } // FiducialMarker::FiducialMarker(const Point<s32> upperLeft, const Point<s32> upperRight, const Point<s32> lowerLeft, const Point<s32> lowerRight, const s16 blockType, const s16 faceType)

    void FiducialMarker::Print() const
    {
      printf("[%d,%d: (%f,%f) (%f,%f) (%f,%f) (%f,%f)] ", blockType, faceType, upperLeft.x, upperLeft.y, upperRight.x, upperRight.y, lowerLeft.x, lowerLeft.y, lowerRight.x, lowerRight.y);
    }
  } // namespace Embedded
} // namespace Anki