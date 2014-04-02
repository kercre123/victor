/**
File: edgeDetection.cpp
Author: Peter Barnum
Created: 2014-02-06

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/edgeDetection.h"

#include "anki/common/robot/serialize.h"
#include "anki/common/robot/draw.h"
#include "anki/vision/robot/histogram.h"

namespace Anki
{
  namespace Embedded
  {
    /*      enum State
    {
    ON_WHITE,
    ON_BLACK
    };*/

    // Float registers are more plentiful on the M4. Could this be faster?
    //typedef f32 State;
    //const f32 ON_WHITE = 0;
    //const f32 ON_BLACK = 1.0f;

    NO_INLINE static void DetectBlurredEdges_horizontal(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists);
    NO_INLINE static void DetectBlurredEdges_vertical(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists);

    Result DetectBlurredEdges(const Array<u8> &image, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists)
    {
      Rectangle<s32> imageRegionOfInterest(0, image.get_size(1), 0, image.get_size(0));

      return DetectBlurredEdges(image, imageRegionOfInterest, grayvalueThreshold, minComponentWidth, everyNLines, edgeLists);
    }

    Result DetectBlurredEdges(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists)
    {
      AnkiConditionalErrorAndReturnValue(image.IsValid() && edgeLists.xDecreasing.IsValid() && edgeLists.xIncreasing.IsValid() && edgeLists.yDecreasing.IsValid() && edgeLists.yIncreasing.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "DetectBlurredEdges", "Arrays are not valid");

      AnkiConditionalErrorAndReturnValue(minComponentWidth > 0,
        RESULT_FAIL_INVALID_SIZE, "DetectBlurredEdges", "minComponentWidth is too small");

      AnkiConditionalErrorAndReturnValue(edgeLists.xDecreasing.get_maximumSize() == edgeLists.xIncreasing.get_maximumSize() &&
        edgeLists.xDecreasing.get_maximumSize() == edgeLists.yDecreasing.get_maximumSize() &&
        edgeLists.xDecreasing.get_maximumSize() == edgeLists.yIncreasing.get_maximumSize(),
        RESULT_FAIL_INVALID_SIZE, "DetectBlurredEdges", "All edgeLists must have the same maximum size");

      edgeLists.xDecreasing.Clear();
      edgeLists.xIncreasing.Clear();
      edgeLists.yDecreasing.Clear();
      edgeLists.yIncreasing.Clear();

      const s32 imageHeight = image.get_size(0);
      const s32 imageWidth = image.get_size(1);
      const s32 imageStride = image.get_stride();

      edgeLists.imageHeight = imageHeight;
      edgeLists.imageWidth = imageWidth;

      // TODO: won't detect an edge on the last horizontal (for x search) or vertical (for y search)
      //       pixel. Is there a fast way to do this?

      DetectBlurredEdges_horizontal(image, imageRegionOfInterest, grayvalueThreshold, minComponentWidth, everyNLines, edgeLists);
      DetectBlurredEdges_vertical(image, imageRegionOfInterest, grayvalueThreshold, minComponentWidth, everyNLines, edgeLists);

      return RESULT_OK;
    } // Result DetectBlurredEdges()

    NO_INLINE static void DetectBlurredEdges_horizontal(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists)
    {
      //
      // Detect horizontal positive and negative transitions
      //

      const s32 imageStride = image.get_stride();

      //const Rectangle<u32> imageRegionOfInterestU32(imageRegionOfInterest.left, imageRegionOfInterest.right, imageRegionOfInterest.top, imageRegionOfInterest.bottom);

      //const u32 minComponentWidthU32 = minComponentWidth;

      s32 xDecreasingSize = edgeLists.xDecreasing.get_size();
      s32 xIncreasingSize = edgeLists.xIncreasing.get_size();
      s32 xMaxSizeM1 = edgeLists.xDecreasing.get_maximumSize() - 1;
      u32 * restrict pXDecreasing = reinterpret_cast<u32 *>(edgeLists.xDecreasing.Pointer(0));
      u32 * restrict pXIncreasing = reinterpret_cast<u32 *>(edgeLists.xIncreasing.Pointer(0));

      for(s32 y=imageRegionOfInterest.top; y<imageRegionOfInterest.bottom; y+=everyNLines) {
        const u8 * restrict pImage = image.Pointer(y,0);

        bool onWhite;

        // Is the first pixel white or black? (probably noisy, but that's okay)
        if(pImage[0] > grayvalueThreshold)
          onWhite = true;
        else
          onWhite = false;

        s32 lastSwitchX = imageRegionOfInterest.left;
        s32 x = imageRegionOfInterest.left;
        while(x < imageRegionOfInterest.right) {
          if(onWhite) {
            // If on white

            while(x < (imageRegionOfInterest.right-3)) {
              const u32 pixels_u8x4 = *reinterpret_cast<const u32 *>(pImage+x);

              if( (pixels_u8x4 & 0xFF) <= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF00) >> 8) <= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF0000) >> 16) <= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF000000) >> 24) <= grayvalueThreshold )
                break;

              x += 4;
            }

            while( (x < imageRegionOfInterest.right) && (pImage[x] > grayvalueThreshold)) {
              x++;
            }

            onWhite = false;

            if(x < (imageRegionOfInterest.right-1)) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                // If there's room, add the point to the list
                if(xDecreasingSize < xMaxSizeM1) {
                  u32 newPoint = (y << 16) + x; // Set x and y in one operation
                  pXDecreasing[xDecreasingSize] = newPoint;

                  xDecreasingSize++;
                }
              }

              lastSwitchX = x;
            } // if(x < (imageRegionOfInterest.right-1)
          } else {
            // If on black

            while(x < (imageRegionOfInterest.right-3)) {
              const u32 pixels_u8x4 = *reinterpret_cast<const u32 *>(pImage+x);

              if( (pixels_u8x4 & 0xFF) >= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF00) >> 8) >= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF0000) >> 16) >= grayvalueThreshold )
                break;

              if( ((pixels_u8x4 & 0xFF000000) >> 24) >= grayvalueThreshold )
                break;

              x += 4;
            }

            while( (x < imageRegionOfInterest.right) && (pImage[x] < grayvalueThreshold)) {
              x++;
            }

            onWhite = true;

            if(x < (imageRegionOfInterest.right-1)) {
              const s32 componentWidth = x - lastSwitchX;

              if(componentWidth >= minComponentWidth) {
                // If there's room, add the point to the list
                if(xIncreasingSize < xMaxSizeM1) {
                  u32 newPoint = (y << 16) + x;
                  pXIncreasing[xIncreasingSize] = newPoint;

                  xIncreasingSize++;
                }
              }

              lastSwitchX = x;
            } // if(x < (imageRegionOfInterest.right-1))
          } // if(onWhite) ... else

          x++;
        } // if(onWhite) ... else
      } // for(u32 y=0; y<imageRegionOfInterest.bottom; y++)

      edgeLists.xDecreasing.set_size(xDecreasingSize);
      edgeLists.xIncreasing.set_size(xIncreasingSize);
    } // DetectBlurredEdges_horizontal()

    NO_INLINE static void DetectBlurredEdges_vertical(const Array<u8> &image, const Rectangle<s32> &imageRegionOfInterest, const u8 grayvalueThreshold, const s32 minComponentWidth, const s32 everyNLines, EdgeLists &edgeLists)
    {
      //
      //  Detect vertical positive and negative transitions
      //

      const s32 imageStride = image.get_stride();

      //const Rectangle<u32> imageRegionOfInterestU32(imageRegionOfInterest.left, imageRegionOfInterest.right, imageRegionOfInterest.top, imageRegionOfInterest.bottom);

      //const u32 minComponentWidthU32 = minComponentWidth;

      s32 yDecreasingSize = edgeLists.yDecreasing.get_size();
      s32 yIncreasingSize = edgeLists.yIncreasing.get_size();
      s32 yMaxSizeM1 = edgeLists.yDecreasing.get_maximumSize() - 1;
      u32 * restrict pYDecreasing = reinterpret_cast<u32 *>(edgeLists.yDecreasing.Pointer(0));
      u32 * restrict pYIncreasing = reinterpret_cast<u32 *>(edgeLists.yIncreasing.Pointer(0));

      for(s32 x=imageRegionOfInterest.left; x<imageRegionOfInterest.right; x+=everyNLines) {
        const u8 * restrict pImage = image.Pointer(imageRegionOfInterest.top, x);

        bool onWhite;

        // Is the first pixel white or black? (probably noisy, but that's okay)
        if(pImage[0] > grayvalueThreshold)
          onWhite = true;
        else
          onWhite = false;

        s32 lastSwitchY = imageRegionOfInterest.top;
        s32 y = imageRegionOfInterest.top;
        while(y < imageRegionOfInterest.bottom) {
          if(onWhite) {
            // If on white

            while(y < (imageRegionOfInterest.bottom-1)) {
              const u8 pixel0 = pImage[0];
              const u8 pixel1 = pImage[imageStride];

              if(pixel0 <= grayvalueThreshold)
                break;

              if(pixel1 <= grayvalueThreshold)
                break;

              y += 2;
              pImage += 2*imageStride;
            }

            while( (y < imageRegionOfInterest.bottom) && (pImage[0] > grayvalueThreshold) ){
              y++;
              pImage += imageStride;
            }

            onWhite = false;

            if(y < (imageRegionOfInterest.bottom-1)) {
              const s32 componentWidth = y - lastSwitchY;

              if(componentWidth >= minComponentWidth) {
                // If there's room, add the point to the list
                if(yDecreasingSize < yMaxSizeM1) {
                  u32 newPoint = (y << 16) + x;
                  pYDecreasing[yDecreasingSize] = newPoint;

                  yDecreasingSize++;
                }
              }

              lastSwitchY = y;
            } // if(y < (imageRegionOfInterest.bottom-1)
          } else {
            // If on black

            while(y < (imageRegionOfInterest.bottom-1)) {
              const u8 pixel0 = pImage[0];
              const u8 pixel1 = pImage[imageStride];

              if(pixel0 >= grayvalueThreshold)
                break;

              if(pixel1 >= grayvalueThreshold)
                break;

              y += 2;
              pImage += 2*imageStride;
            }

            while( (y < imageRegionOfInterest.bottom) && (pImage[0] < grayvalueThreshold) ) {
              y++;
              pImage += imageStride;
            }

            onWhite = true;

            if(y < (imageRegionOfInterest.bottom-1)) {
              const s32 componentWidth = y - lastSwitchY;

              if(componentWidth >= minComponentWidth) {
                // If there's room, add the point to the list
                if(yIncreasingSize < yMaxSizeM1) {
                  u32 newPoint = (y << 16) + x;
                  pYIncreasing[yIncreasingSize] = newPoint;

                  yIncreasingSize++;
                }
              }

              lastSwitchY = y;
            } // if(y < (imageRegionOfInterest.bottom-1)
          } // if(onWhite) ... else

          y++;
          pImage += imageStride;
        } // while(y < imageRegionOfInterest.bottom)
      } // for(u32 x=0; x<imageRegionOfInterest.right; x++)

      edgeLists.yDecreasing.set_size(yDecreasingSize);
      edgeLists.yIncreasing.set_size(yIncreasingSize);
    } // DetectBlurredEdges_vertical()

    Result EdgeLists::Serialize(const char *objectName, SerializedBuffer &buffer) const
    {
      s32 totalDataLength = this->get_serializationSize();

      void *segment = buffer.Allocate("EdgeLists", objectName, totalDataLength);

      if(segment == NULL) {
        return RESULT_FAIL;
      }

      if(SerializedBuffer::SerializeDescriptionStrings("EdgeLists", objectName, &segment, totalDataLength) != RESULT_OK)
        return RESULT_FAIL;

      // Serialize the template lists
      SerializedBuffer::SerializeRawBasicType<s32>("imageHeight", this->imageHeight, &segment, totalDataLength);
      SerializedBuffer::SerializeRawBasicType<s32>("imageWidth", this->imageWidth, &segment, totalDataLength);
      SerializedBuffer::SerializeRawFixedLengthList<Point<s16> >("xDecreasing", this->xDecreasing, &segment, totalDataLength);
      SerializedBuffer::SerializeRawFixedLengthList<Point<s16> >("xIncreasing", this->xIncreasing, &segment, totalDataLength);
      SerializedBuffer::SerializeRawFixedLengthList<Point<s16> >("yDecreasing", this->yDecreasing, &segment, totalDataLength);
      SerializedBuffer::SerializeRawFixedLengthList<Point<s16> >("yIncreasing", this->yIncreasing, &segment, totalDataLength);

      return RESULT_OK;
    }

    Result EdgeLists::Deserialize(char *objectName, void** buffer, s32 &bufferLength, MemoryStack &memory)
    {
      // TODO: check if the name is correct
      if(SerializedBuffer::DeserializeDescriptionStrings(NULL, objectName, buffer, bufferLength) != RESULT_OK)
        return RESULT_FAIL;

      this->imageHeight = SerializedBuffer::DeserializeRawBasicType<s32>(NULL, buffer, bufferLength);
      this->imageWidth  = SerializedBuffer::DeserializeRawBasicType<s32>(NULL, buffer, bufferLength);
      this->xDecreasing = SerializedBuffer::DeserializeRawFixedLengthList<Point<s16> >(NULL, buffer, bufferLength, memory);
      this->xIncreasing = SerializedBuffer::DeserializeRawFixedLengthList<Point<s16> >(NULL, buffer, bufferLength, memory);
      this->yDecreasing = SerializedBuffer::DeserializeRawFixedLengthList<Point<s16> >(NULL, buffer, bufferLength, memory);
      this->yIncreasing = SerializedBuffer::DeserializeRawFixedLengthList<Point<s16> >(NULL, buffer, bufferLength, memory);

      return RESULT_OK;
    }

    s32 EdgeLists::get_serializationSize() const
    {
      // TODO: make the correct length

      const s32 xDecreasingUsed = this->xDecreasing.get_size();
      const s32 xIncreasingUsed = this->xIncreasing.get_size();
      const s32 yDecreasingUsed = this->yDecreasing.get_size();
      const s32 yIncreasingUsed = this->yIncreasing.get_size();

      const s32 numTemplatePixels =
        RoundUp<size_t>(xDecreasingUsed, MEMORY_ALIGNMENT) +
        RoundUp<size_t>(xIncreasingUsed, MEMORY_ALIGNMENT) +
        RoundUp<size_t>(yDecreasingUsed, MEMORY_ALIGNMENT) +
        RoundUp<size_t>(yIncreasingUsed, MEMORY_ALIGNMENT);

      const s32 requiredBytes = 512 + numTemplatePixels*sizeof(Point<s16>) + 14*SerializedBuffer::DESCRIPTION_STRING_LENGTH;

      return requiredBytes;
    }

#ifdef ANKICORETECH_EMBEDDED_USE_OPENCV
    cv::Mat EdgeLists::DrawIndexes() const
    {
      AnkiConditionalErrorAndReturnValue(this->imageHeight > 16 && this->imageHeight < 2000 && this->imageWidth > 16 && this->imageWidth < 2000,
        cv::Mat(), "EdgeLists::DrawIndexes", "This object is invalid");

      return DrawIndexes(this->imageHeight, this->imageWidth, this->xDecreasing, this->xIncreasing, this->yDecreasing, this->yIncreasing);
    }

    cv::Mat EdgeLists::DrawIndexes(
      const s32 imageHeight, const s32 imageWidth,
      const FixedLengthList<Point<s16> > &indexPoints1,
      const FixedLengthList<Point<s16> > &indexPoints2,
      const FixedLengthList<Point<s16> > &indexPoints3,
      const FixedLengthList<Point<s16> > &indexPoints4)
    {
      const u8 colors[4][3] = {
        {128,0,0},
        {0,128,0},
        {0,0,128},
        {128,128,0}};

      AnkiConditionalErrorAndReturnValue(
        imageHeight > 0 && imageHeight < 2000 && imageWidth > 0 && imageWidth < 2000 &&
        indexPoints1.IsValid() && indexPoints2.IsValid() && indexPoints3.IsValid() && indexPoints4.IsValid(),
        cv::Mat(), "EdgeLists::DrawIndexes", "inputs are not valid");

      const s32 scratchSize = 10000000;
      MemoryStack scratch(malloc(scratchSize), scratchSize);

      Array<u8> image1(imageHeight, imageWidth, scratch);
      Array<u8> image2(imageHeight, imageWidth, scratch);
      Array<u8> image3(imageHeight, imageWidth, scratch);
      Array<u8> image4(imageHeight, imageWidth, scratch);

      DrawPoints(indexPoints1, 1, image1);
      DrawPoints(indexPoints2, 2, image2);
      DrawPoints(indexPoints3, 3, image3);
      DrawPoints(indexPoints4, 4, image4);

      cv::Mat totalImage(imageHeight, imageWidth, CV_8UC3);
      totalImage.setTo(0);

      for(s32 y=0; y<imageHeight; y++) {
        for(s32 x=0; x<imageWidth; x++) {
          u8* pTotalImage = totalImage.ptr<u8>(y,x);

          if(image1[y][x] != 0) {
            for(s32 c=0; c<3; c++) {
              pTotalImage[c] += colors[0][c];
            }
          }

          if(image2[y][x] != 0) {
            for(s32 c=0; c<3; c++) {
              pTotalImage[c] += colors[1][c];
            }
          }

          if(image3[y][x] != 0) {
            for(s32 c=0; c<3; c++) {
              pTotalImage[c] += colors[2][c];
            }
          }

          if(image4[y][x] != 0) {
            for(s32 c=0; c<3; c++) {
              pTotalImage[c] += colors[3][c];
            }
          }
        }
      }

      free(scratch.get_buffer());

      return totalImage;
    }

#endif // #ifdef ANKICORETECH_EMBEDDED_USE_OPENCV

    u8 ComputeGrayvalueThrehold(
      const Array<u8> &image,
      const Rectangle<s32> &imageRegionOfInterest,
      const s32 yIncrement,
      const s32 xIncrement,
      const f32 blackPercentile,
      const f32 whitePercentile,
      Histogram &histogram,
      MemoryStack scratch)
    {
      histogram = ComputeHistogram(image, imageRegionOfInterest, yIncrement, xIncrement, scratch);

      const s32 grayvalueBlack = ComputePercentile(histogram, blackPercentile);
      const s32 grayvalueWhite = ComputePercentile(histogram, whitePercentile);

      const u8 grayvalueThreshold = static_cast<u8>( (grayvalueBlack + grayvalueWhite) / 2 );

      return grayvalueThreshold;
    }
  } // namespace Embedded
} // namespace Anki
