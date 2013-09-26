#include "anki/embeddedVision/dataStructures_vision.h"

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
      //printf("[%d: (%d->%d, %d)] ", static_cast<s32>(this->id), static_cast<s32>(this->xStart), static_cast<s32>(this->xEnd), static_cast<s32>(this->y));
      printf("[%u: (%d->%d, %d)] ", this->id, this->xStart, this->xEnd, this->y);
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
    {
    } // FiducialMarker::FiducialMarker()

    void FiducialMarker::Print() const
    {
      printf("[%d,%d: (%d,%d) (%d,%d) (%d,%d) (%d,%d)] ", blockType, faceType, corners[0].x, corners[0].y, corners[1].x, corners[1].y, corners[2].x, corners[2].y, corners[3].x, corners[3].y);
    }

    FiducialMarker& FiducialMarker::operator= (const FiducialMarker &marker2)
    {
      this->blockType = marker2.blockType;
      this->faceType = marker2.faceType;
      this->homography = marker2.homography;

      for(s32 i=0; i<4; i++) {
        this->corners[i] = marker2.corners[i];
      }

      return *this;
    }
  } // namespace Embedded
} // namespace Anki