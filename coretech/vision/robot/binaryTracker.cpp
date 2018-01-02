/**
File: binaryTracker.cpp
Author: Peter Barnum
Created: 2014-02-21

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/binaryTracker.h"
#include "coretech/common/robot/matlabInterface.h"
#include "coretech/common/robot/interpolate.h"
#include "coretech/common/robot/arrayPatterns.h"
#include "coretech/common/robot/find.h"
#include "coretech/common/robot/benchmarking.h"
#include "coretech/common/robot/draw.h"
#include "coretech/common/robot/comparisons.h"
#include "coretech/common/robot/serialize.h"
#include "coretech/common/robot/compress.h"

#include "coretech/vision/robot/fiducialDetection.h"
#include "coretech/vision/robot/imageProcessing.h"
#include "coretech/vision/robot/transformations.h"

#include "coretech/common/robot/hostIntrinsics_m4.h"

#include "coretech/vision/robot/marker_battery.h"

#include "util/math/math.h"

#include <math.h>

#define USE_ARM_ACCELERATION

#ifndef USE_ARM_ACCELERATION
#warning not using USE_ARM_ACCELERATION
#endif

namespace Anki
{
  namespace Embedded
  {
    namespace TemplateTracker
    {
      NO_INLINE static s32 RoundS32_minusPointFive(f32 x)
      {
#if !defined(USE_ARM_ACCELERATION)
        // Some platforms may not round to zero correctly, so do the function calls
        if(x > 0)
          return static_cast<s32>(floorf(x));
        else
          return static_cast<s32>(ceilf(x - 1.0f));
#else
        // The M4 rounds to zero correctly, without the function call
        if(x > 0)
          return static_cast<s32>(x);
        else
          return static_cast<s32>(x - 1.0f);
#endif
      }

      BinaryTracker::EdgeDetectionParameters::EdgeDetectionParameters()
        : type(EDGE_TYPE_GRAYVALUE), threshold_yIncrement(-1), threshold_xIncrement(-1), threshold_blackPercentile(-1), threshold_whitePercentile(-1), threshold_scaleRegionPercent(-1), minComponentWidth(-1), maxDetectionsPerType(-1), combHalfWidth(-1), combResponseThreshold(-1), everyNLines(-1)
      {
      }

      BinaryTracker::EdgeDetectionParameters::EdgeDetectionParameters(EdgeDetectionType type, s32 threshold_yIncrement, s32 threshold_xIncrement, f32 threshold_blackPercentile, f32 threshold_whitePercentile, f32 threshold_scaleRegionPercent, s32 minComponentWidth, s32 maxDetectionsPerType, s32 combHalfWidth, s32 combResponseThreshold, s32 everyNLines)
        : type(type), threshold_yIncrement(threshold_yIncrement), threshold_xIncrement(threshold_xIncrement), threshold_blackPercentile(threshold_blackPercentile), threshold_whitePercentile(threshold_whitePercentile), threshold_scaleRegionPercent(threshold_scaleRegionPercent), minComponentWidth(minComponentWidth), maxDetectionsPerType(maxDetectionsPerType), combHalfWidth(combHalfWidth), combResponseThreshold(combResponseThreshold), everyNLines(everyNLines)
      {
      }

      BinaryTracker::BinaryTracker()
        : isValid(false)
      {
      }

      BinaryTracker::BinaryTracker(
        const Array<u8> &templateImage,
        const Quadrilateral<f32> &templateQuad,
        const f32 scaleTemplateRegionPercent, //< Shrinks the region if less-than 1.0, expands the region if greater-than 1.0
        const EdgeDetectionParameters &edgeDetectionParams,
        MemoryStack &fastMemory,
        MemoryStack &slowMemory)
        : isValid(false)
      {
        this->templateImageHeight = templateImage.get_size(0);
        this->templateImageWidth = templateImage.get_size(1);

        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0,
          "BinaryTracker::BinaryTracker", "template widths and heights must be greater than zero");

        AnkiConditionalErrorAndReturn(AreValid(templateImage, fastMemory, slowMemory),
          "BinaryTracker::BinaryTracker", "Invalid objects");

        // TODO: make this work for non-qvga resolution
        Point<f32> centerOffset((templateImageWidth-1) / 2.0f, (templateImageHeight-1) / 2.0f);
        this->transformation = Transformations::PlanarTransformation_f32(Transformations::TRANSFORM_PROJECTIVE, templateQuad, centerOffset, slowMemory);

        this->templateQuad = templateQuad;

        this->templateEdges.xDecreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastMemory);
        this->templateEdges.xIncreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastMemory);
        this->templateEdges.yDecreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastMemory);
        this->templateEdges.yIncreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastMemory);

        this->templateImage = Array<u8>(templateImageHeight, templateImageWidth, slowMemory);
        this->templateImage.Set(templateImage);

        this->originalTemplateOrientation = 0.0f;

        AnkiConditionalErrorAndReturn(AreValid(this->templateEdges.xDecreasing, this->templateEdges.xIncreasing, this->templateEdges.yDecreasing, this->templateEdges.yIncreasing),
          "BinaryTracker::BinaryTracker", "Could not allocate local memory");

        const Rectangle<s32> edgeDetection_imageRegionOfInterest = templateQuad.ComputeBoundingRectangle<s32>().ComputeScaledRectangle<s32>(edgeDetectionParams.threshold_scaleRegionPercent);

        this->templateIntegerCounts = IntegerCounts(
          this->templateImage, edgeDetection_imageRegionOfInterest,
          edgeDetectionParams.threshold_yIncrement,
          edgeDetectionParams.threshold_xIncrement,
          fastMemory);

        AnkiConditionalErrorAndReturn(this->templateIntegerCounts.IsValid() ,
          "BinaryTracker::BinaryTracker", "Could not allocate local memory");

        this->lastGrayvalueThreshold = ComputeGrayvalueThreshold(this->templateIntegerCounts, edgeDetectionParams.threshold_blackPercentile, edgeDetectionParams.threshold_whitePercentile);

        //this->lastImageIntegerCounts.Set(this->templateIntegerCounts);

        const Rectangle<s32> templateRect = templateQuad.ComputeBoundingRectangle<s32>().ComputeScaledRectangle<s32>(scaleTemplateRegionPercent);

        Result result = RESULT_FAIL;
        if(edgeDetectionParams.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
          result = DetectBlurredEdges_GrayvalueThreshold(templateImage, templateRect, this->lastGrayvalueThreshold, edgeDetectionParams.minComponentWidth, edgeDetectionParams.everyNLines, this->templateEdges);
        } else if(edgeDetectionParams.type == TemplateTracker::BinaryTracker::EDGE_TYPE_DERIVATIVE) {
          result = DetectBlurredEdges_DerivativeThreshold(templateImage, templateRect, edgeDetectionParams.combHalfWidth, edgeDetectionParams.combResponseThreshold, edgeDetectionParams.everyNLines, this->templateEdges);
        }

        this->lastUsedGrayvalueThreshold = this->lastGrayvalueThreshold;

        AnkiConditionalErrorAndReturn(result == RESULT_OK,
          "BinaryTracker::BinaryTracker", "DetectBlurredEdge failed");

        this->isValid = true;
      } // BinaryTracker::BinaryTracker()

      BinaryTracker::BinaryTracker(
        const Anki::Vision::MarkerType markerType,
        const Array<u8> &templateImage,
        const Quadrilateral<f32> &templateQuad,
        const f32 scaleTemplateRegionPercent,
        const EdgeDetectionParameters &edgeDetectionParams,
        MemoryStack &fastMemory,
        MemoryStack &slowMemory)
        : isValid(false)
      {
        const bool useRealTemplateImage = true;

        this->templateImageHeight = templateImage.get_size(0);
        this->templateImageWidth = templateImage.get_size(1);

        // Only a few markers are currently supported

        // TODO: Update this now that we've removed the battery marker image

        //AnkiConditionalErrorAndReturn(markerType == Anki::Vision::MARKER_BATTERIES,
        //  "BinaryTracker::BinaryTracker", "markerType %d is not supported for header initialization", markerType);
        AnkiError("BinaryTracker::BinaryTracker", "BinaryTracker needs to be updated now that Battery marker has been removed.");

        AnkiConditionalErrorAndReturn(templateImageHeight > 0 && templateImageWidth > 0,
          "BinaryTracker::BinaryTracker", "template widths and heights must be greater than zero");

        // TODO: make this work for non-qvga resolution
        Point<f32> centerOffset((templateImageWidth-1) / 2.0f, (templateImageHeight-1) / 2.0f);
        this->transformation = Transformations::PlanarTransformation_f32(Transformations::TRANSFORM_PROJECTIVE, templateQuad, centerOffset, slowMemory);

        this->templateQuad = templateQuad;

        this->templateEdges.xDecreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastMemory);
        this->templateEdges.xIncreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastMemory);
        this->templateEdges.yDecreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastMemory);
        this->templateEdges.yIncreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastMemory);

        //this->templateIntegerCounts = IntegerCounts(256, fastMemory);
        //this->lastImageIntegerCounts = IntegerCounts(256, fastMemory);

        this->templateImage = Array<u8>(templateImageHeight, templateImageWidth, slowMemory);

        AnkiConditionalErrorAndReturn(
          AreValid(this->templateEdges.xDecreasing, this->templateEdges.xIncreasing, this->templateEdges.yDecreasing, this->templateEdges.yIncreasing, this->templateImage),
          "BinaryTracker::BinaryTracker", "Could not allocate local memory");

        this->templateImage.Set(templateImage);

        // Place the header image in the templateImage
        {
          PUSH_MEMORY_STACK(slowMemory);

          const s32 borderWidth = edgeDetectionParams.minComponentWidth + 2;

          //
          // Decode the compressed binary image to an uncompressed image
          //

          Array<u8> binaryTemplate;

          // TODO: BinaryTracker needs to be updated now that Battery marker has been removed
          if(false) { //markerType == Anki::Vision::MARKER_BATTERIES) {
            binaryTemplate = DecodeRunLengthBinary<u8>(
              &battery[0], battery_WIDTH,
              battery_ORIGINAL_HEIGHT, battery_ORIGINAL_WIDTH, Flags::Buffer(false,false,false),
              slowMemory);
          }

          AnkiConditionalErrorAndReturn(binaryTemplate.IsValid(),
            "BinaryTracker::BinaryTracker", "Could not allocate local memory");

          Matrix::DotMultiply<u8,u8,u8>(binaryTemplate, 255, binaryTemplate);

          const s32 binaryTemplateHeight = binaryTemplate.get_size(0);
          const s32 binaryTemplateWidth = binaryTemplate.get_size(1);

          //
          // Create a padded version of the binary template
          //

          const s32 binaryTemplateWithBorderHeight = binaryTemplateHeight + 2*borderWidth;
          const s32 binaryTemplateWithBorderWidth = binaryTemplateWidth + 2*borderWidth;
          Array<u8> binaryTemplateWithBorder(binaryTemplateWithBorderHeight, binaryTemplateWithBorderWidth, slowMemory, Flags::Buffer(false,false,false));

          AnkiConditionalErrorAndReturn(binaryTemplateWithBorder.IsValid(),
            "BinaryTracker::BinaryTracker", "Could not allocate local memory");

          binaryTemplateWithBorder.Set(255);

          for(s32 y=0; y<binaryTemplateHeight; y++) {
            const u8 * restrict pBinaryTemplate = binaryTemplate.Pointer(y,0);
            u8 * restrict pBinaryTemplateWithBorder = binaryTemplateWithBorder.Pointer(y+borderWidth,borderWidth);

            memcpy(pBinaryTemplateWithBorder, pBinaryTemplate, binaryTemplateWidth);
          }

          //
          // Compute the warp between the binary header and fiducial detection
          //

          const f32 binaryTemplateHeightF32 = static_cast<f32>(binaryTemplate.get_size(0));
          const f32 binaryTemplateWidthF32 = static_cast<f32>(binaryTemplate.get_size(1));

          FixedLengthList<Point<f32> > binaryCorners(4, slowMemory);
          binaryCorners.set_size(4);
          binaryCorners[0] = Point<f32>(0.5f+borderWidth,                        0.5f+borderWidth);
          binaryCorners[1] = Point<f32>(0.5f+borderWidth,                        binaryTemplateHeightF32-0.5f+borderWidth);
          binaryCorners[2] = Point<f32>(binaryTemplateWidthF32-0.5f+borderWidth, 0.5f+borderWidth);
          binaryCorners[3] = Point<f32>(binaryTemplateWidthF32-0.5f+borderWidth, binaryTemplateHeightF32-0.5f+borderWidth);

          FixedLengthList<Point<f32> > templateCorners(4, slowMemory);
          templateCorners.set_size(4);
          templateCorners[0] = templateQuad[0];
          templateCorners[1] = templateQuad[1];
          templateCorners[2] = templateQuad[2];
          templateCorners[3] = templateQuad[3];

          Array<f32> binaryTemplateHomography(3, 3, slowMemory);
          bool numericalFailure;
          Matrix::EstimateHomography(binaryCorners, templateCorners, binaryTemplateHomography, numericalFailure, slowMemory);

          Transformations::PlanarTransformation_f32 binaryTemplateTransform(
            Transformations::TRANSFORM_PROJECTIVE,
            Quadrilateral<f32>(binaryCorners[0], binaryCorners[1], binaryCorners[2], binaryCorners[3]),
            binaryTemplateHomography,
            Point<f32>(0.0f,0.0f),
            slowMemory);

          this->originalTemplateOrientation = binaryTemplateTransform.get_transformedOrientation(slowMemory);

          Array<u8> rotatedBinaryTemplateWithBorder(binaryTemplateWithBorder.get_size(0), binaryTemplateWithBorder.get_size(1), slowMemory);

          s32 rotationOrder[4];
          if(this->originalTemplateOrientation >= (-M_PI_F/4) && this->originalTemplateOrientation < (M_PI_F/4)) { // No rotation
            rotatedBinaryTemplateWithBorder.Set(binaryTemplateWithBorder);

            rotationOrder[0] = 0;
            rotationOrder[1] = 1;
            rotationOrder[2] = 2;
            rotationOrder[3] = 3;
          } else if(this->originalTemplateOrientation >= (M_PI_F/4) && this->originalTemplateOrientation < (3*M_PI_F/4)) { // Rotate 90 degrees clockwise
            Matrix::Rotate90<u8,u8>(binaryTemplateWithBorder, rotatedBinaryTemplateWithBorder);

            rotationOrder[0] = 2;
            rotationOrder[1] = 0;
            rotationOrder[2] = 3;
            rotationOrder[3] = 1;
          } else if(this->originalTemplateOrientation >= (3*M_PI_F/4) && this->originalTemplateOrientation < (5*M_PI_F/4)) { // Rotate 180 degrees clockwise
            Matrix::Rotate180<u8,u8>(binaryTemplateWithBorder, rotatedBinaryTemplateWithBorder);

            rotationOrder[0] = 3;
            rotationOrder[1] = 2;
            rotationOrder[2] = 1;
            rotationOrder[3] = 0;
          } else { // Rotate 270 degrees clockwise
            Matrix::Rotate270<u8,u8>(binaryTemplateWithBorder, rotatedBinaryTemplateWithBorder);

            rotationOrder[0] = 1;
            rotationOrder[1] = 3;
            rotationOrder[2] = 0;
            rotationOrder[3] = 2;
          }

          FixedLengthList<Point<f32> > rotatedBinaryCorners(4, slowMemory);
          rotatedBinaryCorners.set_size(4);

          for(s32 i=0; i<4; i++) {
            rotatedBinaryCorners[i] = binaryCorners[rotationOrder[i]];
          }

          Matrix::EstimateHomography(rotatedBinaryCorners, templateCorners, binaryTemplateHomography, numericalFailure, slowMemory);

          binaryTemplateTransform.set_homography(binaryTemplateHomography);

          // Extract the edges, and transform them to the actual fiducial detection location
          // TODO: add subpixel
          const Rectangle<s32> templateRect(0, binaryTemplateWithBorderWidth-1, 0, binaryTemplateWithBorderHeight-1);

          Result result = RESULT_FAIL;
          if(edgeDetectionParams.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
            result = DetectBlurredEdges_GrayvalueThreshold(rotatedBinaryTemplateWithBorder, templateRect, 128, edgeDetectionParams.minComponentWidth, edgeDetectionParams.everyNLines, this->templateEdges);
          } else if(edgeDetectionParams.type == TemplateTracker::BinaryTracker::EDGE_TYPE_DERIVATIVE) {
            result = DetectBlurredEdges_DerivativeThreshold(rotatedBinaryTemplateWithBorder, templateRect, edgeDetectionParams.combHalfWidth, edgeDetectionParams.combResponseThreshold, edgeDetectionParams.everyNLines, this->templateEdges);
          }

          this->templateEdges.imageHeight = templateImageHeight;
          this->templateEdges.imageWidth = templateImageWidth;

          AnkiConditionalErrorAndReturn(result == RESULT_OK,
            "BinaryTracker::BinaryTracker", "DetectBlurredEdge failed");

          binaryTemplateTransform.Transform(this->templateEdges.xDecreasing, this->templateEdges.xDecreasing, slowMemory);
          binaryTemplateTransform.Transform(this->templateEdges.xIncreasing, this->templateEdges.xIncreasing, slowMemory);
          binaryTemplateTransform.Transform(this->templateEdges.yDecreasing, this->templateEdges.yDecreasing, slowMemory);
          binaryTemplateTransform.Transform(this->templateEdges.yIncreasing, this->templateEdges.yIncreasing, slowMemory);

          //this->templateImage.Show("template real", false, false, true);
          //this->ShowTemplate("Template", false, true);

          if(!useRealTemplateImage) {
            // Just warp the binary header image to the template image
            bool numericalFailure;
            Matrix::EstimateHomography(templateCorners, binaryCorners, binaryTemplateHomography, numericalFailure, slowMemory);
            const Point<f32> centerOffset(0.0f, 0.0f);
            Meshgrid<f32> originalCoordinates( LinearSequence<f32>(0.5f, 1.0f, templateImageWidth-0.5f), LinearSequence<f32>(0.5f, 1.0f, templateImageHeight-0.5f));

            Interp2_Projective<u8,u8>(binaryTemplateWithBorder, originalCoordinates, binaryTemplateHomography, centerOffset, this->templateImage, INTERPOLATE_LINEAR, 255);
          }

          //this->templateImage.Show("template binary", true, false, true);

          //Matlab matlab(false);

          //matlab.PutArray(binaryTemplate, "binaryTemplate");
          //matlab.PutArray(binaryTemplateWithBorder, "binaryTemplateWithBorder");
          //matlab.PutArray(this->templateImage, "templateImage");
        }

        const Rectangle<s32> edgeDetection_imageRegionOfInterest = templateQuad.ComputeBoundingRectangle<s32>().ComputeScaledRectangle<s32>(edgeDetectionParams.threshold_scaleRegionPercent);

        this->templateIntegerCounts = IntegerCounts(
          this->templateImage, edgeDetection_imageRegionOfInterest,
          edgeDetectionParams.threshold_yIncrement,
          edgeDetectionParams.threshold_xIncrement,
          fastMemory);

        AnkiConditionalErrorAndReturn(this->templateIntegerCounts.IsValid() ,
          "BinaryTracker::BinaryTracker", "Could not allocate local memory");

        this->lastGrayvalueThreshold = ComputeGrayvalueThreshold(this->templateIntegerCounts, edgeDetectionParams.threshold_blackPercentile, edgeDetectionParams.threshold_whitePercentile);

        //this->lastImageIntegerCounts.Set(this->templateIntegerCounts);
        this->lastUsedGrayvalueThreshold = this->lastGrayvalueThreshold;

        this->isValid = true;
      }

      Result BinaryTracker::ShowTemplate(const char * windowName, const bool waitForKeypress, const bool fitImageToWindow, const f32 displayScale) const
      {
#if !ANKICORETECH_EMBEDDED_USE_OPENCV || defined(__GNUC__)
        return RESULT_FAIL;
#else
        //if(!this->IsValid())
        //  return RESULT_FAIL;

        cv::Mat toShow = this->templateEdges.DrawIndexes(displayScale);

        if(toShow.cols == 0)
          return RESULT_FAIL;

        if(fitImageToWindow) {
          cv::namedWindow(windowName, CV_WINDOW_NORMAL);
        } else {
          cv::namedWindow(windowName, CV_WINDOW_AUTOSIZE);
        }

        cv::imshow(windowName, toShow);

        if(waitForKeypress)
          cv::waitKey();

        return RESULT_OK;
#endif // #if !ANKICORETECH_EMBEDDED_USE_OPENCV ... #else
      } // Result BinaryTracker::ShowTemplate()

      bool BinaryTracker::IsValid() const
      {
        if(!this->isValid)
          return false;

        if(!AreValid(templateEdges.xDecreasing, templateEdges.xIncreasing, templateEdges.yDecreasing, templateEdges.yIncreasing))
          return false;

        return true;
      } // bool BinaryTracker::IsValid()

      Result BinaryTracker::UpdateTransformation(const Array<f32> &update, const f32 scale, MemoryStack scratch, Transformations::TransformType updateType)
      {
        return this->transformation.Update(update, scale, scratch, updateType);
      }

      Result BinaryTracker::Serialize(const char *objectName, SerializedBuffer &buffer) const
      {
        s32 totalDataLength = this->get_serializationSize();

        void *segment = buffer.Allocate("BinaryTracker", objectName, totalDataLength);

        if(segment == NULL) {
          return RESULT_FAIL;
        }

        if(SerializedBuffer::SerializeDescriptionStrings("BinaryTracker", objectName, &segment, totalDataLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawBasicType<bool>("templateEdges.isValid", this->isValid, &segment, totalDataLength) != RESULT_OK)
          return RESULT_FAIL;

        // First, serialize the transformation
        if(this->transformation.SerializeRaw("transformation", &segment, totalDataLength) != RESULT_OK)
          return RESULT_FAIL;

        // Next, serialize the template lists
        if(SerializedBuffer::SerializeRawBasicType<s32>("templateEdges.imageHeight", this->templateEdges.imageHeight, &segment, totalDataLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawBasicType<s32>("templateEdges.imageWidth", this->templateEdges.imageWidth, &segment, totalDataLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawFixedLengthList<Point<s16> >("templateEdges.xDecreasing", this->templateEdges.xDecreasing, &segment, totalDataLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawFixedLengthList<Point<s16> >("templateEdges.xIncreasing", this->templateEdges.xIncreasing, &segment, totalDataLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawFixedLengthList<Point<s16> >("templateEdges.yDecreasing", this->templateEdges.yDecreasing, &segment, totalDataLength) != RESULT_OK)
          return RESULT_FAIL;

        if(SerializedBuffer::SerializeRawFixedLengthList<Point<s16> >("templateEdges.yIncreasing", this->templateEdges.yIncreasing, &segment, totalDataLength) != RESULT_OK)
          return RESULT_FAIL;

        return RESULT_OK;
      }

      Result BinaryTracker::Deserialize(char *objectName, void** buffer, s32 &bufferLength, MemoryStack &memory)
      {
        // TODO: check if the name is correct
        if(SerializedBuffer::DeserializeDescriptionStrings(NULL, objectName, buffer, bufferLength) != RESULT_OK)
          return RESULT_FAIL;

        this->isValid = SerializedBuffer::DeserializeRawBasicType<bool>(NULL, buffer, bufferLength);

        // First, deserialize the transformation
        //this->transformation = Transformations::PlanarTransformation_f32(Transformations::TRANSFORM_PROJECTIVE, memory);
        this->transformation.Deserialize(objectName, buffer, bufferLength, memory);

        // Next, deserialize the template lists
        this->templateEdges.imageHeight = SerializedBuffer::DeserializeRawBasicType<s32>(NULL, buffer, bufferLength);
        this->templateEdges.imageWidth  = SerializedBuffer::DeserializeRawBasicType<s32>(NULL, buffer, bufferLength);
        this->templateEdges.xDecreasing = SerializedBuffer::DeserializeRawFixedLengthList<Point<s16> >(NULL, buffer, bufferLength, memory);
        this->templateEdges.xIncreasing = SerializedBuffer::DeserializeRawFixedLengthList<Point<s16> >(NULL, buffer, bufferLength, memory);
        this->templateEdges.yDecreasing = SerializedBuffer::DeserializeRawFixedLengthList<Point<s16> >(NULL, buffer, bufferLength, memory);
        this->templateEdges.yIncreasing = SerializedBuffer::DeserializeRawFixedLengthList<Point<s16> >(NULL, buffer, bufferLength, memory);

        this->templateImageHeight = this->templateEdges.imageHeight;
        this->templateImageWidth = this->templateEdges.imageWidth;

        this->isValid = true;

        return RESULT_OK;
      }

      s32 BinaryTracker::get_numTemplatePixels() const
      {
        return this->templateEdges.xDecreasing.get_size() +
          this->templateEdges.xIncreasing.get_size() +
          this->templateEdges.yDecreasing.get_size() +
          this->templateEdges.yIncreasing.get_size();
      }

      Result BinaryTracker::set_transformation(const Transformations::PlanarTransformation_f32 &transformation)
      {
        return this->transformation.Set(transformation);
      }

      Transformations::PlanarTransformation_f32 BinaryTracker::get_transformation() const
      {
        return transformation;
      }

      Result BinaryTracker::ComputeAllIndexLimits(const EdgeLists &imageEdges, AllIndexLimits &allLimits, MemoryStack &memory)
      {
        allLimits.xDecreasing_yStartIndexes = Array<s32>(1, imageEdges.imageHeight+1, memory, Flags::Buffer(false,false,false));
        allLimits.xIncreasing_yStartIndexes = Array<s32>(1, imageEdges.imageHeight+1, memory, Flags::Buffer(false,false,false));
        allLimits.yDecreasing_xStartIndexes = Array<s32>(1, imageEdges.imageWidth+1, memory, Flags::Buffer(false,false,false));
        allLimits.yIncreasing_xStartIndexes = Array<s32>(1, imageEdges.imageWidth+1, memory, Flags::Buffer(false,false,false));

        AnkiConditionalErrorAndReturnValue(
          AreValid(allLimits.xDecreasing_yStartIndexes, allLimits.xIncreasing_yStartIndexes, allLimits.yDecreasing_xStartIndexes, allLimits.yIncreasing_xStartIndexes),
          RESULT_FAIL_OUT_OF_MEMORY, "BinaryTracker::ComputeAllIndexLimits", "Could not allocate local memory");

        ComputeIndexLimitsVertical(imageEdges.xDecreasing, allLimits.xDecreasing_yStartIndexes);

        ComputeIndexLimitsVertical(imageEdges.xIncreasing, allLimits.xIncreasing_yStartIndexes);

        ComputeIndexLimitsHorizontal(imageEdges.yDecreasing, allLimits.yDecreasing_xStartIndexes);

        ComputeIndexLimitsHorizontal(imageEdges.yIncreasing, allLimits.yIncreasing_xStartIndexes);

        return RESULT_OK;
      } // BinaryTracker::ComputeAllIndexLimits()

      Result BinaryTracker::UpdateTrack_Normal(
        const Array<u8> &nextImage,
        const EdgeDetectionParameters &edgeDetectionParams,
        const s32 matching_maxTranslationDistance, const s32 matching_maxProjectiveDistance,
        const s32 verify_maxTranslationDistance, const u8 verify_maxPixelDifference, const s32 verify_coordinateIncrement,
        s32 &numMatches,
        s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
        s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
        s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
        MemoryStack fastScratch,
        MemoryStack slowScratch)
      {
        return UpdateTrack_Generic(
          UPDATE_VERSION_NORMAL,
          nextImage,
          edgeDetectionParams,
          matching_maxTranslationDistance, matching_maxProjectiveDistance,
          verify_maxTranslationDistance, verify_maxPixelDifference, verify_coordinateIncrement,
          0, 0, 0,
          numMatches,
          verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
          fastScratch,
          slowScratch);
      }

      // WARNING: using a list is liable to be slower than normal, and not be more accurate
      Result BinaryTracker::UpdateTrack_List(
        const Array<u8> &nextImage,
        const EdgeDetectionParameters &edgeDetectionParams,
        const s32 matching_maxTranslationDistance, const s32 matching_maxProjectiveDistance,
        const s32 verify_maxTranslationDistance, const u8 verify_maxPixelDifference, const s32 verify_coordinateIncrement,
        s32 &numMatches,
        s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
        s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
        s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
        MemoryStack fastScratch,
        MemoryStack slowScratch)
      {
        return UpdateTrack_Generic(
          UPDATE_VERSION_LIST,
          nextImage,
          edgeDetectionParams,
          matching_maxTranslationDistance, matching_maxProjectiveDistance,
          verify_maxTranslationDistance, verify_maxPixelDifference, verify_coordinateIncrement,
          0, 0, 0,
          numMatches,
          verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
          fastScratch,
          slowScratch);
      }

      Result BinaryTracker::UpdateTrack_Ransac(
        const Array<u8> &nextImage,
        const EdgeDetectionParameters &edgeDetectionParams,
        const s32 matching_maxProjectiveDistance,
        const s32 verify_maxTranslationDistance, const u8 verify_maxPixelDifference, const s32 verify_coordinateIncrement,
        const s32 ransac_maxIterations,
        const s32 ransac_numSamplesPerType, //< for four types
        const s32 ransac_inlinerDistance,
        s32 &numMatches,
        s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
        s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
        s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
        MemoryStack fastScratch,
        MemoryStack slowScratch)
      {
        return UpdateTrack_Generic(
          UPDATE_VERSION_RANSAC,
          nextImage,
          edgeDetectionParams,
          0, matching_maxProjectiveDistance,
          verify_maxTranslationDistance, verify_maxPixelDifference, verify_coordinateIncrement,
          ransac_maxIterations, ransac_numSamplesPerType, ransac_inlinerDistance,
          numMatches,
          verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
          fastScratch,
          slowScratch);
      }

      Result BinaryTracker::UpdateTrack_Generic(
        const UpdateVersion version,
        const Array<u8> &nextImage,
        const EdgeDetectionParameters &edgeDetectionParams,
        const s32 matching_maxTranslationDistance, const s32 matching_maxProjectiveDistance,
        const s32 verify_maxTranslationDistance, const u8 verify_maxPixelDifference, const s32 verify_coordinateIncrement,
        const s32 ransac_maxIterations,
        const s32 ransac_numSamplesPerType, //< for four types
        const s32 ransac_inlinerDistance,
        s32 &numMatches,
        s32 &verify_meanAbsoluteDifference, //< For all pixels in the template, compute the mean difference between the template and the final warped template
        s32 &verify_numInBounds, //< How many template pixels are in the image, after the template is warped?
        s32 &verify_numSimilarPixels, //< For all pixels in the template, how many are within verifyMaxPixelDifference grayvalues? Use in conjunction with get_numTemplatePixels() or numInBounds for a percentage.
        MemoryStack fastScratch,
        MemoryStack slowScratch)
      {
        Result lastResult = RESULT_FAIL;

        EdgeLists nextImageEdges;

        nextImageEdges.xDecreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastScratch);
        nextImageEdges.xIncreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastScratch);
        nextImageEdges.yDecreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastScratch);
        nextImageEdges.yIncreasing = FixedLengthList<Point<s16> >(edgeDetectionParams.maxDetectionsPerType, fastScratch);

        AnkiConditionalErrorAndReturnValue(AreValid(nextImageEdges.xDecreasing, nextImageEdges.xIncreasing, nextImageEdges.yDecreasing, nextImageEdges.yIncreasing),
          RESULT_FAIL_OUT_OF_MEMORY, "BinaryTracker::UpdateTrack", "Could not allocate local scratch");

        BeginBenchmark("ut_DetectEdges");

        if(edgeDetectionParams.type == TemplateTracker::BinaryTracker::EDGE_TYPE_GRAYVALUE) {
          lastResult = DetectBlurredEdges_GrayvalueThreshold(nextImage, this->lastGrayvalueThreshold, edgeDetectionParams.minComponentWidth, edgeDetectionParams.everyNLines, nextImageEdges);
        } else if(edgeDetectionParams.type == TemplateTracker::BinaryTracker::EDGE_TYPE_DERIVATIVE) {
          lastResult = DetectBlurredEdges_DerivativeThreshold(nextImage, edgeDetectionParams.combHalfWidth, edgeDetectionParams.combResponseThreshold, edgeDetectionParams.everyNLines, nextImageEdges);
        }

        this->lastUsedGrayvalueThreshold = this->lastGrayvalueThreshold;

        EndBenchmark("ut_DetectEdges");

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::UpdateTrack", "DetectBlurredEdge failed");

        // First, to speed up the correspondence search, find the min and max of x or y points
        AllIndexLimits allLimits;

        BeginBenchmark("ut_IndexLimits");

        if((lastResult = BinaryTracker::ComputeAllIndexLimits(nextImageEdges, allLimits, fastScratch)) != RESULT_OK)
          return lastResult;

        EndBenchmark("ut_IndexLimits");

        // Second, compute the actual correspondence and refine the homography

        const s32 maxMatchesPerType = 4000;

        // The ransac version is different because:
        // 1. It doesn't do a translation step
        // 2. It does a verify step automatically, but the others need to do one after running
        if(version == UPDATE_VERSION_RANSAC) {
          BeginBenchmark("ut_projective_ransac");
          lastResult = IterativelyRefineTrack_Projective_Ransac(nextImageEdges, allLimits, matching_maxProjectiveDistance, maxMatchesPerType, ransac_maxIterations, ransac_numSamplesPerType, ransac_inlinerDistance, numMatches, fastScratch, slowScratch);
          EndBenchmark("ut_projective_ransac");

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::UpdateTrack", "IterativelyRefineTrack_Projective failed");
        } else { // if(version == UPDATE_VERSION_RANSAC) {
          BeginBenchmark("ut_translation");

          lastResult = IterativelyRefineTrack_Translation(nextImageEdges, allLimits, matching_maxTranslationDistance, fastScratch);

          EndBenchmark("ut_translation");

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::UpdateTrack", "IterativelyRefineTrack_Translation failed");

          if(version == UPDATE_VERSION_NORMAL) {
            BeginBenchmark("ut_projective_normal");
            lastResult = IterativelyRefineTrack_Projective(nextImageEdges, allLimits, matching_maxProjectiveDistance, fastScratch);
            EndBenchmark("ut_projective_normal");
          } else if(version == UPDATE_VERSION_LIST) {
            BeginBenchmark("ut_projective_list");
            lastResult = IterativelyRefineTrack_Projective_List(nextImageEdges, allLimits, matching_maxProjectiveDistance, maxMatchesPerType, fastScratch, slowScratch);
            EndBenchmark("ut_projective_list");
          } else {
            return RESULT_FAIL;
          }

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::UpdateTrack", "IterativelyRefineTrack_Projective failed");

          BeginBenchmark("ut_verify");

          lastResult = VerifyTrack(nextImageEdges, allLimits, verify_maxTranslationDistance, numMatches);

          EndBenchmark("ut_verify");

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::UpdateTrack", "VerifyTrack failed");
        } // if(version == UPDATE_VERSION_RANSAC) ... else

        BeginBenchmark("ut_grayvalueThreshold");

        const Quadrilateral<f32> curWarpedCorners = this->get_transformation().get_transformedCorners(fastScratch);

        const Rectangle<s32> edgeDetection_imageRegionOfInterest = curWarpedCorners.ComputeBoundingRectangle<s32>().ComputeScaledRectangle<s32>(edgeDetectionParams.threshold_scaleRegionPercent);

        IntegerCounts lastImageIntegerCounts(
          nextImage,
          edgeDetection_imageRegionOfInterest,
          edgeDetectionParams.threshold_yIncrement,
          edgeDetectionParams.threshold_xIncrement,
          fastScratch);

        this->lastGrayvalueThreshold = ComputeGrayvalueThreshold(lastImageIntegerCounts, edgeDetectionParams.threshold_blackPercentile, edgeDetectionParams.threshold_whitePercentile);

        EndBenchmark("ut_grayvalueThreshold");

        BeginBenchmark("ut_verifyTransformation");

        {
          const f32 templateRegionHeight = static_cast<f32>(templateImageHeight);
          const f32 templateRegionWidth = static_cast<f32>(templateImageWidth);

          //lastResult = this->transformation.VerifyTransformation_Projective_LinearInterpolate(
          lastResult = this->transformation.VerifyTransformation_Projective_NearestNeighbor(
            templateImage, this->templateIntegerCounts, this->templateQuad.ComputeBoundingRectangle<f32>(),
            nextImage, lastImageIntegerCounts,
            templateRegionHeight, templateRegionWidth, verify_coordinateIncrement,
            verify_maxPixelDifference, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
            fastScratch);

          //Array<u8> downsampledTemplate(templateImageHeight/8, templateImageWidth/8, fastScratch);
          //Array<u8> downsampledNext(templateImageHeight/8, templateImageWidth/8, fastScratch);

          //Rectangle<f32> boundingRect = this->templateQuad.ComputeBoundingRectangle();
          //Rectangle<f32> downsampledRect(boundingRect.left/8, boundingRect.right/8, boundingRect.top/8, boundingRect.bottom/8);

          //ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(templateImage, 3, downsampledTemplate, fastScratch);
          //ImageProcessing::DownsampleByPowerOfTwo<u8,u32,u8>(nextImage, 3, downsampledNext, fastScratch);

          //const f32 templateRegionHeight = static_cast<f32>(templateImageHeight/8);
          //const f32 templateRegionWidth = static_cast<f32>(templateImageWidth/8);

          //lastResult = this->transformation.VerifyTransformation_Projective_LinearInterpolate(
          //  downsampledTemplate, downsampledRect,
          //  downsampledNext,
          //  templateRegionHeight, templateRegionWidth,
          //  verify_maxPixelDifference, verify_meanAbsoluteDifference, verify_numInBounds, verify_numSimilarPixels,
          //  fastScratch);
        }
        EndBenchmark("ut_verifyTransformation");

        return lastResult;
      } // Result BinaryTracker::UpdateTrack()

      Result BinaryTracker::ComputeIndexLimitsVertical(const FixedLengthList<Point<s16> > &points, Array<s32> &yStartIndexes)
      {
        const Point<s16> * restrict pPoints = points.Pointer(0);
        s32 * restrict pIndexes = yStartIndexes.Pointer(0,0);

        const s32 maxY = yStartIndexes.get_size(1) - 1;
        const s32 numPoints = points.get_size();

        s32 iPoint = 0;
        s32 lastY = -1;

        while(iPoint < numPoints) {
          while((iPoint < numPoints) && (pPoints[iPoint].y == lastY)) {
            iPoint++;
          }

          for(s32 y=lastY+1; y<=pPoints[iPoint].y; y++) {
            pIndexes[y] = iPoint;
          }

          lastY = pPoints[iPoint].y;
        }

        lastY = pPoints[numPoints-1].y;

        for(s32 y=lastY+1; y<=maxY; y++) {
          pIndexes[y] = numPoints;
        }

        return RESULT_OK;
      } // BinaryTracker::ComputeIndexLimitsVertical()

      Result BinaryTracker::ComputeIndexLimitsHorizontal(const FixedLengthList<Point<s16> > &points, Array<s32> &xStartIndexes)
      {
        const Point<s16> * restrict pPoints = points.Pointer(0);
        s32 * restrict pIndexes = xStartIndexes.Pointer(0,0);

        const s32 maxX = xStartIndexes.get_size(1) - 1;
        const s32 numPoints = points.get_size();

        s32 iPoint = 0;
        s32 lastX = -1;

        while(iPoint < numPoints) {
          while((iPoint < numPoints) && (pPoints[iPoint].x == lastX)) {
            iPoint++;
          }

          for(s32 x=lastX+1; x<=pPoints[iPoint].x; x++) {
            pIndexes[x] = iPoint;
          }

          lastX = pPoints[iPoint].x;
        }

        lastX = pPoints[numPoints-1].x;

        for(s32 x=lastX+1; x<=maxX; x++) {
          pIndexes[x] = numPoints;
        }

        return RESULT_OK;
      } // BinaryTracker::ComputeIndexLimitsHorizontal()

      Result BinaryTracker::FindVerticalCorrespondences_Translation(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &xStartIndexes,
        s32 &sumY,
        s32 &numCorrespondences)
      {
        const s32 numTemplatePoints = templatePoints.get_size();
        //const s32 numNewPoints = newPoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset(1.0f);

        // TODO: if the homography is just translation, we can do this faster (just slightly, as most of the cost is the search)
        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        sumY = 0;
        f32 numCorrespondencesF32 = 0.0f;

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pXStartIndexes = xStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          // TODO: verify the -0.5f is correct
          const s32 warpedXrounded = RoundS32_minusPointFive(warpedX + centerOffset.x);
          const s32 warpedYrounded = RoundS32_minusPointFive(warpedY + centerOffset.y);

          if(warpedXrounded >= 0 && warpedXrounded < imageWidth) {
            const s32 minY = warpedYrounded - maxMatchingDistance;
            const s32 maxY = warpedYrounded + maxMatchingDistance;

            s32 curIndex = pXStartIndexes[warpedXrounded];
            const s32 endIndex = pXStartIndexes[warpedXrounded+1];

            // Find the start of the valid matches
            while( (curIndex<endIndex) && (pNewPoints[curIndex].y<minY) ) {
              curIndex++;
            }

            // For every valid match, increment the sum and counter
            while( (curIndex<endIndex) && (pNewPoints[curIndex].y<=maxY) ) {
              const s32 offset = pNewPoints[curIndex].y - warpedYrounded;

              sumY += offset;
              numCorrespondencesF32 += 1.0f;

              curIndex++;
            }
          } // if(warpedXrounded >= 0 && warpedXrounded < imageWidth)
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

        numCorrespondences = Round<s32>(numCorrespondencesF32);

        return RESULT_OK;
      } // Result BinaryTracker::FindVerticalCorrespondences_Translation()

      Result BinaryTracker::FindHorizontalCorrespondences_Translation(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &yStartIndexes,
        s32 &sumX,
        s32 &numCorrespondences)
      {
        const s32 numTemplatePoints = templatePoints.get_size();
        //const s32 numNewPoints = newPoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset(1.0f);

        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        sumX = 0;
        f32 numCorrespondencesF32 = 0.0f;

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pYStartIndexes = yStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          // TODO: verify the -0.5f is correct
          const s32 warpedXrounded = RoundS32_minusPointFive(warpedX + centerOffset.x);
          const s32 warpedYrounded = RoundS32_minusPointFive(warpedY + centerOffset.y);

          if(warpedYrounded >= 0 && warpedYrounded < imageHeight) {
            const s32 minX = warpedXrounded - maxMatchingDistance;
            const s32 maxX = warpedXrounded + maxMatchingDistance;

            s32 curIndex = pYStartIndexes[warpedYrounded];
            const s32 endIndex = pYStartIndexes[warpedYrounded+1];

            // Find the start of the valid matches
            while( (curIndex<endIndex) && (pNewPoints[curIndex].x<minX) ) {
              curIndex++;
            }

            // For every valid match, increment the sum and counter
            while( (curIndex<endIndex) && (pNewPoints[curIndex].x<=maxX) ) {
              const s32 offset = pNewPoints[curIndex].x - warpedXrounded;

              sumX += offset;
              numCorrespondencesF32 += 1.0f;

              curIndex++;
            }
          } // if(warpedYrounded >= 0 && warpedYrounded < imageHeight)
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

        numCorrespondences = Round<s32>(numCorrespondencesF32);

        return RESULT_OK;
      } // Result BinaryTracker::FindHorizontalCorrespondences_Translation()

      NO_INLINE Result BinaryTracker::FindVerticalCorrespondences_Projective(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &xStartIndexes,
        Array<f32> &AtA,
        Array<f32> &Atb_t)
      {
        const s32 numTemplatePoints = templatePoints.get_size();
        //const s32 numNewPoints = newPoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset(1.0f);

        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        // These addresses should be known at compile time, so should be faster
#if !defined(USE_ARM_ACCELERATION) // natural C
        f32 AtA_raw[8][8];
        f32 Atb_t_raw[8];

        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=0; ja<8; ja++) {
            AtA_raw[ia][ja] = 0;
          }
          Atb_t_raw[ia] = 0;
        }
#else // ARM optimized
        f32 AtA_raw33 = 0, AtA_raw34 = 0, AtA_raw35 = 0, AtA_raw36 = 0, AtA_raw37 = 0;
        f32 AtA_raw44 = 0, AtA_raw45 = 0, AtA_raw46 = 0, AtA_raw47 = 0;
        f32 AtA_raw55 = 0, AtA_raw56 = 0, AtA_raw57 = 0;
        f32 AtA_raw66 = 0, AtA_raw67 = 0;
        f32 AtA_raw77 = 0;

        f32 Atb_t_raw3 = 0, Atb_t_raw4 = 0, Atb_t_raw5 = 0, Atb_t_raw6 = 0, Atb_t_raw7 = 0;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pXStartIndexes = xStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          // TODO: verify the -0.5f is correct
          const s32 warpedXrounded = RoundS32_minusPointFive(warpedX + centerOffset.x);
          const s32 warpedYrounded = RoundS32_minusPointFive(warpedY + centerOffset.y);

          if(warpedXrounded >= 0 && warpedXrounded < imageWidth) {
            const s32 minY = warpedYrounded - maxMatchingDistance;
            const s32 maxY = warpedYrounded + maxMatchingDistance;

            s32 curIndex = pXStartIndexes[warpedXrounded];
            const s32 endIndex = pXStartIndexes[warpedXrounded+1];

            // Find the start of the valid matches
            while( (curIndex<endIndex) && (pNewPoints[curIndex].y<minY) ) {
              curIndex++;
            }

            // For every valid match, increment the sum and counter
            while( (curIndex<endIndex) && (pNewPoints[curIndex].y<=maxY) ) {
              const s32 offset = pNewPoints[curIndex].y - warpedYrounded;
              const f32 yp = warpedY + static_cast<f32>(offset);

#if !defined(USE_ARM_ACCELERATION) // natural C
              const f32 aValues[8] = {0, 0, 0, -xc, -yc, -1, xc*yp, yc*yp};

              const f32 bValue = -yp;

              for(s32 ia=0; ia<8; ia++) {
                for(s32 ja=ia; ja<8; ja++) {
                  AtA_raw[ia][ja] += aValues[ia] * aValues[ja];
                }
                Atb_t_raw[ia] += bValue * aValues[ia];
              }
#else // ARM optimized
              const f32 aValues6 = xc*yp;
              const f32 aValues7 = yc*yp;

              AtA_raw33 += xc * xc;
              AtA_raw34 += xc * yc;
              AtA_raw35 += xc;
              AtA_raw36 -= xc * aValues6;
              AtA_raw37 -= xc * aValues7;

              AtA_raw44 += yc * yc;
              AtA_raw45 += yc;
              AtA_raw46 -= yc * aValues6;
              AtA_raw47 -= yc * aValues7;

              AtA_raw55 += 1;
              AtA_raw56 -= aValues6;
              AtA_raw57 -= aValues7;

              AtA_raw66 += aValues6 * aValues6;
              AtA_raw67 += aValues6 * aValues7;

              AtA_raw77 += aValues7 * aValues7;

              Atb_t_raw3 += yp * xc;
              Atb_t_raw4 += yp * yc;
              Atb_t_raw5 += yp;
              Atb_t_raw6 -= yp * aValues6;
              Atb_t_raw7 -= yp * aValues7;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

              curIndex++;
            }
          } // if(warpedXrounded >= 0 && warpedXrounded < imageWidth)
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

#if !defined(USE_ARM_ACCELERATION) // natural C
        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=ia; ja<8; ja++) {
            AtA[ia][ja] = AtA_raw[ia][ja];
          }
          Atb_t[0][ia] = Atb_t_raw[ia];
        }
#else // ARM optimized
        AtA[3][3] = AtA_raw33; AtA[3][4] = AtA_raw34; AtA[3][5] = AtA_raw35; AtA[3][6] = AtA_raw36; AtA[3][7] = AtA_raw37;
        AtA[4][4] = AtA_raw44; AtA[4][5] = AtA_raw45; AtA[4][6] = AtA_raw46; AtA[4][7] = AtA_raw47;
        AtA[5][5] = AtA_raw55; AtA[5][6] = AtA_raw56; AtA[5][7] = AtA_raw57;
        AtA[6][6] = AtA_raw66; AtA[6][7] = AtA_raw67;
        AtA[7][7] = AtA_raw77;

        Atb_t[0][3] = Atb_t_raw3; Atb_t[0][4] = Atb_t_raw4; Atb_t[0][5] = Atb_t_raw5; Atb_t[0][6] = Atb_t_raw6; Atb_t[0][7] = Atb_t_raw7;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        return RESULT_OK;
      } // NO_INLINE Result BinaryTracker::FindVerticalCorrespondences_Projective()

      NO_INLINE Result BinaryTracker::FindHorizontalCorrespondences_Projective(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &yStartIndexes,
        Array<f32> &AtA,
        Array<f32> &Atb_t)
      {
        const s32 numTemplatePoints = templatePoints.get_size();
        //const s32 numNewPoints = newPoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset(1.0f);

        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        // These addresses should be known at compile time, so should be faster
#if !defined(USE_ARM_ACCELERATION) // natural C
        f32 AtA_raw[8][8];
        f32 Atb_t_raw[8];

        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=0; ja<8; ja++) {
            AtA_raw[ia][ja] = 0;
          }
          Atb_t_raw[ia] = 0;
        }
#else // ARM optimized
        f32 AtA_raw00 = 0, AtA_raw01 = 0, AtA_raw02 = 0, AtA_raw06 = 0, AtA_raw07 = 0;
        f32 AtA_raw11 = 0, AtA_raw12 = 0, AtA_raw16 = 0, AtA_raw17 = 0;
        f32 AtA_raw22 = 0, AtA_raw26 = 0, AtA_raw27 = 0;
        f32 AtA_raw66 = 0, AtA_raw67 = 0;
        f32 AtA_raw77 = 0;

        f32 Atb_t_raw0 = 0, Atb_t_raw1 = 0, Atb_t_raw2 = 0, Atb_t_raw6 = 0, Atb_t_raw7 = 0;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pYStartIndexes = yStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          // TODO: verify the -0.5f is correct
          const s32 warpedXrounded = RoundS32_minusPointFive(warpedX + centerOffset.x);
          const s32 warpedYrounded = RoundS32_minusPointFive(warpedY + centerOffset.y);

          if(warpedYrounded >= 0 && warpedYrounded < imageHeight) {
            const s32 minX = warpedXrounded - maxMatchingDistance;
            const s32 maxX = warpedXrounded + maxMatchingDistance;

            s32 curIndex = pYStartIndexes[warpedYrounded];
            const s32 endIndex = pYStartIndexes[warpedYrounded+1];

            // Find the start of the valid matches
            while( (curIndex<endIndex) && (pNewPoints[curIndex].x<minX) ) {
              curIndex++;
            }

            // For every valid match, increment the sum and counter
            while( (curIndex<endIndex) && (pNewPoints[curIndex].x<=maxX) ) {
              const s32 offset = pNewPoints[curIndex].x - warpedXrounded;
              const f32 xp = warpedX + static_cast<f32>(offset);

#if !defined(USE_ARM_ACCELERATION) // natural C
              const f32 aValues[8] = {xc, yc, 1, 0, 0, 0, -xc*xp, -yc*xp};

              const f32 bValue = xp;

              for(s32 ia=0; ia<8; ia++) {
                for(s32 ja=ia; ja<8; ja++) {
                  AtA_raw[ia][ja] += aValues[ia] * aValues[ja];
                }

                Atb_t_raw[ia] += aValues[ia] * bValue;
              }
#else // ARM optimized
              const f32 aValues6 = -xc*xp;
              const f32 aValues7 = -yc*xp;

              AtA_raw00 += xc * xc;
              AtA_raw01 += xc * yc;
              AtA_raw02 += xc;
              AtA_raw06 += xc * aValues6;
              AtA_raw07 += xc * aValues7;

              AtA_raw11 += yc * yc;
              AtA_raw12 += yc;
              AtA_raw16 += yc * aValues6;
              AtA_raw17 += yc * aValues7;

              AtA_raw22 += 1;
              AtA_raw26 += aValues6;
              AtA_raw27 += aValues7;

              AtA_raw66 += aValues6 * aValues6;
              AtA_raw67 += aValues6 * aValues7;

              AtA_raw77 += aValues7 * aValues7;

              Atb_t_raw0 += xc * xp;
              Atb_t_raw1 += yc * xp;
              Atb_t_raw2 += xp;
              Atb_t_raw6 += aValues6 * xp;
              Atb_t_raw7 += aValues7 * xp;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

              curIndex++;
            }
          } // if(warpedYrounded >= 0 && warpedYrounded < imageHeight)
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

#if !defined(USE_ARM_ACCELERATION) // natural C
        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=ia; ja<8; ja++) {
            AtA[ia][ja] = AtA_raw[ia][ja];
          }
          Atb_t[0][ia] = Atb_t_raw[ia];
        }
#else // ARM optimized
        AtA[0][0] = AtA_raw00; AtA[0][1] = AtA_raw01; AtA[0][2] = AtA_raw02; AtA[0][6] = AtA_raw06; AtA[0][7] = AtA_raw07;
        AtA[1][1] = AtA_raw11; AtA[1][2] = AtA_raw12; AtA[1][6] = AtA_raw16; AtA[1][7] = AtA_raw17;
        AtA[2][2] = AtA_raw22; AtA[2][6] = AtA_raw26; AtA[2][7] = AtA_raw27;
        AtA[6][6] = AtA_raw66; AtA[6][7] = AtA_raw67;
        AtA[7][7] = AtA_raw77;

        Atb_t[0][0] = Atb_t_raw0; Atb_t[0][1] = Atb_t_raw1; Atb_t[0][2] = Atb_t_raw2; Atb_t[0][6] = Atb_t_raw6; Atb_t[0][7] = Atb_t_raw7;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        return RESULT_OK;
      } // NO_INLINE Result BinaryTracker::FindHorizontalCorrespondences_Projective()

      NO_INLINE Result BinaryTracker::FindVerticalCorrespondences_Verify(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &xStartIndexes, //< Computed by ComputeIndexLimitsHorizontal
        s32 &numTemplatePixelsMatched)
      {
        const s32 numTemplatePoints = templatePoints.get_size();
        //const s32 numNewPoints = newPoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset(1.0f);

        // TODO: if the homography is just translation, we can do this faster (just slightly, as most of the cost is the search)
        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        f32 numTemplatePixelsMatchedF32 = 0.0f;

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pXStartIndexes = xStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          // TODO: verify the -0.5f is correct
          const s32 warpedXrounded = RoundS32_minusPointFive(warpedX + centerOffset.x);
          const s32 warpedYrounded = RoundS32_minusPointFive(warpedY + centerOffset.y);

          if(warpedXrounded >= 0 && warpedXrounded < imageWidth) {
            const s32 minY = warpedYrounded - maxMatchingDistance;
            const s32 maxY = warpedYrounded + maxMatchingDistance;

            s32 curIndex = pXStartIndexes[warpedXrounded];
            const s32 endIndex = pXStartIndexes[warpedXrounded+1];

            // Find the start of the valid matches
            while( (curIndex<endIndex) && (pNewPoints[curIndex].y<minY) ) {
              curIndex++;
            }

            // If there is a valid match, increment numTemplatePixelsMatchedF32
            if( (curIndex<endIndex) && (pNewPoints[curIndex].y<=maxY) ) {
              numTemplatePixelsMatchedF32 += 1.0f;
            }
          } // if(warpedXrounded >= 0 && warpedXrounded < imageWidth)
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

        numTemplatePixelsMatched = Round<s32>(numTemplatePixelsMatchedF32);

        return RESULT_OK;
      }

      NO_INLINE Result BinaryTracker::FindHorizontalCorrespondences_Verify(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
        s32 &numTemplatePixelsMatched)
      {
        const s32 numTemplatePoints = templatePoints.get_size();
        //const s32 numNewPoints = newPoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset(1.0f);

        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        f32 numTemplatePixelsMatchedF32 = 0.0f;

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pYStartIndexes = yStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          // TODO: verify the -0.5f is correct
          const s32 warpedXrounded = RoundS32_minusPointFive(warpedX + centerOffset.x);
          const s32 warpedYrounded = RoundS32_minusPointFive(warpedY + centerOffset.y);

          if(warpedYrounded >= 0 && warpedYrounded < imageHeight) {
            const s32 minX = warpedXrounded - maxMatchingDistance;
            const s32 maxX = warpedXrounded + maxMatchingDistance;

            s32 curIndex = pYStartIndexes[warpedYrounded];
            const s32 endIndex = pYStartIndexes[warpedYrounded+1];

            // Find the start of the valid matches
            while( (curIndex<endIndex) && (pNewPoints[curIndex].x<minX) ) {
              curIndex++;
            }

            // If there is a valid match, increment numTemplatePixelsMatchedF32
            if( (curIndex<endIndex) && (pNewPoints[curIndex].x<=maxX) ) {
              numTemplatePixelsMatchedF32 += 1.0f;
            }
          } // if(warpedYrounded >= 0 && warpedYrounded < imageHeight)
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

        numTemplatePixelsMatched = Round<s32>(numTemplatePixelsMatchedF32);

        return RESULT_OK;
      }

      NO_INLINE Result BinaryTracker::FindVerticalCorrespondences_List(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &xStartIndexes, //< Computed by ComputeIndexLimitsHorizontal
        FixedLengthList<IndexCorrespondence> &matchingIndexes)
      {
        const s32 numTemplatePoints = templatePoints.get_size();
        //const s32 numNewPoints = newPoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset(1.0f);

        // TODO: if the homography is just translation, we can do this faster (just slightly, as most of the cost is the search)
        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        IndexCorrespondence * restrict pMatchingIndexes = matchingIndexes.Pointer(0);
        s32 numMatchingIndexes = 0;
        const s32 maxMatchingIndexes = matchingIndexes.get_maximumSize();

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pXStartIndexes = xStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          // TODO: verify the -0.5f is correct
          const s32 warpedXrounded = RoundS32_minusPointFive(warpedX + centerOffset.x);
          const s32 warpedYrounded = RoundS32_minusPointFive(warpedY + centerOffset.y);

          if(warpedXrounded >= 0 && warpedXrounded < imageWidth) {
            const s32 minY = warpedYrounded - maxMatchingDistance;
            const s32 maxY = warpedYrounded + maxMatchingDistance;

            s32 curIndex = pXStartIndexes[warpedXrounded];
            const s32 endIndex = pXStartIndexes[warpedXrounded+1];

            // Find the start of the valid matches
            while( (curIndex<endIndex) && (pNewPoints[curIndex].y<minY) ) {
              curIndex++;
            }

            // For every valid match, increment the sum and counter
            while( (curIndex<endIndex) && (pNewPoints[curIndex].y<=maxY) ) {
              if(numMatchingIndexes < (maxMatchingIndexes-1)) {
                const s32 offset = pNewPoints[curIndex].y - warpedYrounded;
                const f32 yp = warpedY + static_cast<f32>(offset);

                //pMatchingIndexes[numMatchingIndexes].templateIndex = iPoint;
                //pMatchingIndexes[numMatchingIndexes].matchedIndex = curIndex;

                pMatchingIndexes[numMatchingIndexes].templatePoint.x = xc;
                pMatchingIndexes[numMatchingIndexes].templatePoint.y = yc;
                pMatchingIndexes[numMatchingIndexes].matchedPoint.x = warpedX;
                pMatchingIndexes[numMatchingIndexes].matchedPoint.y = yp;
                numMatchingIndexes++;
              }

              curIndex++;
            }
          } // if(warpedXrounded >= 0 && warpedXrounded < imageWidth)
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

        matchingIndexes.set_size(numMatchingIndexes);

        return RESULT_OK;
      }

      // Note: Clears the list matchingIndexes before starting
      NO_INLINE Result BinaryTracker::FindHorizontalCorrespondences_List(
        const s32 maxMatchingDistance,
        const Transformations::PlanarTransformation_f32 &transformation,
        const FixedLengthList<Point<s16> > &templatePoints,
        const FixedLengthList<Point<s16> > &newPoints,
        const s32 imageHeight,
        const s32 imageWidth,
        const Array<s32> &yStartIndexes, //< Computed by ComputeIndexLimitsVertical
        FixedLengthList<IndexCorrespondence> &matchingIndexes)
      {
        const s32 numTemplatePoints = templatePoints.get_size();
        //const s32 numNewPoints = newPoints.get_size();

        const Array<f32> &homography = transformation.get_homography();
        const Point<f32> &centerOffset = transformation.get_centerOffset(1.0f);

        const f32 h00 = homography[0][0]; const f32 h01 = homography[0][1]; const f32 h02 = homography[0][2];
        const f32 h10 = homography[1][0]; const f32 h11 = homography[1][1]; const f32 h12 = homography[1][2];
        const f32 h20 = homography[2][0]; const f32 h21 = homography[2][1]; const f32 h22 = 1.0f;

        AnkiAssert(FLT_NEAR(homography[2][2], 1.0f));

        IndexCorrespondence * restrict pMatchingIndexes = matchingIndexes.Pointer(0);
        s32 numMatchingIndexes = 0;
        const s32 maxMatchingIndexes = matchingIndexes.get_maximumSize();

        const Point<s16> * restrict pTemplatePoints = templatePoints.Pointer(0);
        const Point<s16> * restrict pNewPoints = newPoints.Pointer(0);
        const s32 * restrict pYStartIndexes = yStartIndexes.Pointer(0,0);

        for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++) {
          const f32 xr = static_cast<f32>(pTemplatePoints[iPoint].x);
          const f32 yr = static_cast<f32>(pTemplatePoints[iPoint].y);

          //
          // Warp x and y based on the current homography
          //

          // Subtract the center offset
          const f32 xc = xr - centerOffset.x;
          const f32 yc = yr - centerOffset.y;

          // Projective warp
          const f32 wpi = 1.0f / (h20*xc + h21*yc + h22);
          const f32 warpedX = (h00*xc + h01*yc + h02) * wpi;
          const f32 warpedY = (h10*xc + h11*yc + h12) * wpi;

          // TODO: verify the -0.5f is correct
          const s32 warpedXrounded = RoundS32_minusPointFive(warpedX + centerOffset.x);
          const s32 warpedYrounded = RoundS32_minusPointFive(warpedY + centerOffset.y);

          if(warpedYrounded >= 0 && warpedYrounded < imageHeight) {
            const s32 minX = warpedXrounded - maxMatchingDistance;
            const s32 maxX = warpedXrounded + maxMatchingDistance;

            s32 curIndex = pYStartIndexes[warpedYrounded];
            const s32 endIndex = pYStartIndexes[warpedYrounded+1];

            // Find the start of the valid matches
            while( (curIndex<endIndex) && (pNewPoints[curIndex].x<minX) ) {
              curIndex++;
            }

            // For every valid match, increment the sum and counter
            while( (curIndex<endIndex) && (pNewPoints[curIndex].x<=maxX) ) {
              if(numMatchingIndexes < (maxMatchingIndexes-1)) {
                const s32 offset = pNewPoints[curIndex].x - warpedXrounded;
                const f32 xp = warpedX + static_cast<f32>(offset);

                //pMatchingIndexes[numMatchingIndexes].templateIndex = iPoint;
                //pMatchingIndexes[numMatchingIndexes].matchedIndex = curIndex;

                pMatchingIndexes[numMatchingIndexes].templatePoint.x = xc;
                pMatchingIndexes[numMatchingIndexes].templatePoint.y = yc;
                pMatchingIndexes[numMatchingIndexes].matchedPoint.x = xp;
                pMatchingIndexes[numMatchingIndexes].matchedPoint.y = warpedY;

                numMatchingIndexes++;
              }

              curIndex++;
            }
          } // if(warpedYrounded >= 0 && warpedYrounded < imageHeight)
        } // for(s32 iPoint=0; iPoint<numTemplatePoints; iPoint++)

        matchingIndexes.set_size(numMatchingIndexes);

        return RESULT_OK;
      }

      NO_INLINE Result BinaryTracker::ApplyVerticalCorrespondenceList_Projective(
        const FixedLengthList<IndexCorrespondence> &matchingIndexes,
        Array<f32> &AtA,
        Array<f32> &Atb_t)
      {
        const s32 numMatchingIndexes = matchingIndexes.get_size();
        const IndexCorrespondence * restrict pMatchingIndexes = matchingIndexes.Pointer(0);

        // These addresses should be known at compile time, so should be faster
#if !defined(USE_ARM_ACCELERATION) // natural C
        f32 AtA_raw[8][8];
        f32 Atb_t_raw[8];

        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=0; ja<8; ja++) {
            AtA_raw[ia][ja] = 0;
          }
          Atb_t_raw[ia] = 0;
        }
#else // ARM optimized
        f32 AtA_raw33 = 0, AtA_raw34 = 0, AtA_raw35 = 0, AtA_raw36 = 0, AtA_raw37 = 0;
        f32 AtA_raw44 = 0, AtA_raw45 = 0, AtA_raw46 = 0, AtA_raw47 = 0;
        f32 AtA_raw55 = 0, AtA_raw56 = 0, AtA_raw57 = 0;
        f32 AtA_raw66 = 0, AtA_raw67 = 0;
        f32 AtA_raw77 = 0;

        f32 Atb_t_raw3 = 0, Atb_t_raw4 = 0, Atb_t_raw5 = 0, Atb_t_raw6 = 0, Atb_t_raw7 = 0;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        for(s32 iMatch=0; iMatch<numMatchingIndexes; iMatch++) {
          const f32 xc = pMatchingIndexes[iMatch].templatePoint.x;
          const f32 yc = pMatchingIndexes[iMatch].templatePoint.y;

          //const f32 xp = pMatchingIndexes[iMatch].matchedPoint.x;
          const f32 yp = pMatchingIndexes[iMatch].matchedPoint.y;

#if !defined(USE_ARM_ACCELERATION) // natural C
          const f32 aValues[8] = {0, 0, 0, -xc, -yc, -1, xc*yp, yc*yp};

          const f32 bValue = -yp;

          for(s32 ia=0; ia<8; ia++) {
            for(s32 ja=ia; ja<8; ja++) {
              AtA_raw[ia][ja] += aValues[ia] * aValues[ja];
            }
            Atb_t_raw[ia] += bValue * aValues[ia];
          }
#else // ARM optimized
          const f32 aValues6 = xc*yp;
          const f32 aValues7 = yc*yp;

          AtA_raw33 += xc * xc;
          AtA_raw34 += xc * yc;
          AtA_raw35 += xc;
          AtA_raw36 -= xc * aValues6;
          AtA_raw37 -= xc * aValues7;

          AtA_raw44 += yc * yc;
          AtA_raw45 += yc;
          AtA_raw46 -= yc * aValues6;
          AtA_raw47 -= yc * aValues7;

          AtA_raw55 += 1;
          AtA_raw56 -= aValues6;
          AtA_raw57 -= aValues7;

          AtA_raw66 += aValues6 * aValues6;
          AtA_raw67 += aValues6 * aValues7;

          AtA_raw77 += aValues7 * aValues7;

          Atb_t_raw3 += yp * xc;
          Atb_t_raw4 += yp * yc;
          Atb_t_raw5 += yp;
          Atb_t_raw6 -= yp * aValues6;
          Atb_t_raw7 -= yp * aValues7;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else
        } // for(s32 iMatch=0; iMatch<numMatchingIndexes; iMatch++)

#if !defined(USE_ARM_ACCELERATION) // natural C
        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=ia; ja<8; ja++) {
            AtA[ia][ja] = AtA_raw[ia][ja];
          }
          Atb_t[0][ia] = Atb_t_raw[ia];
        }
#else // ARM optimized
        AtA[3][3] = AtA_raw33; AtA[3][4] = AtA_raw34; AtA[3][5] = AtA_raw35; AtA[3][6] = AtA_raw36; AtA[3][7] = AtA_raw37;
        AtA[4][4] = AtA_raw44; AtA[4][5] = AtA_raw45; AtA[4][6] = AtA_raw46; AtA[4][7] = AtA_raw47;
        AtA[5][5] = AtA_raw55; AtA[5][6] = AtA_raw56; AtA[5][7] = AtA_raw57;
        AtA[6][6] = AtA_raw66; AtA[6][7] = AtA_raw67;
        AtA[7][7] = AtA_raw77;

        Atb_t[0][3] = Atb_t_raw3; Atb_t[0][4] = Atb_t_raw4; Atb_t[0][5] = Atb_t_raw5; Atb_t[0][6] = Atb_t_raw6; Atb_t[0][7] = Atb_t_raw7;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        return RESULT_OK;
      }

      NO_INLINE Result BinaryTracker::ApplyHorizontalCorrespondenceList_Projective(
        const FixedLengthList<IndexCorrespondence> &matchingIndexes,
        Array<f32> &AtA,
        Array<f32> &Atb_t)
      {
        const s32 numMatchingIndexes = matchingIndexes.get_size();
        const IndexCorrespondence * restrict pMatchingIndexes = matchingIndexes.Pointer(0);

        // These addresses should be known at compile time, so should be faster
#if !defined(USE_ARM_ACCELERATION) // natural C
        f32 AtA_raw[8][8];
        f32 Atb_t_raw[8];

        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=0; ja<8; ja++) {
            AtA_raw[ia][ja] = 0;
          }
          Atb_t_raw[ia] = 0;
        }
#else // ARM optimized
        f32 AtA_raw00 = 0, AtA_raw01 = 0, AtA_raw02 = 0, AtA_raw06 = 0, AtA_raw07 = 0;
        f32 AtA_raw11 = 0, AtA_raw12 = 0, AtA_raw16 = 0, AtA_raw17 = 0;
        f32 AtA_raw22 = 0, AtA_raw26 = 0, AtA_raw27 = 0;
        f32 AtA_raw66 = 0, AtA_raw67 = 0;
        f32 AtA_raw77 = 0;

        f32 Atb_t_raw0 = 0, Atb_t_raw1 = 0, Atb_t_raw2 = 0, Atb_t_raw6 = 0, Atb_t_raw7 = 0;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        for(s32 iMatch=0; iMatch<numMatchingIndexes; iMatch++) {
          const f32 xc = pMatchingIndexes[iMatch].templatePoint.x;
          const f32 yc = pMatchingIndexes[iMatch].templatePoint.y;

          const f32 xp = pMatchingIndexes[iMatch].matchedPoint.x;
          //const f32 yp = pMatchingIndexes[iMatch].matchedPoint.y;

#if !defined(USE_ARM_ACCELERATION) // natural C
          const f32 aValues[8] = {xc, yc, 1, 0, 0, 0, -xc*xp, -yc*xp};

          const f32 bValue = xp;

          for(s32 ia=0; ia<8; ia++) {
            for(s32 ja=ia; ja<8; ja++) {
              AtA_raw[ia][ja] += aValues[ia] * aValues[ja];
            }

            Atb_t_raw[ia] += aValues[ia] * bValue;
          }
#else // ARM optimized
          const f32 aValues6 = -xc*xp;
          const f32 aValues7 = -yc*xp;

          AtA_raw00 += xc * xc;
          AtA_raw01 += xc * yc;
          AtA_raw02 += xc;
          AtA_raw06 += xc * aValues6;
          AtA_raw07 += xc * aValues7;

          AtA_raw11 += yc * yc;
          AtA_raw12 += yc;
          AtA_raw16 += yc * aValues6;
          AtA_raw17 += yc * aValues7;

          AtA_raw22 += 1;
          AtA_raw26 += aValues6;
          AtA_raw27 += aValues7;

          AtA_raw66 += aValues6 * aValues6;
          AtA_raw67 += aValues6 * aValues7;

          AtA_raw77 += aValues7 * aValues7;

          Atb_t_raw0 += xc * xp;
          Atb_t_raw1 += yc * xp;
          Atb_t_raw2 += xp;
          Atb_t_raw6 += aValues6 * xp;
          Atb_t_raw7 += aValues7 * xp;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else
        } // for(s32 iMatch=0; iMatch<numMatchingIndexes; iMatch++)

#if !defined(USE_ARM_ACCELERATION) // natural C
        for(s32 ia=0; ia<8; ia++) {
          for(s32 ja=ia; ja<8; ja++) {
            AtA[ia][ja] = AtA_raw[ia][ja];
          }
          Atb_t[0][ia] = Atb_t_raw[ia];
        }
#else // ARM optimized
        AtA[0][0] = AtA_raw00; AtA[0][1] = AtA_raw01; AtA[0][2] = AtA_raw02; AtA[0][6] = AtA_raw06; AtA[0][7] = AtA_raw07;
        AtA[1][1] = AtA_raw11; AtA[1][2] = AtA_raw12; AtA[1][6] = AtA_raw16; AtA[1][7] = AtA_raw17;
        AtA[2][2] = AtA_raw22; AtA[2][6] = AtA_raw26; AtA[2][7] = AtA_raw27;
        AtA[6][6] = AtA_raw66; AtA[6][7] = AtA_raw67;
        AtA[7][7] = AtA_raw77;

        Atb_t[0][0] = Atb_t_raw0; Atb_t[0][1] = Atb_t_raw1; Atb_t[0][2] = Atb_t_raw2; Atb_t[0][6] = Atb_t_raw6; Atb_t[0][7] = Atb_t_raw7;
#endif // #if !defined(USE_ARM_ACCELERATION) ... #else

        return RESULT_OK;
      }

      Result BinaryTracker::IterativelyRefineTrack_Translation(
        const EdgeLists &nextImageEdges,
        const AllIndexLimits &allLimits,
        const s32 matching_maxDistance,
        MemoryStack scratch)
      {
        Result lastResult;

        s32 sumX_xDecreasing;
        s32 sumX_xIncreasing;
        s32 sumY_yDecreasing;
        s32 sumY_yIncreasing;

        s32 numX_xDecreasing;
        s32 numX_xIncreasing;
        s32 numY_yDecreasing;
        s32 numY_yIncreasing;

        lastResult = BinaryTracker::FindHorizontalCorrespondences_Translation(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.xDecreasing,
          nextImageEdges.xDecreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.xDecreasing_yStartIndexes,
          sumX_xDecreasing, numX_xDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Translation", "FindHorizontalCorrespondences 1 failed");

        lastResult = BinaryTracker::FindHorizontalCorrespondences_Translation(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.xIncreasing,
          nextImageEdges.xIncreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.xIncreasing_yStartIndexes,
          sumX_xIncreasing, numX_xIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Translation", "FindHorizontalCorrespondences 2 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_Translation(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.yDecreasing,
          nextImageEdges.yDecreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.yDecreasing_xStartIndexes,
          sumY_yDecreasing, numY_yDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Translation", "FindVerticalCorrespondences 1 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_Translation(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.yIncreasing,
          nextImageEdges.yIncreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.yIncreasing_xStartIndexes,
          sumY_yIncreasing, numY_yIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Translation", "FindVerticalCorrespondences 2 failed");

        // Update the transformation
        {
          Array<f32> update(1,2,scratch);

          const s32 sumX = sumX_xDecreasing + sumX_xIncreasing;
          const s32 numX = numX_xDecreasing + numX_xIncreasing;

          const s32 sumY = sumY_yDecreasing + sumY_yIncreasing;
          const s32 numY = numY_yDecreasing + numY_yIncreasing;

          if(numX < 1 || numY < 1)
            return RESULT_OK;

          update[0][0] = static_cast<f32>(-sumX) / static_cast<f32>(numX);
          update[0][1] = static_cast<f32>(-sumY) / static_cast<f32>(numY);

          //update.Print("update");

          this->transformation.Update(update, 1.0f, scratch, Transformations::TRANSFORM_TRANSLATION);
        }

        return RESULT_OK;
      } // Result BinaryTracker::IterativelyRefineTrack_Translation()

      Result BinaryTracker::IterativelyRefineTrack_Projective(
        const EdgeLists &nextImageEdges,
        const AllIndexLimits &allLimits,
        const s32 matching_maxDistance,
        MemoryStack scratch)
      {
        Result lastResult;

        Array<f32> AtA_xDecreasing(8,8,scratch);
        Array<f32> AtA_xIncreasing(8,8,scratch);
        Array<f32> AtA_yDecreasing(8,8,scratch);
        Array<f32> AtA_yIncreasing(8,8,scratch);

        Array<f32> Atb_t_xDecreasing(1,8,scratch);
        Array<f32> Atb_t_xIncreasing(1,8,scratch);
        Array<f32> Atb_t_yDecreasing(1,8,scratch);
        Array<f32> Atb_t_yIncreasing(1,8,scratch);

        lastResult = BinaryTracker::FindHorizontalCorrespondences_Projective(
          matching_maxDistance, this->transformation,
          this->templateEdges.xDecreasing, nextImageEdges.xDecreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.xDecreasing_yStartIndexes, AtA_xDecreasing, Atb_t_xDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Projective", "FindHorizontalCorrespondences 1 failed");

        lastResult = BinaryTracker::FindHorizontalCorrespondences_Projective(
          matching_maxDistance, this->transformation,
          this->templateEdges.xIncreasing, nextImageEdges.xIncreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.xIncreasing_yStartIndexes, AtA_xIncreasing, Atb_t_xIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Projective", "FindHorizontalCorrespondences 2 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_Projective(
          matching_maxDistance, this->transformation,
          this->templateEdges.yDecreasing, nextImageEdges.yDecreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.yDecreasing_xStartIndexes, AtA_yDecreasing, Atb_t_yDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Projective", "FindVerticalCorrespondences 1 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_Projective(
          matching_maxDistance, this->transformation,
          this->templateEdges.yIncreasing, nextImageEdges.yIncreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.yIncreasing_xStartIndexes, AtA_yIncreasing, Atb_t_yIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Projective", "FindVerticalCorrespondences 2 failed");

        // Update the transformation
        {
          Array<f32> newHomography(3, 3, scratch);

          Array<f32> AtA(8,8,scratch);
          Array<f32> Atb_t(1,8,scratch);

          // The total AtA and Atb matrices are just the elementwise sums of their partial versions
          for(s32 y=0; y<8; y++) {
            for(s32 x=0; x<8; x++) {
              AtA[y][x] = AtA_xDecreasing[y][x] + AtA_xIncreasing[y][x] + AtA_yDecreasing[y][x] + AtA_yIncreasing[y][x];
            }

            Atb_t[0][y] = Atb_t_xDecreasing[0][y] + Atb_t_xIncreasing[0][y] + Atb_t_yDecreasing[0][y] + Atb_t_yIncreasing[0][y];
          }

          Matrix::MakeSymmetric(AtA, false);

          //AtA.Print("AtA");
          //Atb_t.Print("Atb_t");
          bool numericalFailure;
          lastResult = Matrix::SolveLeastSquaresWithCholesky<f32>(AtA, Atb_t, false, numericalFailure);

          //Atb_t.Print("result");

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::IterativelyRefineTrack_Projective", "SolveLeastSquaresWithCholesky failed");

          if(numericalFailure){
            AnkiWarn("BinaryTracker::IterativelyRefineTrack_Projective", "numericalFailure");
            return RESULT_OK;
          }

          const f32 * restrict pAtb_t = Atb_t.Pointer(0,0);

          newHomography[0][0] = pAtb_t[0]; newHomography[0][1] = pAtb_t[1]; newHomography[0][2] = pAtb_t[2];
          newHomography[1][0] = pAtb_t[3]; newHomography[1][1] = pAtb_t[4]; newHomography[1][2] = pAtb_t[5];
          newHomography[2][0] = pAtb_t[6]; newHomography[2][1] = pAtb_t[7]; newHomography[2][2] = 1.0f;

          //newHomography.Print("newHomography");

          this->transformation.set_homography(newHomography);
        }

        return RESULT_OK;
      } // Result BinaryTracker::IterativelyRefineTrack_Projective()

      Result BinaryTracker::IterativelyRefineTrack_Projective_List(
        const EdgeLists &nextImageEdges,
        const AllIndexLimits &allLimits,
        const s32 matching_maxDistance,
        const s32 maxMatchesPerType,
        MemoryStack fastScratch,
        MemoryStack slowScratch)
      {
        Result lastResult;

        Array<f32> AtA_xDecreasing(8,8,fastScratch);
        Array<f32> AtA_xIncreasing(8,8,fastScratch);
        Array<f32> AtA_yDecreasing(8,8,fastScratch);
        Array<f32> AtA_yIncreasing(8,8,fastScratch);

        Array<f32> Atb_t_xDecreasing(1,8,fastScratch);
        Array<f32> Atb_t_xIncreasing(1,8,fastScratch);
        Array<f32> Atb_t_yDecreasing(1,8,fastScratch);
        Array<f32> Atb_t_yIncreasing(1,8,fastScratch);

        FixedLengthList<IndexCorrespondence> matchingIndexes_xDecreasing(maxMatchesPerType, slowScratch);
        FixedLengthList<IndexCorrespondence> matchingIndexes_xIncreasing(maxMatchesPerType, slowScratch);
        FixedLengthList<IndexCorrespondence> matchingIndexes_yDecreasing(maxMatchesPerType, slowScratch);
        FixedLengthList<IndexCorrespondence> matchingIndexes_yIncreasing(maxMatchesPerType, slowScratch);

        lastResult = BinaryTracker::FindHorizontalCorrespondences_List(
          matching_maxDistance, this->transformation,
          this->templateEdges.xDecreasing, nextImageEdges.xDecreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.xDecreasing_yStartIndexes, matchingIndexes_xDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_List_Projective", "FindHorizontalCorrespondences_List 1 failed");

        lastResult = BinaryTracker::ApplyHorizontalCorrespondenceList_Projective(matchingIndexes_xDecreasing, AtA_xDecreasing, Atb_t_xDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_List_Projective", "ApplyHorizontalCorrespondenceList_Projective 1 failed");

        lastResult = BinaryTracker::FindHorizontalCorrespondences_List(
          matching_maxDistance, this->transformation,
          this->templateEdges.xIncreasing, nextImageEdges.xIncreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.xIncreasing_yStartIndexes, matchingIndexes_xIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_List_Projective", "FindHorizontalCorrespondences_List 2 failed");

        lastResult = BinaryTracker::ApplyHorizontalCorrespondenceList_Projective(matchingIndexes_xIncreasing, AtA_xIncreasing, Atb_t_xIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_List_Projective", "ApplyHorizontalCorrespondenceList_Projective 2 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_List(
          matching_maxDistance, this->transformation,
          this->templateEdges.yDecreasing, nextImageEdges.yDecreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.yDecreasing_xStartIndexes, matchingIndexes_yDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_List_Projective", "FindVerticalCorrespondences_List 1 failed");

        lastResult = BinaryTracker::ApplyVerticalCorrespondenceList_Projective(matchingIndexes_yDecreasing, AtA_yDecreasing, Atb_t_yDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_List_Projective", "ApplyVerticalCorrespondenceList_Projective 1 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_List(
          matching_maxDistance, this->transformation,
          this->templateEdges.yIncreasing, nextImageEdges.yIncreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.yIncreasing_xStartIndexes, matchingIndexes_yIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_List_Projective", "FindVerticalCorrespondences_List 2 failed");

        lastResult = BinaryTracker::ApplyVerticalCorrespondenceList_Projective(matchingIndexes_yIncreasing, AtA_yIncreasing, Atb_t_yIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_List_Projective", "ApplyVerticalCorrespondenceList_Projective 2 failed");

        // Update the transformation
        {
          Array<f32> newHomography(3, 3, fastScratch);

          Array<f32> AtA(8,8,fastScratch);
          Array<f32> Atb_t(1,8,fastScratch);

          // The total AtA and Atb matrices are just the elementwise sums of their partial versions
          for(s32 y=0; y<8; y++) {
            for(s32 x=0; x<8; x++) {
              AtA[y][x] = AtA_xDecreasing[y][x] + AtA_xIncreasing[y][x] + AtA_yDecreasing[y][x] + AtA_yIncreasing[y][x];
            }

            Atb_t[0][y] = Atb_t_xDecreasing[0][y] + Atb_t_xIncreasing[0][y] + Atb_t_yDecreasing[0][y] + Atb_t_yIncreasing[0][y];
          }

          Matrix::MakeSymmetric(AtA, false);

          //AtA.Print("AtA");
          //Atb_t.Print("Atb_t");
          bool numericalFailure;
          lastResult = Matrix::SolveLeastSquaresWithCholesky<f32>(AtA, Atb_t, false, numericalFailure);

          //Atb_t.Print("result");

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::IterativelyRefineTrack_List_Projective", "SolveLeastSquaresWithCholesky failed");

          if(numericalFailure){
            AnkiWarn("BinaryTracker::IterativelyRefineTrack_List_Projective", "numericalFailure");
            return RESULT_OK;
          }

          const f32 * restrict pAtb_t = Atb_t.Pointer(0,0);

          newHomography[0][0] = pAtb_t[0]; newHomography[0][1] = pAtb_t[1]; newHomography[0][2] = pAtb_t[2];
          newHomography[1][0] = pAtb_t[3]; newHomography[1][1] = pAtb_t[4]; newHomography[1][2] = pAtb_t[5];
          newHomography[2][0] = pAtb_t[6]; newHomography[2][1] = pAtb_t[7]; newHomography[2][2] = 1.0f;

          //newHomography.Print("newHomography");

          this->transformation.set_homography(newHomography);
        }

        return RESULT_OK;
      }

      Result BinaryTracker::IterativelyRefineTrack_Projective_Ransac(
        const EdgeLists &nextImageEdges,
        const AllIndexLimits &allLimits,
        const s32 matching_maxDistance,
        const s32 maxMatchesPerType,
        const s32 ransac_maxIterations,
        const s32 ransac_numSamplesPerType,
        const s32 ransac_inlinerDistance,
        s32 &bestNumInliers,
        MemoryStack fastScratch,
        MemoryStack slowScratch)
      {
        Result lastResult;

        Array<f32> AtA_xDecreasing(8,8,fastScratch);
        Array<f32> AtA_xIncreasing(8,8,fastScratch);
        Array<f32> AtA_yDecreasing(8,8,fastScratch);
        Array<f32> AtA_yIncreasing(8,8,fastScratch);

        Array<f32> Atb_t_xDecreasing(1,8,fastScratch);
        Array<f32> Atb_t_xIncreasing(1,8,fastScratch);
        Array<f32> Atb_t_yDecreasing(1,8,fastScratch);
        Array<f32> Atb_t_yIncreasing(1,8,fastScratch);

        Array<f32> originalHomography(3,3,slowScratch);
        originalHomography.Set(this->transformation.get_homography());

        Array<f32> bestHomography = Eye<f32>(3,3,slowScratch);
        bestNumInliers = -1;

        FixedLengthList<IndexCorrespondence> matchingIndexes_xDecreasing(maxMatchesPerType, slowScratch);
        FixedLengthList<IndexCorrespondence> matchingIndexes_xIncreasing(maxMatchesPerType, slowScratch);
        FixedLengthList<IndexCorrespondence> matchingIndexes_yDecreasing(maxMatchesPerType, slowScratch);
        FixedLengthList<IndexCorrespondence> matchingIndexes_yIncreasing(maxMatchesPerType, slowScratch);

        lastResult = BinaryTracker::FindHorizontalCorrespondences_List(
          matching_maxDistance, this->transformation,
          this->templateEdges.xDecreasing, nextImageEdges.xDecreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.xDecreasing_yStartIndexes, matchingIndexes_xDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Projective_Ransac", "FindHorizontalCorrespondences_List 1 failed");

        lastResult = BinaryTracker::FindHorizontalCorrespondences_List(
          matching_maxDistance, this->transformation,
          this->templateEdges.xIncreasing, nextImageEdges.xIncreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.xIncreasing_yStartIndexes, matchingIndexes_xIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Projective_Ransac", "FindHorizontalCorrespondences_List 2 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_List(
          matching_maxDistance, this->transformation,
          this->templateEdges.yDecreasing, nextImageEdges.yDecreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.yDecreasing_xStartIndexes, matchingIndexes_yDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Projective_Ransac", "FindVerticalCorrespondences_List 1 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_List(
          matching_maxDistance, this->transformation,
          this->templateEdges.yIncreasing, nextImageEdges.yIncreasing,
          nextImageEdges.imageHeight, nextImageEdges.imageWidth,
          allLimits.yIncreasing_xStartIndexes, matchingIndexes_yIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::IterativelyRefineTrack_Projective_Ransac", "FindVerticalCorrespondences_List 2 failed");

        // First, try a match with all the data

        // TODO: implement this, perhaps with a different search distance. Or perhaps just call the normal function

        // Second, do ransac iterations to try to get a better match

        const s32 num_xDecreasing = matchingIndexes_xDecreasing.get_size();
        const s32 num_xIncreasing = matchingIndexes_xIncreasing.get_size();
        const s32 num_yDecreasing = matchingIndexes_yDecreasing.get_size();
        const s32 num_yIncreasing = matchingIndexes_yIncreasing.get_size();

        FixedLengthList<IndexCorrespondence> sampledMatchingIndexes_xDecreasing(ransac_numSamplesPerType, fastScratch);
        FixedLengthList<IndexCorrespondence> sampledMatchingIndexes_xIncreasing(ransac_numSamplesPerType, fastScratch);
        FixedLengthList<IndexCorrespondence> sampledMatchingIndexes_yDecreasing(ransac_numSamplesPerType, fastScratch);
        FixedLengthList<IndexCorrespondence> sampledMatchingIndexes_yIncreasing(ransac_numSamplesPerType, fastScratch);

        sampledMatchingIndexes_xDecreasing.set_size(ransac_numSamplesPerType);
        sampledMatchingIndexes_xIncreasing.set_size(ransac_numSamplesPerType);
        sampledMatchingIndexes_yDecreasing.set_size(ransac_numSamplesPerType);
        sampledMatchingIndexes_yIncreasing.set_size(ransac_numSamplesPerType);

        const IndexCorrespondence * restrict pMatchingIndexes_xDecreasing = matchingIndexes_xDecreasing.Pointer(0);
        const IndexCorrespondence * restrict pMatchingIndexes_xIncreasing = matchingIndexes_xIncreasing.Pointer(0);
        const IndexCorrespondence * restrict pMatchingIndexes_yDecreasing = matchingIndexes_yDecreasing.Pointer(0);
        const IndexCorrespondence * restrict pMatchingIndexes_yIncreasing = matchingIndexes_yIncreasing.Pointer(0);

        IndexCorrespondence * restrict pSampledMatchingIndexes_xDecreasing = sampledMatchingIndexes_xDecreasing.Pointer(0);
        IndexCorrespondence * restrict pSampledMatchingIndexes_xIncreasing = sampledMatchingIndexes_xIncreasing.Pointer(0);
        IndexCorrespondence * restrict pSampledMatchingIndexes_yDecreasing = sampledMatchingIndexes_yDecreasing.Pointer(0);
        IndexCorrespondence * restrict pSampledMatchingIndexes_yIncreasing = sampledMatchingIndexes_yIncreasing.Pointer(0);

        for(s32 iteration=0; iteration<ransac_maxIterations; iteration++) {
          for(s32 iSample=0; iSample<ransac_numSamplesPerType; iSample++) {
            const s32 index_xDecreasing = RandS32(0, num_xDecreasing);
            const s32 index_xIncreasing = RandS32(0, num_xIncreasing);
            const s32 index_yDecreasing = RandS32(0, num_yDecreasing);
            const s32 index_yIncreasing = RandS32(0, num_yIncreasing);

            //CoreTechPrint("Samples: %d %d %d %d\n", index_xDecreasing, index_xIncreasing, index_yDecreasing, index_yIncreasing);

            pSampledMatchingIndexes_xDecreasing[iSample] = pMatchingIndexes_xDecreasing[index_xDecreasing];
            pSampledMatchingIndexes_xIncreasing[iSample] = pMatchingIndexes_xIncreasing[index_xIncreasing];
            pSampledMatchingIndexes_yDecreasing[iSample] = pMatchingIndexes_yDecreasing[index_yDecreasing];
            pSampledMatchingIndexes_yIncreasing[iSample] = pMatchingIndexes_yIncreasing[index_yIncreasing];
          }

          AtA_xDecreasing.SetZero();
          AtA_xIncreasing.SetZero();
          AtA_yDecreasing.SetZero();
          AtA_yIncreasing.SetZero();

          Atb_t_xDecreasing.SetZero();
          Atb_t_xIncreasing.SetZero();
          Atb_t_yDecreasing.SetZero();
          Atb_t_yIncreasing.SetZero();

          lastResult = BinaryTracker::ApplyHorizontalCorrespondenceList_Projective(sampledMatchingIndexes_xDecreasing, AtA_xDecreasing, Atb_t_xDecreasing);

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::IterativelyRefineTrack_Projective_Ransac", "ApplyHorizontalCorrespondenceList_Projective 1 failed");

          lastResult = BinaryTracker::ApplyHorizontalCorrespondenceList_Projective(sampledMatchingIndexes_xIncreasing, AtA_xIncreasing, Atb_t_xIncreasing);

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::IterativelyRefineTrack_Projective_Ransac", "ApplyHorizontalCorrespondenceList_Projective 2 failed");

          lastResult = BinaryTracker::ApplyVerticalCorrespondenceList_Projective(sampledMatchingIndexes_yDecreasing, AtA_yDecreasing, Atb_t_yDecreasing);

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::IterativelyRefineTrack_Projective_Ransac", "ApplyVerticalCorrespondenceList_Projective 1 failed");

          lastResult = BinaryTracker::ApplyVerticalCorrespondenceList_Projective(sampledMatchingIndexes_yIncreasing, AtA_yIncreasing, Atb_t_yIncreasing);

          AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
            lastResult, "BinaryTracker::IterativelyRefineTrack_Projective_Ransac", "ApplyVerticalCorrespondenceList_Projective 2 failed");

          // Update the transformation, and count the number of inliers
          {
            PUSH_MEMORY_STACK(fastScratch);

            Array<f32> newHomography(3, 3, fastScratch);

            Array<f32> AtA(8,8,fastScratch);
            Array<f32> Atb_t(1,8,fastScratch);

            // The total AtA and Atb matrices are just the elementwise sums of their partial versions
            for(s32 y=0; y<8; y++) {
              for(s32 x=0; x<8; x++) {
                AtA[y][x] = AtA_xDecreasing[y][x] + AtA_xIncreasing[y][x] + AtA_yDecreasing[y][x] + AtA_yIncreasing[y][x];
              }

              Atb_t[0][y] = Atb_t_xDecreasing[0][y] + Atb_t_xIncreasing[0][y] + Atb_t_yDecreasing[0][y] + Atb_t_yIncreasing[0][y];
            }

            Matrix::MakeSymmetric(AtA, false);

            //AtA.Print("AtA");
            //Atb_t.Print("Atb_t");
            bool numericalFailure;
            lastResult = Matrix::SolveLeastSquaresWithCholesky<f32>(AtA, Atb_t, false, numericalFailure);

            //Atb_t.Print("result");

            AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
              lastResult, "BinaryTracker::IterativelyRefineTrack_Projective_Ransac", "SolveLeastSquaresWithCholesky failed");

            if(!numericalFailure) {
              const f32 * restrict pAtb_t = Atb_t.Pointer(0,0);

              newHomography[0][0] = pAtb_t[0]; newHomography[0][1] = pAtb_t[1]; newHomography[0][2] = pAtb_t[2];
              newHomography[1][0] = pAtb_t[3]; newHomography[1][1] = pAtb_t[4]; newHomography[1][2] = pAtb_t[5];
              newHomography[2][0] = pAtb_t[6]; newHomography[2][1] = pAtb_t[7]; newHomography[2][2] = 1.0f;

              //newHomography.Print("newHomography");

              this->transformation.set_homography(newHomography);

              s32 numInliers;
              lastResult = VerifyTrack(nextImageEdges, allLimits, ransac_inlinerDistance, numInliers);

              AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
                lastResult, "BinaryTracker::IterativelyRefineTrack_Projective_Ransac", "VerifyTrack failed");

              if(numInliers > bestNumInliers) {
                bestNumInliers = numInliers;
                bestHomography.Set(this->transformation.get_homography());
                //CoreTechPrint("inliers: %d\n", numInliers);
                //bestHomography.Print("bestHomography");
              }
            } // if(!numericalFailure)

            this->transformation.set_homography(originalHomography);
          } // Update the transformation, and count the number of inliers
        } // for(s32 iteration=0; iteration<ransac_maxIterations; iteration++)

        this->transformation.set_homography(bestHomography);

        return RESULT_OK;
      }

      Result BinaryTracker::VerifyTrack(
        const EdgeLists &nextImageEdges,
        const AllIndexLimits &allLimits,
        const s32 matching_maxDistance,
        s32 &numTemplatePixelsMatched)
      {
        Result lastResult;

        s32 numTemplatePixelsMatched_xDecreasing;
        s32 numTemplatePixelsMatched_xIncreasing;
        s32 numTemplatePixelsMatched_yDecreasing;
        s32 numTemplatePixelsMatched_yIncreasing;

        numTemplatePixelsMatched = 0;

        lastResult = BinaryTracker::FindHorizontalCorrespondences_Verify(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.xDecreasing,
          nextImageEdges.xDecreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.xDecreasing_yStartIndexes,
          numTemplatePixelsMatched_xDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::VerifyTrack", "FindHorizontalCorrespondences 1 failed");

        lastResult = BinaryTracker::FindHorizontalCorrespondences_Verify(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.xIncreasing,
          nextImageEdges.xIncreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.xIncreasing_yStartIndexes,
          numTemplatePixelsMatched_xIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::VerifyTrack", "FindHorizontalCorrespondences 2 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_Verify(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.yDecreasing,
          nextImageEdges.yDecreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.yDecreasing_xStartIndexes,
          numTemplatePixelsMatched_yDecreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::VerifyTrack", "FindVerticalCorrespondences 1 failed");

        lastResult = BinaryTracker::FindVerticalCorrespondences_Verify(
          matching_maxDistance,
          this->transformation,
          this->templateEdges.yIncreasing,
          nextImageEdges.yIncreasing,
          nextImageEdges.imageHeight,
          nextImageEdges.imageWidth,
          allLimits.yIncreasing_xStartIndexes,
          numTemplatePixelsMatched_yIncreasing);

        AnkiConditionalErrorAndReturnValue(lastResult == RESULT_OK,
          lastResult, "BinaryTracker::VerifyTrack", "FindVerticalCorrespondences 2 failed");

        // Compute the overall sum
        numTemplatePixelsMatched = numTemplatePixelsMatched_xDecreasing + numTemplatePixelsMatched_xIncreasing + numTemplatePixelsMatched_yDecreasing + numTemplatePixelsMatched_yIncreasing;

        return RESULT_OK;
      }

      s32 BinaryTracker::get_serializationSize() const
      {
        // TODO: make the correct length

        const s32 xDecreasingUsed = this->templateEdges.xDecreasing.get_size();
        const s32 xIncreasingUsed = this->templateEdges.xIncreasing.get_size();
        const s32 yDecreasingUsed = this->templateEdges.yDecreasing.get_size();
        const s32 yIncreasingUsed = this->templateEdges.yIncreasing.get_size();

        const size_t numTemplatePixels =
          RoundUp<size_t>(xDecreasingUsed, MEMORY_ALIGNMENT) +
          RoundUp<size_t>(xIncreasingUsed, MEMORY_ALIGNMENT) +
          RoundUp<size_t>(yDecreasingUsed, MEMORY_ALIGNMENT) +
          RoundUp<size_t>(yIncreasingUsed, MEMORY_ALIGNMENT);

        AnkiAssert(numTemplatePixels < std::numeric_limits<s32>::max());
        const s32 requiredBytes = 512 + static_cast<s32>(numTemplatePixels)*sizeof(Point<s16>) + Transformations::PlanarTransformation_f32::get_serializationSize() + 16*SerializedBuffer::DESCRIPTION_STRING_LENGTH;

        return requiredBytes;
      }

      s32 BinaryTracker::get_lastUsedGrayvalueThrehold() const
      {
        return this->lastUsedGrayvalueThreshold;
      }

      s32 BinaryTracker::get_lastGrayvalueThreshold() const
      {
        return this->lastGrayvalueThreshold;
      }
    } // namespace TemplateTracker
  } // namespace Embedded
} // namespace Anki
