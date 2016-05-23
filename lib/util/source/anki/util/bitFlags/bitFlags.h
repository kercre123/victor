/**
 * File: bitFlags.h
 *
 * Author: Mark Wesley
 * Created: 10/20/2014
 *
 * Description: Templated bit flags container - helper for working with bit flags inside a given type (e.g. uint8/16/32/64)
 *
 * Copyright: Anki, Inc. 2014
 *
 **/


#ifndef Util_BitFlags_BitFlags_H__
#define Util_BitFlags_BitFlags_H__


#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include "util/bitFlags/bitFlagStorage.h"
#include "util/math/numericCast.h"



namespace Anki
{
namespace Util
{
  
  // T = type for the underlying storage, e.g. uint32_t to store 32 bit flags
  // ET = type for the enum used to index each bit
  template<typename T, typename ET>
  class BitFlags
  {
  public:
    
    using StorageType = T;
    using BitShiftType = uint32_t; // allows up to 2^32 bits - e.g. up to a 2 GB variable...
    
    BitFlags()
      : _bitFlags(0u)
    {
    }
    
    BitFlags(const T& val)
      : _bitFlags(val)
    {
    }
    
    inline void SetFlags(const T& newVal) { _bitFlags = newVal; }
    inline void ClearFlags()       { _bitFlags = 0; }
    inline T    GetFlags() const   { return _bitFlags; }
    inline bool AreAnyFlagsSet() const { return _bitFlags != 0; }
    
    static T GetBitMask(BitShiftType bitMaskShiftVal)
    {
      assert(bitMaskShiftVal >= 0);
      assert(bitMaskShiftVal < sizeof(T)*8u);
      
      const T bitMask = T(1u) << bitMaskShiftVal;
      return bitMask;
    }
    
    void SetBitMask(const T& bitMask, bool newVal)
    {
      if (newVal)
      {
        _bitFlags |=  bitMask;
      }
      else
      {
        _bitFlags &= ~bitMask;
      }
    }
    
    inline bool IsBitMaskSet(const T& bitMask) const
    {
      return  ((_bitFlags & bitMask) == bitMask);
    }
    
    inline bool AreAnyBitsInMaskSet(const T& bitMask) const
    {
      return  ((_bitFlags & bitMask) != 0);
    }
    
    inline bool IsBitFlagSet(ET bitFlag) const
    {
      const T bitMask = GetBitMask( Anki::Util::numeric_cast<BitShiftType>(bitFlag) );
      return IsBitMaskSet(bitMask);
    }
    
    inline void SetBitFlag(ET bitFlag, bool newVal)
    {
      const T bitMask = GetBitMask( Anki::Util::numeric_cast<BitShiftType>(bitFlag) );
      SetBitMask(bitMask, newVal);
    }
    
  private:
    
    StorageType	_bitFlags;
  };

  
  // Typedefs for the suggested instantiations of this template
  // Usage, e.g: BitFlags32<MyEnum>         myBitFlags;
  // or          BitFlags<uint32_t, MyEnum> myBitFlags;
  // For > 64bits use BitFlagStorage for a structure of multiple built in uint types that supports bit shifts and masks
  // or          BitFlags<BitFlagStorage<uint64_t, 3>, MyEnum> myBitFlags;
  
  template<typename ET>
  using BitFlags8 = BitFlags<uint8_t, ET>;
  
  template<typename ET>
  using BitFlags16 = BitFlags<uint16_t, ET>;

  template<typename ET>
  using BitFlags32 = BitFlags<uint32_t, ET>;

  template<typename ET>
  using BitFlags64 = BitFlags<uint64_t, ET>;

  template<typename ET>
  using BitFlags96 = BitFlags<BitFlagStorage<uint32_t, 3>, ET>;

  template<typename ET>
  using BitFlags128 = BitFlags<BitFlagStorage<uint64_t, 2>, ET>;

  template<typename ET>
  using BitFlags192 = BitFlags<BitFlagStorage<uint64_t, 3>, ET>;

  template<typename ET>
  using BitFlags256 = BitFlags<BitFlagStorage<uint64_t, 4>, ET>;

  
}; // namespace
}; // namespace

#endif // Util_BitFlags_BitFlags_H__

