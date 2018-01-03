/**
 * File: objectDetectorModel_tensorflow.h
 *
 * Author: Andrew Stein
 * Date:   9/24/2017
 *
 * Description: Base class to provide shared functionality for Tensorflow model wrappers (AOT and "regular").
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Anki_Vision_ObjectDetector_TensorflowModel_H__
#define __Anki_Vision_ObjectDetector_TensorflowModel_H__

#include "coretech/vision/engine/image.h"

#include "util/logging/logging.h"

#include <fstream>

namespace Anki {
namespace Vision{
 
static const char * const kLogChannelName = "VisionSystem";
  
class TensorflowModel
{
protected:
  using DetectedObject = ObjectDetector::DetectedObject;
  
  bool _isDetectionMode;
  std::vector<std::string> _labels;
  
  struct {
    std::string   graph; // = "tensorflow/examples/label_image/data/inception_v3_2016_08_28_frozen.pb";
    std::string   input_layer; // = "input";
    //std::string output_layer; // = "InceptionV3/Predictions/Reshape_1";
    std::string   mode; // "classification" or "detection"
    std::string   output_scores_layer; // only output required in "classification" mode
    std::string   output_boxes_layer;
    std::string   output_classes_layer;
    std::string   output_num_detections_layer;
    std::string   labels; // = "tensorflow/examples/label_image/data/imagenet_slim_labels.txt";
    
    int32_t       input_width; // = 299;
    int32_t       input_height; // = 299;
    uint8_t       input_mean_R; // = 0;
    uint8_t       input_mean_G; // = 0;
    uint8_t       input_mean_B; // = 0;
    int32_t       input_std; // = 255;
    bool          do_crop; // = true;
    
    int32_t       top_K; // = 1;
    float         min_score; // in [0,1]
    
  } _params;
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  Result ReadLabelsFile(const std::string& fileName)
  {
    std::ifstream file(fileName);
    if (!file)
    {
      PRINT_NAMED_ERROR("ObjectDetector.ReadLabelsFile.LabelsFileNotFound",
                        "%s", fileName.c_str());
      return RESULT_FAIL;
    }
    
    _labels.clear();
    std::string line;
    while (std::getline(file, line)) {
      _labels.push_back(line);
    }
    /*
     Pad the vector to be a multiple of 16, b/c TF expects that (?)
     *found_label_count = result->size();
     const int padding = 16;
     while (result->size() % padding) {
     result->emplace_back();
     }
     */
    return RESULT_OK;
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void ResizeImage(const ImageRGB& img, const s32 width, const s32 height, const bool doCrop,
                   ImageRGB& imgResized, Point2i& upperLeft)
  {
    if(doCrop)
    {
      Rectangle<s32> centerRect(img.GetNumCols()/2 - width/2,
                                img.GetNumRows()/2 - height/2,
                                width, height);
      
      imgResized = img.GetROI(centerRect);
      
      // *After* GetROI, which could modify centerRect!
      upperLeft = centerRect.GetTopLeft();
      
      if(!_isDetectionMode)
      {
        if(imgResized.GetNumCols() != width || imgResized.GetNumRows() != height)
        {
          imgResized.Resize(height, width);
        }
      }
    }
    else
    {
      upperLeft = {0,0};
      imgResized.Allocate(height, width);
      img.Resize(imgResized);
    }
    
    DEV_ASSERT(_isDetectionMode || (imgResized.GetNumRows() == height && imgResized.GetNumCols() == width),
               "ObjectDetector.Model.ResizeImageFail");
  }
  
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  void NormalizeImage(const ImageRGB& imgResized,
                      const PixelRGB_<f32>& mean, const s32 std,
                      Array2d<PixelRGB_<f32>>& imgNormalized)
  {
    // Expecting imgNormalized to already be allocated the right size (since it could be wrapped around an
    // existing buffer)
    DEV_ASSERT(imgNormalized.GetNumRows() == imgResized.GetNumRows() &&
               imgNormalized.GetNumCols() == imgResized.GetNumCols(),
               "ObjectDetector.Model.NormalizeImage.NormalizedImageNotAllocated");
    
    //const PixelRGB_<f32> mean(_params.input_mean_R, _params.input_mean_G, _params.input_mean_B);
    const f32 oneOverStd = 1.f / (f32)std;
    // TODO: Use more efficient cv::subtract(imgResized.get_CvMat_(), mean, imgNormalized);
    //  Getting "error: (-215) type2 == CV_64F && (sz2.height == 1 || sz2.height == 4) in function arithm_op" now though
    // imgNormalized *= 1.f / (f32)_params.input_std;
    std::function<PixelRGB_<f32>(const PixelRGB&)> normalizeFcn = [&mean,oneOverStd](const PixelRGB& p_in)
    {
      const PixelRGB_<f32> p_out(((f32)p_in.r() - mean.r()) * oneOverStd,
                                 ((f32)p_in.g() - mean.g()) * oneOverStd,
                                 ((f32)p_in.b() - mean.b()) * oneOverStd);
      return p_out;
    };
    
    imgResized.ApplyScalarFunction(normalizeFcn, imgNormalized);
    
    if(false)
    {
      // For debugging normalization. Can use this matlab code:
      //   file = fopen('/tmp/img_norm.bin', 'rb')
      //   img = fread(file, 299*299*3, 'float');
      //   imagesc(permute(reshape(permute(reshape(img, 3, 299*299), [2 1]), [299 299 3]), [2 1 3]))
      //   fclose(file)
      FILE* file = fopen("/tmp/img_norm.bin", "wb");
      const Vision::PixelRGB_<f32>* p = const_cast<const Array2d<Vision::PixelRGB_<f32>>*>(&imgNormalized)->GetDataPointer();
      fwrite(p, sizeof(float), 299*299*3, file);
      fclose(file);
    }
  }
  
  Result GetClassificationOutputs(const ImageRGB& img, const float* output_data, std::list<DetectedObject>& objects)
  {
    if(!_isDetectionMode)
    {
      DEV_ASSERT(nullptr != output_data,
                 "ObjectDetector.Model.GetClassificationOutputs.OutputDataNull");
      
      if(1 == _params.top_K)
      {
        f32 maxScore = _params.min_score;
        s32 maxIndex = -1;
        for(s32 i=0; i<_labels.size(); ++i)
        {
          if(output_data[i] > maxScore)
          {
            maxScore = output_data[i];
            maxIndex = i;
          }
        }
        
        if(maxIndex >= 0)
        {
          DEV_ASSERT_MSG(maxIndex < _labels.size(),
                         "ObjectDetector.Model.GetClassificationOutputs.MaxIndexTooLarge",
                         "%d >= %zu", maxIndex, _labels.size());
          DetectedObject object{
            .timestamp = img.GetTimestamp(),
            .score     = maxScore,
            .name      = _labels[maxIndex],
            .rect      = Rectangle<s32>(0,0,img.GetNumCols(), img.GetNumRows()) // TODO: get from object detection model
          };
          objects.push_back(std::move(object));
        }
      }
      else
      {
        // TODO: top_K != 1 (apparently the top_k op is not build into static libtensorflow-core.a)
        //std::partial_sort(<#_RandomAccessIterator __first#>, <#_RandomAccessIterator __middle#>, <#_RandomAccessIterator __last#>, <#_Compare __comp#>)
        DEV_ASSERT(false, "ObjectDetector.Model.GetClassificationOutputs.OnlyTopOneSupported");
        return RESULT_FAIL;
      }
    }
    
    return RESULT_OK;
  }
};
  
} // namespace Vision
} // namespace Anki

#endif /* __Anki_Vision_ObjectDetector_TensorflowModel_H__ */
