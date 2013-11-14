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

    // A non-const version of ConstArraySlice
    // Warning: A "const ArraySlice" doesn't have a const Array. Only ConstArraySlice has a const
    //          Array. This allows for implicit conversion to non-const function parameters.
    template<typename Type> class ArraySlice : public ConstArraySlice<Type>
    {
    public:
      ArraySlice();

      // Directly convert an array to an ArraySlice, so all Arrays can be used as input
      // The Array parameter is not a reference, to allow for implicit conversion
      ArraySlice(Array<Type> array);

      // It's probably easier to call array.operator() than this constructor directly
      // The Array parameter is not a reference, to allow for implicit conversion
      ArraySlice(Array<Type> array, const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice);

      //ArraySlice<Type>& ArraySlice<Type>::operator= (Array<Type> & rightHandSide); // Implicit conversion

      // If automaticTranspose==true, then you can set a MxN slice with a NxM input
      // Matlab allows this for vectors, though this will also work for arbitrary-sized arrays
      Result Set(const ConstArraySliceExpression<Type> &input, bool automaticTranspose=true);

      Array<Type>& get_array();
    }; // template<typename Type> class ArraySlice

    // An ConstArraySliceExpression is like a ConstArraySlice, but can also be transposed
    // It may have other abilities in the future, but will probably always be const
    template<typename Type> class ConstArraySliceExpression : public ConstArraySlice<Type>
    {
    public:
      ConstArraySliceExpression();

      ConstArraySliceExpression(const Array<Type> input, bool isTransposed=false);

      ConstArraySliceExpression(const ArraySlice<Type> &input, bool isTransposed=false);

      ConstArraySliceExpression(const ConstArraySlice<Type> &input, bool isTransposed=false);

      // ArraySlice Transpose doesn't modify the data, it just sets a flag
      // This object isn't modified, but the returned object is.
      ConstArraySliceExpression<Type> Transpose() const;

      bool get_isTransposed() const;

    protected:
      bool isTransposed;
    };

    // To simplify the creation of kernels using an ArraySlice, and to aid the compiler optimizer,
    // an ArraySliceLimits can be initialized at the beginning of the function, then used as the
    // limits for the inner loops.

    // The suffix of in# and out# refer to the number of input and output matrices.
    // If output == 0, then the output is a scalar.

    // In1 and out0 is a special, ultra-simple case, for one matrix input and a scalar output
    template<typename Type> class ArraySliceLimits_in1_out0
    {
    public:
      // Was this ArraySliceLimits initialized?
      bool isValid;

      Type xStart;
      Type xIncrement;
      Type xEnd;
      s32  xSize;

      Type yStart;
      Type yIncrement;
      Type yEnd;
      s32  ySize;

      ArraySliceLimits_in1_out0(const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice);
    };

    // One input, one output
    template<typename Type> class ArraySliceLimits_in1_out1
    {
    public:
      // Was this ArraySliceLimits initialized?
      bool isValid;

      // Can a simple (non-transposed) iteration be performed?
      bool isSimpleIteration;

      // These are the current values for the iterations
      s32 out1Y;
      s32 out1X;
      s32 in1Y;
      s32 in1X;

      Type in1_yIncrement;
      Type in1_xIncrement;

      Type out1_yIncrement;
      Type out1_xIncrement;

      s32  out1_ySize;
      s32  out1_xSize;

      s32 in1InnerIncrementY;

      ArraySliceLimits_in1_out1(
        const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice, bool in1_isTransposed,
        const LinearSequence<Type> &out1_ySlice, const LinearSequence<Type> &out1_xSlice);

      // This should be called at the top of the y-iteration loop, before the x-iteration loop. This will update the out# and in# values for X and Y.
      inline void IncrementTop();

      // This should be called at the botom of the y-iteration loop, after the x-iteration loop. This will update the out# and in# values for X and Y.
      inline void IncrementBottom();

    protected:
      Type in1_yStart;
      Type in1_yEnd;
      Type in1_xStart;
      Type in1_xEnd;
      bool in1_isTransposed;

      Type out1_yStart;
      Type out1_yEnd;
      Type out1_xStart;
      Type out1_xEnd;
    };

    // Two inputs, one output
    template<typename Type> class ArraySliceLimits_in2_out1
    {
    public:
      // Was this ArraySliceLimits initialized?
      bool isValid;

      // Can a simple (non-transposed) iteration be performed?
      bool isSimpleIteration;

      // These are the current values for the iterations
      s32 out1Y;
      s32 out1X;
      s32 in1Y;
      s32 in1X;
      s32 in2Y;
      s32 in2X;

      Type in1_yIncrement;
      Type in1_xIncrement;

      Type in2_yIncrement;
      Type in2_xIncrement;

      Type out1_yIncrement;
      Type out1_xIncrement;

      s32  out1_ySize;
      s32  out1_xSize;

      s32 in1InnerIncrementX;
      s32 in1InnerIncrementY;
      s32 in2InnerIncrementX;
      s32 in2InnerIncrementY;

      ArraySliceLimits_in2_out1(
        const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice, bool in1_isTransposed,
        const LinearSequence<Type> &in2_ySlice, const LinearSequence<Type> &in2_xSlice, bool in2_isTransposed,
        const LinearSequence<Type> &out1_ySlice, const LinearSequence<Type> &out1_xSlice);

      // This should be called at the top of the y-iteration loop, before the x-iteration loop. This will update the out# and in# values for X and Y.
      inline void IncrementTop();

      // This should be called at the botom of the y-iteration loop, after the x-iteration loop. This will update the out# and in# values for X and Y.
      inline void IncrementBottom();

    protected:
      Type in1_yStart;
      Type in1_yEnd;
      Type in1_xStart;
      Type in1_xEnd;
      bool in1_isTransposed;

      Type in2_yStart;
      Type in2_yEnd;
      Type in2_xStart;
      Type in2_xEnd;
      bool in2_isTransposed;

      Type out1_yStart;
      Type out1_yEnd;
      Type out1_xStart;
      Type out1_xEnd;
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
      return this->array;
    }

    template<typename Type> ArraySlice<Type>::ArraySlice()
      //: array(Array<Type>()), ySlice(LinearSequence<Type>()), xSlice(LinearSequence<Type>())
      : ConstArraySlice<Type>()
    {
    }

    template<typename Type> ArraySlice<Type>::ArraySlice(Array<Type> array)
      //: array(array), ySlice(LinearSequence<s32>(0,array.get_size(0)-1)), xSlice(LinearSequence<s32>(0,array.get_size(1)-1))
      : ConstArraySlice<Type>(array)
    {
    }

    template<typename Type> ArraySlice<Type>::ArraySlice(Array<Type> array, const LinearSequence<s32> &ySlice, const LinearSequence<s32> &xSlice)
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

    template<typename Type> Result ArraySlice<Type>::Set(const ConstArraySliceExpression<Type> &input, bool automaticTranspose)
    {
      AnkiConditionalErrorAndReturnValue(this->get_array().IsValid(),
        RESULT_FAIL, "ArraySlice<Type>::Set", "Invalid array");

      AnkiConditionalErrorAndReturnValue(input.get_array().IsValid(),
        RESULT_FAIL, "ArraySlice<Type>::Set", "Invalid array");

      AnkiConditionalErrorAndReturnValue(this->get_array().get_rawDataPointer() != input.get_array().get_rawDataPointer(),
        RESULT_FAIL, "ArraySlice<Type>::Set", "Arrays must be in different memory locations");

      ArraySliceLimits_in1_out1<s32> limits(
        input.get_ySlice(), input.get_xSlice(), input.get_isTransposed(),
        this->get_ySlice(), this->get_xSlice());

      if(!limits.isValid) {
        if(automaticTranspose) {
          // If we're allowed to transpose, give it another shot
          limits = ArraySliceLimits_in1_out1<s32> (input.get_ySlice(), input.get_xSlice(), !input.get_isTransposed(), this->get_ySlice(), this->get_xSlice());

          if(!limits.isValid) {
            AnkiError("ArraySlice<Type>::Set", "Subscripted assignment dimension mismatch");
            return RESULT_FAIL;
          }
        } else {
          AnkiError("ArraySlice<Type>::Set", "Subscripted assignment dimension mismatch");
          return RESULT_FAIL;
        }
      }

      Array<Type> &out1Array = this->get_array();
      const Array<Type> &in1Array = input.get_array();

      if(limits.isSimpleIteration) {
        // If the input isn't transposed, we will do the maximally efficient loop iteration

        for(s32 y=0; y<limits.out1_ySize; y++) {
          const Type * restrict pIn1 = in1Array.Pointer(limits.in1Y, 0);
          Type * restrict pOut1 = out1Array.Pointer(limits.out1Y, 0);

          limits.IncrementTop();

          for(s32 x=0; x<limits.out1_xSize; x++) {
            pOut1[limits.out1X] = pIn1[limits.in1X];

            limits.out1X += limits.out1_xIncrement;
            limits.in1X += limits.in1_xIncrement;
          }

          limits.IncrementBottom();
        }
      } else {
        for(s32 y=0; y<limits.out1_ySize; y++) {
          Type * restrict pOut1 = out1Array.Pointer(limits.out1Y, 0);

          limits.IncrementTop();

          for(s32 x=0; x<limits.out1_xSize; x++) {
            const Type pIn1 = *in1Array.Pointer(limits.in1Y, limits.in1X);

            pOut1[limits.out1X] = pIn1;

            limits.out1X += limits.out1_xIncrement;
            limits.in1Y += limits.in1InnerIncrementY;
          }

          limits.IncrementBottom();
        }
      }

      return RESULT_OK;
    }

    template<typename Type> Array<Type>& ArraySlice<Type>::get_array()
    {
      return this->array;
    }

    template<typename Type> ConstArraySliceExpression<Type>::ConstArraySliceExpression()
      : ConstArraySlice<Type>(), isTransposed(false)
    {
    }

    template<typename Type> ConstArraySliceExpression<Type>::ConstArraySliceExpression(const Array<Type> input, bool isTransposed)
      : ConstArraySlice<Type>(input), isTransposed(isTransposed)
    {
    }

    template<typename Type> ConstArraySliceExpression<Type>::ConstArraySliceExpression(const ArraySlice<Type> &input, bool isTransposed)
      : ConstArraySlice<Type>(input), isTransposed(isTransposed)
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

    template<typename Type> ArraySliceLimits_in1_out0<Type>::ArraySliceLimits_in1_out0(const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice)
      : isValid(true),
      xStart(in1_xSlice.get_start()), xIncrement(in1_xSlice.get_increment()), xEnd(in1_xSlice.get_end()), xSize(in1_xSlice.get_size()),
      yStart(in1_ySlice.get_start()), yIncrement(in1_ySlice.get_increment()), yEnd(in1_ySlice.get_end()), ySize(in1_ySlice.get_size())
    {
    } // ArraySliceLimits_in1_out0

    template<typename Type> ArraySliceLimits_in1_out1<Type>::ArraySliceLimits_in1_out1(const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice, bool in1_isTransposed, const LinearSequence<Type> &out1_ySlice, const LinearSequence<Type> &out1_xSlice)
      : in1_yStart(in1_ySlice.get_start()), in1_yIncrement(in1_ySlice.get_increment()), in1_yEnd(in1_ySlice.get_end()),
      in1_xStart(in1_xSlice.get_start()), in1_xIncrement(in1_xSlice.get_increment()), in1_xEnd(in1_xSlice.get_end()), in1_isTransposed(in1_isTransposed),
      out1_yStart(out1_ySlice.get_start()), out1_yIncrement(out1_ySlice.get_increment()), out1_yEnd(out1_ySlice.get_end()),
      out1_xStart(out1_xSlice.get_start()), out1_xIncrement(out1_xSlice.get_increment()), out1_xEnd(out1_xSlice.get_end()),
      out1_ySize(out1_ySlice.get_size()), out1_xSize(out1_xSlice.get_size())
    {
      isValid = false;

      if(!in1_isTransposed) {
        if(out1_xSlice.get_size() == in1_xSlice.get_size() && out1_ySlice.get_size() == in1_ySlice.get_size()) {
          isValid = true;
          isSimpleIteration = true;

          this->in1Y = this->in1_yStart;
          this->out1Y = this->out1_yStart;

          // These shouldn't be used
          this->in1InnerIncrementY = 0;
        }
      } else { // if(!in1_isTransposed)
        if(out1_xSlice.get_size() == in1_ySlice.get_size() && out1_ySlice.get_size() == in1_xSlice.get_size()) {
          isValid = true;
          isSimpleIteration = false;

          this->in1X = this->in1_xStart;
          this->out1Y = this->out1_yStart;

          this->in1InnerIncrementY = this->in1_yIncrement;
        }
      } // if(!in1_isTransposed) ... else

      if(!isValid) {
        AnkiError("ArraySliceLimits_in1_out1", "Subscripted assignment dimension mismatch");
        return;
      }
    } // ArraySliceLimits_in1_out1

    // This should be called at the top of the y-iteration loop, before the x-iteration loop. This will update the out1 and in# values for X and Y.
    template<typename Type> inline void ArraySliceLimits_in1_out1<Type>::IncrementTop()
    {
      if(isSimpleIteration) {
        this->in1X = this->in1_xStart;
        this->out1X = this->out1_xStart;
      } else { // if(isSimpleIteration)
        this->in1Y = this->in1_yStart;
        this->out1X = this->out1_xStart;
      } // if(isSimpleIteration) ... else
    } // ArraySliceLimits_in1_out1<Type>::IncrementTop()

    // This should be called at the botom of the y-iteration loop, after the x-iteration loop. This will update the out and in# values for X and Y.
    template<typename Type> inline void ArraySliceLimits_in1_out1<Type>::IncrementBottom()
    {
      if(isSimpleIteration) {
        this->in1Y += this->in1_yIncrement;
        this->out1Y += this->out1_yIncrement;
      } else { // if(isSimpleIteration)
        this->in1X += this->in1_xIncrement;
        this->out1Y += this->out1_yIncrement;
      } // if(isSimpleIteration) ... else
    } // ArraySliceLimits_in1_out1<Type>::IncrementBottom()

    template<typename Type> ArraySliceLimits_in2_out1<Type>::ArraySliceLimits_in2_out1(const LinearSequence<Type> &in1_ySlice, const LinearSequence<Type> &in1_xSlice, bool in1_isTransposed, const LinearSequence<Type> &in2_ySlice, const LinearSequence<Type> &in2_xSlice, bool in2_isTransposed, const LinearSequence<Type> &out1_ySlice, const LinearSequence<Type> &out1_xSlice)
      : in1_yStart(in1_ySlice.get_start()), in1_yIncrement(in1_ySlice.get_increment()), in1_yEnd(in1_ySlice.get_end()),
      in1_xStart(in1_xSlice.get_start()), in1_xIncrement(in1_xSlice.get_increment()), in1_xEnd(in1_xSlice.get_end()), in1_isTransposed(in1_isTransposed),
      in2_yStart(in2_ySlice.get_start()), in2_yIncrement(in2_ySlice.get_increment()), in2_yEnd(in2_ySlice.get_end()),
      in2_xStart(in2_xSlice.get_start()), in2_xIncrement(in2_xSlice.get_increment()), in2_xEnd(in2_xSlice.get_end()), in2_isTransposed(in2_isTransposed),
      out1_yStart(out1_ySlice.get_start()), out1_yIncrement(out1_ySlice.get_increment()), out1_yEnd(out1_ySlice.get_end()),
      out1_xStart(out1_xSlice.get_start()), out1_xIncrement(out1_xSlice.get_increment()), out1_xEnd(out1_xSlice.get_end()),
      out1_ySize(out1_ySlice.get_size()), out1_xSize(out1_xSlice.get_size())
    {
      isValid = false;

      this->in1InnerIncrementY = 0;
      this->in1InnerIncrementX = 0;
      this->in2InnerIncrementY = 0;
      this->in2InnerIncrementX = 0;

      if(!in1_isTransposed && !in2_isTransposed) {
        const bool sizesMatch = (in1_xSlice.get_size() == in2_xSlice.get_size()) && (in1_xSlice.get_size() == out1_xSize) && (in1_ySlice.get_size() == in2_ySlice.get_size()) && (in1_ySlice.get_size() == out1_ySize);

        if(sizesMatch) {
          isValid = true;
          isSimpleIteration = true;

          this->in1Y = this->in1_yStart;
          this->in2Y = this->in2_yStart;
          this->out1Y = this->out1_yStart;
        }
      } else { // if(!in1_isTransposed)
        isSimpleIteration = false;

        bool sizesMatch = false;

        if(in1_isTransposed && in2_isTransposed) {
          sizesMatch = (in1_xSlice.get_size() == in2_xSlice.get_size()) && (in1_xSlice.get_size() == out1_ySlice.get_size()) && (in1_ySlice.get_size() == in2_ySlice.get_size()) && (in1_ySlice.get_size() == out1_xSlice.get_size());
          this->in1InnerIncrementY = in1_yIncrement;
          this->in2InnerIncrementY = in2_yIncrement;
        } else if(in1_isTransposed) {
          sizesMatch = (in1_xSlice.get_size() == in2_ySlice.get_size()) && (in1_xSlice.get_size() == out1_ySlice.get_size()) && (in1_ySlice.get_size() == in2_xSlice.get_size()) && (in1_ySlice.get_size() == out1_xSlice.get_size());
          this->in1InnerIncrementY = in1_yIncrement;
          this->in2InnerIncrementX = in2_xIncrement;
        } else if(in2_isTransposed) {
          sizesMatch = (in1_xSlice.get_size() == in2_ySlice.get_size()) && (in1_xSlice.get_size() == out1_xSlice.get_size()) && (in1_ySlice.get_size() == in2_xSlice.get_size()) && (in1_ySlice.get_size() == out1_ySlice.get_size());
          this->in1InnerIncrementX = in1_xIncrement;
          this->in2InnerIncrementY = in2_yIncrement;
        } else {
          assert(false); // should not be possible
        }

        if(!sizesMatch) {
          AnkiError("ArraySliceLimits_in2_out1", "Subscripted assignment dimension mismatch");
          return;
        }

        isValid = true;

        this->in1X = in1_xStart;
        this->in1Y = in1_yStart;
        this->in2X = in2_xStart;
        this->in2Y = in2_yStart;

        this->out1Y = out1_yStart;
      } // if(!in1_isTransposed) ... else
    } // ArraySliceLimits_in1_out1

    // This should be called at the top of the y-iteration loop, before the x-iteration loop. This will update the out1 and in# values for X and Y.
    template<typename Type> inline void ArraySliceLimits_in2_out1<Type>::IncrementTop()
    {
      if(isSimpleIteration) {
        this->out1X = this->out1_xStart;
        this->in1X = this->in1_xStart;
        this->in2X = this->in2_xStart;
      } else { // if(isSimpleIteration)
        this->out1X = this->out1_xStart;

        if(in1_isTransposed) {
          this->in1Y = in1_yStart;
        } else {
          this->in1X = in1_xStart;
        }

        if(in2_isTransposed) {
          this->in2Y = in2_yStart;
        } else {
          this->in2X = in2_xStart;
        }
      } // if(isSimpleIteration) ... else
    } // ArraySliceLimits_in2_out1<Type>::IncrementTop()

    // This should be called at the botom of the y-iteration loop, after the x-iteration loop. This will update the out and in# values for X and Y.
    template<typename Type> inline void ArraySliceLimits_in2_out1<Type>::IncrementBottom()
    {
      if(isSimpleIteration) {
        this->in1Y += this->in1_yIncrement;
        this->in2Y += this->in2_yIncrement;
        this->out1Y += this->out1_yIncrement;
      } else { // if(isSimpleIteration)
        this->out1Y += this->out1_yIncrement;

        if(in1_isTransposed) {
          this->in1X += in1_xIncrement;
        } else {
          this->in1Y += in1_yIncrement;
        }

        if(in2_isTransposed) {
          this->in2X += in2_xIncrement;
        } else {
          this->in2Y += in2_yIncrement;
        }
      } // if(isSimpleIteration) ... else
    } // ArraySliceLimits_in2_out1<Type>::IncrementBottom()
  } // namespace Embedded
} // namespace Anki

#endif // _ANKICORETECHEMBEDDED_COMMON_ARRAYSLICES_H_
