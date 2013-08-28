//#include "anki/embeddedCommon.h"

//#include <iostream>

#include "anki/embeddedCommon/array2d.h"

namespace Anki
{
  namespace Embedded
  {
    template<> void Array2d<u8>::Print() const
    {
      assert(this->rawDataPointer != NULL && this->data != NULL);

      for(s32 y=0; y<size[0]; y++) {
        const u8 * rowPointer = Pointer(y, 0);
        for(s32 x=0; x<size[1]; x++) {
          //std::cout << static_cast<s32>(rowPointer[x]) << " ";
          printf("%d ", static_cast<s32>(rowPointer[x]));
        }
        // std::cout << "\n";
        printf("\n");
      }
    }
  } // namespace Embedded
} // namespace Anki