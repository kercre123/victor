#include "anki/embeddedVision.h"

namespace Anki
{
  namespace Embedded
  {
    Component1d::Component1d()
      : xStart(-1), xEnd(-1)
    {
    } // Component1d::Component1d()

    Component1d::Component1d(const s16 xStart, const s16 xEnd)
      : xStart(xStart), xEnd(xEnd)
    {
    } // Component1d::Component1d(const s16 xStart, const s16 xEnd)

    void Component1d::Print() const
    {
      printf("(%d->%d) ", xStart, xEnd);
    } // void Component1d::Print() const

    Component2d::Component2d()
      : xStart(-1), xEnd(-1), y(-1), id(0)
    {
    } // Component2d::Component2d()

    Component2d::Component2d(const s16 xStart, const s16 xEnd, const s16 y, const u16 id)
      : xStart(xStart), xEnd(xEnd), y(y), id(id)
    {
    } // Component2d::Component2d(const s16 xStart, const s16 xEnd)

    void Component2d::Print() const
    {
      printf("(%d: %d, %d->%d) ", id, y, xStart, xEnd);
    } // void Component2d::Print() const
  } // namespace Embedded
} // namespace Anki