#ifndef _ANKICORETECHEMBEDDED_COMMON_ARRAYSLICES_H_
#define _ANKICORETECHEMBEDDED_COMMON_ARRAYSLICES_H_

#include "anki/common/robot/config.h"
#include "anki/common/robot/array2d.h"

namespace Anki
{
  namespace Embedded
  {
#pragma mark --- Class Definitions ---

    template<typename Type> class ConstArraySlice;
    template<typename Type> class ArraySlice;
    template<typename Type> class ConstArraySliceExpression;

    // TODO: support non-int indexes
    // TODO: add lazy transpose?
    // TODO: is there a better way of doing this than a completely different class, different only by const?
    template<typename Type> class ConstArraySlice
    {
    public:
      ConstArraySlice();

      // Directly convert an array to an ArraySlice, so all Arrays can be used as input
      ConstArraySlice(const Array<Type> &array);

      // It's probably easier to call array.operator() than this constructor directly
      ConstArraySlice(const Array<Type> &array, const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice);

      //ConstArraySlice<Type>& ConstArraySlice<Type>::operator= (const Array<Type> & rightHandSide); // Implicit conversion

      // ArraySlice Transpose doesn't modify the data, it just sets a flag
      ConstArraySliceExpression<Type> Transpose() const;

      const LinearSequence<s32>& get_ySlice() const;

      const LinearSequence<s32>& get_xSlice() const;

      const Array<Type>& get_array() const;

    protected:
      LinearSequence<s32> ySlice;
      LinearSequence<s32> xSlice;

      Array<Type> array;
    }; // template<typename Type> class ArraySlice

    template<typename Type> class ArraySlice : public ConstArraySlice<Type>
    {
    public:
      ArraySlice();

      // Directly convert an array to an ArraySlice, so all Arrays can be used as input
      ArraySlice(Array<Type> &array);

      // It's probably easier to call array.operator() than this constructor directly
      ArraySlice(Array<Type> &array, const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice);

      //ArraySlice<Type>& ArraySlice<Type>::operator= (Array<Type> & rightHandSide); // Implicit conversion

      // If automaticTranspose==true, then you can set a MxN slice with a NxM input
      // Matlab allows this for vectors, though this will also work for arbitrary-sized arrays
      Result Set(ConstArraySliceExpression<Type> input, bool automaticTranspose=true);

      Array<Type>& get_array();
    }; // template<typename Type> class ArraySlice

    // An ConstArraySliceExpression is like a ConstArraySlice, but can also be transposed
    // It may have other abilities in the future, but will probably always be const
    template<typename Type> class ConstArraySliceExpression : public ConstArraySlice<Type>
    {
    public:
      ConstArraySliceExpression();

      ConstArraySliceExpression(const ConstArraySlice<Type> &input, bool isTransposed=false);

      // ArraySlice Transpose doesn't modify the data, it just sets a flag
      // This object isn't modified, but the returned object is.
      ConstArraySliceExpression<Type> Transpose() const;

      bool get_isTransposed() const;

    protected:
      bool isTransposed;
    };

    // To aid the compiler optimizer, an ArraySliceLimits can be initialized at the beginning of the
    // function, then used as the limits for the inner loops.
    template<typename Type> class ArraySliceLimits
    {
    public:
      s32 xStart;
      s32 xIncrement;
      s32 xEnd;
      s32 xSize;

      s32 yStart;
      s32 yIncrement;
      s32 yEnd;
      s32 ySize;

      ArraySliceLimits(ConstArraySlice<Type> &slice);
    };

#pragma mark --- Implementations ---

    template<typename Type> ConstArraySlice<Type>::ConstArraySlice()
      : array(Array<Type>()), ySlice(LinearSequence<Type>()), xSlice(LinearSequence<Type>())
    {
    }

    template<typename Type> ConstArraySlice<Type>::ConstArraySlice(const Array<Type> &array)
      : array(array), ySlice(LinearSequence<s32>(0,array.get_size(0)-1)), xSlice(LinearSequence<s32>(0,array.get_size(1)-1))
    {
    }

    template<typename Type> ConstArraySlice<Type>::ConstArraySlice(const Array<Type> &array, const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice)
      : array(array), ySlice(ySlice), xSlice(xSlice)
    {
    }

    /*template<typename Type> ConstArraySlice<Type>& ConstArraySlice<Type>::operator= (const Array<Type> & rightHandSide)
    {
    this->array = rightHandSide;
    this->ySlice = LinearSequence<s32>(0,array.get_size(0)-1);
    this->xSlice = LinearSequence<s32>(0,array.get_size(1)-1);

    return *this;
    }*/

    template<typename Type> ConstArraySliceExpression<Type> ConstArraySlice<Type>::Transpose() const
    {
      ConstArraySliceExpression<Type> expression(*this, true);

      return expression;
    }

    template<typename Type> const LinearSequence<s32>& ConstArraySlice<Type>::get_ySlice() const
    {
      return ySlice;
    }

    template<typename Type> const LinearSequence<s32>& ConstArraySlice<Type>::get_xSlice() const
    {
      return xSlice;
    }

    template<typename Type> const Array<Type>& ConstArraySlice<Type>::get_array() const
    {
      return array;
    }

    template<typename Type> ArraySlice<Type>::ArraySlice()
      //: array(Array<Type>()), ySlice(LinearSequence<Type>()), xSlice(LinearSequence<Type>())
      : ConstArraySlice<Type>()
    {
    }

    template<typename Type> ArraySlice<Type>::ArraySlice(Array<Type> &array)
      //: array(array), ySlice(LinearSequence<s32>(0,array.get_size(0)-1)), xSlice(LinearSequence<s32>(0,array.get_size(1)-1))
      : ConstArraySlice<Type>(array)
    {
    }

    template<typename Type> ArraySlice<Type>::ArraySlice(Array<Type> &array, const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice)
      //: array(array), ySlice(ySlice), xSlice(xSlice)
      : ConstArraySlice<Type>(array, ySlice, xSlice)
    {
    }

    /*template<typename Type> ArraySlice<Type>& ArraySlice<Type>::operator= (Array<Type> & rightHandSide)
    {
    this->array = rightHandSide;
    this->ySlice = LinearSequence<s32>(0,array.get_size(0)-1);
    this->xSlice = LinearSequence<s32>(0,array.get_size(1)-1);

    return *this;
    }*/

    template<typename Type> Result ArraySlice<Type>::Set(ConstArraySliceExpression<Type> input, bool automaticTranspose=true)
    {
      AnkiConditionalErrorAndReturnValue(this->get_array().IsValid(),
        RESULT_FAIL, "ArraySlice<Type>::Set", "Invalid array");

      AnkiConditionalErrorAndReturnValue(input.get_array().IsValid(),
        RESULT_FAIL, "ArraySlice<Type>::Set", "Invalid array");

      AnkiConditionalErrorAndReturnValue(this->get_array().get_rawDataPointer() != input.get_array().get_rawDataPointer(),
        RESULT_FAIL, "ArraySlice<Type>::Set", "Arrays must be in different memory locations");

      const ArraySliceLimits<Type> thisLimits(*this);
      const ArraySliceLimits<Type> inputLimits(input);

      Array<Type> &thisArray = this->array;
      const Array<Type> &inputArray = input.get_array();

      if(thisLimits.xSize == inputLimits.xSize && thisLimits.ySize == inputLimits.ySize) {
        // If the input isn't transposed, we will do the maximally efficient loop iteration

        s32 thisY = thisLimits.yStart;
        s32 inputY = inputLimits.yStart;

        for(s32 y=0; y<thisLimits.ySize; y++) {
          Type * restrict pThis = thisArray.Pointer(thisY, 0);
          const Type * restrict pInput = inputArray.Pointer(inputY, 0);

          s32 thisX = thisLimits.xStart;
          s32 inputX = inputLimits.xStart;

          for(s32 x=0; x<thisLimits.xSize; x++) {
            pThis[thisX] = pInput[inputX];

            thisX += thisLimits.xIncrement;
            inputX += inputLimits.xIncrement;
          }

          thisY += thisLimits.yIncrement;
          inputY += inputLimits.yIncrement;
        }
      } else if((automaticTranspose||input.get_isTransposed()) && (thisLimits.xSize == inputLimits.ySize && thisLimits.ySize == inputLimits.xSize)) {
        // If the input is transposed or if automaticTransposing is allowed, then we will do an inefficent loop iteration
        // TODO: make fast if needed

        s32 thisY = thisLimits.yStart;
        s32 inputX = inputLimits.xStart;

        // TODO: replace the Pointer() call with an addition, if speed is a problem
        for(s32 y=0; y<thisLimits.ySize; y++) {
          Type * restrict pThis = thisArray.Pointer(thisY, 0);

          s32 thisX = thisLimits.xStart;
          s32 inputY = inputLimits.yStart;

          for(s32 x=0; x<thisLimits.xSize; x++) {
            const Type pInput = *inputArray.Pointer(inputY, inputX);

            pThis[thisX] = pInput;

            thisX += thisLimits.xIncrement;
            inputY += inputLimits.yIncrement;
          }

          thisY += thisLimits.yIncrement;
          inputX += inputLimits.xIncrement;
        }
      } else {
        AnkiError("ArraySlice<Type>::Set", "Subscripted assignment dimension mismatch");
        return RESULT_FAIL;
      }

      return RESULT_OK;
    }

    template<typename Type> Array<Type>& ArraySlice<Type>::get_array()
    {
      return array;
    }

    template<typename Type> ConstArraySliceExpression<Type>::ConstArraySliceExpression()
      : ConstArraySlice<Type>(), isTransposed(false)
    {
    }

    template<typename Type> ConstArraySliceExpression<Type>::ConstArraySliceExpression(const ConstArraySlice<Type> &input, bool isTransposed)
      : ConstArraySlice<Type>(input), isTransposed(isTransposed)
    {
    }

    template<typename Type> ConstArraySliceExpression<Type> ConstArraySliceExpression<Type>::Transpose() const
    {
      ConstArraySliceExpression<Type> expression(*this, !this->get_isTransposed());

      return expression;
    }

    template<typename Type> bool ConstArraySliceExpression<Type>::get_isTransposed() const
    {
      return isTransposed;
    }

    template<typename Type> ArraySliceLimits<Type>::ArraySliceLimits(ConstArraySlice<Type> &slice)
      : xStart(slice.get_xSlice().get_start()), xIncrement(slice.get_xSlice().get_increment()), xEnd(slice.get_xSlice().get_end()), xSize(slice.get_xSlice().get_size()),
      yStart(slice.get_ySlice().get_start()), yIncrement(slice.get_ySlice().get_increment()), yEnd(slice.get_ySlice().get_end()), ySize(slice.get_ySlice().get_size())
    {
    }
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAYSLICES_H_
