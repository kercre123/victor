/**
 * File: objectDetector_tensorflow.cpp
 *
 * Author: Andrew Stein
 * Date:   5/17/2018
 *
 * Description: <See header>
 *
 * Copyright: Anki, Inc. 2018
 **/

// NOTE: this wrapper completely compiles out if we're using a different model (e.g. TFLite)
#ifdef VIC_NEURALNETS_USE_TENSORFLOW

#include "coretech/common/shared/types.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/vision/neuralnets/neuralNetModel_tensorflow.h"

#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/framework/graph_def_util.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/errors.h"
#include "tensorflow/core/lib/core/stringpiece.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/logging.h" 
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/platform/types.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"
#include "tensorflow/core/util/memmapped_file_system.h"
#include "tensorflow/core/util/stat_summarizer.h"

#include "util/fileUtils/fileUtils.h"
#include "util/helpers/quoteMacro.h"
#include "util/logging/logging.h"

#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <cmath>
#include <fstream>

namespace Anki {
namespace Vision {

#define LOG_CHANNEL "NeuralNets"

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NeuralNetModel::NeuralNetModel(const std::string cachePath)
: INeuralNetModel(cachePath)
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
NeuralNetModel::~NeuralNetModel()
{
  LOG_INFO("NeuralNetModel.Destructor", "");
  if(_session)
  {
    tensorflow::Status sessionCloseStatus = _session->Close();
    if (!sessionCloseStatus.ok() ) 
    {
      PRINT_NAMED_WARNING("NeuralNetModel.Destructor.CloseSessionFailed", "Status: %s",
                          sessionCloseStatus.ToString().c_str());
    }
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetModel::LoadModel(const std::string& modelPath, const Json::Value& config)
{
  const Result result = _params.SetFromConfig(config);
  if(RESULT_OK != result) 
  {
    PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.SetParamsFromConfigFailed", "");
    return result;
  }
  
  const std::string graphFileName = Util::FileUtils::FullFilePath({modelPath, _params.graphFile});
  
  if (!Util::FileUtils::FileExists(graphFileName))
                  
  {
    PRINT_NAMED_ERROR("NeuralNetModel.Model.LoadGraph.GraphFileDoesNotExist", "%s",
                      graphFileName.c_str());
    return RESULT_FAIL;
  }

  if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.LoadModel.FoundGraphFile",
             "%s", graphFileName.c_str());
  }

  tensorflow::GraphDef graphDef;
  
  tensorflow::Session* sessionPtr = nullptr;

  if(_params.memoryMapGraph)
  {
    // Using memory-mapped graphs needs more testing/work, but this is a start. (VIC-3141)
    
    // See also: https://www.tensorflow.org/mobile/optimizing

    // Note that this is a class member because it needs to persist as long as we
    // use the graph referring to it
    _memmappedEnv.reset(new tensorflow::MemmappedEnv(tensorflow::Env::Default()));

    tensorflow::Status mmapStatus = _memmappedEnv->InitializeFromFile(graphFileName);
    
    tensorflow::Status loadGraphStatus = ReadBinaryProto(_memmappedEnv.get(),
        tensorflow::MemmappedFileSystem::kMemmappedPackageDefaultGraphDef,
        &graphDef);

    if (!loadGraphStatus.ok())
    {
      PRINT_NAMED_ERROR("NeuralNetModel.Model.LoadGraph.MemoryMapBinaryProtoFailed",
                        "Status: %s", loadGraphStatus.ToString().c_str());
      return RESULT_FAIL;
    }

    LOG_INFO("NeuralNetModel.LoadModel.MemMappedModelLoadSuccess", "%s", graphFileName.c_str());

    tensorflow::SessionOptions options;
    options.config.mutable_graph_options()
        ->mutable_optimizer_options()
        ->set_opt_level(::tensorflow::OptimizerOptions::L0);
    options.env = _memmappedEnv.get();

    tensorflow::Status session_status = tensorflow::NewSession(options, &sessionPtr);

    if (!session_status.ok())
    {
      PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.NewMemoryMappedSessionFailed",
                        "Status: %s", session_status.ToString().c_str());
      return RESULT_FAIL;
    }

    _session.reset(sessionPtr);
  }
  else
  {
    tensorflow::Status loadGraphStatus = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), 
                                                                       graphFileName, &graphDef);
    if (!loadGraphStatus.ok())
    {
      PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.ReadBinaryProtoFailed",
                        "Status: %s", loadGraphStatus.ToString().c_str());
      return RESULT_FAIL;
    }

    LOG_INFO("NeuralNetModel.LoadModel.ReadBinaryProtoSuccess", "%s", graphFileName.c_str());

    sessionPtr = tensorflow::NewSession(tensorflow::SessionOptions());
  } 

  if(_session)
  {
    tensorflow::Status sessionCloseStatus = _session->Close();
    if (!sessionCloseStatus.ok() ) 
    {
      PRINT_NAMED_WARNING("NeuralNetModel.LoadModel.CloseSessionFailed", "Status: %s",
                          sessionCloseStatus.ToString().c_str());
    }
  }
  _session.reset(sessionPtr);

  tensorflow::Status sessionCreateStatus = _session->Create(graphDef);

  if (!sessionCreateStatus.ok())
  {
    PRINT_NAMED_ERROR("NeuralNetModel.LoadModel.CreateSessionFailed",
                      "Status: %s", sessionCreateStatus.ToString().c_str());
    return RESULT_FAIL;
  }

  LOG_INFO("NeuralNetModel.LoadModel.SessionCreated", "");

  if (_params.verbose)
  {
    //const std::string graph_str = tensorflow::SummarizeGraphDef(graphDef);
    //std::cout << graph_str << std::endl;
    
    // Print some weights from each layer as a sanity check
    int node_count = graphDef.node_size();
    for (int i = 0; i < node_count; i++)
    {
      const auto n = graphDef.node(i);
      LOG_INFO("NeuralNetModel.LoadModel.Summary", "Layer %d - Name: %s, Op: %s", i, n.name().c_str(), n.op().c_str());
      if(n.op() == "Const")
      {
        tensorflow::Tensor t;
        if (!t.FromProto(n.attr().at("value").tensor())) {
          LOG_INFO("NeuralNetModel.LoadModel.SummaryFail", "Failed to create Tensor from proto");
          continue;
        }

        LOG_INFO("NeuralNetModel.LoadModel.Summary", "%s", t.DebugString().c_str());
      }
      else if(n.op() == "Conv2D")
      {
        const auto& filterNodeName = n.input(1);
        LOG_INFO("NeuralNetModel.LoadModel.Summary", "Filter input from Conv2D node: %s", filterNodeName.c_str());
      }
    }
  }

  const std::string labelsFileName = Util::FileUtils::FullFilePath({modelPath, _params.labelsFile});
  Result readLabelsResult = ReadLabelsFile(labelsFileName, _labels);
  if (RESULT_OK == readLabelsResult)
  {
    LOG_INFO("NeuralNetModel.LoadModel.ReadLabelFileSuccess", "%s", labelsFileName.c_str());
  }
  return readLabelsResult;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void NeuralNetModel::GetDetectedObjects(const std::vector<tensorflow::Tensor>& outputRensors, TimeStamp_t timestamp,
                                        std::list<Vision::SalientPoint>& salientPoints)
{
  DEV_ASSERT(outputRensors.size() == 4, "NeuralNetModel.GetDetectedObjects.WrongNumOutputs");

  const int numDetections = (int)outputRensors[3].tensor<float,1>().data()[0];

  if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.GetDetectedObjects.NumDetections", "%d raw detections", numDetections);
  }

  if(numDetections > 0)
  {
    const float* scores  = outputRensors[0].tensor<float, 2>().data();
    const float* classes = outputRensors[1].tensor<float, 2>().data();
    
    auto const& boxesTensor = outputRensors[2].tensor<float, 3>();
    
    const float* boxes = boxesTensor.data();
    
    for(int i=0; i<numDetections; ++i)
    {
      if(scores[i] >= _params.minScore)
      {
        const float* box = boxes + (4*i);
        const float xmin = box[0];
        const float ymin = box[1];
        const float xmax = box[2];
        const float ymax = box[3];
        
        const size_t labelIndex = (size_t)(classes[i]);

        const Rectangle<int32_t> bbox(xmin, ymin, xmax-xmin, ymax-ymin);
        const Poly2i poly(bbox);
        
        Vision::SalientPoint salientPoint(timestamp,
                                          (float)(xmin+xmax) * 0.5f,
                                          (float)(ymin+ymax) * 0.5f,
                                          scores[i],
                                          bbox.Area(),
                                          Vision::SalientPointType::Object,
                                          (labelIndex < _labels.size() ? _labels[labelIndex] : "<UNKNOWN>"),
                                          poly.ToCladPoint2dVector());
        
        salientPoints.emplace_back(std::move(salientPoint));
      }
    }

    if(_params.verbose)
    {
      std::string salientPointsStr;
      for(auto const& salientPoint : salientPoints) {
        salientPointsStr += salientPoint.description + " ";
      }

      LOG_INFO("NeuralNetModel.GetDetectedObjects.ReturningObjects",
               "Returning %d salient points with score above %f: %s",
               (int)salientPoints.size(), _params.minScore, salientPointsStr.c_str());
    }
  } 
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetModel::Detect(cv::Mat& img, const TimeStamp_t t, std::list<Vision::SalientPoint>& salientPoints)
{
  tensorflow::Tensor imageTensor;

  if(_params.useGrayscale)
  {
    cv::cvtColor(img, img, CV_BGR2GRAY);
  }

  const char* typeStr = (_params.useFloatInput ? "FLOAT" : "UINT8");

  if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.Detect.Resizing", "From [%dx%dx%d] image to [%dx%dx%d] %s tensor",
             img.cols, img.rows, img.channels(), 
             _params.inputWidth, _params.inputHeight, (_params.useGrayscale ? 1 : 3), 
             typeStr);
  }

  const auto kResizeMethod = CV_INTER_LINEAR;

  if(_params.useFloatInput)
  {
    // Resize uint8 image data, and *then* convert smaller image to float below
    // TODO: Resize and convert directly into the tensor
    if(img.rows != _params.inputHeight || img.cols != _params.inputWidth)
    {
      cv::resize(img, img, cv::Size(_params.inputWidth,_params.inputHeight), 0, 0, kResizeMethod);
    } 
    else if(_params.verbose)
    {
      LOG_INFO("NeuralNetModel.Detect.SkipResize", "Skipping actual resize: image already correct size");
    }
    DEV_ASSERT(img.isContinuous(), "NeuralNetModel.Detect.ImageNotContinuous");

    imageTensor = tensorflow::Tensor(tensorflow::DT_FLOAT, {
      1, _params.inputHeight, _params.inputWidth, img.channels()
    });

    // Scale/shift resized image directly into the tensor data    
    const auto cvType = (img.channels() == 1 ? CV_32FC1 : CV_32FC3);
    
    cv::Mat cvTensor(_params.inputHeight, _params.inputWidth, cvType,
                     imageTensor.tensor<float, 4>().data());

    img.convertTo(cvTensor, cvType, 1.f/_params.inputScale, _params.inputShift);
  
  }
  else 
  {
    imageTensor = tensorflow::Tensor(tensorflow::DT_UINT8, {
      1, _params.inputHeight, _params.inputWidth, img.channels()
    });

    // Resize uint8 input image directly into the uint8 tensor data    
    cv::Mat cvTensor(_params.inputHeight, _params.inputWidth, 
                     (img.channels() == 1 ? CV_8UC1 : CV_8UC3),
                     imageTensor.tensor<uint8_t, 4>().data());

    cv::resize(img, cvTensor, cv::Size(_params.inputWidth, _params.inputHeight), 0, 0, kResizeMethod);
  }
    
  if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.Detect.RunningSession", "Input=[%dx%dx%d], %s, %d output(s)",
             img.cols, img.rows, img.channels(), typeStr, (int)_params.outputLayerNames.size());
  }

  std::vector<tensorflow::Tensor> outputTensors;
  if (RESULT_FAIL == Run(imageTensor, outputTensors))
  {
    return RESULT_FAIL;
  }

  // Note: If your expected network output is a tensor where
  // column/row major matter there is no programatic check
  // whether the tensor is row or column major. Specifically
  // DFP's network output GetLocalizedBinaryClassification
  // is column major while objectness's output
  // GetSalientPointsFromResponseMap is row major 
  // however they both report the same format. VIC-4386
  Result result = RESULT_OK;
  switch(_params.outputType)
  {
    case NeuralNetParams::OutputType::Classification:
    {
      DEV_ASSERT(outputTensors.size() == 1, "NeuralNetModel.Detect.Classification.WrongNumOutputTensors");
      const float* outputData = outputTensors[0].tensor<float, 2>().data();
      ClassificationOutputHelper(outputData, t, salientPoints);
      break;
    }
    case NeuralNetParams::OutputType::BinaryLocalization:
    {
      DEV_ASSERT(outputTensors.size() == 1, "NeuralNetModel.Detect.BinaryLocalization.WrongNumOutputTensors");
      auto const& outputTensor = outputTensors[0];
      
      // This raw (Eigen) tensor data appears to be _column_ major (i.e. "not row major"). Ensure that remains true.
      DEV_ASSERT( !(outputTensor.tensor<float, 2>().Options & Eigen::RowMajor),
                 "NeuralNetModel.Detect.OutputNotRowMajor");
      
      const float* outputData = outputTensor.tensor<float, 2>().data();
      
      LocalizedBinaryOutputHelper(outputData, t, salientPoints);
      break;
    }
    case NeuralNetParams::OutputType::AnchorBoxes:
    {
      GetDetectedObjects(outputTensors, t, salientPoints);
      break;
    }
    case NeuralNetParams::OutputType::Segmentation:
    {
      // If there are more than two segmentation classes the responses will
      // be in the channels not in a list
      DEV_ASSERT(outputTensors.size() == 1, "NeuralNetModel.Detect.Segmentation.WrongNumOutputTensors");
      const int numberOfChannels = 2;
      tensorflow::Tensor squeezedTensor(tensorflow::DT_FLOAT,
                                        tensorflow::TensorShape({_params.inputWidth, _params.inputHeight, 
                                                                 numberOfChannels}));

      // Reshape tensor from [1, inputWidth, inputHeight, 2] to [inputWidth, inputHeight, 2]
      if (!squeezedTensor.CopyFrom(outputTensors[0], tensorflow::TensorShape({_params.inputWidth,
                                                                           _params.inputHeight,
                                                                           numberOfChannels})))
      {
        PRINT_NAMED_ERROR("NeuralNetModel.GetSalientPointsFromResponseMap.CopyFromFailed", "");
        return RESULT_FAIL;
      }
      const float* outputData = squeezedTensor.tensor<float, 3>().data();
      ResponseMapOutputHelper(outputData, t, numberOfChannels, salientPoints);
      break;
    }
    default:
      LOG_ERROR("NeuralNetModel.Detect.UnknownOutputType", "");
  }

  if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.Detect.SessionComplete", "");
  }

  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result NeuralNetModel::Run(tensorflow::Tensor imageTensor, std::vector<tensorflow::Tensor>& outputTensors)
{
  tensorflow::Status runStatus;
  if (0 == _params.benchmarkRuns)
  {
    runStatus = _session->Run({{_params.inputLayerName, imageTensor}},
                              _params.outputLayerNames, {}, &outputTensors);
  }
  else
  {
    std::unique_ptr<tensorflow::StatSummarizer> stats;
    tensorflow::StatSummarizerOptions statsOptions;
    statsOptions.show_run_order = true;
    statsOptions.run_order_limit = 0;
    statsOptions.show_time = true;
    statsOptions.time_limit = 10;
    statsOptions.show_memory = true;
    statsOptions.memory_limit = 10;
    statsOptions.show_type = true;
    statsOptions.show_summary = true;
    stats.reset(new tensorflow::StatSummarizer(statsOptions));

    tensorflow::RunOptions runOptions;
    if (nullptr != stats)
    {
      runOptions.set_trace_level(tensorflow::RunOptions::FULL_TRACE);
    }
    else
    {
      PRINT_NAMED_ERROR("ObjectDetector.Detect.Run.StatsSummarizerInitFail", "");
      return RESULT_FAIL;
    }

    tensorflow::RunMetadata runMetadata;
    for (uint32_t i = 0; i < _params.benchmarkRuns; ++i)
    {
      runStatus = _session->Run(runOptions, {{_params.inputLayerName, imageTensor}},
                                _params.outputLayerNames, {}, &outputTensors, &runMetadata);
      if (!runStatus.ok())
      {
        break;
      }
      if (nullptr != stats)
      {
        DEV_ASSERT(runMetadata.has_step_stats(), "ObjectDetector.Detect.Run.NullBenchmarkStats");
        const tensorflow::StepStats& step_stats = runMetadata.step_stats();
        stats->ProcessStepStats(step_stats);
      }
    }
    // Print all the stats to the logs
    // Note: right now all the stats are accummlated so for an average,
    // division by benchmarkRuns (which shows up as count in the tensorflow
    // stat summary) is neccesary.
    stats->PrintStepStats();
  }

  if (!runStatus.ok())
  {
    PRINT_NAMED_ERROR("ObjectDetector.Detect.Run.DetectionSessionRunFail", "%s", runStatus.ToString().c_str());
    return RESULT_FAIL;
  }
  else
  {
    return RESULT_OK;
  }
}

} // namespace Vision
} // namespace Anki

#endif /* VIC_NEURALNETS_USE_TENSORFLOW */
