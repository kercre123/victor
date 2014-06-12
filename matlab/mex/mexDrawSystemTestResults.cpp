/**
File: mexDrawSystemTestResults.cpp
Author: Peter Barnum
Created: 2014-06-11

Draw results of the system tests

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/draw_vision.h"

#include "mex.h"
#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/robot/matlabInterface.h"

using namespace Anki::Embedded;

void DrawOneMarker(cv::Mat &drawnImage, const Quadrilateral<f32> &corners, const char * name, const f32 showImageDetectionsScale, const cv::Scalar &quadColor, const cv::Scalar &topBarColor)
{
}

void DrawOneQuad(cv::Mat &drawnImage, const Quadrilateral<f32> &quad, const cv::Scalar &color, const s32 lineWidth, const f32 showImageDetectionsScale)
{
  const s32 numLines = 4;
  const s32 lineIndexes[numLines+1] = {0,1,3,2,0};

  for(s32 i=0; i<numLines; i++) {
    const s32 index0 = lineIndexes[i];
    const s32 index1 = lineIndexes[i+1];
    cv::line(drawnImage, cv::Point(saturate_cast<s32>(showImageDetectionsScale*quad[index0].x), saturate_cast<s32>(showImageDetectionsScale*quad[index0].y)),  cv::Point(saturate_cast<s32>(showImageDetectionsScale*quad[index1].x), saturate_cast<s32>(showImageDetectionsScale*quad[index1].y)), color, lineWidth);
  }
}

void DrawQuads(cv::Mat &drawnImage, const FixedLengthList<Quadrilateral<f32> > &detectedQuads, const FixedLengthList<s32> &detectedQuads_type, const f32 showImageDetectionsScale)
{
  AnkiAssert(detectedQuads.get_size() == detectedQuads_type.get_size());

  const s32 numQuads = detectedQuads.get_size();

  const Quadrilateral<f32> * pDetectedQuads = detectedQuads.Pointer(0);
  const s32 * pDetectedQuads_type = detectedQuads_type.Pointer(0);

  for(s32 iQuad=0; iQuad<numQuads; iQuad++) {
    const Quadrilateral<f32> &curQuad = pDetectedQuads[iQuad];

    DrawOneQuad(drawnImage, curQuad, cv::Scalar(255,255,255), 3, showImageDetectionsScale);
    DrawOneQuad(drawnImage, curQuad, cv::Scalar(0,0,0), 1, showImageDetectionsScale);

    if(detectedQuads_type[iQuad] > 0) {
      // Find the leftmost corner, with a slight preference to choose the upper one
      Point<f32> minPoint = Point<f32>(FLT_MAX, FLT_MAX);
      for(s32 i=0; i<4; i++) {
        if(curQuad.corners[i].x < (minPoint.x+3)) {
          minPoint = curQuad.corners[i];
        }
      }

      for(s32 i=0; i<4; i++) {
        if(curQuad.corners[i].x < (minPoint.x+3) && curQuad.corners[i].y < minPoint.y) {
          minPoint = curQuad.corners[i];
        }
      }

      if(detectedQuads_type[iQuad] < 100000) {
        char validText[1024];
        snprintf(validText, 1024, "%d", detectedQuads_type[iQuad]);

        cv::Point scaledMinPoint(saturate_cast<f32>(showImageDetectionsScale*minPoint.x), saturate_cast<f32>(showImageDetectionsScale*minPoint.y));

        CvPutTextWithBackground(drawnImage, validText, scaledMinPoint, cv::FONT_HERSHEY_PLAIN, showImageDetectionsScale, cv::Scalar(255, 255, 255), cv::Scalar(0, 0, 0), 1, 8, true);
      } // if(detectedQuads_type[iQuad] < 100000)
    } // if(detectedQuads_type[iQuad] > 0)
  } // for(s32 iQuad=0; iQuad<numQuads; iQuad++)
} // void DrawQuads(cv::Mat &drawnImage, const FixedLengthList<Quadrilateral<f32> > &detectedQuads, const FixedLengthList<s32> &detectedQuads_type)

void DrawMarkers(
  cv::Mat &drawnImage,
  const FixedLengthList<Quadrilateral<f32> > &detectedMarkers,
  const FixedLengthList<s32> &detectedMarkers_type,
  const FixedLengthList<const char *> &detectedMarkersNames,
  const f32 showImageDetectionsScale)
{
  AnkiAssert(detectedMarkers.get_size() == detectedMarkers_type.get_size());

  const s32 numQuads = detectedMarkers.get_size();

  const Quadrilateral<f32> * pdetectedMarkers = detectedMarkers.Pointer(0);
  const s32 * pdetectedMarkers_type = detectedMarkers_type.Pointer(0);

  for(s32 iMarker=0; iMarker<numQuads; iMarker++) {
    const Quadrilateral<f32> &curQuad = pdetectedMarkers[iMarker];
  } // for(s32 iMarker=0; iMarker<numQuads; iMarker++)
}

cv::Mat DrawSystemTestResults(const Array<u8> &inputImage, const f32 showImageDetectionsScale, const char * outputFilenameImage, const FixedLengthList<Quadrilateral<f32> > &detectedQuads, const FixedLengthList<s32> &detectedQuads_type, const FixedLengthList<Quadrilateral<f32> > &detectedMarkers, const FixedLengthList<s32> &detectedMarkers_type)
{
  const s32 borderWidth = 50;

  const s32 imageHeight = inputImage.get_size(0);
  const s32 imageWidth = inputImage.get_size(1);

  cv::Mat drawnImage(imageHeight+borderWidth, imageWidth, CV_8UC3);
  drawnImage.setTo(0);

  s32 cx = 0;
  for(s32 y=0; y<imageHeight; y++) {
    const u8 * restrict pInputImage = inputImage.Pointer(y,0);

    for(s32 x=0; x<imageWidth; x++) {
      drawnImage.data[cx++] = pInputImage[x];
      drawnImage.data[cx++] = pInputImage[x];
      drawnImage.data[cx++] = pInputImage[x];
    }
  }

  DrawQuads(drawnImage, detectedQuads, detectedQuads_type);

  return drawnImage;
} // DrawSystemTestResults()

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  const s32 bufferSize = 10000000;

  //AnkiConditionalErrorAndReturn(nrhs == 6 && nlhs == 1, "mexDrawSystemTestResults", "Call this function as following: drawnImage = mexDrawSystemTestResults(uint8(image), quadsCellArray, decode_minContrastRatio, quadRefinementIterations, numRefinementSamples, returnInvalidMarkers);");

  MemoryStack memory(mxMalloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexDetectFiducialMarkers_quadInput", "Memory could not be allocated");

  Array<u8> inputImage = mxArrayToArray<u8>(prhs[0], memory);
  Array<Array<f64> > detectedQuadsRaw = mxCellArrayToArray<f64>(prhs[1], memory);
  Array<s32> detectedQuads_typeRaw = mxArrayToArray<s32>(prhs[2], memory);
  Array<Array<f64> > detectedMarkersRaw = mxCellArrayToArray<f64>(prhs[3], memory);
  Array<s32> detectedMarkers_typeRaw = mxArrayToArray<s32>(prhs[4], memory);
  const char * outputFilenameImage = mxArrayToString(prhs[5]);
  const f32 showImageDetectionsScale = static_cast<f32>(mxGetScalar(prhs[6]));

  AnkiAssert(AreEqualSize(detectedQuadsRaw, detectedQuads_typeRaw));
  AnkiAssert(AreEqualSize(detectedMarkersRaw, detectedMarkers_typeRaw));

  AnkiAssert(detectedQuadsRaw.get_size(0) == 1);
  AnkiAssert(detectedMarkersRaw.get_size(0) == 1);

  const s32 numQuads = detectedQuadsRaw.get_size(1);
  const s32 numMarkers = detectedMarkersRaw.get_size(1);

  FixedLengthList<Quadrilateral<f32> > detectedQuads = FixedLengthList<Quadrilateral<f32> >(numQuads, memory, Flags::Buffer(true, false, true));
  FixedLengthList<s32> detectedQuads_type = FixedLengthList<s32>(numQuads, memory, Flags::Buffer(true, false, true));
  FixedLengthList<Quadrilateral<f32> > detectedMarkers = FixedLengthList<Quadrilateral<f32> >(numMarkers, memory, Flags::Buffer(true, false, true));
  FixedLengthList<s32> detectedMarkers_type = FixedLengthList<s32>(numMarkers, memory, Flags::Buffer(true, false, true));

  for(s32 iQuad=0; iQuad<numQuads; iQuad++) {
    for(s32 iCorner=0; iCorner<4; iCorner++) {
      detectedQuads[iQuad].corners[iCorner].x = static_cast<f32>(detectedQuadsRaw[0][iQuad][iCorner][0] - 1);
      detectedQuads[iQuad].corners[iCorner].y = static_cast<f32>(detectedQuadsRaw[0][iQuad][iCorner][1] - 1);
    }

    detectedQuads_type[iQuad] = detectedQuads_typeRaw[0][iQuad];
  } // for(s32 iQuad=0; iQuad<numQuads; iQuad++)

  for(s32 iMarker=0; iMarker<numMarkers; iMarker++) {
    for(s32 iCorner=0; iCorner<4; iCorner++) {
      detectedMarkers[iMarker].corners[iCorner].x = static_cast<f32>(detectedMarkersRaw[0][iMarker][iCorner][0] - 1);
      detectedMarkers[iMarker].corners[iCorner].y = static_cast<f32>(detectedMarkersRaw[0][iMarker][iCorner][1] - 1);
    }

    detectedMarkers_type[iMarker] = detectedMarkers_typeRaw[0][iMarker];
  } // for(s32 iMarker=0; iMarker<numMarkers; iMarker++)

  cv::Mat drawnImage = DrawSystemTestResults(inputImage, showImageDetectionsScale, outputFilenameImage, detectedQuads, detectedQuads_type, detectedMarkers, detectedMarkers_type);

  plhs[0] = imageArrayToMxArray<u8>(drawnImage.data, drawnImage.rows, drawnImage.cols, drawnImage.channels());

  return;
}
