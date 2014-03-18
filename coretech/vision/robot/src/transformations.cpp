/**
File: transformations.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/transformations.h"
#include "anki/vision/robot/fiducialDetection.h"
#include "anki/common/robot/opencvLight.h"
#include "anki/common/robot/arrayPatterns.h"
#include "anki/common/robot/interpolate.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Transformations
    {
      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Array<f32> &initialHomography, MemoryStack &memory)
      {
        this->centerOffset = initialCorners.ComputeCenter();

        this->Init(transformType, initialCorners, initialHomography, centerOffset, memory);
      }

      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, MemoryStack &memory)
      {
        this->homography = Array<f32>();
        this->centerOffset = initialCorners.ComputeCenter();

        this->Init(transformType, initialCorners, homography, centerOffset, memory);
      }

      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, MemoryStack &memory)
      {
        this->homography = Array<f32>();
        this->initialCorners = Quadrilateral<f32>(Point<f32>(0.0f,0.0f), Point<f32>(0.0f,0.0f), Point<f32>(0.0f,0.0f), Point<f32>(0.0f,0.0f));
        this->centerOffset = initialCorners.ComputeCenter();

        this->Init(transformType, initialCorners, homography, centerOffset, memory);
      }

      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Array<f32> &initialHomography, const Point<f32> &centerOffset, MemoryStack &memory)
      {
        this->Init(transformType, initialCorners, initialHomography, centerOffset, memory);
      }

      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Point<f32> &centerOffset, MemoryStack &memory)
      {
        this->homography = Array<f32>();

        this->Init(transformType, initialCorners, homography, centerOffset, memory);
      }

      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, const Point<f32> &centerOffset, MemoryStack &memory)
      {
        this->homography = Array<f32>();
        this->initialCorners = Quadrilateral<f32>(Point<f32>(0.0f,0.0f), Point<f32>(0.0f,0.0f), Point<f32>(0.0f,0.0f), Point<f32>(0.0f,0.0f));

        this->Init(transformType, initialCorners, homography, centerOffset, memory);
      }

      Result PlanarTransformation_f32::Init(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Array<f32> &initialHomography, const Point<f32> &centerOffset, MemoryStack &memory)
      {
        this->isValid = false;

        AnkiConditionalErrorAndReturnValue(transformType==TRANSFORM_TRANSLATION || transformType==TRANSFORM_AFFINE || transformType==TRANSFORM_PROJECTIVE,
          RESULT_FAIL_INVALID_PARAMETERS, "PlanarTransformation_f32::Init", "Invalid transformType %d", transformType);

        this->transformType = transformType;
        this->centerOffset = centerOffset;
        this->initialCorners = initialCorners;

        // Store the initial quad recentered around the centerOffset.
        // get_transformedCorners() will add it back
        for(s32 iPoint=0; iPoint<4; iPoint++) {
          this->initialCorners[iPoint] -= this->centerOffset;
        }

        this->homography = Eye<f32>(3, 3, memory);

        if(initialHomography.IsValid()) {
          this->homography.Set(initialHomography);
        }

        this->isValid = true;

        return RESULT_OK;
      }

      PlanarTransformation_f32::PlanarTransformation_f32()
      {
        this->isValid = false;
      }

      Result PlanarTransformation_f32::TransformPoints(
        const Array<f32> &xIn, const Array<f32> &yIn,
        const f32 scale,
        Array<f32> &xOut, Array<f32> &yOut) const
      {
        return TransformPointsStatic(xIn, yIn, scale, this->centerOffset, xOut, yOut, this->get_transformType(), this->get_homography());
      }

      Result PlanarTransformation_f32::Update(const Array<f32> &update, const f32 scale, MemoryStack scratch, TransformType updateType)
      {
        AnkiConditionalErrorAndReturnValue(update.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "PlanarTransformation_f32::Update", "update is not valid");

        AnkiConditionalErrorAndReturnValue(update.get_size(0) == 1,
          RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

        if(updateType == TRANSFORM_UNKNOWN) {
          updateType = this->transformType;
        }

        // An Object of a given transformation type can only be updated with a simpler transformation
        if(this->transformType == TRANSFORM_TRANSLATION) {
          AnkiConditionalErrorAndReturnValue(updateType == TRANSFORM_TRANSLATION,
            RESULT_FAIL_INVALID_PARAMETERS, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else if(this->transformType == TRANSFORM_AFFINE) {
          AnkiConditionalErrorAndReturnValue(updateType == TRANSFORM_TRANSLATION || updateType == TRANSFORM_AFFINE,
            RESULT_FAIL_INVALID_PARAMETERS, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else if(this->transformType == TRANSFORM_PROJECTIVE) {
          AnkiConditionalErrorAndReturnValue(updateType == TRANSFORM_TRANSLATION|| updateType == TRANSFORM_AFFINE || updateType == TRANSFORM_PROJECTIVE,
            RESULT_FAIL_INVALID_PARAMETERS, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else {
          AnkiAssert(false);
        }

        const f32 * pUpdate = update[0];

        if(updateType == TRANSFORM_TRANSLATION) {
          AnkiConditionalErrorAndReturnValue(update.get_size(1) == TRANSFORM_TRANSLATION>>8,
            RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

          // this.tform(1:2,3) = this.tform(1:2,3) - update;
          homography[0][2] -= pUpdate[0];
          homography[1][2] -= pUpdate[1];
        } else { // if(updateType == TRANSFORM_TRANSLATION)
          Array<f32> updateArray(3,3,scratch);

          if(updateType == TRANSFORM_AFFINE) {
            AnkiConditionalErrorAndReturnValue(update.get_size(1) == TRANSFORM_AFFINE>>8,
              RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

            updateArray[0][0] = 1.0f + pUpdate[0]; updateArray[0][1] = pUpdate[1];        updateArray[0][2] = pUpdate[2];
            updateArray[1][0] = pUpdate[3];        updateArray[1][1] = 1.0f + pUpdate[4]; updateArray[1][2] = pUpdate[5];
            updateArray[2][0] = 0.0f;              updateArray[2][1] = 0.0f;              updateArray[2][2] = 1.0f;
          } else if(updateType == TRANSFORM_PROJECTIVE) {
            AnkiConditionalErrorAndReturnValue(update.get_size(1) == TRANSFORM_PROJECTIVE>>8,
              RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

            // tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; update(7:8)' 0];
            updateArray[0][0] = 1.0f + pUpdate[0]; updateArray[0][1] = pUpdate[1];        updateArray[0][2] = pUpdate[2];
            updateArray[1][0] = pUpdate[3];        updateArray[1][1] = 1.0f + pUpdate[4]; updateArray[1][2] = pUpdate[5];
            updateArray[2][0] = pUpdate[6];        updateArray[2][1] = pUpdate[7];        updateArray[2][2] = 1.0f;
          } else {
            AnkiError("PlanarTransformation_f32::Update", "Unknown transformation type %d", updateType);
            return RESULT_FAIL_INVALID_PARAMETERS;
          }

          // this.tform = this.tform*inv(tformUpdate);
          Invert3x3(
            updateArray[0][0], updateArray[0][1], updateArray[0][2],
            updateArray[1][0], updateArray[1][1], updateArray[1][2],
            updateArray[2][0], updateArray[2][1], updateArray[2][2]);

          Array<f32> newHomography(3,3,scratch);

          Matrix::Multiply(this->homography, updateArray, newHomography);

          if(!FLT_NEAR(newHomography[2][2], 1.0f)) {
            Matrix::DotDivide<f32,f32,f32>(newHomography, newHomography[2][2], newHomography);
          }

          this->homography.Set(newHomography);
        } // if(updateType == TRANSFORM_TRANSLATION) ... else

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::Print(const char * const variableName)
      {
        printf(variableName);
        printf(": center");
        this->centerOffset.Print();
        printf("\n");

        return this->homography.Print("homography");
      }

      Quadrilateral<f32> PlanarTransformation_f32::TransformQuadrilateral(const Quadrilateral<f32> &in, MemoryStack scratch, const f32 scale) const
      {
        Array<f32> xIn(1,4,scratch);
        Array<f32> yIn(1,4,scratch);
        Array<f32> xOut(1,4,scratch);
        Array<f32> yOut(1,4,scratch);

        for(s32 i=0; i<4; i++) {
          xIn[0][i] = in.corners[i].x;
          yIn[0][i] = in.corners[i].y;
        }

        TransformPoints(xIn, yIn, scale, xOut, yOut);

        Quadrilateral<f32> out;

        for(s32 i=0; i<4; i++) {
          out.corners[i].x = xOut[0][i];
          out.corners[i].y = yOut[0][i];
        }

        return out;
      }

      Result PlanarTransformation_f32::TransformArray(const Array<u8> &in,
        Array<u8> &out,
        MemoryStack scratch,
        const f32 scale) const
      {
        Result lastResult;

        AnkiConditionalErrorAndReturnValue(in.IsValid() && out.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "PlanarTransformation_f32::TransformArray", "inputs are not valid");

        AnkiConditionalErrorAndReturnValue(in.get_size(0) == out.get_size(0) && in.get_size(1) == out.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::TransformArray", "input and output are different sizes");

        const s32 arrHeight = in.get_size(0);
        const s32 arrWidth = in.get_size(1);

        Array<f32> homographyInv(3,3,scratch);
        homographyInv.Set(this->homography);

        Invert3x3(
          homographyInv[0][0], homographyInv[0][1], homographyInv[0][2],
          homographyInv[1][0], homographyInv[1][1], homographyInv[1][2],
          homographyInv[2][0], homographyInv[2][1], homographyInv[2][2]);

        //const s32 numPoints = in.get_size(0) * in.get_size(1);

        Array<f32> xIn(arrHeight,arrWidth,scratch);
        Array<f32> yIn(arrHeight,arrWidth,scratch);
        Array<f32> xTransformed(arrHeight,arrWidth,scratch);
        Array<f32> yTransformed(arrHeight,arrWidth,scratch);

        //s32 ci = 0;
        for(s32 y=0; y<arrHeight; y++) {
          f32 * restrict pXIn = xIn.Pointer(y,0);
          f32 * restrict pYIn = yIn.Pointer(y,0);

          for(s32 x=0; x<arrWidth; x++) {
            pXIn[x] = static_cast<f32>(x) - this->centerOffset.x;
            pYIn[x] = static_cast<f32>(y) - this->centerOffset.y;
          }
        }

        TransformPointsStatic(xIn, yIn, scale, this->centerOffset, xTransformed, yTransformed, this->get_transformType(), homographyInv);

        /*xIn.Print("xIn", 0,10,0,10);
        yIn.Print("yIn", 0,10,0,10);

        xTransformed.Print("xTransformed", 0,10,0,10);
        yTransformed.Print("yTransformed", 0,10,0,10);*/

        if((lastResult = Interp2<u8,u8>(in, xTransformed, yTransformed, out, INTERPOLATE_LINEAR, 0)) != RESULT_OK)
          return lastResult;

        return RESULT_OK;
      }

      bool PlanarTransformation_f32::IsValid() const
      {
        if(!this->isValid)
          return false;

        if(!this->homography.IsValid())
          return false;

        return true;
      }

      Result PlanarTransformation_f32::Set(const PlanarTransformation_f32 &newTransformation)
      {
        Result lastResult;

        const TransformType originalType = this->get_transformType();

        if((lastResult = this->set_transformType(newTransformation.get_transformType())) != RESULT_OK) {
          this->set_transformType(originalType);
          return lastResult;
        }

        if((lastResult = this->set_homography(newTransformation.get_homography())) != RESULT_OK) {
          this->set_transformType(originalType);
          return lastResult;
        }

        this->centerOffset = newTransformation.get_centerOffset();

        this->initialCorners = initialCorners;

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::Serialize(void * buffer, const s32 bufferLength) const
      {
        // TODO: make the correct length
        if(bufferLength < 512) {
          return RESULT_FAIL;
        }

        char * bufferChar = reinterpret_cast<char*>(buffer);

        memcpy(bufferChar, reinterpret_cast<const void*>(&this->isValid), sizeof(this->isValid));
        bufferChar += sizeof(this->isValid);

        const s32 transformTypeS32 = static_cast<s32>(this->transformType);
        memcpy(bufferChar, reinterpret_cast<const void*>(&transformTypeS32), sizeof(transformTypeS32));
        bufferChar += sizeof(transformTypeS32);

        const s32 numArrayBytes = this->homography.get_stride()*this->homography.get_size(0);
        memcpy(bufferChar, reinterpret_cast<const void*>(this->homography.Pointer(0,0)), numArrayBytes);
        bufferChar += numArrayBytes;

        memcpy(bufferChar, reinterpret_cast<const void*>(&this->initialCorners), sizeof(this->initialCorners));
        bufferChar += sizeof(this->initialCorners);

        memcpy(bufferChar, reinterpret_cast<const void*>(&this->centerOffset), sizeof(this->centerOffset));

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::Deserialize(const void* buffer, const s32 bufferLength)
      {
        const char * bufferChar = reinterpret_cast<const char*>(buffer);

        this->isValid = *reinterpret_cast<const bool*>(bufferChar);
        bufferChar += sizeof(this->isValid);

        this->transformType = static_cast<TransformType>(*reinterpret_cast<const s32*>(bufferChar));
        bufferChar += sizeof(s32);

        const s32 numArrayBytes = this->homography.get_stride()*this->homography.get_size(0);
        memcpy(this->homography.Pointer(0,0), bufferChar, numArrayBytes);
        bufferChar += numArrayBytes;

        this->initialCorners = *reinterpret_cast<const Quadrilateral<f32>*>(bufferChar);
        bufferChar += sizeof(this->initialCorners);

        this->centerOffset = *reinterpret_cast<const Point<f32>*>(bufferChar);

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::set_transformType(const TransformType transformType)
      {
        if(transformType == TRANSFORM_TRANSLATION || transformType == TRANSFORM_AFFINE || transformType == TRANSFORM_PROJECTIVE) {
          this->transformType = transformType;
        } else {
          AnkiError("PlanarTransformation_f32::set_transformType", "Unknown transformation type %d", transformType);
          return RESULT_FAIL_INVALID_PARAMETERS;
        }

        return RESULT_OK;
      }

      TransformType PlanarTransformation_f32::get_transformType() const
      {
        return transformType;
      }

      Result PlanarTransformation_f32::set_homography(const Array<f32>& in)
      {
        if(this->homography.Set(in) != 9)
          return RESULT_FAIL_INVALID_SIZE;

        AnkiAssert(FLT_NEAR(in[2][2], 1.0f));

        return RESULT_OK;
      }

      const Array<f32>& PlanarTransformation_f32::get_homography() const
      {
        return this->homography;
      }

      Result PlanarTransformation_f32::set_initialCorners(const Quadrilateral<f32> &initialCorners)
      {
        this->initialCorners = initialCorners;

        return RESULT_OK;
      }

      const Quadrilateral<f32>& PlanarTransformation_f32::get_initialCorners() const
      {
        return this->initialCorners;
      }

      Result PlanarTransformation_f32::set_centerOffset(const Point<f32> &centerOffset)
      {
        this->centerOffset = centerOffset;

        return RESULT_OK;
      }

      const Point<f32>& PlanarTransformation_f32::get_centerOffset() const
      {
        return this->centerOffset;
      }

      Quadrilateral<f32> PlanarTransformation_f32::get_transformedCorners(MemoryStack scratch) const
      {
        return this->TransformQuadrilateral(this->get_initialCorners(), scratch);
      }

      Result PlanarTransformation_f32::TransformPointsStatic(
        const Array<f32> &xIn, const Array<f32> &yIn,
        const f32 scale,
        const Point<f32> &centerOffset,
        Array<f32> &xOut, Array<f32> &yOut,
        const TransformType transformType,
        const Array<f32> &homography)
      {
        AnkiConditionalErrorAndReturnValue(homography.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "PlanarTransformation_f32::TransformPoints", "homography is not valid");

        AnkiConditionalErrorAndReturnValue(xIn.IsValid() && yIn.IsValid() && xOut.IsValid() && yOut.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "PlanarTransformation_f32::TransformPoints", "All inputs and outputs must be allocated and valid");

        AnkiConditionalErrorAndReturnValue(xIn.get_rawDataPointer() != xOut.get_rawDataPointer() && yIn.get_rawDataPointer() != yOut.get_rawDataPointer(),
          RESULT_FAIL_ALIASED_MEMORY, "PlanarTransformation_f32::TransformPoints", "In and Out arrays must be in different memory locations");

        AnkiConditionalErrorAndReturnValue(
          xIn.get_size(0) == yIn.get_size(0) && xIn.get_size(0) == xOut.get_size(0) && xIn.get_size(0) == yOut.get_size(0) &&
          xIn.get_size(1) == yIn.get_size(1) && xIn.get_size(1) == xOut.get_size(1) && xIn.get_size(1) == yOut.get_size(1),
          RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::TransformPoints", "All inputs and outputs must be the same size");

        const s32 numPointsY = xIn.get_size(0);
        const s32 numPointsX = xIn.get_size(1);

        const Point<f32> centerOffsetScaled(centerOffset.x / scale, centerOffset.y / scale);

        if(transformType == TRANSFORM_TRANSLATION) {
          const f32 dx = homography[0][2] / scale;
          const f32 dy = homography[1][2] / scale;

          for(s32 y=0; y<numPointsY; y++) {
            const f32 * restrict pXIn = xIn.Pointer(y,0);
            const f32 * restrict pYIn = yIn.Pointer(y,0);
            f32 * restrict pXOut = xOut.Pointer(y,0);
            f32 * restrict pYOut = yOut.Pointer(y,0);

            for(s32 x=0; x<numPointsX; x++) {
              pXOut[x] = pXIn[x] + dx + centerOffsetScaled.x;
              pYOut[x] = pYIn[x] + dy + centerOffsetScaled.y;
            }
          }
        } else if(transformType == TRANSFORM_AFFINE) {
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];

          AnkiAssert(FLT_NEAR(homography[2][0], 0.0f));
          AnkiAssert(FLT_NEAR(homography[2][1], 0.0f));
          AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

          for(s32 y=0; y<numPointsY; y++) {
            const f32 * restrict pXIn = xIn.Pointer(y,0);
            const f32 * restrict pYIn = yIn.Pointer(y,0);
            f32 * restrict pXOut = xOut.Pointer(y,0);
            f32 * restrict pYOut = yOut.Pointer(y,0);

            if(FLT_NEAR(scale, 1.0f)) {
              for(s32 x=0; x<numPointsX; x++) {
                const f32 xIn = pXIn[x];
                const f32 yIn = pYIn[x];

                const f32 xp = (h00*xIn + h01*yIn + h02);
                const f32 yp = (h10*xIn + h11*yIn + h12);

                // Restore center offset
                pXOut[x] = xp + centerOffset.x;
                pYOut[x] = yp + centerOffset.y;
              }
            } else {
              for(s32 x=0; x<numPointsX; x++) {
                const f32 xIn = pXIn[x] * scale;
                const f32 yIn = pYIn[x] * scale;

                //// Remove center offset
                //const f32 xc = pXIn[x] - centerOffset.x;
                //const f32 yc = pYIn[x] - centerOffset.y;

                const f32 xp = (h00*xIn + h01*yIn + h02);
                const f32 yp = (h10*xIn + h11*yIn + h12);

                // Restore center offset
                pXOut[x] = (xp + centerOffsetScaled.x) / scale;
                pYOut[x] = (yp + centerOffsetScaled.y) / scale;
              }
            } // if(FLT_NEAR(scale, 1.0f))
          }
        } else if(transformType == TRANSFORM_PROJECTIVE) {
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
          const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

          AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

          for(s32 y=0; y<numPointsY; y++) {
            const f32 * restrict pXIn = xIn.Pointer(y,0);
            const f32 * restrict pYIn = yIn.Pointer(y,0);
            f32 * restrict pXOut = xOut.Pointer(y,0);
            f32 * restrict pYOut = yOut.Pointer(y,0);

            if(FLT_NEAR(scale, 1.0f)) {
              for(s32 x=0; x<numPointsX; x++) {
                const f32 xIn = pXIn[x];
                const f32 yIn = pYIn[x];

                const f32 wpi = 1.0f / (h20*xIn + h21*yIn + h22);

                //// Remove center offset
                //const f32 xc = xIn - centerOffset.x;
                //const f32 yc = yIn - centerOffset.y;

                const f32 xp = (h00*xIn + h01*yIn + h02) * wpi;
                const f32 yp = (h10*xIn + h11*yIn + h12) * wpi;

                // Restore center offset
                pXOut[x] = xp + centerOffset.x;
                pYOut[x] = yp + centerOffset.y;
              }
            } else {
              for(s32 x=0; x<numPointsX; x++) {
                const f32 xIn = pXIn[x] * scale;
                const f32 yIn = pYIn[x] * scale;

                const f32 wpi = 1.0f / (h20*xIn + h21*yIn + h22);

                //// Remove center offset
                //const f32 xc = xIn - centerOffset.x;
                //const f32 yc = yIn - centerOffset.y;

                const f32 xp = (h00*xIn + h01*yIn + h02) * wpi;
                const f32 yp = (h10*xIn + h11*yIn + h12) * wpi;

                // Restore center offset
                pXOut[x] = (xp + centerOffsetScaled.x) / scale;
                pYOut[x] = (yp + centerOffsetScaled.y) / scale;
              }
            } // if(FLT_NEAR(scale, 1.0f))
          }
        } else {
          // Should be checked earlier
          AnkiAssert(false);
          return RESULT_FAIL;
        }

        return RESULT_OK;
      }

      Result ComputeHomographyFromQuad(const Quadrilateral<s16> &quad, Array<f32> &homography, MemoryStack scratch)
      {
        Result lastResult;

        AnkiConditionalErrorAndReturnValue(homography.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "ComputeHomographyFromQuad", "homography is not valid");

        AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
          RESULT_FAIL_INVALID_OBJECT, "ComputeHomographyFromQuad", "scratch is not valid");

        // TODO: I got rid of sorting, but now we have an extra copy here that can be removed.
        //Quadrilateral<s16> sortedQuad = quad.ComputeClockwiseCorners();
        Quadrilateral<s16> sortedQuad = quad;

        FixedLengthList<Point<f32> > originalPoints(4, scratch);
        FixedLengthList<Point<f32> > transformedPoints(4, scratch);

        originalPoints.PushBack(Point<f32>(0,0));
        originalPoints.PushBack(Point<f32>(0,1));
        originalPoints.PushBack(Point<f32>(1,0));
        originalPoints.PushBack(Point<f32>(1,1));

        transformedPoints.PushBack(Point<f32>(sortedQuad[0].x, sortedQuad[0].y));
        transformedPoints.PushBack(Point<f32>(sortedQuad[1].x, sortedQuad[1].y));
        transformedPoints.PushBack(Point<f32>(sortedQuad[2].x, sortedQuad[2].y));
        transformedPoints.PushBack(Point<f32>(sortedQuad[3].x, sortedQuad[3].y));

        if((lastResult = Matrix::EstimateHomography(originalPoints, transformedPoints, homography, scratch)) != RESULT_OK)
          return lastResult;

        return RESULT_OK;
      } // Result ComputeHomographyFromQuad(FixedLengthList<Quadrilateral<s16> > quads, FixedLengthList<Array<f32> > &homographies, MemoryStack scratch)
    } // namespace Transformations
  } // namespace Embedded
} // namespace Anki
