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
  } // namespace Embedded
} // namespace Anki