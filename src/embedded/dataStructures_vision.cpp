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
      printf("[%d: (%d->%d, %d)] ", id, xStart, xEnd, y);
    } // void ConnectedComponentSegment::Print() const
  } // namespace Embedded
} // namespace Anki