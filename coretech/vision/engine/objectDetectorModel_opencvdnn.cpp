/**
 * File: objectDetectorModel_opencvdnn.cpp
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description: Implementation of ObjectDetector Model class which wrapps OpenCV's DNN module.
 *
 * Copyright: Anki, Inc. 2017
 **/

// TODO: put this back if/when we start supporting other ObjectDetectorModels
//// The contents of this file are only used when the build is using *neither* TF or TF Lite
//#if (!defined(USE_TENSORFLOW) || !USE_TENSORFLOW) && (!defined(USE_TENSORFLOW_LITE) || !USE_TENSORFLOW_LITE)

#include "coretech/vision/engine/objectDetector.h"
#include "coretech/vision/engine/image.h"
#include "coretech/vision/engine/profiler.h"

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/jsonTools.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"

#include "opencv2/dnn.hpp"

#include <list>
#include <fstream>

namespace Anki {
namespace Vision {
  
static const char * const kLogChannelName = "VisionSystem";
  
// Useful just for printing every frame, since detection is slow, even through the profiler
// already has settings for printing based on time. Set to 0 to disable.
CONSOLE_VAR(s32, kObjectDetector_PrintTimingFrequency, "Vision.ObjectDetector", 1);

class ObjectDetector::Model
{
public:

  Model(Profiler& profiler) : _profiler(profiler) { }
  
  Result LoadModel(const std::string& modelPath, const Json::Value& config);
  
  Result Run(const ImageRGB& img, std::list<ObjectDetector::DetectedObject>& objects);
  
private:
  
  Result ReadLabelsFile(const std::string& fileName);
  
  // Member variables:
  struct {
    
    std::string graph;
    std::string labels;
    std::string mode;
    s32    input_width;
    s32    input_height;
    f32    input_mean_R;
    f32    input_mean_G;
    f32    input_mean_B;
    f32    input_std;
    
    s32    top_K;
    f32    min_score; // in [0,1]
    
  } _params;
  
  cv::dnn::Net              _network;
  std::vector<std::string>  _labels;
  bool                      _isDetectionMode;
  
  Profiler& _profiler;
  
}; // class Model
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::LoadModel(const std::string& modelPath, const Json::Value& config)
{
# define GetFromConfig(keyName) \
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
  
  if(_params.graph.substr(_params.graph.size()-3,3) == ".pb")
  {
    // Tensorflow models:
    const std::string graphFileName = Util::FileUtils::FullFilePath({modelPath,_params.graph});
    const std::string graphTxtFileName = Util::FileUtils::FullFilePath({modelPath,_params.graph + "txt"});
    try
    {
      if(Util::FileUtils::FileExists(graphTxtFileName)) {
        _network = cv::dnn::readNetFromTensorflow(graphFileName, graphTxtFileName);
      }
      else {
        _network = cv::dnn::readNetFromTensorflow(graphFileName);
      }
    }
    catch(cv::Exception& e)
    {
      PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.ReadNetFromTensorFlowFailed",
                        "Reading %s: %s",
                        _params.graph.c_str(), e.what());
      return RESULT_FAIL;
    }
  }
  else
  {
    // Caffe models:
    const std::string protoFileName = Util::FileUtils::FullFilePath({modelPath,_params.graph + ".prototxt"});
    const std::string modelFileName = Util::FileUtils::FullFilePath({modelPath,_params.graph + ".caffemodel"});
    try
    {
      _network = cv::dnn::readNetFromCaffe(protoFileName, modelFileName);
    }
    catch(cv::Exception& e)
    {
      PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.ReadNetCaffeFlowFailed",
                        "Reading %s.prototxt/caffemodel: %s",
                        _params.graph.c_str(), e.what());
      return RESULT_FAIL;
    }
  }
  
  if(_network.empty())
  {
    return RESULT_FAIL;
  }
  
  PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.OpenCvDNN.LoadedGraph",
                "%s", _params.graph.c_str());
  
  // Report network complexity in FLOPS
  {
    const cv::dnn::MatShape shape = {1, 3, _params.input_height, _params.input_width};
    auto const numFLOPS = _network.getFLOPS(shape);
    
    const char* flopsPrefix = "M";
    f32 flopsDivisor = 1e6f;
    if(numFLOPS > 1e12)
    {
      flopsPrefix = "T";
      flopsDivisor = 1e12f;
    }
    else if(numFLOPS > 1e9)
    {
      flopsPrefix = "G";
      flopsDivisor = 1e9f;
    }
    
    PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadModel.NetworkFLOPS", "Input %dx%d: %.3f %sFLOPS",
                  _params.input_width, _params.input_height,
                  (f32)numFLOPS / flopsDivisor, flopsPrefix);
  }
  
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
  
  const std::string labelsFileName = Util::FileUtils::FullFilePath({modelPath, _params.labels});
  Result readLabelsResult = ReadLabelsFile(labelsFileName);
  if(RESULT_OK == readLabelsResult)
  {
    PRINT_CH_INFO(kLogChannelName, "ObjectDetector.Model.LoadGraph.ReadLabelFileSuccess", "%s",
                  labelsFileName.c_str());
  }
  
  return readLabelsResult;
  
# undef GetFromConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::ReadLabelsFile(const std::string& fileName)
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
  
  return RESULT_OK;
}
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Model::Run(const ImageRGB& img, std::list<ObjectDetector::DetectedObject>& objects)
{
  const cv::Size processingSize(_params.input_width, _params.input_height);
  const f32 scale = 1.f / (f32)_params.input_std;
  const cv::Scalar mean(_params.input_mean_R, _params.input_mean_G, _params.input_mean_B);
  const bool kSwapRedBlue = false; // Our image class is already RGB
  cv::Mat dnnBlob = cv::dnn::blobFromImage(img.get_CvMat_(), scale, processingSize, mean, kSwapRedBlue);
  
  _network.setInput(dnnBlob);
  
  _profiler.Tic("ObjectDetector.Model.Run.ForwardInference");
  cv::Mat detections = _network.forward();
  _profiler.Toc("ObjectDetector.Model.Run.ForwardInference");
  
  if(kObjectDetector_PrintTimingFrequency > 0)
  {
    static int printCount = kObjectDetector_PrintTimingFrequency;
    if(--printCount == 0)
    {
      _profiler.PrintAverageTiming();
      printCount = kObjectDetector_PrintTimingFrequency;
    }
  }
  
  DetectedObject object;
  bool wasObjectDetected = false;
  
  if(_isDetectionMode)
  {
    const Array2d<float> detectionMat(detections.size[2], detections.size[3], detections.ptr<float>());
    
    const int numDetections = detections.size[2];
    for(int iDetection=0; iDetection<numDetections; ++iDetection)
    {
      const float* detection = detectionMat.GetRow(iDetection);
      const float confidence = detection[2];
      
      if(confidence >= _params.min_score)
      {
        const int labelIndex = detection[1];
        const int xmin = std::max(0,(int)std::round(detection[3] * (float)img.GetNumCols()));
        const int ymin = std::max(0,(int)std::round(detection[4] * (float)img.GetNumRows()));
        const int xmax = std::min(img.GetNumCols(), (int)std::round(detection[5] * (float)img.GetNumCols()));
        const int ymax = std::min(img.GetNumRows(), (int)std::round(detection[6] * (float)img.GetNumRows()));
        
        DEV_ASSERT_MSG(xmax > xmin, "ObjectDetector.Model.Run.InvalidDetectionBoxWidth",
                       "xmin=%d xmax=%d", xmin, xmax);
        DEV_ASSERT_MSG(ymax > ymin, "ObjectDetector.Model.Run.InvalidDetectionBoxHeight",
                       "ymin=%d ymax=%d", ymin, ymax);
        
        const bool labelIndexOOB = (labelIndex < 0 || labelIndex > _labels.size());
        
        object.timestamp = img.GetTimestamp();
        object.score     = confidence;
        object.name      = (labelIndexOOB ? "UNKNOWN" :  _labels.at((size_t)labelIndex));
        object.rect      = Rectangle<s32>(xmin, ymin, xmax-xmin, ymax-ymin);
        
        wasObjectDetected = true;
      }
    }
  }
  else // classification mode
  {
    int maxLabelIndex = -1;
    float maxScore = _params.min_score;
    const float* scores = detections.ptr<float>(0);
    
    // NOTE: this is more general / safer than using cols. (Some architectures seem to output a 4D tensor
    //       and not set cols)
    const int numDetections = detections.size[1];
    
    DEV_ASSERT_MSG(numDetections == _labels.size(), "ObjectDetector.Model.Run.UnexpectedResultSize",
                   "NumLabels:%zu, DNN returned %d values", _labels.size(), numDetections);
    
    for(int i=0; i<numDetections; ++i)
    {
      if(scores[i] > maxScore)
      {
        maxLabelIndex = i;
        maxScore = scores[i];
      }
    }
    
    if(maxLabelIndex >= 0)
    {
      object.timestamp = img.GetTimestamp();
      object.score     = maxScore;
      object.name      = _labels.at((size_t)maxLabelIndex);
      object.rect      = Rectangle<s32>(0,0,img.GetNumCols(),img.GetNumRows());
      
      wasObjectDetected = true;
    }
  }
  
  if(wasObjectDetected)
  {
    PRINT_CH_DEBUG(kLogChannelName,
                   (_isDetectionMode ? "ObjectDetector.Model.Run.ObjectDetected" : "ObjectDetector.Model.Run.ObjectClassified"),
                   "Name:%s Score:%.3f Box:[%d %d %d %d] t:%ums",
                   object.name.c_str(), object.score,
                   object.rect.GetX(), object.rect.GetY(), object.rect.GetWidth(), object.rect.GetHeight(),
                   object.timestamp);
    
    objects.emplace_back(std::move(object));
  }
  
  return RESULT_OK;
}
  
} // namespace Vision
} // namespace Anki

//#endif // #if !defined(USE_TENSORFLOW) || !USE_TENSORFLOW

