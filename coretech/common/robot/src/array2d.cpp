/**
File: array2d.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/common/robot/array2d.h"
#include "anki/common/robot/errorHandling.h"
#include "anki/common/robot/utilities.h"
#include "anki/common/robot/utilities_c.h"

//using namespace std;

namespace Anki
{
  namespace Embedded
  {
    template<> Result Array<bool>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<u8>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<s8>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<u16>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<s16>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<u32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<s32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<u64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<s64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<f32>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<f64>::Print(const char * const variableName, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, 1, minY, maxY, minX, maxX);
    }

    template<> Result Array<f32>::PrintAlternate(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, version, minY, maxY, minX, maxX);
    }

    template<> Result Array<f64>::PrintAlternate(const char * const variableName, const s32 version, const s32 minY, const s32 maxY, const s32 minX, const s32 maxX) const
    {
      return PrintBasicType(variableName, version, minY, maxY, minX, maxX);
    }

    C_Array_s32 get_C_Array_s32(Array<s32> &array)
    {
      C_Array_s32 cVersion;

      cVersion.size0 = array.get_size(0);
      cVersion.size1 = array.get_size(1);
      cVersion.stride = array.get_stride();
      cVersion.flags = array.get_flags().get_rawFlags();
      cVersion.data = array.Pointer(0,0);

      return cVersion;
    } // C_Array_s32 get_C_Array_s32(Array<s32> &array)
  } // namespace Embedded
} // namespace Anki