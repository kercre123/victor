/**
 * File: objectDetectorModel_tensorflow.cpp
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description: Implementation of ObjectDetector Model class which wraps TensorFlow.
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef USE_TENSORFLOW
#error Expecting USE_TENSORFLOW to be defined
#endif

#if USE_TENSORFLOW && (!defined(TENSORFLOW_USE_AOT) || !TENSORFLOW_USE_AOT)

#include "anki/vision/basestation/objectDetector.h"
#include "anki/vision/basestation/objectDetectorModel_tensorflow.h"
#include "anki/vision/basestation/image.h"
#include "anki/vision/basestation/profiler.h"

#include "anki/common/basestation/array2d_impl.h"
#include "anki/common/basestation/jsonTools.h"

#include "util/cpuProfiler/cpuProfiler.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

#include <list>

//#include "tensorflow/cc/ops/const_op.h"
//#include "tensorflow/contrib/image/kernels/image_ops.h"
//#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"

namespace Anki {
namespace Vision {

class ObjectDetector::Model : TensorflowModel
{
public:

  Model(Profiler& profiler) : _profiler(profiler) { }
  
  // ObjectDetector expects LoadModel and Run to exist
  Result LoadModel(const std::string& modelPath, const Json::Value& config);
  Result Run(const ImageRGB& img, std::list<DetectedObject>& objects);
  
private:
  
  using Tensor = tensorflow::Tensor;
  using Status = tensorflow::Status;
  
  // If crop=true, the middle [input_width x input_height] is cropped out.
  // Otherwise, the entire image is scaled.
  // See also: https://github.com/tensorflow/tensorflow/issues/6955
  Status CreateTensorFromImage(const ImageRGB& img, std::vector<Tensor>* out_tensors);
  
  std::unique_ptr<tensorflow::Session> _session;
  
  Profiler _profiler;

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
  Status load_graph_status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), graph_file_name, &graph_def);
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
  Result result = RESULT_FAIL;
  
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
  
  //imgResized.Display("Resized", 0);
  
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
    result = RESULT_OK;
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
    const float* output_data = output_scores.tensor<f32, 2>().data();
    result = GetClassificationOutputs(img, output_data, objects);
  }
  
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
tensorflow::Status ObjectDetector::Model::CreateTensorFromImage(const ImageRGB& img, std::vector<Tensor>* out_tensors)
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

  
} // namespace Vision
} // namespace Anki

#endif // #if defined(USE_TENSORFLOW) && USE_TENSORFLOW
