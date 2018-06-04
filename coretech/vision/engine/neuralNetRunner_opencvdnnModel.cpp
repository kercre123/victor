/**
 * File: objectDetectorModel_opencvdnn.cpp
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description: Implementation of NeuralNetRunner Model class which wrapps OpenCV's DNN module.
 *
 * Copyright: Anki, Inc. 2017
 **/

// TODO: put this back if/when we start supporting other NeuralNetRunnerModels
//// The contents of this file are only used when the build is using *neither* TF or TF Lite
//#if (!defined(USE_TENSORFLOW) || !USE_TENSORFLOW) && (!defined(USE_TENSORFLOW_LITE) || !USE_TENSORFLOW_LITE)

#include "coretech/vision/engine/neuralNetRunner.h"
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
CONSOLE_VAR(s32, kNeuralNetRunner_PrintTimingFrequency, "Vision.NeuralNetRunner", 1);

class NeuralNetRunner::Model
{
public:

  Model(Profiler& profiler) : _profiler(profiler) { }
  
  Result LoadModel(const std::string& modelPath, const Json::Value& config);
  
  Result Run(const ImageRGB& img, std::list<SalientPoint>& salientPoints);
  
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
Result NeuralNetRunner::Model::LoadModel(const std::string& modelPath, const Json::Value& config)
{
# define GetFromConfig(keyName) \
  if(false == JsonTools::GetValueOptional(config, QUOTE(keyName), _params.keyName)) \
  { \
    PRINT_NAMED_ERROR("NeuralNetRunner.Init.MissingConfig", QUOTE(keyName)); \
    return RESULT_FAIL; \
  }
  
  GetFromConfig(labels);
  GetFromConfig(top_K);
  GetFromConfig(min_score);
  
  if(!ANKI_VERIFY(Util::IsFltGEZero(_params.min_score) && Util::IsFltLE(_params.min_score, 1.f),
                  "NeuralNetRunner.Model.LoadModel.BadMinScore",
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
    if(Util::FileUtils::FileExists(graphFileName))
    {
      try {
        if(Util::FileUtils::FileExists(graphTxtFileName)) {
          _network = cv::dnn::readNetFromTensorflow(graphFileName, graphTxtFileName);
        }
        else {
          _network = cv::dnn::readNetFromTensorflow(graphFileName);
        }
      }
      catch(const cv::Exception& e) {
        PRINT_NAMED_ERROR("NeuralNetRunner.Model.LoadModel.ReadTensorflowModelCvException",
                          "Model file: %s, Error: %s", graphFileName.c_str(), e.what());
        return RESULT_FAIL;
      }
      catch(...) {
        PRINT_NAMED_ERROR("NeuralNetRunner.Model.LoadModel.ReadTensorflowModelUnknownException",
                          "Model file: %s", graphFileName.c_str());
        return RESULT_FAIL;
      }
      
    }
    else
    {
      PRINT_NAMED_ERROR("NeuralNetRunner.Model.LoadModel.TensorflowModelFileNotFound",
                        "Model file: %s", graphFileName.c_str());
      return RESULT_FAIL;
    }
  }
  else
  {
    // Caffe models:
    const std::string protoFileName = Util::FileUtils::FullFilePath({modelPath,_params.graph + ".prototxt"});
    const std::string modelFileName = Util::FileUtils::FullFilePath({modelPath,_params.graph + ".caffemodel"});
    if(Util::FileUtils::FileExists(protoFileName) && Util::FileUtils::FileExists(modelFileName))
    {
      try {
        _network = cv::dnn::readNetFromCaffe(protoFileName, modelFileName);
      }
      catch(const cv::Exception& e) {
        PRINT_NAMED_ERROR("NeuralNetRunner.Model.LoadModel.ReadCaffeModelCvException",
                          "Model file: %s, Error: %s", _params.graph.c_str(), e.what());
        return RESULT_FAIL;
      }
      catch(...) {
        PRINT_NAMED_ERROR("NeuralNetRunner.Model.LoadModel.ReadCaffeModelUnknownException",
                          "Model file: %s", _params.graph.c_str());
        return RESULT_FAIL;
      }
    }
    else
    {
      PRINT_NAMED_ERROR("NeuralNetRunner.Model.LoadModel.CaffeFilesNotFound",
                        "Reading %s/%s.{prototxt|caffemodel}", modelPath.c_str(), _params.graph.c_str());
      return RESULT_FAIL;
    }
  }
  
  if(_network.empty())
  {
    PRINT_NAMED_ERROR("NeuralNetRunner.Model.LoadModel.ReadNetFailed", "");
    return RESULT_FAIL;
  }
  
  PRINT_CH_INFO(kLogChannelName, "NeuralNetRunner.Model.OpenCvDNN.LoadedGraph",
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
    
    PRINT_CH_INFO(kLogChannelName, "NeuralNetRunner.Model.LoadModel.NetworkFLOPS", "Input %dx%d: %.3f %sFLOPS",
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
      PRINT_NAMED_ERROR("NeuralNetRunner.Model.LoadGraph.UnknownMode",
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
    PRINT_CH_INFO(kLogChannelName, "NeuralNetRunner.Model.LoadGraph.ReadLabelFileSuccess", "%s",
                  labelsFileName.c_str());
  }
  
  return readLabelsResult;
  
# undef GetFromConfig
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetRunner::Model::ReadLabelsFile(const std::string& fileName)
{
  std::ifstream file(fileName);
  if (!file)
  {
    PRINT_NAMED_ERROR("NeuralNetRunner.ReadLabelsFile.LabelsFileNotFound",
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
Result NeuralNetRunner::Model::Run(const ImageRGB& img, std::list<SalientPoint>& salientPoints)
{
  const cv::Size processingSize(_params.input_width, _params.input_height);
  const f32 scale = 1.f / (f32)_params.input_std;
  const cv::Scalar mean(_params.input_mean_R, _params.input_mean_G, _params.input_mean_B);
  const bool kSwapRedBlue = false; // Our image class is already RGB
  cv::Mat dnnBlob = cv::dnn::blobFromImage(img.get_CvMat_(), scale, processingSize, mean, kSwapRedBlue);
  
  _network.setInput(dnnBlob);
  
  _profiler.Tic("NeuralNetRunner.Model.Run.ForwardInference");
  cv::Mat detections = _network.forward();
  _profiler.Toc("NeuralNetRunner.Model.Run.ForwardInference");
  
  if(kNeuralNetRunner_PrintTimingFrequency > 0)
  {
    static int printCount = kNeuralNetRunner_PrintTimingFrequency;
    if(--printCount == 0)
    {
      _profiler.PrintAverageTiming();
      printCount = kNeuralNetRunner_PrintTimingFrequency;
    }
  }
  
  SalientPoint salientPoint;
  bool wasSalientPointDetected = false;
  
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
        const float xmin = std::max(0.f, detection[3] * (float)img.GetNumCols());
        const float ymin = std::max(0.f, detection[4] * (float)img.GetNumRows());
        const float xmax = std::min((float)img.GetNumCols(), detection[5] * (float)img.GetNumCols());
        const float ymax = std::min((float)img.GetNumRows(), detection[6] * (float)img.GetNumRows());
        
        DEV_ASSERT_MSG(Util::IsFltGT(xmax, xmin), "NeuralNetRunner.Model.Run.InvalidDetectionBoxWidth",
                       "xmin=%f xmax=%f", xmin, xmax);
        DEV_ASSERT_MSG(Util::IsFltGT(ymax, ymin), "NeuralNetRunner.Model.Run.InvalidDetectionBoxHeight",
                       "ymin=%f ymax=%f", ymin, ymax);
        
        const bool labelIndexOOB = (labelIndex < 0 || labelIndex > _labels.size());
        
        salientPoint.timestamp     = img.GetTimestamp();
        salientPoint.x_img         = (f32)(xmin+xmax) * 0.5f;
        salientPoint.y_img         = (f32)(ymin+ymax) * 0.5f;
        salientPoint.area_fraction = (xmax-xmin)*(ymax-ymin) / (float)img.GetNumElements();
        salientPoint.score         = confidence;
        salientPoint.description   = (labelIndexOOB ? "UNKNOWN" :  _labels.at((size_t)labelIndex));
        
        // TODO: fill in shape
        //object.shape         = {{xmin,ymin}, {xmax,ymin}, {xmax,ymax}, {xmin,ymax}};
        
        wasSalientPointDetected = true;
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
    
    DEV_ASSERT_MSG(numDetections == _labels.size(), "NeuralNetRunner.Model.Run.UnexpectedResultSize",
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
      salientPoint.timestamp     = img.GetTimestamp();
      salientPoint.score         = maxScore;
      salientPoint.description   = _labels.at((size_t)maxLabelIndex);
      salientPoint.x_img         = 0.5f;
      salientPoint.y_img         = 0.5f;
      salientPoint.area_fraction = 1.f;
      
      // TODO: object.shape =
      //object.rect        = Rectangle<s32>(0,0,img.GetNumCols(),img.GetNumRows());
      
      wasSalientPointDetected = true;
    }
  }
  
  if(wasSalientPointDetected)
  {
    PRINT_CH_DEBUG(kLogChannelName,
                   "NeuralNetRunner.Model.Run.SalientPointDetected",
                   "Name:%s Score:%.3f t:%ums",
                   salientPoint.description.c_str(), salientPoint.score, salientPoint.timestamp);
    
    salientPoints.emplace_back(std::move(salientPoint));
  }
  
  return RESULT_OK;
}
  
} // namespace Vision
} // namespace Anki

//#endif // #if !defined(USE_TENSORFLOW) || !USE_TENSORFLOW

