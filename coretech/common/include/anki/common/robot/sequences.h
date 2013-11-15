#ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_
#define _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_

#include "anki/common/robot/config.h"

namespace Anki
{
  namespace Embedded
  {
    const s32 AllElements = 0x8000000;

#pragma mark --- Class Definitions ---
    template<typename Type> class Sequence
    {
    }; // class Sequence

    template<typename Type> class LinearSequence : public Sequence<Type>
    {
    public:

      LinearSequence();

      // Matlab equivalent: start:end
      LinearSequence(const Type start, const Type end);

      // Matlab equivalent: start:increment:end
      LinearSequence(const Type start, const Type increment, const Type end);

      Type get_start() const;

      Type get_increment() const;

      Type get_end() const;

      s32 get_size() const;

    protected:
      Type start;
      Type increment;
      Type end;

      s32 size;

      static s32 computeSize(const Type start, const Type increment, const Type end);
    }; // class LinearSequence

    // IndexSequence creates the input for slicing an Array
    // If start or end is less than 0, it is equivalent to (end+value)
    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type end, s32 arraySize);
    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type increment, Type end, s32 arraySize);
    LinearSequence<s32> IndexSequence(s32 arraySize); // Internally, it sets start==0, end=arraySize-1, like the Matlab colon operator array(:,:)

    // TODO: linspace
    //template<typename Type> LinearSequence<Type> Linspace(const Type start, const Type end, const s32 size);

    // TODO: Logspace
    //template<typename Type> class Logspace : public Sequence<Type>
    //{
    //public:
    //protected:
    //};

    // TODO: Meshgrid
    //template<typename Type> class Meshgrid
    //{
    //public:
    //  // Matlab equivalent:
    //  Meshgrid(LinearSequence<Type> xGridVector, LinearSequence<Type> yGridVector);
    //protected:
    //};

#pragma mark --- Implementations ---

    template<typename Type> LinearSequence<Type>::LinearSequence()
      : start(-1), increment(static_cast<Type>(-1)), end(-1)
    {
      this->size = -1;
    }

    template<typename Type> LinearSequence<Type>::LinearSequence(const Type start, const Type end)
      : start(start), increment(static_cast<Type>(1))
    {
      this->size = computeSize(this->start, this->increment, end);
      this->end = this->start + (this->size-1) * this->increment;
    }

    template<typename Type> LinearSequence<Type>::LinearSequence(const Type start, const Type increment, const Type end)
      : start(start), increment(increment)
    {
      this->size = computeSize(this->start, this->increment, end);
      this->end = this->start + (this->size-1) * this->increment;
    }

    template<typename Type> Type LinearSequence<Type>::get_start() const
    {
      return start;
    }

    template<typename Type> Type LinearSequence<Type>::get_increment() const
    {
      return increment;
    }

    template<typename Type> Type LinearSequence<Type>::get_end() const
    {
      return end;
    }

    template<typename Type> s32 LinearSequence<Type>::get_size() const
    {
      return size;
    }

    // TODO: instantiate for float
    template<typename Type> s32 LinearSequence<Type>::computeSize(const Type start, const Type increment, const Type end)
    {
      assert(increment != static_cast<Type>(0));

      // 10:-1:12
      if(increment < 0 && start < end) {
        return 0;
      }

      // 12:1:10
      if(increment > 0 && start > end) {
        return 0;
      }

      const Type minLimit = MIN(start, end);
      const Type maxLimit = MAX(start, end);
      const Type incrementMagnitude = ABS(increment);

      const Type validRange = maxLimit - minLimit + 1;
      const s32 size = (validRange+incrementMagnitude-1)/incrementMagnitude;

      return size;
    }

    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type end, s32 arraySize)
    {
      return IndexSequence(start, static_cast<Type>(1), end, arraySize);
    }

    template<typename Type> LinearSequence<Type> IndexSequence(Type start, Type increment, Type end, s32 arraySize)
    {
      if(start < 0)
        start += arraySize;

      assert(start >=0 && start < arraySize);

      if(end < 0)
        end += arraySize;

      assert(end >=0 && end < arraySize);

      LinearSequence<Type> sequence(start, increment, end);

      return sequence;
    }

#pragma mark --- Specializations ---

    template<> s32 LinearSequence<f32>::computeSize(const f32 start, const f32 increment, const f32 end);
    template<> s32 LinearSequence<f64>::computeSize(const f64 start, const f64 increment, const f64 end);
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_