#include "anki/common.h"

#include <iostream>

namespace Anki
{
  template<> void Matrix<u8>::Print() const
  {
    assert(this->rawDataPointer != NULL && this->data != NULL);

    for(s32 y=0; y<size[0]; y++) {
      const u8 * rowPointer = Pointer(y, 0);
      for(s32 x=0; x<size[1]; x++) {
        std::cout << static_cast<s32>(rowPointer[x]) << " ";
      }
      std::cout << "\n";
    }
  }

  template<> s32 Matrix<u8>::Set(const std::string values)
  {
    assert(this->rawDataPointer != NULL && this->data != NULL);

    std::istringstream iss(values);
    s32 numValuesSet = 0;

    for(s32 y=0; y<size[0]; y++) {
      u8 * restrict rowPointer = Pointer(y, 0);
      for(s32 x=0; x<size[1]; x++) {
        s32 value;
        if(iss >> value) {
          rowPointer[x] = static_cast<u8>(value);
          numValuesSet++;
        } else {
          rowPointer[x] = 0;
        }
      }
    }

    return numValuesSet;
  }
} // namespace Anki