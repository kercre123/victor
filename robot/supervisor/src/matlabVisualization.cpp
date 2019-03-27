/**
 * File: matlabVisualization.cpp
 *
 * Author: Andrew Stein
 * Date:   3/28/2014
 *
 * Description: Code for visualizing the VisionSystem's tracking and detection
 *              status using a Matlab Engine.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "matlabVisualization.h"

#include "anki/common/robot/matlabInterface.h"

#if defined(SIMULATOR) && ANKICORETECH_EMBEDDED_USE_MATLAB
#define USE_MATLAB_VISUALIZATION 0
#else
#define USE_MATLAB_VISUALIZATION 0
#endif

#if USE_MATLAB_VISUALIZATION
#include "matlabVisualization.h"
#endif

namespace Anki {
  
  using namespace Embedded;
  
  namespace Vector {
    
    namespace MatlabVisualization {

#if USE_MATLAB_VISUALIZATION      
      static Matlab matlabViz_;     
      static bool beforeCalled_;
      static const bool SHOW_TRACKER_PREDICTION = false;
      static const bool saveTrackingResults_ = false;
#endif      
      
      Result Initialize()
      {
#if USE_MATLAB_VISUALIZATION
        //matlabViz_ = Matlab(false);
        matlabViz_.EvalStringEcho("h_fig  = figure('Name', 'VisionSystem'); "
                                  "h_axes = axes('Pos', [.1 .1 .8 .8], 'Parent', h_fig); "
                                  "h_img  = imagesc(0, 'Parent', h_axes); "
                                  "axis(h_axes, 'image', 'off'); "
                                  "hold(h_axes, 'on'); "
                                  "colormap(h_fig, gray); "
                                  "h_trackedQuad = plot(nan, nan, 'b', 'LineWidth', 2, "
                                  "                     'Parent', h_axes); "
                                  "imageCtr = 0; ");
        
        if(SHOW_TRACKER_PREDICTION) {
          matlabViz_.EvalStringEcho("h_fig_tform = figure('Name', 'TransformAdjust'); "
                                    "colormap(h_fig_tform, gray);");
        }
        
        beforeCalled_ = false;
#endif
        return RESULT_OK;
      }
      
      Result ResetFiducialDetection(const Array<u8>& image)
      {
#if USE_MATLAB_VISUALIZATION
        matlabViz_.EvalStringEcho("delete(findobj(h_axes, 'Tag', 'DetectedQuad'));");
        matlabViz_.PutArray(image, "detectionImage");
        matlabViz_.EvalStringEcho("set(h_img, 'CData', detectionImage); "
                                  "set(h_axes, 'XLim', [.5 size(detectionImage,2)+.5], "
                                  "            'YLim', [.5 size(detectionImage,1)+.5]);");
#endif
        return RESULT_OK;
      }
      
      Result SendFiducialDetection(const Quadrilateral<f32> &corners,
                                   const Vision::MarkerType &markerCode )
      {
#if USE_MATLAB_VISUALIZATION
        matlabViz_.PutQuad(corners, "detectedQuad");
        matlabViz_.EvalStringEcho("detectedQuad = double(detectedQuad); "
                                  "plot(detectedQuad([1 2 4 3 1],1)+1, "
                                  "     detectedQuad([1 2 4 3 1],2)+1, "
                                  "     'r', 'LineWidth', 2, "
                                  "     'Parent', h_axes, "
                                  "     'Tag', 'DetectedQuad'); "
                                  "plot(detectedQuad([1 3],1)+1, "
                                  "     detectedQuad([1 3],2)+1, "
                                  "     'g', 'LineWidth', 2, "
                                  "     'Parent', h_axes, "
                                  "     'Tag', 'DetectedQuad'); "
                                  "text(mean(detectedQuad(:,1))+1, "
                                  "     mean(detectedQuad(:,2))+1, "
                                  "     '%s', 'Hor', 'c', 'Color', 'y', "
                                  "     'FontSize', 16, 'FontWeight', 'b', "
                                  "     'Interpreter', 'none', "
                                  "     'Tag', 'DetectedQuad');",
                                  Vision::MarkerTypeStrings[markerCode]);
#endif
        return RESULT_OK;
      }
      
      Result SendDrawNow()
      {
#if USE_MATLAB_VISUALIZATION
        matlabViz_.EvalString("drawnow");
#endif
        return RESULT_OK;
      }
      
      Result SendTrackInit(const Array<u8> &image, const Quadrilateral<f32>& quad)
      {
#if USE_MATLAB_VISUALIZATION
        ResetFiducialDetection(image);
        
        matlabViz_.PutQuad(quad, "templateQuad");
        
        matlabViz_.EvalStringEcho("h_template = axes('Pos', [0 0 .33 .33], 'Tag', 'TemplateAxes'); "
                                  "imagesc(detectionImage, 'Parent', h_template); hold on; "
                                  "plot(templateQuad([1 2 4 3 1],1), "
                                  "     templateQuad([1 2 4 3 1],2), 'r', "
                                  "     'LineWidth', 2, "
                                  "     'Parent', h_template); "
                                  "set(h_template, 'XLim', [0.9*min(templateQuad(:,1)) 1.1*max(templateQuad(:,1))], "
                                  "                'YLim', [0.9*min(templateQuad(:,2)) 1.1*max(templateQuad(:,2))]);");
        
        if(saveTrackingResults_) {
#if USE_MATLAB_TRACKER
          const char* fnameStr1 = "matlab";
#else // not matlab tracker:
          const char* fnameStr1 = "embedded";
#endif
#if DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_AFFINE
          const char* fnameStr2 = "affine";
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_PROJECTIVE
          const char* fnameStr2 = "projective";
#elif DOCKING_ALGORITHM == DOCKING_LUCAS_KANADE_SAMPLED_PROJECTIVE
          const char* fnameStr2 = "sampledProjective";
#else
          const char* fnameStr2 = "unknown";
#endif
          
          matlabViz_.EvalStringEcho("saveDir = fullfile('~', 'temp', '%s', '%s'); "
                                    "if ~isdir(saveDir), mkdir(saveDir); end, "
                                    "fid = fopen(fullfile(saveDir, 'quads.txt'), 'wt'); saveCtr = 0; "
                                    "fileCloser = onCleanup(@()fclose(fid)); "
                                    "imwrite(detectionImage, fullfile(saveDir, sprintf('image_%%.5d.png', saveCtr))); "
                                    "fprintf(fid, '[%%d] (%%f,%%f) (%%f,%%f) (%%f,%%f) (%%f,%%f)\\n', "
                                    "        saveCtr, "
                                    "        templateQuad(1,1), templateQuad(1,2), "
                                    "        templateQuad(2,1), templateQuad(2,2), "
                                    "        templateQuad(3,1), templateQuad(3,2), "
                                    "        templateQuad(4,1), templateQuad(4,2));",
                                    fnameStr1, fnameStr2);
        }
#endif // #if USE_MATLAB_VISUALIZATION
        return RESULT_OK;
      }
      
      /*
      Result SendTrackInit(const Array<u8> &image,
                               const VisionSystem::Tracker& tracker,
                               MemoryStack scratch)
      {
        return SendTrackInit(image, tracker.get_transformation().get_transformedCorners(scratch));
      }
      */
      
      Result SendTrack(const Array<u8>& image,
                           const Quadrilateral<f32>& quad,
                           const bool converged)
      {
#if USE_MATLAB_VISUALIZATION
        matlabViz_.PutArray(image, "trackingImage");
        matlabViz_.PutQuad(quad, "transformedQuad");
        
        //            matlabViz_.EvalStringExplicit("imwrite(trackingImage, "
        //                                          "sprintf('~/temp/trackingImage%.3d.png', imageCtr)); "
        //                                          "imageCtr = imageCtr + 1;");
        matlabViz_.EvalStringEcho("set(h_img, 'CData', trackingImage); "
                                  "set(h_axes, 'XLim', [.5 size(trackingImage,2)+.5], "
                                  "            'YLim', [.5 size(trackingImage,1)+.5]);"
                                  "set(h_trackedQuad, 'Visible', 'on', "
                                  "            'XData', transformedQuad([1 2 4 3 1],1)+1, "
                                  "            'YData', transformedQuad([1 2 4 3 1],2)+1); ");
        
        if(saveTrackingResults_) {
          matlabViz_.EvalStringEcho("saveCtr = saveCtr + 1; "
                                    "imwrite(trackingImage, fullfile(saveDir, sprintf('image_%%.5d.png', saveCtr))); "
                                    "fprintf(fid, '[%%d] (%%f,%%f) (%%f,%%f) (%%f,%%f) (%%f,%%f)\\n', "
                                    "        saveCtr, "
                                    "        transformedQuad(1,1), transformedQuad(1,2), "
                                    "        transformedQuad(2,1), transformedQuad(2,2), "
                                    "        transformedQuad(3,1), transformedQuad(3,2), "
                                    "        transformedQuad(4,1), transformedQuad(4,2));");
        }
        
        if(converged)
        {
          matlabViz_.EvalStringEcho("title(h_axes, 'Tracking Succeeded', 'FontSize', 16);");
        } else  {
          matlabViz_.EvalStringEcho( //"set(h_trackedQuad, 'Visible', 'off'); "
                                    "title(h_axes, 'Tracking Failed', 'FontSize', 15); ");
          //        "delete(findobj(0, 'Tag', 'TemplateAxes'));");
          
          if(saveTrackingResults_) {
            matlabViz_.EvalStringEcho("fclose(fid);");
          }
        }
        
        matlabViz_.EvalString("drawnow");
#endif // #if USE_MATLAB_VISUALIZATION
        
        return RESULT_OK;
      }
      /*
      Result SendTrack(const Array<u8>& image,
                           const VisionSystem::Tracker& tracker,
                           const bool converged,
                           MemoryStack scratch)
      {
#if USE_MATLAB_VISUALIZATION        
        return SendTrack(image, tracker.get_transformation().get_transformedCorners(scratch), converged);
#else
        return RESULT_OK;
#endif        
      }
      */
      
      void SendTrackerPrediction_Helper(s32 subplotNum, const char *titleStr)
      {
#if USE_MATLAB_VISUALIZATION        
        matlabViz_.EvalStringEcho("h = subplot(1,2,%d, 'Parent', h_fig_tform), "
                                  "hold(h, 'off'), imagesc(img, 'Parent', h), axis(h, 'image'), hold(h, 'on'), "
                                  "plot(quad([1 2 4 3 1],1), quad([1 2 4 3 1],2), 'r', 'LineWidth', 2, 'Parent', h); "
                                  "title(h, '%s');", subplotNum, titleStr);
#endif        
      }
      
      Result SendTrackerPrediction_Before(const Array<u8>& image,
                                              const Quadrilateral<f32>& quad)
      {
#if USE_MATLAB_VISUALIZATION
        if(SHOW_TRACKER_PREDICTION) {
          matlabViz_.PutArray(image, "img");
          matlabViz_.PutQuad(quad, "quad");
          SendTrackerPrediction_Helper(1, "Before Prediction");
          beforeCalled_ = true;
        }
#endif
        return RESULT_OK;
      }
      
      Result SendTrackerPrediction_After(const Quadrilateral<f32>& quad)
      {
#if USE_MATLAB_VISUALIZATION
        if(SHOW_TRACKER_PREDICTION) {
          AnkiAssert(beforeCalled_ = true);
          matlabViz_.PutQuad(quad, "quad");
          SendTrackerPrediction_Helper(2, "After Prediction");
        }
#endif
        return RESULT_OK;
      }

      
    } // namespace MatlabVisualization
  } // namespace Vector
} // namespace Anki

