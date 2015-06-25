/**
File: laplacianPeaks.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "anki/vision/robot/fiducialDetection.h"
#include "anki/vision/robot/imageProcessing.h"

#include "anki/common/robot/benchmarking.h"

//using namespace std;

typedef struct
{
  f32 slope;
  f32 intercept;
  bool switched;
} LineFits;

namespace Anki
{
  namespace Embedded
  {
    Result ExtractLineFitsPeaks(const FixedLengthList<Point<s16> > &boundary, FixedLengthList<Point<s16> > &peaks, const s32 imageHeight, const s32 imageWidth, MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(boundary.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "boundary is not valid");

      AnkiConditionalErrorAndReturnValue(peaks.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "peaks is not valid");

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(peaks.get_maximumSize() == 4,
          RESULT_FAIL_INVALID_PARAMETER, "ComputeQuadrilateralsFromConnectedComponents", "Currently only four peaks supported");
      
      const s32 boundaryLength = boundary.get_size();
        
      const f32 sigma = static_cast<f32>(boundaryLength) / 64.0f;

      // Copy the boundary to a f32 openCV array
      cv::Mat_<f32> boundaryCv(2, boundaryLength);
      const Point<s16> * restrict pBoundary = boundary.Pointer(0);
      f32 * restrict pBoundaryCv0 = boundaryCv.ptr<f32>(0,0);
      f32 * restrict pBoundaryCv1 = boundaryCv.ptr<f32>(1,0);
      for(s32 i=0; i<boundaryLength; i++) {
        pBoundaryCv0[i] = pBoundary[i].x;
        pBoundaryCv1[i] = pBoundary[i].y;
      }
      
      const s32 ksize = static_cast<s32>(ceilf((((sigma - 0.8f)/0.3f) + 1.0f)*2.0f + 1.0f) );
      cv::Mat gk = cv::getGaussianKernel(ksize, sigma, CV_32F).t();
      
      cv::Mat_<f32> dxKernel(1,3);
      dxKernel.at<f32>(0,0) = -0.5f; dxKernel.at<f32>(0,1) = 0; dxKernel.at<f32>(0,2) = 0.5f;

      cv::Mat_<f32> dg;
      cv::Mat_<f32> dB;
      
      cv::filter2D(gk, dg, CV_32F, dxKernel, cv::Point(-1,-1), 0, cv::BORDER_DEFAULT);

      /*printf("gk %d:\n", gk.cols);
      for(s32 i=0; i<gk.cols; i++) {
        printf("%0.3f ", gk.at<f32>(0,i));
      }
      printf("\n");
      
      printf("dB %d:\n", dB.cols);
      for(s32 i=0; i<dB.cols; i++) {
        printf("%0.3f ", dB.at<f32>(0,i));
      }
      printf("\n");*/
      
/*      printf("dg %d:\n", dg.cols);
      for(s32 i=0; i<dg.cols; i++) {
        printf("%0.3f ", dg.at<f32>(0,i));
      }
      printf("\n\n");*/
      
      // TODO: when cv::filter2D supports cv::BORDER_WRAP, this can be done faster
      //cv::filter2D(boundaryCv, dB, CV_32F, dg, cv::Point(-1,-1), 0, cv::BORDER_WRAP);
      
/*      cv::Mat_<f32> boundaryLarge(2, boundaryLength + dg.cols + 2);
      boundaryLarge.setTo(0);
      boundaryCv.copyTo(boundaryLarge(cv::Rect(dg.cols/2+1, 0, boundaryLength, 2)));*/
      cv::Mat_<f32> boundaryLarge(2, boundaryLength + 2*dg.cols + 2);
      boundaryLarge.setTo(0);
      boundaryCv(cv::Rect(boundaryLength-dg.cols, 0, dg.cols, 2)).copyTo(boundaryLarge(cv::Rect(0, 0, dg.cols, 2)));
      boundaryCv(cv::Rect(0, 0, dg.cols, 2)).copyTo(boundaryLarge(cv::Rect(boundaryLarge.cols-dg.cols-2, 0, dg.cols, 2)));
      boundaryCv.copyTo(boundaryLarge(cv::Rect(dg.cols, 0, boundaryLength, 2)));
      
      cv::filter2D(boundaryLarge, dB, CV_32F, dg, cv::Point(-1,-1), 0, cv::BORDER_DEFAULT);
      
      //dB = dB(cv::Range::all(), cv::Range(dg.cols, dg.cols+boundaryLength)).clone();
      dB = dB(cv::Rect(dg.cols, 0, boundaryLength, 2)).clone();
      
      //boundaryLarge(cv::Range::all(), cv::Range(dg.cols/2+1, dg.cols/2+1+boundaryLength)) = 5;
      
      /*GaussianBlur(repeat(points, 3, 1), ret, cv::Size(0,0), sigma);
int rows = points.rows;
result = Mat(result, Range(rows, 2 * rows - 1), Range::all());*/
      /*
      printf("boundaryCv %d:\n", boundaryCv.cols);
      for(s32 i=0; i<boundaryCv.cols; i++) {
        printf("%0.3f ", boundaryCv.at<f32>(0,i));
      }
      printf("\n");
      for(s32 i=0; i<boundaryCv.cols; i++) {
        printf("%0.3f ", boundaryCv.at<f32>(1,i));
      }
      printf("\n\n");
      
      printf("boundaryLarge %d:\n", boundaryLarge.cols);
      for(s32 i=0; i<boundaryLarge.cols; i++) {
        printf("%0.3f ", boundaryLarge.at<f32>(0,i));
      }
      printf("\n");
      for(s32 i=0; i<boundaryLarge.cols; i++) {
        printf("%0.3f ", boundaryLarge.at<f32>(1,i));
      }
      printf("\n\n");
      
      
      printf("dB %d:\n", dB.cols);
      for(s32 i=0; i<dB.cols; i++) {
        printf("%0.3f ", dB.at<f32>(0,i));
      }
      printf("\n\n");*/


      cv::Mat_<f32> bin(1, boundaryLength);
      
      const f32 * restrict pDB0 = dB.ptr<f32>(0,0);
      const f32 * restrict pDB1 = dB.ptr<f32>(1,0);
      f32 * restrict pBin = bin.ptr<f32>(0,0);
      for(s32 i=0; i<boundaryLength; i++) {
        pBin[i] = atan2f(pDB1[i], pDB0[i]);
      }
      
/*      printf("angles %d:\n", bin.cols);
      for(s32 i=0; i<bin.cols; i++) {
        printf("%0.3f ", bin.at<f32>(0,i));
      }
      printf("\n\n");*/

      // NOTE: this histogram is a bit different than matlab's
      s32 histSize = 16;
      f32 range[] = {-PI_F, PI_F};
      const f32 *ranges[] = { range };

      cv::Mat hist;
      cv::calcHist(&bin, 1, 0, cv::Mat(), hist, 1, &histSize, ranges, true, false);

/*      printf("hist %d:\n", hist.rows);
      for(s32 i=0; i<hist.rows; i++) {
        printf("%0.3f ", hist.at<f32>(i,0));
      }
      printf("\n\n");*/

      cv::Mat maxBins;
      cv::sortIdx(hist, maxBins, CV_SORT_EVERY_COLUMN | CV_SORT_DESCENDING);
      
/*      printf("maxBins %d:\n", maxBins.rows);
      for(s32 i=0; i<maxBins.rows; i++) {
        printf("%d ", maxBins.at<s32>(i,0));
      }
      printf("\n\n");*/
      
      bool didFitFourLines = true;
      
      LineFits lineFits[4];
      f32 p[4][2];
      
      for(s32 iBin=0; iBin<4; iBin++) {
        // boundaryIndex = find(bin==maxBins(iBin));
        std::vector<s32> boundaryIndex;
        
        // WARNING: May have some corner cases
        const f32 minAngle = -PI_F + PI_F/16*2*  maxBins.at<s32>(iBin,0);
        const f32 maxAngle = -PI_F + PI_F/16*2* (maxBins.at<s32>(iBin,0)+1);
        
        for(s32 i=0; i<boundaryLength; i++) {
          if(pBin[i] >= minAngle && pBin[i] <= maxAngle) {
            boundaryIndex.push_back(i);
          }
        }
        
        const s32 numSide = boundaryIndex.size();
        
        if(numSide > 1) {
          // all(boundary(boundaryIndex,2)==boundary(boundaryIndex(1),2))
          bool allAreTheFirst = true;
          for(s32 i=0; i<numSide; i++) {
            if(boundaryIndex[i] == boundaryIndex[0]) {
              allAreTheFirst = false;
              break;
            }
          }

          if(allAreTheFirst) {
            p[iBin][0] = 0;
            p[iBin][1] = boundary.Pointer(boundaryIndex[0])->y;
            lineFits[iBin].switched = true;
          } else {
            // A = [boundary(boundaryIndex,2) ones(numSide,1)];
            // p{iBin} = A \ boundary(boundaryIndex,1);
          
            cv::Mat_<f32> A(boundaryIndex.size(),2);
            cv::Mat_<f32> b(boundaryIndex.size(), 1);
            cv::Mat_<f32> x(2, 1);

            for(s32 i=0; i<boundaryIndex.size(); i++) {
              A.at<f32>(i,0) = boundary.Pointer(boundaryIndex[i])->y;
              A.at<f32>(i,1) = 1;
              b.at<f32>(i,0) = boundary.Pointer(boundaryIndex[i])->x;
            }
            
            // TODO: for speed, switch to QR with gram
            cv::solve(A, b, x, cv::DECOMP_SVD);
            
            p[iBin][0] = x.at<f32>(0,0);
            p[iBin][1] = x.at<f32>(1,0);
            
            lineFits[iBin].switched = false;
          }
          
          lineFits[iBin].slope = p[iBin][0];
          lineFits[iBin].intercept = p[iBin][1];
        } else {
          didFitFourLines = false;
          break;
        } // if(numSide > 1) ... else
      } // for(s32 iBin=0; iBin<4; iBin++)
      
      std::vector<cv::Point_<f32> > corners;
      
      if(didFitFourLines) {
        for(s32 iLine=0; iLine<4; iLine++) {
          for(s32 jLine=iLine+1; jLine<4; jLine++) {
            f32 xInt = 0;
            f32 yInt = 0;
            
            if(lineFits[iLine].switched == lineFits[jLine].switched) {
              xInt = (lineFits[jLine].intercept - lineFits[iLine].intercept) / (lineFits[iLine].slope - lineFits[jLine].slope);
              yInt = lineFits[iLine].slope*xInt + lineFits[iLine].intercept;
              
              if(lineFits[iLine].switched == true) {
                std::swap(xInt, yInt);
              }
            } else if(lineFits[iLine].switched == true && lineFits[jLine].switched == false) {
              yInt = (lineFits[iLine].slope*lineFits[jLine].intercept + lineFits[iLine].intercept) / (1 - lineFits[iLine].slope*lineFits[jLine].slope);
              xInt = lineFits[jLine].slope*yInt + lineFits[jLine].intercept;
            } else if(lineFits[iLine].switched == false && lineFits[jLine].switched == true) {
              yInt = (lineFits[jLine].slope*lineFits[iLine].intercept + lineFits[jLine].intercept) / (1 - lineFits[iLine].slope*lineFits[jLine].slope);
              xInt = lineFits[iLine].slope*yInt + lineFits[iLine].intercept;
            }
            
            if(xInt >= 1 && xInt <= imageWidth && yInt >= 1 && yInt <= imageHeight) {
              corners.push_back(cv::Point_<f32>(xInt, yInt));
            }
          } // for(s32 jLine=0; jLine<4; jLine++)
        } // for(s32 iLine=0; iLine<4; iLine++)
      } // if(didFitFourLines)
      
      if(corners.size() == 4) {
        Quadrilateral<f32> quad(
          Point<f32>(corners[0].x, corners[0].y),
          Point<f32>(corners[1].x, corners[1].y),
          Point<f32>(corners[2].x, corners[2].y),
          Point<f32>(corners[3].x, corners[3].y));
        
        Quadrilateral<f32> sortedQuad = quad.ComputeClockwiseCorners<f32>();
        
        peaks.set_size(4);
        
        for(s32 i=0; i<4; i++) {
          peaks[i] = Point<s16>(saturate_cast<s16>(sortedQuad.corners[i].x), saturate_cast<s16>(sortedQuad.corners[i].y));
        }
        
        // Reorder the indexes to be in the order
        // [corner1, theCornerOppositeCorner1, corner2, corner3]
        //corners = corners([1 4 2 3],:);
      } else {
        corners.clear();
        
        for(s32 i=0; i<4; i++) {
          peaks[i] = Point<s16>(0, 0);
        }
        
        peaks.Clear();
      }
      
      return RESULT_OK;
    } // ExtractLineFitsPeaks()
  
    Result ExtractLaplacianPeaks(const FixedLengthList<Point<s16> > &boundary, const s32 minPeakRatio, FixedLengthList<Point<s16> > &peaks, MemoryStack scratch)
    {
      //BeginBenchmark("elp_part1");

      AnkiConditionalErrorAndReturnValue(boundary.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "boundary is not valid");

      AnkiConditionalErrorAndReturnValue(peaks.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "peaks is not valid");

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(peaks.get_maximumSize() == 4,
        RESULT_FAIL_INVALID_PARAMETER, "ComputeQuadrilateralsFromConnectedComponents", "Currently only four peaks supported");

      const s32 maximumTemporaryPeaks = boundary.get_size() / 3; // Worst case
      const s32 numSigmaFractionalBits = 8;
      const s32 numStandardDeviations = 3;

      Result lastResult;

      const s32 boundaryLength = boundary.get_size();

      //sigma = boundaryLength/64;
      const s32 sigma = boundaryLength << (numSigmaFractionalBits-6); // SQ23.8

      // spacing about 1/4 of side-length
      //spacing = max(3, round(boundaryLength/16));
      const s32 spacing = MAX(3,  boundaryLength >> 4);

      //  stencil = [1 zeros(1, spacing-2) -2 zeros(1, spacing-2) 1];
      const s32 stencilLength = 1 + spacing - 2 + 1 + spacing - 2 + 1;
      FixedPointArray<s16> stencil(1, stencilLength, 0, scratch, Flags::Buffer(false,false,false)); // SQ15.0
      stencil.SetZero();
      *stencil.Pointer(0, 0) = 1;
      *stencil.Pointer(0, spacing-1) = -2;
      *stencil.Pointer(0, stencil.get_size(1)-1) = 1;

      //dg2 = conv(stencil, gaussian_kernel(sigma));
      FixedPointArray<s16> gaussian = ImageProcessing::Get1dGaussianKernel<s16>(sigma, numSigmaFractionalBits, numStandardDeviations, scratch); // SQ7.8
      FixedPointArray<s16> differenceOfGaussian(1, gaussian.get_size(1)+stencil.get_size(1)-1, numSigmaFractionalBits, scratch, Flags::Buffer(false,false,false)); // SQ7.8

      if((lastResult = ImageProcessing::Correlate1d<s16,s32,s16>(stencil, gaussian, differenceOfGaussian)) != RESULT_OK)
        return lastResult;

      FixedPointArray<s16> boundaryXFiltered(1, boundary.get_size(), numSigmaFractionalBits, scratch, Flags::Buffer(false,false,false)); // SQ23.8
      FixedPointArray<s16> boundaryYFiltered(1, boundary.get_size(), numSigmaFractionalBits, scratch, Flags::Buffer(false,false,false)); // SQ23.8

      if(!boundaryYFiltered.IsValid())
        return RESULT_FAIL_INVALID_OBJECT;

      //gaussian.Print("gaussian");
      //differenceOfGaussian.Print("differenceOfGaussian");

      //EndBenchmark("elp_part1");

      //BeginBenchmark("elp_part2");

      //r_smooth = imfilter(boundary, dg2(:), 'circular');
      {
        PUSH_MEMORY_STACK(scratch);
        FixedPointArray<s16> boundaryX(1, boundary.get_size(), 0, scratch, Flags::Buffer(false,false,false)); // SQ15.0

        if(!boundaryX.IsValid())
          return RESULT_FAIL_INVALID_OBJECT;

        const Point<s16> * restrict pConstBoundary = boundary.Pointer(0);
        s16 * restrict pBoundaryX = boundaryX.Pointer(0,0);

        const s32 lengthBoundary = boundary.get_size();
        for(s32 i=0; i<lengthBoundary; i++) {
          pBoundaryX[i] = pConstBoundary[i].x;
        }

        if((lastResult = ImageProcessing::Correlate1dCircularAndSameSizeOutput<s16,s32,s16>(boundaryX, differenceOfGaussian, boundaryXFiltered, scratch)) != RESULT_OK)
          return lastResult;

        //boundaryX.Print("boundaryX");
        //boundaryXFiltered.Print("boundaryXFiltered");
      } // PUSH_MEMORY_STACK(scratch);

      {
        PUSH_MEMORY_STACK(scratch);
        FixedPointArray<s16> boundaryY(1, boundary.get_size(), 0, scratch); // SQ15.0

        if(!boundaryY.IsValid())
          return RESULT_FAIL_INVALID_OBJECT;

        const Point<s16> * restrict pConstBoundary = boundary.Pointer(0);
        s16 * restrict pBoundaryY = boundaryY.Pointer(0,0);

        const s32 lengthBoundary = boundary.get_size();
        for(s32 i=0; i<lengthBoundary; i++) {
          pBoundaryY[i] = pConstBoundary[i].y;
        }

        if((lastResult = ImageProcessing::Correlate1dCircularAndSameSizeOutput<s16,s32,s16>(boundaryY, differenceOfGaussian, boundaryYFiltered, scratch)) != RESULT_OK)
          return lastResult;

        //boundaryY.Print("boundaryY");
        //boundaryYFiltered.Print("boundaryYFiltered");
      } // PUSH_MEMORY_STACK(scratch);

      //EndBenchmark("elp_part2");

      //BeginBenchmark("elp_part3");

      //r_smooth = sum(r_smooth.^2, 2);
      FixedPointArray<s32> boundaryFilteredAndCombined(1, boundary.get_size(), 2*numSigmaFractionalBits, scratch, Flags::Buffer(false,false,false)); // SQ15.16
      s32 * restrict pBoundaryFilteredAndCombined = boundaryFilteredAndCombined.Pointer(0,0);

      const s16 * restrict pConstBoundaryXFiltered = boundaryXFiltered.Pointer(0,0);
      const s16 * restrict pConstBoundaryYFiltered = boundaryYFiltered.Pointer(0,0);

      const s32 lengthBoundary = boundary.get_size();
      for(s32 i=0; i<lengthBoundary; i++) {
        //const s32 xSquared = (pConstBoundaryXFiltered[i] * pConstBoundaryXFiltered[i]) >> numSigmaFractionalBits; // SQ23.8
        //const s32 ySquared = (pConstBoundaryYFiltered[i] * pConstBoundaryYFiltered[i]) >> numSigmaFractionalBits; // SQ23.8
        const s32 xSquared = (pConstBoundaryXFiltered[i] * pConstBoundaryXFiltered[i]); // SQ31.0 (multiplied by 2^numSigmaFractionalBits)
        const s32 ySquared = (pConstBoundaryYFiltered[i] * pConstBoundaryYFiltered[i]); // SQ31.0 (multiplied by 2^numSigmaFractionalBits)

        pBoundaryFilteredAndCombined[i] = xSquared + ySquared;
      }

      FixedLengthList<s32> localMaxima(maximumTemporaryPeaks, scratch, Flags::Buffer(false,false,false));

      const s32 * restrict pConstBoundaryFilteredAndCombined = boundaryFilteredAndCombined.Pointer(0,0);

      //EndBenchmark("elp_part3");

      //BeginBenchmark("elp_part4");

      // Find local maxima -- these should correspond to the corners of the square.
      // NOTE: one of the comparisons is >= while the other is >, in order to
      // combat rare cases where we have two responses next to each other that are exactly equal.
      // localMaxima = find(r_smooth >= r_smooth([end 1:end-1]) & r_smooth > r_smooth([2:end 1]));

      if(pConstBoundaryFilteredAndCombined[0] > pConstBoundaryFilteredAndCombined[1] &&
        pConstBoundaryFilteredAndCombined[0] >= pConstBoundaryFilteredAndCombined[boundary.get_size()-1]) {
          localMaxima.PushBack(0);
      }

      for(s32 i=1; i<(lengthBoundary-1); i++) {
        if(pConstBoundaryFilteredAndCombined[i] > pConstBoundaryFilteredAndCombined[i+1] &&
          pConstBoundaryFilteredAndCombined[i] >= pConstBoundaryFilteredAndCombined[i-1]) {
            localMaxima.PushBack(i);
        }
      }

      if(pConstBoundaryFilteredAndCombined[boundary.get_size()-1] > pConstBoundaryFilteredAndCombined[0] &&
        pConstBoundaryFilteredAndCombined[boundary.get_size()-1] >= pConstBoundaryFilteredAndCombined[boundary.get_size()-2]) {
          localMaxima.PushBack(boundary.get_size()-1);
      }

      //boundaryFilteredAndCombined.Print("boundaryFilteredAndCombined");
      //for(s32 i=0; i<boundaryFilteredAndCombined.get_size(1); i++) {
      //  CoreTechPrint("%d\n", boundaryFilteredAndCombined[0][i]);
      //}

      //localMaxima.Print("localMaxima");

      //EndBenchmark("elp_part4");

      //BeginBenchmark("elp_part5");

      const Point<s16> * restrict pConstBoundary = boundary.Pointer(0);

      //localMaxima.Print("localMaxima");

      // Select the index of the top 5 local maxima (we will be keeping 4, but
      // use the ratio of the 4th to 5th as a filter for bad quads below)
      // TODO: make efficient
      s32 * restrict pLocalMaxima = localMaxima.Pointer(0);
      s32 maximaValues[5];
      s32 maximaIndexes[5];
      for(s32 i=0; i<5; i++) {
        maximaValues[i] = s32_MIN;
        maximaIndexes[i] = -1;
      }

      const s32 numLocalMaxima = localMaxima.get_size();
      for(s32 iMax=0; iMax<5; iMax++) {
        for(s32 i=0; i<numLocalMaxima; i++) {
          const s32 localMaximaIndex = pLocalMaxima[i];
          if(pConstBoundaryFilteredAndCombined[localMaximaIndex] > maximaValues[iMax]) {
            maximaValues[iMax] = pConstBoundaryFilteredAndCombined[localMaximaIndex];
            maximaIndexes[iMax] = localMaximaIndex;
          }
        }
        //CoreTechPrint("Maxima %d/%d is #%d %d\n", iMax, 4, maximaIndexes[iMax], maximaValues[iMax]);
        pBoundaryFilteredAndCombined[maximaIndexes[iMax]] = s32_MIN;
      }
      
      peaks.Clear();
      
      // Ratio of last of the 4 peaks to the next (fifth) peak should be fairly large
      // if there are 4 distinct corners.
      //printf("Ratio of 4th to 5th local maxima: %f\n", ((f32)maximaValues[3])/((f32)maximaValues[4]));
      if(maximaValues[3] < minPeakRatio * maximaValues[4]) {
        // Just return no peaks, which will skip this quad
        return RESULT_OK;
      }

      // Copy the maxima to the output peaks, ordered by their original index order, so they are
      // still in clockwise or counterclockwise order

      bool whichUsed[4];
      for(s32 i=0; i<4; i++) {
        whichUsed[i] = false;
      }

      for(s32 iMax=0; iMax<4; iMax++) {
        s32 curMinIndex = -1;
        for(s32 i=0; i<4; i++) {
          if(!whichUsed[i] && maximaIndexes[i] >= 0) {
            if(curMinIndex == -1 || maximaIndexes[curMinIndex] > maximaIndexes[i]) {
              curMinIndex = i;
            }
          }
        }

        //if(maximaIndexes[iMax] >= 0) {
        if(curMinIndex >= 0) {
          whichUsed[curMinIndex] = true;
          peaks.PushBack(pConstBoundary[maximaIndexes[curMinIndex]]);
        }
      }

      //EndBenchmark("elp_part5");

      //boundary.Print();
      //peaks.Print();

      return RESULT_OK;
    } // Result ExtractLaplacianPeaks(const FixedLengthList<Point<s16> > &boundary, MemoryStack scratch)
  } // namespace Embedded
} // namespace Anki
