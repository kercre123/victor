/**
 * File: objectDetector.cpp
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 **/

#include "anki/vision/basestation/objectDetector.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/imageCache.h"

#include "anki/common/basestation/array2d_impl.h"
#include "anki/common/basestation/jsonTools.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

#include "opencv2/dnn.hpp"

#include <list>
#include <fstream>

#define USE_TENSORFLOW 0

#if USE_TENSORFLOW

// Whether or not to use the Ahead-of-Time (AOT) compiled Tensorflow model
#ifndef TENSORFLOW_USE_AOT
# error Expecting TENSORFLOW_USE_AOT to be defined by gyp!
#endif

#if TENSORFLOW_USE_AOT
#  define EIGEN_USE_THREADS
#  define EIGEN_USE_CUSTOM_THREAD_POOL
//#define EIGEN_USE_SIMPLE_THREAD_POOL
#  include "tensorflow/aot_test/aot_test.h"
#  include "third_party/eigen3/unsupported/Eigen/CXX11/Tensor"
#else
//#  include "tensorflow/cc/ops/const_op.h"
//#  include "tensorflow/contrib/image/kernels/image_ops.h"
//#  include "tensorflow/cc/ops/standard_ops.h"
#  include "tensorflow/core/framework/graph.pb.h"
#  include "tensorflow/core/framework/tensor.h"
#  include "tensorflow/core/graph/default_device.h"
#  include "tensorflow/core/graph/graph_def_builder.h"
#  include "tensorflow/core/lib/core/errors.h"
#  include "tensorflow/core/lib/core/stringpiece.h"
#  include "tensorflow/core/lib/core/threadpool.h"
#  include "tensorflow/core/lib/io/path.h"
#  include "tensorflow/core/lib/strings/stringprintf.h"
#  include "tensorflow/core/platform/init_main.h"
#  include "tensorflow/core/platform/logging.h"
#  include "tensorflow/core/platform/types.h"
#  include "tensorflow/core/public/session.h"
#  include "tensorflow/core/util/command_line_flags.h"
#endif

#endif // USE_TENSORFLOW


namespace Anki {
namespace Vision {
  
static const char * const kLogChannelName = "VisionSystem";
  
#if USE_TENSORFLOW
  
class ObjectDetector::Model
{
public:
  
  Model()
# if TENSORFLOW_USE_AOT
  : _graph(TF_TestAOT::AllocMode::ARGS_RESULTS_AND_TEMPS)
# endif
  {
    
  }
  
  Result LoadModel(const std::string& modelPath, const Json::Value& config)
  {
    ANKI_CPU_PROFILE("ObjectDetector.LoadModel");
    
#   define GetFromConfig(keyName) \
    if(false == JsonTools::GetValueOptional(config, QUOTE(keyName), _params.keyName)) \
    { \
      PRINT_NAMED_ERROR("ObjectDetector.Init.MissingConfig", QUOTE(keyName)); \
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
    
#   if TENSORFLOW_USE_AOT
    {
      // NOTE: no graph to load b/c it's compiled into the AOT library itself
      const size_t expectedSize = _params.input_height * _params.input_width * 3 * sizeof(f32);
      ANKI_VERIFY(_graph.ArgSizes()[0] == expectedSize ,
                  "ObjectDetector.Model.Run.UnexpectedArgSize", "%ld vs. %zu",
                  _graph.ArgSizes()[0], expectedSize);
      
      // TODO: Specify num threads in config?
      Eigen::ThreadPool thread_pool(1);  // Number of threads in your pool. Set as you want.
      Eigen::ThreadPoolDevice thread_pool_device(&thread_pool, thread_pool.NumThreads());
      _graph.set_thread_pool(&thread_pool_device);
    }
#   else
    {
      // Additional parameters when not using AOT
      GetFromConfig(graph);
      GetFromConfig(mode);
      if(_params.mode == "detection")
      {
        _isDetectionMode = true;
      }
      else
      {
        if(_params.mode != "classification")
        {
          PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.UnknownMode",
                            "Expecting 'classification' or 'detection'. Got '%s'.",
                            _params.mode.c_str());
          return RESULT_FAIL;
        }
        _isDetectionMode = false;
      }
      
      GetFromConfig(input_layer);
      GetFromConfig(output_scores_layer);
      GetFromConfig(input_height);
      GetFromConfig(input_width);
      GetFromConfig(do_crop);
      
      if(_params.output_scores_layer.empty())
      {
        PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.UnspecifiedScoresLayer", "");
        return RESULT_FAIL;
      }
      
      if(_isDetectionMode)
      {
        // These are only required/used in detection mode
        GetFromConfig(output_boxes_layer);
        GetFromConfig(output_classes_layer);
        GetFromConfig(output_num_detections_layer);
      }
      else
      {
        // Only required in classification mode
        GetFromConfig(input_mean_R);
        GetFromConfig(input_mean_G);
        GetFromConfig(input_mean_B);
        GetFromConfig(input_std);
      }
      
      const std::string graph_file_name = Util::FileUtils::FullFilePath({modelPath, _params.graph});
      
      if(!ANKI_VERIFY(Util::FileUtils::FileExists(graph_file_name),
                      "ObjectDetector.Model.LoadGraph.GraphFileDoesNotExist", "%s",
                      graph_file_name.c_str()))
      {
        return RESULT_FAIL;
      }
      
      tensorflow::GraphDef graph_def;
      Status load_graph_status = ReadBinaryProto(tensorflow::Env::Default(), graph_file_name, &graph_def);
      if (!load_graph_status.ok())
      {
        PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.ReadBinaryProtoFailed",
                          "Status: %s", load_graph_status.ToString().c_str());
        return RESULT_FAIL;
      }
      
      PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadGraph.ModelLoadSuccess", "%s",
                    graph_file_name.c_str());
      
      _session.reset(tensorflow::NewSession(tensorflow::SessionOptions()));
      Status session_create_status = _session->Create(graph_def);
      if (!session_create_status.ok())
      {
        PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.CreateSessionFailed",
                          "Status: %s", session_create_status.ToString().c_str());
        return RESULT_FAIL;
      }
    }
#   endif
    
    const std::string labels_file_name = Util::FileUtils::FullFilePath({modelPath, _params.labels});
    Result readLabelsResult = ReadLabelsFile(labels_file_name);
    if(RESULT_OK == readLabelsResult)
    {
      PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadGraph.ReadLabelFileSuccess", "%s",
                    labels_file_name.c_str());
    }
    return readLabelsResult;
  }
  
  
  Result Run(const ImageRGB& img, std::list<DetectedObject>& objects)
  {
    const float* output_data = nullptr;
    
#   if TENSORFLOW_USE_AOT
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
      
      output_data = _graph.result0_data();
    }
#   else
    {
      ImageRGB imgResized;
      Point2i upperLeft(0,0);
      if(_params.input_width == 0 && _params.input_height == 0)
      {
        imgResized = img;
      }
      else
      {
        ResizeImage(img, _params.input_width, _params.input_height, _params.do_crop, imgResized, upperLeft);
      }
     
      imgResized.Display("Resized", 0);
      
      std::vector<Tensor> image_tensors;
      Status read_tensor_status = CreateTensorFromImage(imgResized, &image_tensors);
      if (!read_tensor_status.ok()) {
        PRINT_NAMED_ERROR("ObjectDetector.Model.Run.CreateTensorFailed", "Status: %s",
                          read_tensor_status.ToString().c_str());
        return RESULT_FAIL;
      }
    
      const Tensor& image_tensor = image_tensors[0];
      std::vector<Tensor> outputs;
      
      if(_isDetectionMode)
      {
        {
          ANKI_CPU_PROFILE("RunModel");
          Status run_status = _session->Run({{_params.input_layer, image_tensor}},
                                            {_params.output_scores_layer,
                                              _params.output_classes_layer,
                                              _params.output_boxes_layer,
                                              _params.output_num_detections_layer}, {}, &outputs);
          if (!run_status.ok()) {
            PRINT_NAMED_ERROR("ObjectDetector.Model.Run.DetectionSessionRunFail",
                              "Status: %s", run_status.ToString().c_str());
            return RESULT_FAIL;
          }
        }
        
        const s32 numDetections = (s32)outputs[3].tensor<f32,1>().data()[0];
        if(numDetections > 0)
        {
          const f32* scores  = outputs[0].tensor<f32, 2>().data();
          const f32* classes = outputs[1].tensor<f32, 2>().data();
          
          auto const& boxesTensor = outputs[2].tensor<f32, 3>();
          
          DEV_ASSERT_MSG(boxesTensor.dimension(0) == 1 &&
                         boxesTensor.dimension(1) == numDetections &&
                         boxesTensor.dimension(2) == 4,
                         "ObjectDetector.Model.Run.UnexpectedOutputBoxesSize",
                         "%dx%dx%d instead of 1x%dx4",
                         (int)boxesTensor.dimension(0), (int)boxesTensor.dimension(1), (int)boxesTensor.dimension(2),
                         numDetections);
                     
          const f32* boxes = boxesTensor.data();
          
          for(s32 i=0; i<std::min(_params.top_K,numDetections); ++i)
          {
            if(scores[i] >= _params.min_score)
            {
              const f32* box = boxes + (4*i);
              const s32 ymin = std::round(box[0] * (f32)imgResized.GetNumRows()) + upperLeft.y();
              const s32 xmin = std::round(box[1] * (f32)imgResized.GetNumCols()) + upperLeft.x();
              const s32 ymax = std::round(box[2] * (f32)imgResized.GetNumRows());
              const s32 xmax = std::round(box[3] * (f32)imgResized.GetNumCols());
              
              DEV_ASSERT_MSG(xmax > xmin, "ObjectDetector.Model.Run.InvalidDetectionBoxWidth",
                             "xmin=%d xmax=%d", xmin, xmax);
              DEV_ASSERT_MSG(ymax > ymin, "ObjectDetector.Model.Run.InvalidDetectionBoxHeight",
                             "ymin=%d ymax=%d", ymin, ymax);
              
              DetectedObject object{
                .timestamp = img.GetTimestamp(),
                .score     = scores[i],
                .name      = _labels[(size_t)classes[i]],
                .rect      = Rectangle<s32>(xmin, ymin, xmax-xmin, ymax-ymin),
              };
              
              PRINT_CH_DEBUG(kLogChannelName, "ObjectDetector.Model.Run.ObjectDetected",
                             "Name:%s Score:%.3f Box:[%d %d %d %d] t:%ums",
                             object.name.c_str(), object.score,
                             object.rect.GetX(), object.rect.GetY(), object.rect.GetWidth(), object.rect.GetHeight(),
                             object.timestamp);
                             
              objects.push_back(std::move(object));
            }
          }
        }
      }
      else
      {
        // Actually run the image through the model.
        ANKI_CPU_PROFILE("RunModel");
        Status run_status = _session->Run({{_params.input_layer, image_tensor}},
                                          {_params.output_scores_layer}, {}, &outputs);
        
        if (!run_status.ok()) {
          PRINT_NAMED_ERROR("ObjectDetector.Model.Run.ClassificationSessionRunFail",
                            "Status: %s", run_status.ToString().c_str());
          return RESULT_FAIL;
        }
        
        const Tensor& output_scores = outputs[0];
        output_data = output_scores.tensor<f32, 2>().data();
      }

    }
#   endif // TENSORFLOW_USE_AOT
    
    if(!_isDetectionMode)
    {
      DEV_ASSERT(nullptr != output_data, "ObjectDetector.Model.Run.OutputDataNull");
      
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
          DEV_ASSERT_MSG(maxIndex < _labels.size(), "ObjectDetector.Model.Run.MaxIndexTooLarge",
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
        DEV_ASSERT(false, "ObjectDetector.Model.Run.OnlyTopOneSupported");
        return RESULT_FAIL;
      }
    }
    
    return RESULT_OK;
  }
  
private:
  
# if !TENSORFLOW_USE_AOT
  using Tensor = tensorflow::Tensor;
  using Status = tensorflow::Status;
# endif
  
  using string = tensorflow::string;
  using int32  = tensorflow::int32;
  
  using DetectedObject = ObjectDetector::DetectedObject;
  
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
  void NormalizeImage(const ImageRGB& imgResized,
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

# if !TENSORFLOW_USE_AOT
  // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
  // If crop=true, the middle [input_width x input_height] is cropped out.
  // Otherwise, the entire image is scaled.
  // See also: https://github.com/tensorflow/tensorflow/issues/6955
  Status CreateTensorFromImage(const ImageRGB& img, std::vector<Tensor>* out_tensors)
  {
    //auto root = tensorflow::Scope::NewRootScope();
    using namespace ::tensorflow::ops;  // NOLINT(build/namespaces)
    
    if(_isDetectionMode)
    {
      Tensor input_raw(tensorflow::DT_UINT8, {
        1, img.GetNumRows(), img.GetNumCols(), img.GetNumChannels()
      });
      
      if(img.IsContinuous())
      {
        // TODO: Avoid copying the data (just "point" the tensor at the image's data pointer?)
        memcpy(input_raw.tensor<u8, 4>().data(), img.GetDataPointer(), img.GetNumElements()*img.GetNumChannels());
      }
      else
      {
        u8* tensor_data = input_raw.tensor<u8,4>().data();
        for(s32 i=0; i<img.GetNumRows(); ++i)
        {
          const s32 rowLength = img.GetNumCols()*img.GetNumChannels();
          const Vision::PixelRGB* img_i = img.GetRow(i);
          memcpy(tensor_data, (u8*)img_i, rowLength);
          tensor_data += rowLength;
        }
      }
      
      out_tensors->push_back(input_raw);
    }
    else
    {
      Tensor input_normalized(tensorflow::DT_FLOAT, {
        1, _params.input_height, _params.input_width, img.GetNumChannels()
      });
      
      Array2d<PixelRGB_<f32>> imgNormalized(_params.input_height, _params.input_width,
                                            reinterpret_cast<PixelRGB_<f32>*>(input_normalized.tensor<float, 4>().data()));
      
      const PixelRGB_<f32> mean(_params.input_mean_R, _params.input_mean_G, _params.input_mean_B);
      NormalizeImage(img, mean, _params.input_std, imgNormalized);
      
      /*
       // This runs the GraphDef network definition that we've just constructed, and
       // returns the results in the output tensor.
       tensorflow::GraphDef graph;
       TF_RETURN_IF_ERROR(root.ToGraphDef(&graph));
       
       std::unique_ptr<tensorflow::Session> session(tensorflow::NewSession(tensorflow::SessionOptions()));
       TF_RETURN_IF_ERROR(session->Create(graph));
       TF_RETURN_IF_ERROR(session->Run({}, {output_name}, {}, out_tensors));
       */
      
      out_tensors->push_back(input_normalized);
    }
    return Status::OK();
  }
# endif
  
  // Member variables:
  
  struct {
#   if !TENSORFLOW_USE_AOT
    // Extra fields needed for non-AOT model
    string graph; // = "tensorflow/examples/label_image/data/inception_v3_2016_08_28_frozen.pb";
    string input_layer; // = "input";
    //string output_layer; // = "InceptionV3/Predictions/Reshape_1";
    string mode; // "classification" or "detection"
    string output_scores_layer; // only output required in "classification" mode
    string output_boxes_layer;
    string output_classes_layer;
    string output_num_detections_layer;
#   endif
    
    string labels; // = "tensorflow/examples/label_image/data/imagenet_slim_labels.txt";
    int32  input_width; // = 299;
    int32  input_height; // = 299;
    u8     input_mean_R; // = 0;
    u8     input_mean_G; // = 0;
    u8     input_mean_B; // = 0;
    int32  input_std; // = 255;
    bool   do_crop; // = true;
    
    int32  top_K; // = 1;
    float  min_score; // in [0,1]
    
  } _params;

  bool _isDetectionMode;
  std::vector<std::string> _labels;

# if TENSORFLOW_USE_AOT
  // If using AOT, the graph is compiled in as a library
  TF_TestAOT _graph;
# else
  // It NOT using AOT, we will spin up a TF Session to run a graph loaded from a file
  std::unique_ptr<tensorflow::Session> _session;
# endif
  
};
  
#else // NOT USE_TENSORFLOW
  
class ObjectDetector::Model
{
public:
  
  Result LoadModel(const std::string& modelPath, const Json::Value& config)
  {
    ANKI_CPU_PROFILE("ObjectDetector.LoadModel");
   
#   define GetFromConfig(keyName) \
    if(false == JsonTools::GetValueOptional(config, QUOTE(keyName), _params.keyName)) \
    { \
      PRINT_NAMED_ERROR("ObjectDetector.Init.MissingConfig", QUOTE(keyName)); \
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
    
    GetFromConfig(graph);
    GetFromConfig(input_height);
    GetFromConfig(input_width);
    GetFromConfig(input_mean_R);
    GetFromConfig(input_mean_G);
    GetFromConfig(input_mean_B);
    GetFromConfig(input_std);
    
    // Caffe models:
    const std::string protoFileName = Util::FileUtils::FullFilePath({modelPath,_params.graph + ".prototxt"});
    const std::string modelFileName = Util::FileUtils::FullFilePath({modelPath,_params.graph + ".caffemodel"});
    _network = cv::dnn::readNetFromCaffe(protoFileName, modelFileName);
    
    // Tensorflow models:
    //_network = cv::dnn::readNetFromTensorflow(graph);
   
    if(_network.empty())
    {
      return RESULT_FAIL;
    }
    
    const std::string labelsFileName = Util::FileUtils::FullFilePath({modelPath, _params.labels});
    Result readLabelsResult = ReadLabelsFile(labelsFileName);
    if(RESULT_OK == readLabelsResult)
    {
      PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadGraph.ReadLabelFileSuccess", "%s",
                    labelsFileName.c_str());
    }
    return readLabelsResult;
  }
  
  Result Run(const ImageRGB& img, std::list<ObjectDetector::DetectedObject>& objects)
  {
    ANKI_CPU_PROFILE("RunModel");
    const cv::Size processingSize(_params.input_width, _params.input_height);
    const f32 scale = 1.f / (f32)_params.input_std;
    const cv::Scalar mean(_params.input_mean_R, _params.input_mean_G, _params.input_mean_B);
    const bool kSwapRedBlue = false; // Our image class is already RGB
    cv::Mat dnnBlob = cv::dnn::blobFromImage(img.get_CvMat_(), scale, processingSize, kSwapRedBlue);
    
    //const cv::dnn::MatShape shape = {1,image.rows,image.cols,3};
    //std::cout << "FLOPS: " << network.getFLOPS(shape) << std::endl;
    
    //std::cout << "Setting blob input: " << dnnBlob.size[0] << "x" << dnnBlob.size[1] << "x" << dnnBlob.size[2] << "x" << dnnBlob.size[3] <<  std::endl;
    _network.setInput(dnnBlob);
    
    //std::cout << "Forward inference" << std::endl;
    cv::Mat detections = _network.forward();
    
    const cv::Scalar kColor(0,0,255);
    const int numDetections = detections.size[2];
    for(int iDetection=0; iDetection<numDetections; ++iDetection)
    {
      const float* detection = detections.ptr<float>(iDetection);
      const float confidence = detection[2];
      
      if(confidence >= _params.min_score)
      {
        const int labelIndex = detection[1];
        const int xmin = std::round(detection[3] * (float)img.GetNumCols());
        const int ymin = std::round(detection[4] * (float)img.GetNumRows());
        const int xmax = std::round(detection[5] * (float)img.GetNumCols());
        const int ymax = std::round(detection[6] * (float)img.GetNumRows());
        
        DEV_ASSERT_MSG(xmax > xmin, "ObjectDetector.Model.Run.InvalidDetectionBoxWidth",
                       "xmin=%d xmax=%d", xmin, xmax);
        DEV_ASSERT_MSG(ymax > ymin, "ObjectDetector.Model.Run.InvalidDetectionBoxHeight",
                       "ymin=%d ymax=%d", ymin, ymax);
        
        DetectedObject object{
          .timestamp = img.GetTimestamp(),
          .score     = confidence,
          .name      = _labels.at((size_t)labelIndex),
          .rect      = Rectangle<s32>(xmin, ymin, xmax-xmin, ymax-ymin),
        };
        
        PRINT_CH_DEBUG(kLogChannelName, "ObjectDetector.Model.Run.ObjectDetected",
                       "Name:%s Score:%.3f Box:[%d %d %d %d] t:%ums",
                       object.name.c_str(), object.score,
                       object.rect.GetX(), object.rect.GetY(), object.rect.GetWidth(), object.rect.GetHeight(),
                       object.timestamp);
        
        objects.push_back(std::move(object));
      }
    }
    
    return RESULT_OK;
  }
  
private:
  
  // Private methods:
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
  
  // Member variables:
  struct {
    
    std::string graph; // = "tensorflow/examples/label_image/data/inception_v3_2016_08_28_frozen.pb";
    std::string labels; // = "tensorflow/examples/label_image/data/imagenet_slim_labels.txt";
    s32    input_width; // = 299;
    s32    input_height; // = 299;
    f32    input_mean_R; // = 0;
    f32    input_mean_G; // = 0;
    f32    input_mean_B; // = 0;
    f32    input_std; // = 255;
    
    s32    top_K; // = 1;
    f32    min_score; // in [0,1]
    
  } _params;
  
  cv::dnn::Net              _network;
  std::vector<std::string>  _labels;
  
};
  
#endif // USE_TENSORFLOW
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::ObjectDetector()
: _model(new Model())
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::~ObjectDetector()
{
  
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Init(const std::string& modelPath, const Json::Value& config)
{
  Result result = _model->LoadModel(modelPath, config);
  return result;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Detect(ImageCache&           imageCache,
                              std::list<DetectedObject>&  objects)
{
  Result result = RESULT_OK;
  
  objects.clear();
  
  // Require color data
  if(imageCache.HasColor())
  {
    const ImageRGB& img = imageCache.GetRGB(ImageCache::Size::Half_NN);
    result = _model->Run(img, objects);
    
    const f32 scale = (f32)imageCache.GetOrigNumRows() / (f32)img.GetNumRows();
    if(!Util::IsNear(scale, 1.f))
    {
      //const f32 widthScale  = (f32)imageCache.GetOrigNumCols() / (f32)img.GetNumCols();
      std::for_each(objects.begin(), objects.end(), [scale](DetectedObject& object)
                    {
                      object.rect = Rectangle<s32>(std::round((f32)object.rect.GetX() * scale),
                                                   std::round((f32)object.rect.GetY() * scale),
                                                   std::round((f32)object.rect.GetWidth() * scale),
                                                   std::round((f32)object.rect.GetHeight() * scale));
                    });
    }
    
    if(ANKI_DEV_CHEATS)
    {
      if(objects.empty())
      {
        PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Detect.NoObjects", "t=%ums", imageCache.GetRGB().GetTimestamp());
      }
      for(auto const& object : objects)
      {
        PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Detect.FoundObject", "t=%ums Name:%s Score:%.3f",
                      imageCache.GetRGB().GetTimestamp(), object.name.c_str(), object.score);
      }
    }
  }
  else
  {
    PRINT_PERIODIC_CH_DEBUG(30, kLogChannelName, "ObjectDetector.Detect.NeedColorData", "");
  }
  
  return result;
}
  
} // namespace Vision
} // namespace Anki
