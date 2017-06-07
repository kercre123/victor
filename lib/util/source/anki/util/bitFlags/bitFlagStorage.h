/**
 * File: bitFlagStorage.h
 *
 * Author: Mark Wesley
 * Created: 02/02/2016
 *
 * Description: Templated container for storing N uintX_t structures and supporting just enough arithmetic operators so
 *              that it can be used inside BitFlags<T>
 *              This allows unlimited number of bits/flags to be stored in BitFlags instead of just the uint64 limit
 *
 * Copyright: Anki, Inc. 2016
 *
 **/


#ifndef __Util_BitFlags_BitFlagStorage_H__
#define __Util_BitFlags_BitFlagStorage_H__


#include <limits>
#include <assert.h>
#include <stdint.h>


namespace Anki
{
namespace Util
{
  
  template<typename InnerVarType, uint32_t kNumVars>
  struct BitFlagStorage
  {
  public:
    
    static_assert(!std::numeric_limits<InnerVarType>::is_signed, "Only supports unsigned inner types");
    
    BitFlagStorage()
    {
      // default constructor does nothing - exactly the same as the underlying types
    }
    
    explicit BitFlagStorage(InnerVarType newVal)
    {
      _storage[0] = newVal;
      for (uint32_t i = 1; i < kNumVars; ++i)
      {
        _storage[i] = 0;
      }
    }
    
    // Note: Following operators not currently defined for this class
    // operator >>=, >>
    // operator +=,  +
    // operator -=,  -
    // operator *=,  *
    // operator /=,  /
    // operator %=,  % (Modulo)
    // pre/post inc/decrement
    // Logical (&&, ||, !)
    
    // ============================== Bit-shift Operators ==============================
    
    BitFlagStorage operator <<(uint32_t shiftAmmount) const
    {
      const uint32_t numBitsPerVar = sizeof(InnerVarType) * 8u;
      
      BitFlagStorage retVal(0);
      
      const uint32_t numVarsOffset = shiftAmmount / numBitsPerVar; // 0 for < 1 entire var
      const uint32_t offsetInVar = shiftAmmount - (numVarsOffset * numBitsPerVar); // 0..(numBitsPerVar-1)
      assert(offsetInVar < numBitsPerVar);
      
      // Every var is shifted into at most 2 destination vars
      for (uint32_t i = 0; i < (kNumVars - numVarsOffset); ++i)
      {
        // i is shifted into (i+numVarsOffset) and (i+numOffset)+1
        
        uint32_t dest = (i + numVarsOffset);
        retVal._storage[dest] |= (_storage[i] << offsetInVar);
        
        ++dest;
        if ((offsetInVar > 0) && (dest < kNumVars))
        {
          // all the bits that overflowed the previous var
          const uint32_t overflowShift = (numBitsPerVar - offsetInVar);
          retVal._storage[dest] |= (_storage[i] >> overflowShift);
        }
      }
      
      return retVal;
    }
    
    BitFlagStorage& operator <<=(uint32_t shiftAmmount)
    {
      *this = *this << shiftAmmount;
      return *this;
    }
    
    // ============================== Bitwise Operators ==============================
    
    BitFlagStorage operator ~() const
    {
      BitFlagStorage retVal;
      
      for (uint32_t i = 0; i < kNumVars; ++i)
      {
        retVal._storage[i] = ~_storage[i];
      }
      return retVal;
    }
    
    BitFlagStorage& operator |=(const BitFlagStorage& rhs)
    {
      for (uint32_t i = 0; i < kNumVars; ++i)
      {
        _storage[i] |= rhs._storage[i];
      }
      return *this;
    }
    
    BitFlagStorage operator |(const BitFlagStorage& rhs) const
    {
      BitFlagStorage retVal = *this;
      retVal |= rhs;
      return retVal;
    }
    
    BitFlagStorage& operator &=(const BitFlagStorage& rhs)
    {
      for (uint32_t i = 0; i < kNumVars; ++i)
      {
        _storage[i] &= rhs._storage[i];
      }
      return *this;
    }
    
    BitFlagStorage operator &(const BitFlagStorage& rhs) const
    {
      BitFlagStorage retVal = *this;
      retVal &= rhs;
      return retVal;
    }
    
    BitFlagStorage& operator ^=(const BitFlagStorage& rhs)
    {
      for (uint32_t i = 0; i < kNumVars; ++i)
      {
        _storage[i] ^= rhs._storage[i];
      }
      return *this;
    }
    
    BitFlagStorage operator ^(const BitFlagStorage& rhs) const
    {
      BitFlagStorage retVal = *this;
      retVal ^= rhs;
      return retVal;
    }
    
    // ============================== Comparison Operators - Same Type ==============================
    
    int Compare(const BitFlagStorage& rhs) const
    {
      // -1==LT, 0==EQ, 1==GT
      for (uint32_t i = kNumVars; i > 0; )
      {
        --i;
        if (_storage[i] < rhs._storage[i])
        {
          return -1;
        }
        else if (_storage[i] > rhs._storage[i])
        {
          return 1;
        }
      }
      
      // exactly equal
      return 0;
    }
    
    bool operator<(const BitFlagStorage& rhs) const
    {
      return (Compare(rhs) < 0);
    }
    
    bool operator<=(const BitFlagStorage& rhs) const
    {
      return (Compare(rhs) <= 0);
    }

    bool operator>=(const BitFlagStorage& rhs) const
    {
      return (Compare(rhs) >= 0);
    }
    
    bool operator>(const BitFlagStorage& rhs) const
    {
      return (Compare(rhs) > 0);
    }
    
    bool operator==(const BitFlagStorage& rhs) const
    {
      return (Compare(rhs) == 0);
    }
    
    bool operator!=(const BitFlagStorage& rhs) const
    {
      return (Compare(rhs) != 0);
    }
    
    // ============================== Comparison Operators - Inner Type ==============================
    // Equivalent to InnerType expanded to BitFlagStorage with all but [0]=InnerType and [>0]=0
    
    int Compare(const InnerVarType& rhs) const
    {
      // -1==LT, 0==EQ, 1==GT
      for (uint32_t i = kNumVars; i > 0; )
      {
        --i;
        InnerVarType rhsValue = (i==0) ? rhs : 0;
        if (_storage[i] < rhsValue)
        {
          return -1;
        }
        else if (_storage[i] > rhsValue)
        {
          return 1;
        }
      }
      
      // exactly equal
      return 0;
    }
    
    bool operator<(const InnerVarType& rhs) const
    {
      return (Compare(rhs) < 0);
    }
    
    bool operator<=(const InnerVarType& rhs) const
    {
      return (Compare(rhs) <= 0);
    }
    
    bool operator>=(const InnerVarType& rhs) const
    {
      return (Compare(rhs) >= 0);
    }
    
    bool operator>(const InnerVarType& rhs) const
    {
      return (Compare(rhs) > 0);
    }
    
    bool operator==(const InnerVarType& rhs) const
    {
      return (Compare(rhs) == 0);
    }
    
    bool operator!=(const InnerVarType& rhs) const
    {
      return (Compare(rhs) != 0);
    }
    
    // ============================== Accessors ==============================
    // Primarily added for unit tests
    
    InnerVarType GetVar(uint32_t i) const
    {
      assert(i < kNumVars);
      return _storage[i];
    }
    
    void SetVar(uint32_t i, const InnerVarType& newVal)
    {
      assert(i < kNumVars);
      _storage[i] = newVal;
    }
    
  private:
    
    InnerVarType _storage[kNumVars]; // from Least to Most signifcant. [0] contains LSB
  };

  
}; // namespace
}; // namespace

#endif // __Util_BitFlags_BitFlagStorage_H__

