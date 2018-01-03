/**
File: mexDrawSystemTestResults.cpp
Author: Peter Barnum
Created: 2014-06-11

Draw results of the system tests

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/draw_vision.h"

#include "mex.h"
#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/robot/matlabInterface.h"

#include <vector>

using namespace Anki::Embedded;

typedef struct ToShowResults
{
  s32 iTest;
  s32 iPose;
  f64 scene_Distance;
  f64 scene_angle;
  f64 scene_CameraExposure;
  f64 scene_light;
  char * testFunctionName;
  s32 numCorrect_positionLabelRotation;
  s32 numCorrect_positionLabel;
  s32 numCorrect_position;
  s32 numQuadsDetected;
  s32 numQuadsNotIgnored;
  s32 numSpurriousDetections;
} ToShowResults;

void DrawOneMarker(
  cv::Mat &drawnImage,
  const Quadrilateral<f32> &corners,
  const char * name,
  const f64 showImageDetectionsScale,
  const cv::Scalar &quadColor,
  const cv::Scalar &topBarColor);

void DrawOneQuad(
  cv::Mat &drawnImage,
  const Quadrilateral<f32> &quad,
  const cv::Scalar &color,
  const s32 lineWidth,
  const f64 showImageDetectionsScale);

void DrawQuads(
  cv::Mat &drawnImage,
  const FixedLengthList<Quadrilateral<f32> > &detectedQuads,
  const FixedLengthList<s32> &detectedQuads_types,
  const f64 showImageDetectionsScale);

void DrawMarkers(
  cv::Mat &drawnImage,
  const FixedLengthList<Quadrilateral<f32> > &detectedMarkers,
  const FixedLengthList<s32> &detectedMarkers_types,
  const FixedLengthList<char *> &detectedMarkers_names,
  const f64 showImageDetectionsScale);

void DrawOneMarker(
  cv::Mat &drawnImage,
  const Quadrilateral<f32> &corners,
  const char * name,
  const f64 showImageDetectionsScale,
  const cv::Scalar &quadColor,
  const cv::Scalar &topBarColor)
{
  // Get the two leftmost points
  Point<f32> minCorners[2] = {Point<f32>(FLT_MAX, FLT_MAX), Point<f32>(FLT_MAX, FLT_MAX)};
  for(s32 iCorner=0; iCorner<4; iCorner++) {
    if(corners.corners[iCorner].x < minCorners[0].x) {
      minCorners[1] = minCorners[0];
      minCorners[0] = corners.corners[iCorner];
    } else if(corners.corners[iCorner].x < minCorners[1].x) {
      minCorners[1] = corners.corners[iCorner];
    }
  }

  DrawOneQuad(drawnImage, corners, quadColor, 3, showImageDetectionsScale);

  cv::line(
    drawnImage,
    cv::Point(saturate_cast<s32>(showImageDetectionsScale*corners[0].x), saturate_cast<s32>(showImageDetectionsScale*corners[0].y)),
    cv::Point(saturate_cast<s32>(showImageDetectionsScale*corners[2].x), saturate_cast<s32>(showImageDetectionsScale*corners[2].y)),
    topBarColor, 3);

  const f32 midX = (minCorners[0].x + minCorners[1].x) / 2;
  const f32 midY = (minCorners[0].y + minCorners[1].y) / 2;

  AnkiAssert(name);

  cv::putText(
    drawnImage,
    name,
    cv::Point(saturate_cast<s32>(midX*showImageDetectionsScale + 5), saturate_cast<s32>(midY*showImageDetectionsScale)),
    cv::FONT_HERSHEY_COMPLEX_SMALL,
    0.25 * showImageDetectionsScale,
    quadColor,
    1,
    8);
}

void DrawOneQuad(
  cv::Mat &drawnImage,
  const Quadrilateral<f32> &quad,
  const cv::Scalar &color,
  const s32 lineWidth,
  const f64 showImageDetectionsScale)
{
  const s32 numLines = 4;
  const s32 lineIndexes[numLines+1] = {0,1,3,2,0};

  for(s32 i=0; i<numLines; i++) {
    const s32 index0 = lineIndexes[i];
    const s32 index1 = lineIndexes[i+1];

    cv::line(
      drawnImage,
      cv::Point(saturate_cast<s32>(showImageDetectionsScale*quad[index0].x), saturate_cast<s32>(showImageDetectionsScale*quad[index0].y)),
      cv::Point(saturate_cast<s32>(showImageDetectionsScale*quad[index1].x), saturate_cast<s32>(showImageDetectionsScale*quad[index1].y)),
      color, lineWidth);
  }
}

void DrawQuads(
  cv::Mat &drawnImage,
  const FixedLengthList<Quadrilateral<f32> > &detectedQuads,
  const FixedLengthList<s32> &detectedQuads_types,
  const f64 showImageDetectionsScale)
{
  AnkiAssert(detectedQuads.get_size() == detectedQuads_types.get_size());

  const s32 numQuads = detectedQuads.get_size();

  if(numQuads == 0)
    return;

  const Quadrilateral<f32> * pDetectedQuads = detectedQuads.Pointer(0);
  //const s32 * pDetectedQuads_types = detectedQuads_types.Pointer(0);

  for(s32 iQuad=0; iQuad<numQuads; iQuad++) {
    const Quadrilateral<f32> &curQuad = pDetectedQuads[iQuad];

    DrawOneQuad(drawnImage, curQuad, cv::Scalar(255,255,255), 3, showImageDetectionsScale);
    DrawOneQuad(drawnImage, curQuad, cv::Scalar(0,0,0), 1, showImageDetectionsScale);

    if(detectedQuads_types[iQuad] > 0) {
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

      if(detectedQuads_types[iQuad] < 100000) {
        char validText[1024];
        snprintf(validText, 1024, "%d", detectedQuads_types[iQuad]);

        cv::Point scaledMinPoint(saturate_cast<s32>(showImageDetectionsScale*minPoint.x), saturate_cast<s32>(showImageDetectionsScale*minPoint.y));

        CvPutTextWithBackground(drawnImage, validText, scaledMinPoint, cv::FONT_HERSHEY_COMPLEX_SMALL, 0.5*showImageDetectionsScale, cv::Scalar(255, 255, 255), cv::Scalar(0, 0, 0), 1, 8, true);
      } // if(detectedQuads_types[iQuad] < 100000)
    } // if(detectedQuads_types[iQuad] > 0)
  } // for(s32 iQuad=0; iQuad<numQuads; iQuad++)
} // void DrawQuads(cv::Mat &drawnImage, const FixedLengthList<Quadrilateral<f32> > &detectedQuads, const FixedLengthList<s32> &detectedQuads_types)

void DrawMarkers(
  cv::Mat &drawnImage,
  const FixedLengthList<Quadrilateral<f32> > &detectedMarkers,
  const FixedLengthList<s32> &detectedMarkers_types,
  const FixedLengthList<char *> &detectedMarkers_names,
  const f64 showImageDetectionsScale)
{
  AnkiAssert(detectedMarkers.get_size() == detectedMarkers_types.get_size());

  const s32 numMarkers = detectedMarkers.get_size();

  if(numMarkers == 0)
    return;

  for(s32 iMarker=0; iMarker<numMarkers; iMarker++) {
    cv::Scalar quadColor;
    cv::Scalar topBarColor(0,0,0);

    if(detectedMarkers_types[iMarker] == 1 || detectedMarkers_types[iMarker] == 2) {
      quadColor = cv::Scalar(170,170,170);
    } else if(detectedMarkers_types[iMarker] == 3) {
      quadColor = cv::Scalar(0,255,0);
    } else if(detectedMarkers_types[iMarker] == 4) {
      quadColor = cv::Scalar(255,0,0);
    } else if(detectedMarkers_types[iMarker] == 5) {
      quadColor = cv::Scalar(0,255,255);
    } else if(detectedMarkers_types[iMarker] == 6) {
      quadColor = cv::Scalar(0,0,255);
    } else {
      AnkiAssert(false);
    }

    DrawOneMarker(
      drawnImage,
      detectedMarkers[iMarker],
      detectedMarkers_names[iMarker],
      showImageDetectionsScale,
      quadColor,
      topBarColor);
  } // for(s32 iMarker=0; iMarker<numMarkers; iMarker++)
}

void DrawTextResults(cv::Mat &drawnImage, const ToShowResults &toShowResults, const f64 showImageDetectionsScale)
{
  const f64 textFontSize = 0.5 * showImageDetectionsScale;
  const s32 textHeightInPixels = saturate_cast<s32>(25*textFontSize);

  char resultsText[1024];

  s32 curY = saturate_cast<s32>(250 * showImageDetectionsScale);

  snprintf(resultsText, 1024, "Test %d %d %s", toShowResults.iTest, toShowResults.iPose, toShowResults.testFunctionName);

  cv::putText(
    drawnImage,
    resultsText,
    cv::Point(0, curY),
    cv::FONT_HERSHEY_COMPLEX_SMALL,
    textFontSize,
    cv::Scalar(255,255,255),
    1,
    8);

  curY += textHeightInPixels;

  snprintf(resultsText, 1024, "Dist:%dmm angle:%d expos:%0.1f light:%d", saturate_cast<s32>(toShowResults.scene_Distance), saturate_cast<s32>(toShowResults.scene_angle), toShowResults.scene_CameraExposure, saturate_cast<s32>(toShowResults.scene_light));

  cv::putText(
    drawnImage,
    resultsText,
    cv::Point(0, curY),
    cv::FONT_HERSHEY_COMPLEX_SMALL,
    textFontSize,
    cv::Scalar(255,255,255),
    1,
    8);

  curY += textHeightInPixels;

  snprintf(resultsText, 1024, "markers:%d/%d/%d/%d/%d", toShowResults.numCorrect_positionLabelRotation, toShowResults.numCorrect_positionLabel, toShowResults.numCorrect_position, toShowResults.numQuadsDetected, toShowResults.numQuadsNotIgnored);

  cv::putText(
    drawnImage,
    resultsText,
    cv::Point(0, curY),
    cv::FONT_HERSHEY_COMPLEX_SMALL,
    textFontSize,
    cv::Scalar(255,255,255),
    1,
    8);

  curY += textHeightInPixels;

  snprintf(resultsText, 1024, "spurrious:%d", toShowResults.numSpurriousDetections);

  cv::putText(
    drawnImage,
    resultsText,
    cv::Point(0, curY),
    cv::FONT_HERSHEY_COMPLEX_SMALL,
    textFontSize,
    cv::Scalar(255,255,255),
    1,
    8);

  curY += textHeightInPixels;
}

cv::Mat DrawSystemTestResults(
  const Array<u8> &inputImage,
  const FixedLengthList<Quadrilateral<f32> > &detectedQuads,
  const FixedLengthList<s32> &detectedQuads_types,
  const FixedLengthList<Quadrilateral<f32> > &detectedMarkers,
  const FixedLengthList<s32> &detectedMarkers_types,
  const FixedLengthList<char *> &detectedMarkers_names,
  const f64 showImageDetectionsScale,
  const ToShowResults &toShowResults)
{
  const s32 borderWidth = 50;

  const s32 imageHeight = inputImage.get_size(0);
  const s32 imageWidth = inputImage.get_size(1);

  cv::Mat drawnImageSmall(imageHeight+borderWidth, imageWidth, CV_8UC3);
  drawnImageSmall.setTo(0);

  s32 cx = 0;
  for(s32 y=0; y<imageHeight; y++) {
    const u8 * restrict pInputImage = inputImage.Pointer(y,0);

    for(s32 x=0; x<imageWidth; x++) {
      drawnImageSmall.data[cx++] = pInputImage[x];
      drawnImageSmall.data[cx++] = pInputImage[x];
      drawnImageSmall.data[cx++] = pInputImage[x];
    }
  }

  cv::Mat drawnImage(
    saturate_cast<s32>(showImageDetectionsScale*(imageHeight+borderWidth)),
    saturate_cast<s32>(showImageDetectionsScale*imageWidth),
    CV_8UC3);

  cv::resize(drawnImageSmall, drawnImage, drawnImage.size());

  DrawQuads(drawnImage, detectedQuads, detectedQuads_types, showImageDetectionsScale);

  DrawMarkers(drawnImage, detectedMarkers, detectedMarkers_types, detectedMarkers_names, showImageDetectionsScale);

  DrawTextResults(drawnImage, toShowResults, showImageDetectionsScale);

  return drawnImage;
} // DrawSystemTestResults()

const mxArray* GetCell(const mxArray * cellArray, const s32 index)
{
  AnkiAssert(mxGetNumberOfDimensions(cellArray) == 2);

  const mwSize *dimensions = mxGetDimensions(cellArray);

  AnkiAssert(dimensions[0] == 1 || dimensions[1] == 1);

  mwIndex subs[2];

  if(dimensions[0] == 1) {
    subs[0] = 0;
    subs[1] = index;
  } else {
    subs[0] = index;
    subs[1] = 0;
  }

  const mwIndex cellIndex = mxCalcSingleSubscript(cellArray, 2, &subs[0]);

  const mxArray * curMatlabArray = mxGetCell(cellArray, cellIndex);

  return curMatlabArray;
}

ToShowResults ParseToShowResults(const mxArray * in)
{
  ToShowResults results;

  results.iTest = saturate_cast<s32>( mxGetScalar(GetCell(in, 0)) );
  results.iPose = saturate_cast<s32>( mxGetScalar(GetCell(in, 1)) );
  results.scene_Distance = mxGetScalar(GetCell(in, 2));
  results.scene_angle = mxGetScalar(GetCell(in, 3));
  results.scene_CameraExposure = mxGetScalar(GetCell(in, 4));
  results.scene_light = mxGetScalar(GetCell(in, 5));
  results.testFunctionName = mxArrayToString(GetCell(in, 6));
  results.numCorrect_positionLabelRotation = saturate_cast<s32>( mxGetScalar(GetCell(in, 7)) );
  results.numCorrect_positionLabel = saturate_cast<s32>( mxGetScalar(GetCell(in, 8)) );
  results.numCorrect_position = saturate_cast<s32>( mxGetScalar(GetCell(in, 9)) );
  results.numQuadsDetected = saturate_cast<s32>( mxGetScalar(GetCell(in, 10)) );
  results.numQuadsNotIgnored = saturate_cast<s32>( mxGetScalar(GetCell(in, 11)) );
  results.numSpurriousDetections = saturate_cast<s32>( mxGetScalar(GetCell(in, 12)) );

  return results;
}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  const s32 bufferSize = 10000000;

  AnkiConditionalErrorAndReturn(nrhs == 9 && nlhs >= 0 && nlhs <= 1,
    "mexDrawSystemTestResults",
    "Call this function as follows:"
    "<drawnImage> = mexDrawSystemTestResults(uint8(detectedQuads), detectedQuads_types, detectedMarkers, detectedMarkers_types, detectedMarkers_names, showImageDetectionsScale, outputFilenameImage, toShowResults);");

  MemoryStack memory(mxMalloc(bufferSize), bufferSize);
  AnkiConditionalErrorAndReturn(memory.IsValid(), "mexDrawSystemTestResults", "Memory could not be allocated");

  Array<u8> inputImage = mxArrayToArray<u8>(prhs[0], memory);
  Array<Array<f64> > detectedQuadsRaw = mxCellArrayToArray<f64>(prhs[1], memory);
  Array<s32> detectedQuads_typesRaw = mxArrayToArray<s32>(prhs[2], memory);
  Array<Array<f64> > detectedMarkersRaw = mxCellArrayToArray<f64>(prhs[3], memory);
  Array<s32> detectedMarkers_typesRaw = mxArrayToArray<s32>(prhs[4], memory);
  Array<char *> detectedMarkers_namesRaw = mxCellArrayToStringArray(prhs[5], memory);
  const f64 showImageDetectionsScale = mxGetScalar(prhs[6]);
  char * outputFilenameImage = mxArrayToString(prhs[7]);
  const mxArray * toShowResultsRaw = prhs[8];

  const s32 numQuads = detectedQuadsRaw.get_numElements();
  const s32 numMarkers = detectedMarkersRaw.get_numElements();

  AnkiAssert(numQuads == detectedQuads_typesRaw.get_numElements());
  AnkiAssert(numMarkers == detectedMarkers_typesRaw.get_numElements());

  FixedLengthList<Quadrilateral<f32> > detectedQuads = FixedLengthList<Quadrilateral<f32> >(numQuads, memory, Flags::Buffer(true, false, true));
  FixedLengthList<s32> detectedQuads_types = FixedLengthList<s32>(numQuads, memory, Flags::Buffer(true, false, true));
  FixedLengthList<Quadrilateral<f32> > detectedMarkers = FixedLengthList<Quadrilateral<f32> >(numMarkers, memory, Flags::Buffer(true, false, true));
  FixedLengthList<s32> detectedMarkers_types = FixedLengthList<s32>(numMarkers, memory, Flags::Buffer(true, false, true));
  FixedLengthList<char *> detectedMarkers_names(numMarkers, memory, Flags::Buffer(true, false, true));

  for(s32 iQuad=0; iQuad<numQuads; iQuad++) {
    const Array<f64> curQuadRaw = detectedQuadsRaw.Element(iQuad);

    AnkiAssert(curQuadRaw.get_size(0) == 4 && curQuadRaw.get_size(1) == 2);

    for(s32 iCorner=0; iCorner<4; iCorner++) {
      detectedQuads[iQuad].corners[iCorner].x = static_cast<f32>(curQuadRaw[iCorner][0] - 1);
      detectedQuads[iQuad].corners[iCorner].y = static_cast<f32>(curQuadRaw[iCorner][1] - 1);
    }

    detectedQuads_types[iQuad] = detectedQuads_typesRaw.Element(iQuad);
  } // for(s32 iQuad=0; iQuad<numQuads; iQuad++)

  for(s32 iMarker=0; iMarker<numMarkers; iMarker++) {
    const Array<f64> curMarkerRaw = detectedMarkersRaw.Element(iMarker);

    AnkiAssert(curMarkerRaw.get_size(0) == 4 && curMarkerRaw.get_size(1) == 2);

    for(s32 iCorner=0; iCorner<4; iCorner++) {
      detectedMarkers[iMarker].corners[iCorner].x = static_cast<f32>(curMarkerRaw[iCorner][0] - 1);
      detectedMarkers[iMarker].corners[iCorner].y = static_cast<f32>(curMarkerRaw[iCorner][1] - 1);
    }

    detectedMarkers_types[iMarker] = detectedMarkers_typesRaw.Element(iMarker);
    detectedMarkers_names[iMarker] = detectedMarkers_namesRaw.Element(iMarker);
  } // for(s32 iMarker=0; iMarker<numMarkers; iMarker++)

  const ToShowResults toShowResults = ParseToShowResults(toShowResultsRaw);

  cv::Mat drawnImage = DrawSystemTestResults(
    inputImage,
    detectedQuads,
    detectedQuads_types,
    detectedMarkers,
    detectedMarkers_types,
    detectedMarkers_names,
    showImageDetectionsScale,
    toShowResults);

  std::vector<int> compression_params;
  compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
  compression_params.push_back(9);

  cv::imwrite(outputFilenameImage, drawnImage, compression_params);

  if(nlhs > 0) {
    plhs[0] = Anki::imageArrayToMxArray<u8>(drawnImage.data, drawnImage.rows, drawnImage.cols, drawnImage.channels());
  }

  mxFree(memory.get_buffer());
  mxFree(toShowResults.testFunctionName);

  return;
}
