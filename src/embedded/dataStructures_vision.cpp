#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    Component1d::Component1d()
      : xStart(-1), xEnd(-1), y(-1)
    {
    } // Component1d::Component1d()

    Component1d::Component1d(const s16 xStart, const s16 xEnd, const s16 y)
      : xStart(xStart), xEnd(xEnd), y(y)
    {
    } // Component1d::Component1d(const s16 xStart, const s16 xEnd, const s16 y)

    void Component1d::Print() const
    {
      printf("(%d->%d, %d) ", xStart, xEnd, y);
    } // void Component1d::Print() const

    Component2dPiece::Component2dPiece()
      : xStart(-1), xEnd(-1), y(-1), id(0)
    {
    } // Component2dPiece::Component2dPiece()

    Component2dPiece::Component2dPiece(const s16 xStart, const s16 xEnd, const s16 y, const u16 id)
      : xStart(xStart), xEnd(xEnd), y(y), id(id)
    {
    } // Component2dPiece::Component2dPiece(const s16 xStart, const s16 xEnd, const s16 y, const u16 id)

    void Component2dPiece::Print() const
    {
      printf("[%d: (%d->%d, %d)] ", id, xStart, xEnd, y);
    } // void Component2dPiece::Print() const
  } // namespace Embedded
} // namespace Anki