/**
 * File: objectDetectorModel_tensorflow_AOT.cpp
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description: Implementation of ObjectDetector Model class which wraps TensorFlow Ahead-of-Time (AOT) compiled model.
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef USE_TENSORFLOW
#error Expecting USE_TENSORFLOW to be defined
#endif

#if USE_TENSORFLOW && defined(TENSORFLOW_USE_AOT) && TENSORFLOW_USE_AOT==1

#include "coretech/vision/engine/objectDetector.h"
#include "coretech/vision/engine/objectDetectorModel_tensorflow.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/profiler.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

#include <list>

#define EIGEN_USE_THREADS
#define EIGEN_USE_CUSTOM_THREAD_POOL
//#define EIGEN_USE_SIMPLE_THREAD_POOL
#include "tensorflow/aot_test/aot_test.h"
#include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"

namespace Anki {
namespace Vision {

class ObjectDetector::Model : TensorflowModel
{
public:
  
  Model()
  : _graph(TF_TestAOT::AllocMode::ARGS_RESULTS_AND_TEMPS)
  {
    
  }
  
  // ObjectDetector expects LoadModel and Run to exist
  Result LoadModel(const std::string& modelPath, const Json::Value& config);
  Result Run(const ImageRGB& img, std::list<DetectedObject>& objects);
  
private:
  
  using string = tensorflow::string;
  using int32  = tensorflow::int32;
  
  void ResizeImage(const ImageRGB& img, const s32 width, const s32 height, const bool doCrop,
                   Vision::ImageRGB& imgResized, Point2i& upperLeft);
  
  void NormalizeImage(const ImageRGB& imgResized,
                      const PixelRGB_<f32>& mean, const s32 std,
                      Array2d<PixelRGB_<f32>>& imgNormalized);

  TF_TestAOT _graph;
  
}; // class ObjectDetector::Model
  

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::LoadModel(const std::string& modelPath, const Json::Value& config)
{
  ANKI_CPU_PROFILE("ObjectDetector.LoadModel");
  
# define GetFromConfig(keyName) \
  if(false == JsonTools::GetValueOptional(config, QUOTE(keyName), _params.keyName)) \
  { \
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.MissingConfig", QUOTE(keyName)); \
    return RESULT_FAIL; \
  }
  
  GetFromConfig(labels);
  GetFromConfig(top_K);
  GetFromConfig(min_score);
  
  if(!ANKI_VERIFY(Util::IsFltGEZero(_params.min_score) && Util::IsFltLE(_params.min_score, 1.f),
                  "ObjectDetector.Model.LoadModel.BadMinScore",
                  "%f not in range [0.0,1.0]", _params.min_score))
  {
    return RESULT_FAIL;
  }
  
  // NOTE: no graph to load b/c it's compiled into the AOT library itself
  const size_t expectedSize = _params.input_height * _params.input_width * 3 * sizeof(f32);
  ANKI_VERIFY(_graph.ArgSizes()[0] == expectedSize ,
              "ObjectDetector.Model.Run.UnexpectedArgSize", "%ld vs. %zu",
              _graph.ArgSizes()[0], expectedSize);
  
  // TODO: Specify num threads in config?
  Eigen::ThreadPool thread_pool(1);  // Number of threads in your pool. Set as you want.
  Eigen::ThreadPoolDevice thread_pool_device(&thread_pool, thread_pool.NumThreads());
  _graph.set_thread_pool(&thread_pool_device);
  
  const std::string labels_file_name = Util::FileUtils::FullFilePath({modelPath, _params.labels});
  Result readLabelsResult = ReadLabelsFile(labels_file_name);
  if(RESULT_OK == readLabelsResult)
  {
    PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadGraph.ReadLabelFileSuccess", "%s",
                  labels_file_name.c_str());
  }
  return readLabelsResult;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::Run(const ImageRGB& img, std::list<DetectedObject>& objects)
{
  DEV_ASSERT(!_isDetectionMode, "ObjectDetector.Model.Run.DetectionModeNotSupportedWithAOT");
  
  // Wrap an image "header" around the graph's input argument data
  DEV_ASSERT((((unsigned long)_graph.arg0_data()) % 16) == 0,
             "ObjectDetector.Model.Run.GraphArgDataNot16ByteAligned");
  
  // Resize and normalize the image directly into the compiled graph's input argument
  Array2d<PixelRGB_<f32>> imgNormalized(_params.input_height, _params.input_width,
                                        reinterpret_cast<PixelRGB_<f32>*>(_graph.arg0_data()));
  
  {
    ANKI_CPU_PROFILE("ResizeAndNormalizeImage");
    // Resize and normalize directly into that image / input argument
    const PixelRGB_<f32> mean(_params.input_mean_R, _params.input_mean_G, _params.input_mean_B);
    ResizeAndNormalizeImage(img, _params.input_width, _params.input_height, _params.do_crop,
                            mean, _params.input_std, imgNormalized);
  }
  
  {
    ANKI_CPU_PROFILE("RunModel");
    const bool success = _graph.Run();
    if(!success) {
      PRINT_NAMED_ERROR("ObjectDetector.Model.Run.GraphRunFailed", "Message: %s",
                        _graph.error_msg().c_str());
      return RESULT_FAIL;
    }
  }
  
  return GetClassificationOutputs(img, _graph.result0_data(), objects);
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectDetector::Model::ResizeImage(const ImageRGB& img, const s32 width, const s32 height, const bool doCrop,
                 Vision::ImageRGB& imgResized, Point2i& upperLeft)
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
             "ObjectDetector.Model.CreateTensorFromImage.ResizeOrCropFail");
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectDetector::Model::NormalizeImage(const ImageRGB& imgResized,
                    const PixelRGB_<f32>& mean, const s32 std,
                    Array2d<PixelRGB_<f32>>& imgNormalized)
{
  // Expecting imgNormalized to already be allocated the right size (since it could be wrapped around an
  // existing buffer)
  DEV_ASSERT(imgNormalized.GetNumRows() == imgResized.GetNumRows() &&
             imgNormalized.GetNumCols() == imgResized.GetNumCols(),
             "ObjectDetector.Model.ResizeAndNormalizeImage.NormalizedImageNotAllocated");
  
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
  
} // namespace Vision
} // namespace Anki

#endif // #if defined(USE_TENSORFLOW) && USE_TENSORFLOW
