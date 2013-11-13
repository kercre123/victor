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

      // Matlab equivalent is the colon operator, for all, like array(:)
      // Internally, it sets startValue==endValue==AllElements
      LinearSequence();

      // Matlab equivalent: startValue:endValue
      // If startValue or endValue is less than 0, it is equivalent to (end+value)
      LinearSequence(const Type startValue, const Type endValue);

      // Matlab equivalent: startValue:increment:endValue
      LinearSequence(const Type startValue, const Type increment, const Type endValue);

      Type get_startValue() const;

      Type get_increment() const;

      Type get_endValue() const;

      s32 get_size() const;

    protected:
      Type startValue;
      Type increment;
      Type endValue;

      s32 size;

      static s32 computeSize(const Type startValue, const Type increment, const Type endValue);
    }; // class LinearSequence

    // TODO: linspace
    //template<typename Type> LinearSequence<Type> Linspace(const Type startValue, const Type endValue, const s32 size);

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
      : startValue(AllElements), increment(static_cast<Type>(1)), endValue(AllElements)
    {
      this->size = AllElements;
    }

    template<typename Type> LinearSequence<Type>::LinearSequence(const Type startValue, const Type endValue)
      : startValue(startValue), increment(static_cast<Type>(1))
    {
      this->size = computeSize(this->startValue, this->increment, endValue);
      this->endValue = this->startValue + (this->size-1) * this->increment;
    }

    template<typename Type> LinearSequence<Type>::LinearSequence(const Type startValue, const Type increment, const Type endValue)
      : startValue(startValue), increment(increment)
    {
      this->size = computeSize(this->startValue, this->increment, endValue);
      this->endValue = this->startValue + (this->size-1) * this->increment;
    }

    template<typename Type> Type LinearSequence<Type>::get_startValue() const
    {
      return startValue;
    }

    template<typename Type> Type LinearSequence<Type>::get_increment() const
    {
      return increment;
    }

    template<typename Type> Type LinearSequence<Type>::get_endValue() const
    {
      return endValue;
    }

    template<typename Type> s32 LinearSequence<Type>::get_size() const
    {
      return size;
    }

    // TODO: instantiate for float
    template<typename Type> s32 LinearSequence<Type>::computeSize(const Type startValue, const Type increment, const Type endValue)
    {
      assert(increment != static_cast<Type>(0));

      // 10:-1:12
      if(increment < 0 && startValue < endValue) {
        return 0;
      }

      // 12:1:10
      if(increment > 0 && startValue > endValue) {
        return 0;
      }

      const Type minLimit = MIN(startValue, endValue);
      const Type maxLimit = MAX(startValue, endValue);
      const Type incrementMagnitude = ABS(increment);

      const Type validRange = maxLimit - minLimit + 1;
      const s32 size = (validRange+incrementMagnitude-1)/incrementMagnitude;

      return size;
    }

#pragma mark --- Specializations ---

    template<> s32 LinearSequence<f32>::computeSize(const f32 startValue, const f32 increment, const f32 endValue);
    template<> s32 LinearSequence<f64>::computeSize(const f64 startValue, const f64 increment, const f64 endValue);
  } // namespace Embedded
} //namespace Anki

#endif // #ifndef _ANKICORETECHEMBEDDED_COMMON_SEQUENCES_H_