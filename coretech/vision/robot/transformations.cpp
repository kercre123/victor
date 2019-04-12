/**
File: transformations.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/transformations.h"
#include "coretech/vision/robot/fiducialDetection.h"
#include "coretech/vision/robot/histogram.h"

#include "coretech/common/robot/opencvLight.h"
#include "coretech/common/robot/arrayPatterns.h"
#include "coretech/common/robot/interpolate.h"
#include "coretech/common/robot/serialize.h"
#include "coretech/common/robot/matlabInterface.h"

namespace Anki
{
  namespace Embedded
  {
    namespace Transformations
    {
      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, const Array<f32> &initialHomography, MemoryStack &memory)
      {
        this->centerOffset = initialCorners.ComputeCenter<f32>();

        this->Init(transformType, initialCorners, initialHomography, centerOffset, memory);
      }

      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, const Quadrilateral<f32> &initialCorners, MemoryStack &memory)
      {
        this->homography = Array<f32>();
        this->centerOffset = initialCorners.ComputeCenter<f32>();

        this->Init(transformType, initialCorners, homography, centerOffset, memory);
      }

      PlanarTransformation_f32::PlanarTransformation_f32(const TransformType transformType, MemoryStack &memory)
      {
        this->homography = Array<f32>();
        this->initialCorners = Quadrilateral<f32>(Point<f32>(0.0f,0.0f), Point<f32>(0.0f,0.0f), Point<f32>(0.0f,0.0f), Point<f32>(0.0f,0.0f));
        this->centerOffset = initialCorners.ComputeCenter<f32>();

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
          RESULT_FAIL_INVALID_PARAMETER, "PlanarTransformation_f32::Init", "Invalid transformType %d", transformType);

        this->transformType = transformType;
        this->centerOffset = centerOffset;
        this->initialCorners = initialCorners;

        this->initialPointsAreZeroCentered = false;

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
        const bool inputPointsAreZeroCentered,
        const bool outputPointsAreZeroCentered,
        Array<f32> &xOut, Array<f32> &yOut) const
      {
        return TransformPointsStatic(
          xIn, yIn,
          scale,
          this->centerOffset, this->get_transformType(), this->get_homography(),
          inputPointsAreZeroCentered, outputPointsAreZeroCentered,
          xOut, yOut);
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
            RESULT_FAIL_INVALID_PARAMETER, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else if(this->transformType == TRANSFORM_AFFINE) {
          AnkiConditionalErrorAndReturnValue(updateType == TRANSFORM_TRANSLATION || updateType == TRANSFORM_AFFINE,
            RESULT_FAIL_INVALID_PARAMETER, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else if(this->transformType == TRANSFORM_PROJECTIVE) {
          AnkiConditionalErrorAndReturnValue(updateType == TRANSFORM_TRANSLATION|| updateType == TRANSFORM_AFFINE || updateType == TRANSFORM_PROJECTIVE,
            RESULT_FAIL_INVALID_PARAMETER, "PlanarTransformation_f32::Update", "cannot update this transform with the update type %d", updateType);
        } else {
          AnkiAssert(false);
        }

        const f32 * pUpdate = update[0];

        if(updateType == TRANSFORM_TRANSLATION) {
          AnkiConditionalErrorAndReturnValue(update.get_size(1) == TRANSFORM_TRANSLATION>>8,
            RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

          // this.tform(1:2,3) = this.tform(1:2,3) - update;
          homography[0][2] -= scale*pUpdate[0];
          homography[1][2] -= scale*pUpdate[1];
        } else { // if(updateType == TRANSFORM_TRANSLATION)
          Array<f32> updateArray(3,3,scratch);

          if(updateType == TRANSFORM_AFFINE) {
            AnkiConditionalErrorAndReturnValue(update.get_size(1) == TRANSFORM_AFFINE>>8,
              RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

            // TODO: does this work with projective?

            updateArray[0][0] = 1.0f + pUpdate[0]; updateArray[0][1] = pUpdate[1];        updateArray[0][2] = pUpdate[2]*scale;
            updateArray[1][0] = pUpdate[3];        updateArray[1][1] = 1.0f + pUpdate[4]; updateArray[1][2] = pUpdate[5]*scale;
            updateArray[2][0] = 0.0f;              updateArray[2][1] = 0.0f;              updateArray[2][2] = 1.0f;
          } else if(updateType == TRANSFORM_PROJECTIVE) {
            AnkiConditionalErrorAndReturnValue(update.get_size(1) == TRANSFORM_PROJECTIVE>>8,
              RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Update", "update is the incorrect size");

            // TODO: does this work with affine?

            // tformUpdate = eye(3) + [update(1:3)'; update(4:6)'; update(7:8)' 0];
            updateArray[0][0] = 1.0f + pUpdate[0]; updateArray[0][1] = pUpdate[1];        updateArray[0][2] = pUpdate[2]*scale;
            updateArray[1][0] = pUpdate[3];        updateArray[1][1] = 1.0f + pUpdate[4]; updateArray[1][2] = pUpdate[5]*scale;
            updateArray[2][0] = pUpdate[6]/scale;  updateArray[2][1] = pUpdate[7]/scale;  updateArray[2][2] = 1.0f;
          } else {
            AnkiError("PlanarTransformation_f32::Update", "Unknown transformation type %d", updateType);
            return RESULT_FAIL_INVALID_PARAMETER;
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

      Result PlanarTransformation_f32::Print(const char * const variableName) const
      {
        CoreTechPrint(variableName);
        CoreTechPrint(": center");
        this->centerOffset.Print();
        CoreTechPrint(" initialCorners");
        this->initialCorners.Print();
        CoreTechPrint("\n");

        return this->homography.Print("homography");
      }

      Quadrilateral<f32> PlanarTransformation_f32::Transform(const Quadrilateral<f32> &in, MemoryStack scratch, const f32 scale) const
      {
        Array<f32> xIn(1,4,scratch);
        Array<f32> yIn(1,4,scratch);
        Array<f32> xOut(1,4,scratch);
        Array<f32> yOut(1,4,scratch);

        for(s32 i=0; i<4; i++) {
          xIn[0][i] = in.corners[i].x;
          yIn[0][i] = in.corners[i].y;
        }

        TransformPoints(xIn, yIn, scale, this->initialPointsAreZeroCentered, false, xOut, yOut);

        Quadrilateral<f32> out;

        for(s32 i=0; i<4; i++) {
          out.corners[i].x = xOut[0][i];
          out.corners[i].y = yOut[0][i];
        }

        return out;
      }

      Result PlanarTransformation_f32::Transform(
        const Array<u8> &in,
        Array<u8> &out,
        MemoryStack scratch,
        const f32 scale) const
      {
        Result lastResult;

        AnkiConditionalErrorAndReturnValue(AreValid(in, out),
          RESULT_FAIL_INVALID_OBJECT, "PlanarTransformation_f32::Transform", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(AreEqualSize(in, out),
          RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Transform", "input and output are different sizes");

        AnkiConditionalErrorAndReturnValue(NotAliased(in, out),
          RESULT_FAIL_ALIASED_MEMORY, "PlanarTransformation_f32::Transform", "in and out cannot be the same");

        const s32 arrHeight = in.get_size(0);
        const s32 arrWidth = in.get_size(1);

        Array<f32> homographyInv(3,3,scratch);
        homographyInv.Set(this->homography);

        Invert3x3(
          homographyInv[0][0], homographyInv[0][1], homographyInv[0][2],
          homographyInv[1][0], homographyInv[1][1], homographyInv[1][2],
          homographyInv[2][0], homographyInv[2][1], homographyInv[2][2]);

        for(s32 y=0; y<3; y++) {
          for(s32 x=0; x<3; x++) {
            homographyInv[y][x] /= homographyInv[2][2];
          }
        }

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
            pXIn[x] = static_cast<f32>(x);
            pYIn[x] = static_cast<f32>(y);
          }
        }

        TransformPointsStatic(
          xIn, yIn, scale,
          this->centerOffset,
          this->get_transformType(), homographyInv,
          false, false,
          xTransformed, yTransformed);

        /*xIn.Print("xIn", 0,10,0,10);
        yIn.Print("yIn", 0,10,0,10);

        xTransformed.Print("xTransformed", 0,10,0,10);
        yTransformed.Print("yTransformed", 0,10,0,10);*/

        if((lastResult = Interp2<u8,u8>(in, xTransformed, yTransformed, out, INTERPOLATE_LINEAR, 0)) != RESULT_OK)
          return lastResult;

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::Transform(
        const FixedLengthList<Point<s16> > &in,
        FixedLengthList<Point<s16> > &out,
        MemoryStack scratch,
        const f32 scale
        ) const
      {
        AnkiConditionalErrorAndReturnValue(AreValid(in, out),
          RESULT_FAIL_INVALID_OBJECT, "PlanarTransformation_f32::Transform", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(in.get_size() == out.get_size(),
          RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::Transform", "input and output are different sizes");

        const s32 numPoints = in.get_size();

        Array<f32> xIn(1,numPoints,scratch,Flags::Buffer(false,false,false));
        Array<f32> yIn(1,numPoints,scratch,Flags::Buffer(false,false,false));
        Array<f32> xOut(1,numPoints,scratch,Flags::Buffer(false,false,false));
        Array<f32> yOut(1,numPoints,scratch,Flags::Buffer(false,false,false));

        const Point<s16> * restrict pIn = in.Pointer(0);

        f32 * restrict pXIn  = xIn.Pointer(0,0);
        f32 * restrict pYIn  = yIn.Pointer(0,0);

        for(s32 i=0; i<numPoints; i++) {
          pXIn[i] = pIn[i].x;
          pYIn[i] = pIn[i].y;
        }

        TransformPoints(xIn, yIn, scale, false, false, xOut, yOut);

        const f32 * restrict pXOut = xOut.Pointer(0,0);
        const f32 * restrict pYOut = yOut.Pointer(0,0);

        Point<s16> * restrict pOut = out.Pointer(0);

        for(s32 i=0; i<numPoints; i++) {
          pOut[i].x = static_cast<s16>(Round<s32>(pXOut[i]));
          pOut[i].y = static_cast<s16>(Round<s32>(pYOut[i]));
        }

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

        this->centerOffset = newTransformation.get_centerOffset(1.0f);

        this->initialCorners = newTransformation.get_initialCorners();

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::Serialize(const char *objectName, SerializedBuffer &buffer) const
      {
        s32 totalDataLength = this->get_serializationSize();

        void *segment = buffer.Allocate("PlanarTransformation_f32", objectName, totalDataLength);

        if(segment == NULL) {
          return RESULT_FAIL;
        }

        return SerializeRaw(objectName, &segment, totalDataLength);
      }

      Result PlanarTransformation_f32::SerializeRaw(const char *objectName, void ** buffer, s32 &bufferLength) const
      {
        if(SerializedBuffer::SerializeDescriptionStrings("PlanarTransformation_f32", objectName, buffer, bufferLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawBasicType<bool>("isValid", this->isValid, buffer, bufferLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawBasicType<s32>("transformType", this->transformType, buffer, bufferLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawArray<f32>("homography", this->homography, buffer, bufferLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawBasicType<Quadrilateral<f32> >("initialCorners", this->initialCorners, buffer, bufferLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawBasicType<Point<f32> >("centerOffset", this->centerOffset, buffer, bufferLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawBasicType<bool>("initialPointsAreZeroCentered", this->initialPointsAreZeroCentered, buffer, bufferLength) != RESULT_OK)
          return RESULT_FAIL;

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::Deserialize(char *objectName, void** buffer, s32 &bufferLength, MemoryStack &memory)
      {
        // TODO: check if the name is correct
        if(SerializedBuffer::DeserializeDescriptionStrings(NULL, objectName, buffer, bufferLength) != RESULT_OK)
          return RESULT_FAIL;

        this->isValid = SerializedBuffer::DeserializeRawBasicType<bool>(NULL, buffer, bufferLength);
        this->transformType = static_cast<Transformations::TransformType>(SerializedBuffer::DeserializeRawBasicType<s32>(NULL, buffer, bufferLength));
        this->homography = SerializedBuffer::DeserializeRawArray<f32>(NULL, buffer, bufferLength, memory);
        this->initialCorners = SerializedBuffer::DeserializeRawBasicType<Quadrilateral<f32> >(NULL, buffer, bufferLength);
        this->centerOffset = SerializedBuffer::DeserializeRawBasicType<Point<f32> >(NULL, buffer, bufferLength);
        this->initialPointsAreZeroCentered = SerializedBuffer::DeserializeRawBasicType<bool>(NULL, buffer, bufferLength);

        AnkiConditionalErrorAndReturnValue(this->homography.IsValid(),
          RESULT_FAIL, "PlanarTransformation_f32::Deserialize", "Parsing error");

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::set_transformType(const TransformType transformType)
      {
        if(transformType == TRANSFORM_TRANSLATION || transformType == TRANSFORM_AFFINE || transformType == TRANSFORM_PROJECTIVE) {
          this->transformType = transformType;
        } else {
          AnkiError("PlanarTransformation_f32::set_transformType", "Unknown transformation type %d", transformType);
          return RESULT_FAIL_INVALID_PARAMETER;
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

        //AnkiAssert(FLT_NEAR(in[2][2], 1.0f));

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

      Result PlanarTransformation_f32::set_initialPointsAreZeroCentered(const bool areTheyCentered)
      {
        this->initialPointsAreZeroCentered = areTheyCentered;
        return RESULT_OK;
      }

      bool PlanarTransformation_f32::get_initialPointsAreZeroCentered() const
      {
        return this->initialPointsAreZeroCentered;
      }

      Result PlanarTransformation_f32::set_centerOffset(const Point<f32> &centerOffset)
      {
        this->centerOffset = centerOffset;

        return RESULT_OK;
      }

      Point<f32> PlanarTransformation_f32::get_centerOffset(const f32 scale) const
      {
        if(FLT_NEAR(scale,1.0f)) {
          return this->centerOffset;
        } else {
          const Point<f32> scaledOffset(this->centerOffset.x / scale, this->centerOffset.y / scale);
          return scaledOffset;
        }
      }

      Quadrilateral<f32> PlanarTransformation_f32::get_transformedCorners(MemoryStack scratch) const
      {
        return this->Transform(this->get_initialCorners(), scratch);
      }

      f32 PlanarTransformation_f32::get_transformedOrientation(MemoryStack scratch) const
      {
        const Quadrilateral<f32> transformedCorners = this->Transform(this->get_initialCorners(), scratch);

        const Point<f32> transformedCenter = transformedCorners.ComputeCenter<f32>();

        const Point<f32> rightMidpoint = Point<f32>(
          (transformedCorners[Quadrilateral<f32>::TopRight].x + transformedCorners[Quadrilateral<f32>::BottomRight].x) / 2,
          (transformedCorners[Quadrilateral<f32>::TopRight].y + transformedCorners[Quadrilateral<f32>::BottomRight].y) / 2);

        const f32 dy = rightMidpoint.y - transformedCenter.y;
        const f32 dx = rightMidpoint.x - transformedCenter.x;

        const f32 orientation = atan2(dy, dx);

        return orientation;
      }

      Result PlanarTransformation_f32::TransformPointsStatic(
        const Array<f32> &xIn, const Array<f32> &yIn,
        const f32 scale,
        const Point<f32>& centerOffset,
        const TransformType transformType,
        const Array<f32> &homography,
        const bool inputPointsAreZeroCentered,
        const bool outputPointsAreZeroCentered,
        Array<f32> &xOut, Array<f32> &yOut)
      {
        AnkiConditionalErrorAndReturnValue(AreValid(homography, xIn, yIn, xOut, yOut),
          RESULT_FAIL_INVALID_OBJECT, "PlanarTransformation_f32::TransformPoints", "Invalid objects");

        AnkiConditionalErrorAndReturnValue(NotAliased(xIn, xOut, yIn, yOut),
          RESULT_FAIL_ALIASED_MEMORY, "PlanarTransformation_f32::TransformPoints", "In and Out arrays must be in different memory locations");

        AnkiConditionalErrorAndReturnValue(AreEqualSize(xIn, yIn, xOut, yOut),
          RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::TransformPoints", "All inputs and outputs must be the same size");

        const s32 numPointsY = xIn.get_size(0);
        const s32 numPointsX = xIn.get_size(1);

        // If the inputs or outputs should be shifted, set the offset appropriately
        const Point<f32> inputCenterOffset = inputPointsAreZeroCentered ? Point<f32>(0.0f, 0.0f) : Point<f32>(centerOffset.x, centerOffset.y);
        const Point<f32> outputCenterOffset = outputPointsAreZeroCentered ? Point<f32>(0.0f, 0.0f) : Point<f32>(centerOffset.x, centerOffset.y);

        if(transformType == TRANSFORM_TRANSLATION) {
          const f32 dx = (homography[0][2] - inputCenterOffset.x + outputCenterOffset.x) / scale;
          const f32 dy = (homography[1][2] - inputCenterOffset.y + outputCenterOffset.y) / scale;

          for(s32 y=0; y<numPointsY; y++) {
            const f32 * restrict pXIn = xIn.Pointer(y,0);
            const f32 * restrict pYIn = yIn.Pointer(y,0);
            f32 * restrict pXOut = xOut.Pointer(y,0);
            f32 * restrict pYOut = yOut.Pointer(y,0);

            for(s32 x=0; x<numPointsX; x++) {
              pXOut[x] = pXIn[x] + dx;
              pYOut[x] = pYIn[x] + dy;
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

                // Remove center offset (offset may be zero)
                const f32 xc = xIn - inputCenterOffset.x;
                const f32 yc = yIn - inputCenterOffset.y;

                const f32 xp = (h00*xc + h01*yc + h02);
                const f32 yp = (h10*xc + h11*yc + h12);

                // Restore center offset (offset may be zero)
                pXOut[x] = xp + outputCenterOffset.x;
                pYOut[x] = yp + outputCenterOffset.y;
              }
            } else {
              for(s32 x=0; x<numPointsX; x++) {
                const f32 xIn = pXIn[x] * scale;
                const f32 yIn = pYIn[x] * scale;

                // Remove center offset (offset may be zero)
                const f32 xc = xIn - inputCenterOffset.x;
                const f32 yc = yIn - inputCenterOffset.y;

                const f32 xp = (h00*xc + h01*yc + h02);
                const f32 yp = (h10*xc + h11*yc + h12);

                // Restore center offset (offset may be zero)
                pXOut[x] = (xp + outputCenterOffset.x) / scale;
                pYOut[x] = (yp + outputCenterOffset.y) / scale;
              }
            } // if(FLT_NEAR(scale, 1.0f))
          }
        } else if(transformType == TRANSFORM_PROJECTIVE) {
          const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
          const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
          const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = homography[2][2];

          //AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

          for(s32 y=0; y<numPointsY; y++) {
            const f32 * restrict pXIn = xIn.Pointer(y,0);
            const f32 * restrict pYIn = yIn.Pointer(y,0);
            f32 * restrict pXOut = xOut.Pointer(y,0);
            f32 * restrict pYOut = yOut.Pointer(y,0);

            if(FLT_NEAR(scale, 1.0f)) {
              for(s32 x=0; x<numPointsX; x++) {
                const f32 xIn = pXIn[x];
                const f32 yIn = pYIn[x];

                // Remove center offset (offset may be zero)
                const f32 xc = xIn - inputCenterOffset.x;
                const f32 yc = yIn - inputCenterOffset.y;

                const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);

                const f32 xp = (h00*xc + h01*yc + h02) * wpi;
                const f32 yp = (h10*xc + h11*yc + h12) * wpi;

                // Restore center offset (offset may be zero)
                pXOut[x] = xp + outputCenterOffset.x;
                pYOut[x] = yp + outputCenterOffset.y;
              }
            } else {
              for(s32 x=0; x<numPointsX; x++) {
                const f32 xIn = pXIn[x] * scale;
                const f32 yIn = pYIn[x] * scale;

                // Remove center offset (offset may be zero)
                const f32 xc = xIn - inputCenterOffset.x;
                const f32 yc = yIn - inputCenterOffset.y;

                const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);

                const f32 xp = (h00*xc + h01*yc + h02) * wpi;
                const f32 yp = (h10*xc + h11*yc + h12) * wpi;

                // Restore center offset (offset may be zero)
                pXOut[x] = (xp + outputCenterOffset.x) / scale;
                pYOut[x] = (yp + outputCenterOffset.y) / scale;
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

      s32 PlanarTransformation_f32::get_serializationSize()
      {
        // TODO: make the correct length
        return 512 + 16*SerializedBuffer::DESCRIPTION_STRING_LENGTH;
      }

      Result ComputeHomographyFromQuad(const Quadrilateral<s16> &quad, Array<f32> &homography, bool &numericalFailure, MemoryStack scratch)
      {
        Quadrilateral<f32> quadF32;
        quadF32.SetCast<s16>(quad);

        return ComputeHomographyFromQuad(quadF32, homography, numericalFailure, scratch);
      }

      Result ComputeHomographyFromQuad(const Quadrilateral<f32> &quad, Array<f32> &homography, bool &numericalFailure, MemoryStack scratch)
      {
        Quadrilateral<f32> originalQuad(
          Point<f32>(0,0),
          Point<f32>(0,1),
          Point<f32>(1,0),
          Point<f32>(1,1));

        return ComputeHomographyFromQuads(originalQuad, quad, homography, numericalFailure, scratch);
      }

      Result ComputeHomographyFromQuads(const Quadrilateral<f32> &originalQuad, const Quadrilateral<f32> &transformedQuad, Array<f32> &homography, bool &numericalFailure, MemoryStack scratch)
      {
        Result lastResult;

        AnkiConditionalErrorAndReturnValue(AreValid(homography, scratch),
          RESULT_FAIL_INVALID_OBJECT, "ComputeHomographyFromQuads", "Invalid objects");

        if(!originalQuad.IsConvex() || !transformedQuad.IsConvex()) {
          AnkiWarn("ComputeHomographyFromQuads", "Quad is not convex");

          homography.SetZero();
          homography[0][0] = 1;
          homography[1][1] = 1;
          homography[2][2] = 1;

          numericalFailure = true;

          return RESULT_OK;
        }

        FixedLengthList<Point<f32> > originalPoints(4, scratch);
        FixedLengthList<Point<f32> > transformedPoints(4, scratch);

        for(s32 i=0; i<4; i++) {
          originalPoints.PushBack(originalQuad[i]);
          transformedPoints.PushBack(transformedQuad[i]);
        }

        if((lastResult = Matrix::EstimateHomography(originalPoints, transformedPoints, homography, numericalFailure, scratch)) != RESULT_OK)
          return lastResult;

        return RESULT_OK;
      } // Result ComputeHomographyFromQuad(FixedLengthList<Quadrilateral<s16> > quads, FixedLengthList<Array<f32> > &homographies, MemoryStack scratch)

      Result PlanarTransformation_f32::VerifyTransformation_Projective_NearestNeighbor(
        const Array<u8> &templateImage,
        const IntegerCounts &templateIntegerCounts,
        const Rectangle<f32> &templateRegionOfInterest,
        const Array<u8> &nextImage,
        const IntegerCounts &nextImageHistogram,
        const f32 templateRegionHeight,
        const f32 templateRegionWidth,
        const s32 templateCoordinateIncrement,
        const u8 maxPixelDifference,
        s32 &meanAbsoluteDifference,
        s32 &numInBounds,
        s32 &numSimilarPixels,
        MemoryStack scratch) const
      {
        const s32 numStatisticsFractionalBits = 14;
        //const f32 lowPercentile = 0.1f;
        const f32 highPercentile = 0.95f;

        AnkiConditionalErrorAndReturnValue(AreEqualSize(templateImage, nextImage),
          RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::VerifyTransformation_Projective", "input images must be the same size");

        const s32 maxPixelDifferenceS32 = maxPixelDifference;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        const s32 whichScale = 0;
        const f32 scale = static_cast<f32>(1 << whichScale);

        const s32 initialImageScaleS32 = templateImage.get_size(1) / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);

        const Point<f32> centerOffsetScaled = this->get_centerOffset(initialImageScaleF32);

        const f32 roi_minX = templateRegionOfInterest.left - templateRegionWidth/2.0f;
        const f32 roi_maxX = templateRegionOfInterest.right - templateRegionWidth/2.0f;
        const f32 roi_minY = templateRegionOfInterest.top - templateRegionHeight/2.0f;
        const f32 roi_maxY = templateRegionOfInterest.bottom - templateRegionHeight/2.0f;

        Meshgrid<f32> originalCoordinates(
          Linspace(roi_minX, roi_maxX, static_cast<s32>(floorf((roi_maxX-roi_minX+1)/(scale)))),
          Linspace(roi_minY, roi_maxY, static_cast<s32>(floorf((roi_maxY-roi_minY+1)/(scale)))));

        const f32 templateCoordinateIncrementF32 = static_cast<f32>(templateCoordinateIncrement);

        const s32 xyReferenceMin = 0;
        const s32 xReferenceMax = nextImageWidth - 1;
        const s32 yReferenceMax = nextImageHeight - 1;

        const LinearSequence<f32> &yGridVector = originalCoordinates.get_yGridVector();
        const LinearSequence<f32> &xGridVector = originalCoordinates.get_xGridVector();

        const f32 yGridStart = yGridVector.get_start();
        const f32 xGridStart = xGridVector.get_start();

        const f32 yGridDelta = yGridVector.get_increment() * templateCoordinateIncrementF32;
        const f32 xGridDelta = xGridVector.get_increment() * templateCoordinateIncrementF32;

        const s32 yIterationMax = yGridVector.get_size();
        const s32 xIterationMax = xGridVector.get_size();

        const Array<f32> &homography = this->get_homography();
        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
        const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32; const f32 h22 = homography[2][2] * initialImageScaleF32; // TODO: should h22 be scaled?

        //const s32 templateMeanS32 = Round<s32>(templateIntegerCounts.mean);
        //const s32 nextImageMeanS32 = Round<s32>(nextImageHistogram.mean);

        //const s32 templateStdDivisor = Round<s32>(static_cast<f32>((1 << numStatisticsFractionalBits)) / templateIntegerCounts.standardDeviation);
        //const s32 nextImageStdDivisor = Round<s32>(static_cast<f32>((1 << numStatisticsFractionalBits)) / nextImageHistogram.standardDeviation);

        //const s32 templateLowS32 = ComputePercentile(templateIntegerCounts, lowPercentile);
        const s32 templateHighS32 = templateIntegerCounts.ComputePercentile(highPercentile);
        const s32 templateHighDivisorS32 = 255*Round<s32>(static_cast<f32>(1 << numStatisticsFractionalBits) / static_cast<f32>(templateHighS32));

        //const s32 nextImageLowS32 = ComputePercentile(nextImageHistogram, lowPercentile);
        const s32 nextImageHighS32 = nextImageHistogram.ComputePercentile(highPercentile);
        const s32 nextImageHighDivisorS32 = 255*Round<s32>(static_cast<f32>(1 << numStatisticsFractionalBits) / static_cast<f32>(nextImageHighS32));

        numInBounds = 0;
        numSimilarPixels = 0;
        s32 totalGrayvalueDifference = 0;

        // TODO: make the x and y limits from 1 to end-2
        //#if !defined(__EDG__)
        //        Matlab matlab(false);
        //        matlab.EvalString("template = zeros(240,320);");
        //        matlab.EvalString("warped = zeros(240,320);");
        //#endif

        f32 yOriginal = yGridStart;
        for(s32 y=0; y<yIterationMax; y+=templateCoordinateIncrement) {
          const u8 * restrict pTemplateImage = templateImage.Pointer(Round<s32>(y+templateRegionOfInterest.top), Round<s32>(templateRegionOfInterest.left));

          f32 xOriginal = xGridStart;

          for(s32 x=0; x<xIterationMax; x+=templateCoordinateIncrement) {
            // TODO: These two could be strength reduced
            const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
            const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

            const f32 normalization = 1.0f / (h20*xOriginal + h21*yOriginal + h22);

            const s32 xTransformedS32 = Round<s32>( (xTransformedRaw * normalization) + centerOffsetScaled.x );
            const s32 yTransformedS32 = Round<s32>( (yTransformedRaw * normalization) + centerOffsetScaled.y );

            xOriginal += xGridDelta;

            // If out of bounds, continue
            if(xTransformedS32 < xyReferenceMin || xTransformedS32 > xReferenceMax || yTransformedS32 < xyReferenceMin || yTransformedS32 > yReferenceMax) {
              continue;
            }

            numInBounds++;

            const s32 nextImagePixelValueRaw = *nextImage.Pointer(yTransformedS32, xTransformedS32);
            const s32 templatePixelValueRaw = pTemplateImage[x];

            const s32 nearestPixelValue  = (nextImagePixelValueRaw * nextImageHighDivisorS32) >> numStatisticsFractionalBits;
            const s32 templatePixelValue = (templatePixelValueRaw * templateHighDivisorS32) >> numStatisticsFractionalBits;

            const s32 grayvalueDifference = ABS(nearestPixelValue - templatePixelValue);

            //#if !defined(__EDG__)
            //            matlab.EvalString("template(%d,%d) = %d; warped(%d,%d) = %d;", yTransformedS32, xTransformedS32, templatePixelValue, yTransformedS32, xTransformedS32, nearestPixelValue);
            //#endif

            totalGrayvalueDifference += grayvalueDifference;

            if(grayvalueDifference <= maxPixelDifferenceS32) {
              numSimilarPixels++;
            }
          } // for(s32 x=0; x<xIterationMax; x++)

          yOriginal += yGridDelta;
        } // for(s32 y=0; y<yIterationMax; y++)

        if(numInBounds > 0) {
          meanAbsoluteDifference = totalGrayvalueDifference / numInBounds;
        }

        return RESULT_OK;
      }

      Result PlanarTransformation_f32::VerifyTransformation_Projective_LinearInterpolate(
        const Array<u8> &templateImage,
        const Rectangle<f32> &templateRegionOfInterest,
        const Array<u8> &nextImage,
        const f32 templateRegionHeight,
        const f32 templateRegionWidth,
        const s32 templateCoordinateIncrement,
        const u8 maxPixelDifference,
        s32 &meanAbsoluteDifference,
        s32 &numInBounds,
        s32 &numSimilarPixels,
        MemoryStack scratch) const
      {
        // This method is heavily based on Interp2_Projective
        // The call would be like: Interp2_Projective<u8,u8>(nextImage, originalCoordinates, interpolationHomography, centerOffset, nextImageTransformed2d, INTERPOLATE_LINEAR, 0);

        AnkiConditionalErrorAndReturnValue(AreEqualSize(templateImage, nextImage),
          RESULT_FAIL_INVALID_SIZE, "PlanarTransformation_f32::VerifyTransformation_Projective", "input images must be the same size");

        const s32 maxPixelDifferenceS32 = maxPixelDifference;

        const s32 nextImageHeight = nextImage.get_size(0);
        const s32 nextImageWidth = nextImage.get_size(1);

        const s32 whichScale = 0;
        const f32 scale = static_cast<f32>(1 << whichScale);

        const s32 initialImageScaleS32 = templateImage.get_size(1) / nextImageWidth;
        const f32 initialImageScaleF32 = static_cast<f32>(initialImageScaleS32);

        const Point<f32> centerOffsetScaled = this->get_centerOffset(initialImageScaleF32);

        const f32 roi_minX = templateRegionOfInterest.left - templateRegionWidth/2.0f;
        const f32 roi_maxX = templateRegionOfInterest.right - templateRegionWidth/2.0f;
        const f32 roi_minY = templateRegionOfInterest.top - templateRegionHeight/2.0f;
        const f32 roi_maxY = templateRegionOfInterest.bottom - templateRegionHeight/2.0f;

        Meshgrid<f32> originalCoordinates(
          Linspace(roi_minX, roi_maxX, static_cast<s32>(floorf((roi_maxX-roi_minX+1)/(scale)))),
          Linspace(roi_minY, roi_maxY, static_cast<s32>(floorf((roi_maxY-roi_minY+1)/(scale)))));

        // Unused, remove?
        //const s32 outHeight = originalCoordinates.get_yGridVector().get_size();
        //const s32 outWidth = originalCoordinates.get_xGridVector().get_size();

        const f32 xyReferenceMin = 0.0f;
        const f32 xReferenceMax = static_cast<f32>(nextImageWidth) - 1.0f;
        const f32 yReferenceMax = static_cast<f32>(nextImageHeight) - 1.0f;

        const LinearSequence<f32> &yGridVector = originalCoordinates.get_yGridVector();
        const LinearSequence<f32> &xGridVector = originalCoordinates.get_xGridVector();

        const f32 yGridStart = yGridVector.get_start();
        const f32 xGridStart = xGridVector.get_start();

        const f32 yGridDelta = yGridVector.get_increment();
        const f32 xGridDelta = xGridVector.get_increment();

        const s32 yIterationMax = yGridVector.get_size();
        const s32 xIterationMax = xGridVector.get_size();

        const Array<f32> &homography = this->get_homography();
        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2] / initialImageScaleF32;
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2] / initialImageScaleF32;
        const f32 h20 = homography[2][0] * initialImageScaleF32; const f32 h21 = homography[2][1] * initialImageScaleF32; const f32 h22 = homography[2][2] * initialImageScaleF32; // TODO: should h22 be scaled?

        numInBounds = 0;
        numSimilarPixels = 0;
        s32 totalGrayvalueDifference = 0;

        // TODO: make the x and y limits from 1 to end-2

        //Matlab matlab(false);
        //matlab.EvalString("template = zeros(240,320);");
        //matlab.EvalString("warped = zeros(240,320);");

        f32 yOriginal = yGridStart;
        for(s32 y=0; y<yIterationMax; y+=templateCoordinateIncrement) {
          const u8 * restrict pTemplateImage = templateImage.Pointer(Round<s32>(y+templateRegionOfInterest.top), Round<s32>(templateRegionOfInterest.left));

          f32 xOriginal = xGridStart;

          for(s32 x=0; x<xIterationMax; x+=templateCoordinateIncrement) {
            // TODO: These two could be strength reduced
            const f32 xTransformedRaw = h00*xOriginal + h01*yOriginal + h02;
            const f32 yTransformedRaw = h10*xOriginal + h11*yOriginal + h12;

            const f32 normalization = 1.0f / (h20*xOriginal + h21*yOriginal + h22);

            const f32 xTransformed = (xTransformedRaw * normalization) + centerOffsetScaled.x;
            const f32 yTransformed = (yTransformedRaw * normalization) + centerOffsetScaled.y;

            xOriginal += xGridDelta * templateCoordinateIncrement;

            const f32 x0 = floorf(xTransformed);
            const f32 x1 = x0 + 1.0f; // ceilf(xTransformed);

            const f32 y0 = floorf(yTransformed);
            const f32 y1 = y0 + 1.0f; // ceilf(yTransformed);

            // If out of bounds, continue
            if(x0 < xyReferenceMin || x1 > xReferenceMax || y0 < xyReferenceMin || y1 > yReferenceMax) {
              continue;
            }

            numInBounds++;

            const f32 alphaX = xTransformed - x0;
            const f32 alphaXinverse = 1 - alphaX;

            const f32 alphaY = yTransformed - y0;
            const f32 alphaYinverse = 1.0f - alphaY;

            const s32 y0S32 = Round<s32>(y0);
            const s32 y1S32 = Round<s32>(y1);
            const s32 x0S32 = Round<s32>(x0);

            const u8 * restrict pReference_y0 = nextImage.Pointer(y0S32, x0S32);
            const u8 * restrict pReference_y1 = nextImage.Pointer(y1S32, x0S32);

            const f32 pixelTL = *pReference_y0;
            const f32 pixelTR = *(pReference_y0+1);
            const f32 pixelBL = *pReference_y1;
            const f32 pixelBR = *(pReference_y1+1);

            const s32 interpolatedPixelValue = Round<s32>(InterpolateBilinear2d<f32>(pixelTL, pixelTR, pixelBL, pixelBR, alphaY, alphaYinverse, alphaX, alphaXinverse));
            const s32 templatePixelValue = pTemplateImage[x];
            const s32 grayvalueDifference = ABS(interpolatedPixelValue - templatePixelValue);

            //matlab.EvalString("template(%d,%d) = %d; warped(%d,%d) = %d;", Round<s32>(y0), Round<s32>(x0), templatePixelValue, Round<s32>(y0), Round<s32>(x0), interpolatedPixelValue);

            totalGrayvalueDifference += grayvalueDifference;

            if(grayvalueDifference <= maxPixelDifferenceS32) {
              numSimilarPixels++;
            }
          } // for(s32 x=0; x<xIterationMax; x++)

          yOriginal += yGridDelta * templateCoordinateIncrement;
        } // for(s32 y=0; y<yIterationMax; y++)

        if(numInBounds > 0) {
          meanAbsoluteDifference = totalGrayvalueDifference / numInBounds;
        }

        return RESULT_OK;
      } // Result PlanarTransformation_f32::VerifyTransformation_Projective_LinearInterpolate()
    } // namespace Transformations
  } // namespace Embedded
} // namespace Anki
