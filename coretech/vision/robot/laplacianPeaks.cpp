/**
File: laplacianPeaks.cpp
Author: Peter Barnum
Created: 2013

Copyright Anki, Inc. 2013
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "coretech/vision/robot/fiducialDetection.h"
#include "coretech/vision/robot/imageProcessing.h"

#include "coretech/common/robot/benchmarking.h"

#define DRAW_LINE_FITS 0

#if DRAW_LINE_FITS
#  include "coretech/vision/engine/image_impl.h"
#endif

#define LINE_FIT_METHOD_KMEANS 0 // Works, but needs profiling to see if termination parameters (and initialization) are good and fast enough
#define LINE_FIT_METHOD_HIST   1 // Has issues with certain edge cases (b/c of fixed bin centers?)
#define LINE_FIT_METHOD_RANSAC 2 // Sampling scheme needs work
#define LINE_FIT_METHOD LINE_FIT_METHOD_KMEANS

namespace Anki
{
  namespace Embedded
  {
#   if DRAW_LINE_FITS
    static const ColorRGBA colorList[5] = {
      NamedColors::GREEN, NamedColors::ORANGE, NamedColors::WHITE, NamedColors::CYAN, NamedColors::DARKGRAY
    };
#   endif
    
    struct LineFits
    {
      f32  slope;
      f32  intercept;
      bool switched;
    };
    
    Result ExtractLineFitsPeaks_Ransac(const FixedLengthList<Point<s16> > &boundary, FixedLengthList<Point<s16> > &peaks, const s32 imageHeight, const s32 imageWidth)
    {
#     if DRAW_LINE_FITS
      Vision::ImageRGB blankImg(imageHeight, imageWidth);
      blankImg.FillWith(Vision::PixelRGB(0,0,0));
#     endif
      
      // Look for four lines that each account for at least inlierFraction of the boundary
      const s32 boundaryLength = boundary.get_size();
      const f32 inlierFraction = 0.125f;
      const s32 numInliersRequired = std::round(inlierFraction * boundaryLength);
      const f32 inlierDistThresh = 1.5f;
      
      LineFits lineFits[4];
      s32 numLinesFit = 0;
      const Point<s16> * restrict pBoundary = boundary.Pointer(0);
      
      std::vector<u8> used;
      used.resize(boundaryLength, 0);
      
      for(s32 iStart = 0; numLinesFit < 4 && iStart < boundaryLength; iStart+=3)
      {
        if(used[iStart]) {
          continue;
        }
        
        s32 dir = 1;
        for(s32 iSampleDist=boundaryLength/4; numLinesFit < 4 && iSampleDist >=2; iSampleDist -= (dir == 1 ? 0 : 3), dir = -dir)
        {
          if(iSampleDist >= boundaryLength) {
            continue;
          }
          
          // Pick a second point
          s32 iEnd = iStart + dir*iSampleDist;
          if(iEnd < 0) {
            iEnd += boundaryLength;
          } else if(iEnd >= boundaryLength) {
            iEnd -= boundaryLength;
          }
          
          assert(iEnd >= 0 && iEnd < boundaryLength);
          
          if(used[iEnd]) {
            continue;
          }
          //CoreTechPrint("RansacLineFit: iStart=%d, iEnd=%d, sampleDist=%d, length=%d\n", iStart, iEnd, iSampleDist, boundaryLength);
          
          // Compute the line
          const s32 dx = pBoundary[iEnd].x - pBoundary[iStart].x;
          const s32 dy = pBoundary[iEnd].y - pBoundary[iStart].y;
          if(dx == 0) {
            assert(numLinesFit >= 0 && numLinesFit < 4);
            lineFits[numLinesFit].switched = true;
            lineFits[numLinesFit].slope = (f32)dx / (f32)dy;
            lineFits[numLinesFit].intercept = pBoundary[iStart].x - lineFits[numLinesFit].slope*pBoundary[iStart].y;
          } else {
            assert(numLinesFit >= 0 && numLinesFit < 4);
            lineFits[numLinesFit].switched = false;
            lineFits[numLinesFit].slope = (f32)dy / (f32)dx;
            lineFits[numLinesFit].intercept = pBoundary[iStart].y - lineFits[numLinesFit].slope*pBoundary[iStart].x;
          }
          
          // Count number of inliers
          std::vector<s32> inlierIndex;
          cv::Point2f lineVec(dx,dy);
          for(s32 iPt=0; iPt<boundaryLength; ++iPt) {
            if(!used[iPt])
            {
              cv::Point2f pointVec(pBoundary[iPt].x - pBoundary[iStart].x, pBoundary[iPt].y - pBoundary[iStart].y);
              
              const f32 area = pointVec.cross(lineVec);
              const f32 dist = std::abs(area / cv::norm(lineVec));
              if(dist < inlierDistThresh) {
                inlierIndex.push_back(iPt);
              }
            }
          }
          
          if(inlierIndex.size() >= numInliersRequired) {
            //CoreTechPrint("RansacLineFit: Found %lu inliers (> %d) for line %d\n", inlierIndex.size(), numInliersRequired, numLinesFit);
            
            cv::Mat_<f32> A(2,2);
            A.setTo(0.f);
            
            cv::Mat_<f32> b(2,1);
            b.setTo(0.f);
            
            // Mark all inliers as used and do final line fits using all the inlier data
            for(auto index : inlierIndex)
            {
              assert(index>=0 && index<used.size());
              used[index] = (u8)true;
#             if DRAW_LINE_FITS
              blankImg.DrawLine(Point2f(pBoundary[index].x, pBoundary[index].y), Point2f(pBoundary[index].x, pBoundary[index].y),colorList[numLinesFit], 2);
#             endif
              
              if(lineFits[numLinesFit].switched) {
                A(0,0) += (pBoundary[index].y * pBoundary[index].y);
                A(0,1) += pBoundary[index].y;
                A(1,1) += 1.f;
                b(0,0) += (pBoundary[index].y * pBoundary[index].x);
                b(1,0) += pBoundary[index].x;
              } else {
                A(0,0) += (pBoundary[index].x * pBoundary[index].x);
                A(0,1) += pBoundary[index].x;
                A(1,1) += 1.f;
                b(0,0) += (pBoundary[index].x * pBoundary[index].y);
                b(1,0) += pBoundary[index].y;
              }
            }
            
            cv::Mat_<f32> p;
            A(1,0) = A(0,1);
            cv::solve(A, b, p);
            assert(p.rows == 2 && p.cols == 1);
            assert(numLinesFit >= 0 && numLinesFit < 4);
            lineFits[numLinesFit].slope = p(0,0);
            lineFits[numLinesFit].intercept = p(1,0);
            
//            CoreTechPrint("RansacLineFit: Fit %sline %d from inliers. Slope=%f, Int=%f\n",
//                          lineFits[numLinesFit].switched ? "switched " : "", numLinesFit,
//                          lineFits[numLinesFit].slope, lineFits[numLinesFit].intercept);
            
#           if DRAW_LINE_FITS
            {
              if(lineFits[numLinesFit].switched) {
                blankImg.DrawLine(Point2f(lineFits[numLinesFit].intercept, 0),
                                  Point2f(lineFits[numLinesFit].slope*(imageHeight-1) + lineFits[numLinesFit].intercept, imageHeight-1),
                                  NamedColors::MAGENTA, 1);
              } else {
                blankImg.DrawLine(Point2f(0, lineFits[numLinesFit].intercept),
                                  Point2f(imageWidth-1, lineFits[numLinesFit].slope*(imageWidth-1) + lineFits[numLinesFit].intercept),
                                  NamedColors::RED, 1);
              }
            }
#           endif
            
            //CoreTechPrint("RansacLineFit: incrementing numLinesFit from %d to %d\n",
            //              numLinesFit, numLinesFit+1);
            ++numLinesFit;
          }
        }
      }
      
      std::vector<cv::Point_<f32> > corners;
      
      if(numLinesFit == 4)
      {
        //CoreTechPrint("RansacLineFit: Found 4 Lines\n");
        
        for(s32 iLine=0; iLine<4; iLine++) {
          for(s32 jLine=iLine+1; jLine<4; jLine++) {
            f32 xInt = 0;
            f32 yInt = 0;
            assert(iLine>=0 && iLine<4);
            assert(jLine>=0 && jLine<4);
            
            if(lineFits[iLine].switched == lineFits[jLine].switched) {
              xInt = (lineFits[jLine].intercept - lineFits[iLine].intercept) / (lineFits[iLine].slope - lineFits[jLine].slope);
              yInt = lineFits[iLine].slope*xInt + lineFits[iLine].intercept;
              
              if(lineFits[iLine].switched == true) {
                std::swap(xInt, yInt);
              }
            } else if(lineFits[iLine].switched == true && lineFits[jLine].switched == false) {
              yInt = (lineFits[jLine].slope * lineFits[iLine].intercept + lineFits[jLine].intercept) / (1.f - lineFits[iLine].slope*lineFits[jLine].slope);
              xInt = lineFits[iLine].slope * yInt + lineFits[iLine].intercept;
            } else if(lineFits[iLine].switched == false && lineFits[jLine].switched == true) {
              yInt = (lineFits[iLine].slope * lineFits[jLine].intercept + lineFits[iLine].intercept)/ (1.f - lineFits[iLine].slope*lineFits[jLine].slope);
              xInt = lineFits[jLine].slope * yInt + lineFits[jLine].intercept;
            }
            
            if(xInt >= 0 && xInt < imageWidth && yInt >= 0 && yInt < imageHeight) {
              //CoreTechPrint("RansacLineFit: Found corner at (%.1f,%.1f) using lines %d and %d\n", xInt, yInt, iLine, jLine);
              corners.push_back(cv::Point_<f32>(xInt, yInt));
              
#             if DRAW_LINE_FITS
              blankImg.DrawLine(Point2f(xInt,yInt), Point2f(xInt,yInt), NamedColors::BLUE, 3);
#             endif
            }
          } // for(s32 jLine=0; jLine<4; jLine++)
        } // for(s32 iLine=0; iLine<4; iLine++)
      } // if(didFitFourLines)
      
      if(corners.size() == 4) {
//        CoreTechPrint("RansacLineFit: populating peaks\n");
        
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
        
        // NOTE: The output is not reordered like the matlab version, because the next step
        //       after ExtractLaplacianPeaks() or ExtractLineFitsPeaks() expects sorted corners.
      } else {
        corners.clear();
        
        for(s32 i=0; i<4; i++) {
          peaks[i] = Point<s16>(0, 0);
        }
        
        peaks.Clear();
      }
      
#     if DRAW_LINE_FITS
      blankImg.Display("LineFits");
#     endif
      
      return RESULT_OK;
      
    } // ExtractLineFitsPeaks_Ransac()
    
    Result ExtractLineFitsPeaks(const FixedLengthList<Point<s16>> &boundary, 
                                FixedLengthList<Point<s16>> &peaks, 
                                const s32 imageHeight, 
                                const s32 imageWidth, 
                                MemoryStack scratch)
    {
      AnkiConditionalErrorAndReturnValue(boundary.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "boundary is not valid");

      AnkiConditionalErrorAndReturnValue(peaks.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "peaks is not valid");

      AnkiConditionalErrorAndReturnValue(scratch.IsValid(),
        RESULT_FAIL_INVALID_OBJECT, "ComputeQuadrilateralsFromConnectedComponents", "scratch is not valid");

      AnkiConditionalErrorAndReturnValue(peaks.get_maximumSize() == 4,
          RESULT_FAIL_INVALID_PARAMETER, "ComputeQuadrilateralsFromConnectedComponents", "Currently only four peaks supported");

#     if LINE_FIT_METHOD == LINE_FIT_METHOD_RANSAC
      return ExtractLineFitsPeaks_Ransac(boundary, peaks, imageHeight, imageWidth);
#     endif
      
      const s32 boundaryLength = boundary.get_size();
        
      const f32 sigma = static_cast<f32>(boundaryLength) / 64.0f;

#     if DRAW_LINE_FITS
      Vision::ImageRGB blankImg(imageHeight, imageWidth);
      blankImg.FillWith(Vision::PixelRGB(0,0,0));
#     endif
      
      // Center point of the boundary (average of all pixel coordinates making up the boundary)
      Point<u32> boundaryCenter(0,0);
      
      // Copy the boundary to a f32 openCV array
      cv::Mat_<f32> boundaryCv(2, boundaryLength);
      const Point<s16> * restrict pBoundary = boundary.Pointer(0);
      f32 * restrict pBoundaryCv0 = boundaryCv.ptr<f32>(0,0);
      f32 * restrict pBoundaryCv1 = boundaryCv.ptr<f32>(1,0);
      for(s32 i=0; i<boundaryLength; i++) {
        const s16 x = pBoundary[i].x;
        const s16 y = pBoundary[i].y;
        pBoundaryCv0[i] = y; // Note that order is (y,x)
        pBoundaryCv1[i] = x;
        boundaryCenter.x += x;
        boundaryCenter.y += y;
      }
      boundaryCenter.x /= boundaryLength;
      boundaryCenter.y /= boundaryLength;
      
      // Create a DoG filter kernel
      s32 ksize = static_cast<s32>(ceilf((((sigma - 0.8f)/0.3f) + 1.0f)*2.0f + 1.0f) ); // Equation from opencv docs
      ksize = 2*(ksize/2) + 1;

      cv::Mat gk = cv::getGaussianKernel(ksize, sigma, CV_32F).t();
      
      cv::Mat_<f32> dxKernel(1,3);
      dxKernel.at<f32>(0,0) = -0.5f; dxKernel.at<f32>(0,1) = 0; dxKernel.at<f32>(0,2) = 0.5f;

      cv::Mat_<f32> dg;
      
      cv::filter2D(gk, dg, CV_32F, dxKernel, cv::Point(-1,-1), 0, cv::BORDER_DEFAULT);
      
      cv::Mat_<f32> dB(2, boundaryLength);
      
      // Circular convolution
      // TODO: may be slow? If/when openCv circular convolution is fixed, switch to that
      // cv::filter2D(dB, dB, -1, dg, cv::Point(-1,-1), 0, cv::BORDER_WRAP); // assertion failure: not supported?
      {
        const f32 * restrict pDg = dg.ptr<f32>();
        
        const f32 * restrict pBoundary0 = boundaryCv.ptr<f32>(0,0);
        const f32 * restrict pBoundary1 = boundaryCv.ptr<f32>(1,0);
        
        f32 * restrict pDB0 = dB.ptr<f32>(0,0);
        f32 * restrict pDB1 = dB.ptr<f32>(1,0);
        
        const s32 ksize2 = ksize / 2;
        
        for(s32 xBoundary=0; xBoundary<boundaryLength; xBoundary++) {
        
          f64 curSum0 = 0;
          f64 curSum1 = 0;
          for(s32 xFilter=0; xFilter<ksize; xFilter++) {
            const s32 dx = xFilter - ksize2;
            
            s32 xBoundaryOffset = xBoundary + dx;
            if(xBoundaryOffset >= boundaryLength) {
              xBoundaryOffset -= boundaryLength;
            } else if(xBoundaryOffset < 0) {
              xBoundaryOffset += boundaryLength;
            }
            
            curSum0 += pBoundary0[xBoundaryOffset] * pDg[xFilter];
            curSum1 += pBoundary1[xBoundaryOffset] * pDg[xFilter];
          }
          
          pDB0[xBoundary] = curSum0;
          pDB1[xBoundary] = curSum1;
       
#         if LINE_FIT_METHOD == LINE_FIT_METHOD_KMEANS
          // Store resulting derivatives as a unit vector, since we are doing
          // k-means clustering on unit vectors on the unit circle instead of
          // clustering angle values directly
          if(pDB0[xBoundary] != 0 || pDB1[xBoundary] != 0) {
            const f32 invLength = 1.f/std::sqrt(pDB0[xBoundary]*pDB0[xBoundary] +
                                                pDB1[xBoundary]*pDB1[xBoundary]);
            pDB0[xBoundary] *= invLength;
            pDB1[xBoundary] *= invLength;
          }
#         endif
        }
      }
      
      LineFits lineFits[4];
      bool didFitFourLines = true;
      
#     if LINE_FIT_METHOD == LINE_FIT_METHOD_HIST
      cv::Mat_<f32> bin(1, boundaryLength);
      
      const f32 * restrict pDB0 = dB.ptr<f32>(0,0);
      const f32 * restrict pDB1 = dB.ptr<f32>(1,0);
      f32 * restrict pBin = bin.ptr<f32>(0,0);
      for(s32 i=0; i<boundaryLength; i++) {
        pBin[i] = atan2f(pDB1[i], pDB0[i]);
      }
      
      // Compute the histogram of orientations, and find the top 4 bins
      // NOTE: this histogram is a bit different than matlab's
      s32 histSize = 16;
      f32 range[] = {-M_PI_F, M_PI_F + 1e-6f};
      const f32 *ranges[] = { range };
      
      cv::Mat hist;
      cv::calcHist(&bin, 1, 0, cv::Mat(), hist, 1, &histSize, ranges, true, false);
      
      cv::Mat maxBins;
      cv::sortIdx(hist, maxBins, CV_SORT_EVERY_COLUMN | CV_SORT_DESCENDING);
      
      // Compute bin centers
      std::vector<Radians> binCenters(histSize);
      for(s32 iBin=0; iBin<histSize; ++iBin) {
        binCenters[iBin] = (2.f*M_PI_F / (f32)histSize) * ((f32)iBin + 0.5f) - M_PI_F;
      }
      
#     elif LINE_FIT_METHOD == LINE_FIT_METHOD_KMEANS

      const s32 numTries = 1;
      cv::TermCriteria criteria(cv::TermCriteria::COUNT+cv::TermCriteria::EPS, 15, .1);
      cv::Mat labels(boundaryLength,1,CV_32SC1);
      s32* restrict pLabels = labels.ptr<s32>(0,0);
      s32 i;
      for(i=0; i<boundaryLength/4; ++i) {
        pLabels[i] = 0;
      }
      for( ; i<boundaryLength/2; ++i) {
        pLabels[i] = 1;
      }
      for( ; i<boundaryLength*3/4; ++i) {
        pLabels[i] = 2;
      }
      for( ; i<boundaryLength; ++i) {
        pLabels[i] = 3;
      }
      cv::Mat_<f32> centers;
      dB = dB.t();
      cv::kmeans(dB, 4, labels, criteria, numTries,
                 cv::KMEANS_USE_INITIAL_LABELS, // | cv::KMEANS_PP_CENTERS,
                 centers);

      const f32 dotProdThresh = std::cos(DEG_TO_RAD(25));
      for(s32 i=0; i<boundaryLength; ++i) {
        // Remove from consideration those points which are not near enough to
        // the centers (expecting some "outliers" on the rounded corners and we
        // don't want those to affect the final line fits below)
        const f32 dotProd = dB.row(i).dot(centers.row(pLabels[i]));
        if(dotProd < dotProdThresh)
        {
          pLabels[i] = -1;
        }
      }
#     elif LINE_FIT_METHOD == LINE_FIT_METHOD_RANSAC
      
      // Nothing to do (returned result above)
      
#     else
#     error Unknown LINE_FIT_METHOD
#     endif
      
      for(s32 iBin=0; iBin<4; iBin++) {
        // boundaryIndex = find(bin==maxBins(iBin));
        std::vector<s32> boundaryIndex;
        
#       if LINE_FIT_METHOD == LINE_FIT_METHOD_KMEANS
        for(s32 i=0; i<boundaryLength; i++) {
          if(pLabels[i] == iBin) {
            boundaryIndex.push_back(i);
#           if DRAW_LINE_FITS
            blankImg.DrawLine(Point2f(pBoundary[i].x, pBoundary[i].y), Point2f(pBoundary[i].x, pBoundary[i].y),colorList[iBin], 2);
#           endif
          } else if(pLabels[i] == -1) {
#           if DRAW_LINE_FITS
            // Show ignored points in dark gray
            blankImg.DrawLine(Point2f(pBoundary[i].x, pBoundary[i].y), Point2f(pBoundary[i].x, pBoundary[i].y),colorList[4], 2);
            
#           endif
          }
        }
        
#       elif LINE_FIT_METHOD == LINE_FIT_METHOD_HIST
        // WARNING: May have some corner cases
        const s32 thisBin  = maxBins.at<s32>(iBin,0);
        
        // Assign each boundary sample to close bin centers (note: can contribute to
        // multiple edges)
        for(s32 i=0; i<boundaryLength; i++) {
          const f32 angDist = (Radians(pBin[i])-binCenters[thisBin]).getAbsoluteVal().ToFloat();
          if(angDist < (2*M_PI_F)/histSize)
          {
            boundaryIndex.push_back(i);
            
#           if DRAW_LINE_FITS
            blankImg.DrawPoint(Point2f(pBoundary[i].x, pBoundary[i].y), colorList[iBin], 2);
#           endif

          }
        }
#       if DRAW_LINE_FITS
        blankImg.DrawText(Point2f(0,imageHeight/2+iBin*15), std::to_string(binCenters[thisBin].getDegrees()), colorList[iBin], 0.5);
#       endif
#       endif // LINE_FIT_METHOD
        
        const s32 numSide = static_cast<s32>(boundaryIndex.size());
        
        // Solve all the line fit equations in slope-intercept format, choosing
        // the most numerical stable by picking whether we have more variation in
        // X or Y direciton. "isVertical" indiciates that we're solving x = my +b
        // instead of the more standard y = mx + b
        if(numSide > 1) {
          // all(boundary(boundaryIndex,2)==boundary(boundaryIndex(1),2))
          s32 minX = std::numeric_limits<s32>::max(), maxX = std::numeric_limits<s32>::min(), minY = std::numeric_limits<s32>::max(), maxY = std::numeric_limits<s32>::min();
          for(s32 i=0; i<numSide; ++i) {
            const s32 curX = pBoundary[boundaryIndex[i]].x;
            const s32 curY = pBoundary[boundaryIndex[i]].y;
            if(curX < minX) {
              minX = curX;
            }
            if(curY < minY) {
              minY = curY;
            }
            if(curX > maxX) {
              maxX = curX;
            }
            if(curY > maxY) {
              maxY = curY;
            }
          }
          
          lineFits[iBin].switched = (maxX - minX) < (maxY - minY);
          
          // Build and solve system of equations for the line fit
          // A = [boundary(boundaryIndex,2) ones(numSide,1)];
          // p{iBin} = A \ boundary(boundaryIndex,1);
          
          const s32 nrows = static_cast<s32>(boundaryIndex.size());
          cv::Mat_<f32> A(nrows, 2);
          cv::Mat_<f32> b(nrows, 1);
          cv::Mat_<f32> x(2, 1);
          
          if(lineFits[iBin].switched) {
            for(s32 i=0; i<boundaryIndex.size(); i++) {
              A.at<f32>(i,0) = boundary.Pointer(boundaryIndex[i])->y;
              A.at<f32>(i,1) = 1;
              b.at<f32>(i,0) = boundary.Pointer(boundaryIndex[i])->x;
            }
          } else {
            for(s32 i=0; i<boundaryIndex.size(); i++) {
              A.at<f32>(i,0) = boundary.Pointer(boundaryIndex[i])->x;
              A.at<f32>(i,1) = 1;
              b.at<f32>(i,0) = boundary.Pointer(boundaryIndex[i])->y;
            }
          }
          
          // TODO: for speed, switch to QR with Gram
          cv::solve(A, b, x, cv::DECOMP_SVD);
          
          lineFits[iBin].slope = x.at<f32>(0,0);
          lineFits[iBin].intercept = x.at<f32>(1,0);
          
#         if DRAW_LINE_FITS
          {
            if(lineFits[iBin].switched) {
              blankImg.DrawLine(Point2f(lineFits[iBin].intercept, 0),
                                Point2f(lineFits[iBin].slope*(imageHeight-1) + lineFits[iBin].intercept, imageHeight-1),
                                NamedColors::MAGENTA, 1);
            } else {
              blankImg.DrawLine(Point2f(0, lineFits[iBin].intercept),
                                Point2f(imageWidth-1, lineFits[iBin].slope*(imageWidth-1) + lineFits[iBin].intercept),
                                NamedColors::RED, 1);
            }
          }
#         endif
          
        } else {
          didFitFourLines = false;
          break;
        } // if(numSide > 1) ... else
      } // for(s32 iBin=0; iBin<4; iBin++)
      
#if DRAW_LINE_FITS
      blankImg.DrawLine(Point2f(boundaryCenter.x, boundaryCenter.y), Point2f(boundaryCenter.x, boundaryCenter.y), NamedColors::CYAN, 3);
#endif
      
      // Vector of the corners of the boundary as well as their squared distance from the center of the boundary
      struct Corner {
        cv::Point2f point;
        u32         distToCenterSq;
      };
      std::vector<Corner> corners;
      
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
              yInt = (lineFits[jLine].slope * lineFits[iLine].intercept + lineFits[jLine].intercept) / (1.f - lineFits[iLine].slope*lineFits[jLine].slope);
              xInt = lineFits[iLine].slope * yInt + lineFits[iLine].intercept;
            } else if(lineFits[iLine].switched == false && lineFits[jLine].switched == true) {
              yInt = (lineFits[iLine].slope * lineFits[jLine].intercept + lineFits[iLine].intercept)/ (1.f - lineFits[iLine].slope*lineFits[jLine].slope);
              xInt = lineFits[jLine].slope * yInt + lineFits[jLine].intercept;
            }
            
            if(xInt >= 0 && xInt < imageWidth && yInt >= 0 && yInt < imageHeight) {
              const u32 distToCenter = std::pow(xInt - boundaryCenter.x, 2) + std::pow(yInt - boundaryCenter.y, 2);
              corners.push_back({cv::Point_<f32>(xInt, yInt), distToCenter});
              
#             if DRAW_LINE_FITS
              blankImg.DrawLine(Point2f(xInt,yInt), Point2f(xInt,yInt), NamedColors::BLUE, 3);
#             endif
            }
          } // for(s32 jLine=0; jLine<4; jLine++)
        } // for(s32 iLine=0; iLine<4; iLine++)
      } // if(didFitFourLines)
      
      // Sort corners by distance to boundary center
      const size_t numToSort = (corners.size() < 4 ? corners.size() : 4);
      std::partial_sort(corners.begin(), corners.begin() + numToSort, corners.end(), [](Corner i, Corner j) {
        return (i.distToCenterSq < j.distToCenterSq);
      });
      
      if(corners.size() >= 4) {
        // Use the four closest corners to the boundary center
        Quadrilateral<f32> quad(
          Point<f32>(corners[0].point.x, corners[0].point.y),
          Point<f32>(corners[1].point.x, corners[1].point.y),
          Point<f32>(corners[2].point.x, corners[2].point.y),
          Point<f32>(corners[3].point.x, corners[3].point.y));
        
        Quadrilateral<f32> sortedQuad = quad.ComputeClockwiseCorners<f32>();
        
        peaks.set_size(4);
        
        for(s32 i=0; i<4; i++) {
          peaks[i] = Point<s16>(saturate_cast<s16>(sortedQuad.corners[i].x), saturate_cast<s16>(sortedQuad.corners[i].y));
        }
        
        // NOTE: The output is not reordered like the matlab version, because the next step
        //       after ExtractLaplacianPeaks() or ExtractLineFitsPeaks() expects sorted corners.
      } else {
        corners.clear();
        
        for(s32 i=0; i<4; i++) {
          peaks[i] = Point<s16>(0, 0);
        }
        
        peaks.Clear();
      }
      
#     if DRAW_LINE_FITS
      blankImg.Display("LineFits");
#     endif
      
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
        maximaValues[i] = std::numeric_limits<s32>::min();
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
        pBoundaryFilteredAndCombined[maximaIndexes[iMax]] = std::numeric_limits<s32>::min();
      }
      
      peaks.Clear();
      
      // Ratio of last of the 4 peaks to the next (fifth) peak should be fairly large
      // if there are 4 distinct corners.
      //CoreTechPrint("Ratio of 4th to 5th local maxima: %f\n", ((f32)maximaValues[3])/((f32)maximaValues[4]));
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
