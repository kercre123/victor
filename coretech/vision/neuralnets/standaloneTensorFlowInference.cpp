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

#if 0

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

class ObjectDetector : TensorflowModel
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
    
    const s32 numDetections = (s32)outputs[3].tensor<float,1>().data()[0];
    if(numDetections > 0)
    {
      const float* scores  = outputs[0].tensor<float, 2>().data();
      const float* classes = outputs[1].tensor<float, 2>().data();
      
      auto const& boxesTensor = outputs[2].tensor<float, 3>();
      
      DEV_ASSERT_MSG(boxesTensor.dimension(0) == 1 &&
                     boxesTensor.dimension(1) == numDetections &&
                     boxesTensor.dimension(2) == 4,
                     "ObjectDetector.Model.Run.UnexpectedOutputBoxesSize",
                     "%dx%dx%d instead of 1x%dx4",
                     (int)boxesTensor.dimension(0), (int)boxesTensor.dimension(1), (int)boxesTensor.dimension(2),
                     numDetections);
      
      const float* boxes = boxesTensor.data();
      
      for(s32 i=0; i<std::min(_params.top_K,numDetections); ++i)
      {
        if(scores[i] >= _params.min_score)
        {
          const float* box = boxes + (4*i);
          const s32 ymin = std::round(box[0] * (float)imgResized.GetNumRows()) + upperLeft.y();
          const s32 xmin = std::round(box[1] * (float)imgResized.GetNumCols()) + upperLeft.x();
          const s32 ymax = std::round(box[2] * (float)imgResized.GetNumRows());
          const s32 xmax = std::round(box[3] * (float)imgResized.GetNumCols());
          
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
    const float* output_data = output_scores.tensor<float, 2>().data();
    result = GetClassificationOutputs(img, output_data, objects);
  }
  
  return result;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
tensorflow::Status ObjectDetector::Model::CreateTensorFromImage(const cv::Mat& img, std::vector<Tensor>* out_tensors)
{


}

  
} // namespace Vision
} // namespace Anki

#endif

#include "json/json.h"

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

#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs/imgcodecs.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <sys/stat.h>

#include <cmath>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

namespace std {
  template <typename T>
  std::string to_string(T value)
  {
      std::ostringstream os ;
      os << value ;
      return os.str() ;
  }
}

static inline std::string FullFilePath(const std::string& path, const std::string& filename)
{
  return path + "/" + filename;
}

static inline bool FileExists(const std::string& filename)
{
  struct stat st;
  const int statResult = stat(filename.c_str(), &st);
  return (statResult == 0);
}

// returns true on success, false on failure
bool ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out)
{
  std::ifstream file(fileName);
  if (!file)
  {
    std::cerr << "ObjectDetector.ReadLabelsFile.LabelsFileNotFound: " << fileName << std::endl;
    return false;
  }
  
  labels_out.clear();
  std::string line;
  while (std::getline(file, line)) {
    labels_out.push_back(line);
  }
  
  std::cout << "ReadLabelsFile read " << labels_out.size() << " labels" << std::endl;

  return true;
}

int main(int argc, char **argv)
{
  if(argc < 6)
  {
    std::cout << "Usage: " << argv[0] << " modelFile labelFile cachePath minScore detectionMode={mobilenet,ssd_mobilenet} <verbose=0>" << std::endl;
    return -1;
  }

  const std::string modelFile(argv[1]);
  const std::string labelsFile(argv[2]);
  const std::string cachePath(argv[3]);
  const float minScore = atof(argv[4]);
  const std::string architecture(argv[5]);

  bool verbose = false;
  if(argc > 6 && atoi(argv[6])>0)
  {
    std::cout << "Using verbose mode" << std::endl;
    verbose = true;
  }

  std::string input_layer("");
  std::vector<std::string> output_layers;
  bool useFloatInput = false;
  if("ssd_mobilenet" == architecture)
  {
    input_layer = "image_tensor";
    output_layers = {"detection_scores", "detection_classes", "detection_boxes", "num_detections"};
  }
  else if("mobilenet" == architecture)
  { 
    input_layer = "input";
    output_layers = {"MobilenetV1/Predictions/Softmax"};
    useFloatInput = true;
  }
  else
  {
    std::cerr << "Unrecognized architecture: " << architecture << std::endl;
    return -1;
  }

  if(verbose)
  {
    std::cout << "Input: " << input_layer << ", Outputs: ";
    for(auto const& output_layer : output_layers)
    {
      std::cout << output_layer << " ";
    }
    std::cout << std::endl;
  }

  tensorflow::GraphDef graph_def;
  tensorflow::Status load_graph_status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), modelFile, &graph_def);
  if (!load_graph_status.ok())
  {
    std::cerr << "ObjectDetector.Model.LoadGraph.ReadBinaryProtoFailed: " << load_graph_status.ToString() << std::endl;
    return -1;
  }

  std::cout << "Loaded model: " << modelFile << std::endl;

  std::vector<std::string> labels;
  const bool readLabelSuccess = ReadLabelsFile(labelsFile, labels);
  if( !readLabelSuccess )
  {
    std::cerr << "ReadLabelsFile failed" << std::endl;
    return -1;
  }
 
  const std::string imageFilename = FullFilePath(cachePath, "objectDetectionImage.png");
  const int kPollFrequency_ms = 10;

  std::unique_ptr<tensorflow::Session> session(tensorflow::NewSession(tensorflow::SessionOptions()));
  tensorflow::Status session_create_status = session->Create(graph_def);
  if (!session_create_status.ok())
  {
    std::cerr << "ObjectDetector.Model.LoadGraph.CreateSessionFailed: " << session_create_status.ToString() << std::endl;
    return -1;
  }

  std::cout << "Created session, waiting for images" << std::endl;

  while(true)
  {
    // Is there an image file available in the cache?
    const bool isImageAvailable = FileExists(imageFilename);

    if(isImageAvailable)
    {
      if(verbose)
      {
        std::cout << "Found image" << std::endl;
      }

      using namespace ::tensorflow::ops;

      cv::Mat img = cv::imread(imageFilename);

      // TODO: specify size on command line or get from graph?
      cv::resize(img, img, cv::Size(224,224), 0, 0, CV_INTER_AREA);

      tensorflow::Tensor image_tensor;

      if(useFloatInput)
      {
        if(verbose)
        {
          std::cout << "Copying in FLOAT " << img.rows << "x" << img.cols << " image data" << std::endl;
        }

        image_tensor = tensorflow::Tensor(tensorflow::DT_FLOAT, {
          1, img.rows, img.cols, img.channels()
        });

        img.convertTo(img, CV_32FC3, 1.f/255.f);
        memcpy(image_tensor.tensor<float, 4>().data(), img.data, img.rows*img.cols*img.channels()*sizeof(float));

        // float* tensor_data = image_tensor.tensor<float,4>().data();
        // for(int i=0; i<img.rows; ++i)
        // {
        //   const uint8_t* img_i = img.ptr<uint8_t>(i);
        //   for(int j=0; j<img.cols*img.channels(); ++j)
        //   {
        //     tensor_data[j] = (float)(img_i[j]) / 255.f;
        //     ++tensor_data;
        //   }
        // }
      
      }
      else 
      {
        if(verbose)
        {
          std::cout << "Copying in UINT8 " << img.rows << "x" << img.cols << " image data" << std::endl;
        }

        image_tensor = tensorflow::Tensor(tensorflow::DT_UINT8, {
          1, img.rows, img.cols, img.channels()
        });

        if(img.isContinuous())
        {
          // TODO: Avoid copying the data (just "point" the tensor at the image's data pointer?)
          memcpy(image_tensor.tensor<uint8_t, 4>().data(), img.data, img.rows*img.cols*img.channels());
        }
        else
        {
          uint8_t* tensor_data = image_tensor.tensor<uint8_t,4>().data();
          for(int i=0; i<img.rows; ++i)
          {
            const int rowLength = img.channels() * img.cols;
            const uint8_t* img_i = img.ptr<uint8_t>(i);
            memcpy(tensor_data, img_i, rowLength);
            tensor_data += rowLength;
          }
        } 
      }
        
      if(verbose)
      {
        std::cout << "Running session with input dtype=" << image_tensor.dtype() << " and " << output_layers.size() << " outputs" << std::endl;
      }

      std::vector<tensorflow::Tensor> output_tensors;
      tensorflow::Status run_status = session->Run({{input_layer, image_tensor}}, output_layers, {}, &output_tensors);

      if (!run_status.ok()) {
        std::cerr << "ObjectDetector.Model.Run.DetectionSessionRunFail: " << run_status.ToString() << std::endl;
        return -1;
      }
    
      if(verbose)
      {
        std::cout << "Session complete" << std::endl;
      }

      Json::Value detectionResult;
      Json::Value& objects = detectionResult["objects"];

      if(output_tensors.size() == 1)
      {
        const float* output_data = output_tensors[0].tensor<float, 2>().data();
        
        float maxScore = minScore;
        int labelIndex = -1;
        for(int i=0; i<labels.size(); ++i)
        {
          if(output_data[i] > maxScore)
          {
            maxScore = output_data[i];
            labelIndex = i;
          }
        }
        
        if(labelIndex >= 0)
        {
          Json::Value& object = objects[0];
          object["timestamp"] = 0;
          object["score"] = maxScore;
          if(labelIndex >= 0 && labelIndex < labels.size())
          {
            object["name"] = labels[(size_t)labelIndex];
          }
          else 
          {
            std::cerr << "Invalid label labelIndex=" << labelIndex << " (have " << labels.size() << " labels)" << std::endl;
            object["name"] = "<UNKNOWN>";
          }
          object["xmin"] = 0;
          object["ymin"] = 0;
          object["xmax"] = img.cols;
          object["ymax"] = img.rows;

          if(verbose)
          {
            std::cout << "Found " << object["name"].asString() << "[" << labelIndex << "] with score=" << object["score"].asFloat() << std::endl;
          }
        }
        else if(verbose)
        {
          std::cout << "Nothing found above minScore=" << minScore << std::endl;
        }
      }
      else
      {
        assert(output_tensors.size() == 4);

        const int numDetections = (int)output_tensors[3].tensor<float,1>().data()[0];

        if(verbose)
        {
          std::cout << "Got " << numDetections << " raw detections" << std::endl;
        }

        if(numDetections > 0)
        {
          const float* scores  = output_tensors[0].tensor<float, 2>().data();
          const float* classes = output_tensors[1].tensor<float, 2>().data();
          
          auto const& boxesTensor = output_tensors[2].tensor<float, 3>();
          
          // DEV_ASSERT_MSG(boxesTensor.dimension(0) == 1 &&
          //                boxesTensor.dimension(1) == numDetections &&
          //                boxesTensor.dimension(2) == 4,
          //                "ObjectDetector.Model.Run.UnexpectedOutputBoxesSize",
          //                "%dx%dx%d instead of 1x%dx4",
          //                (int)boxesTensor.dimension(0), (int)boxesTensor.dimension(1), (int)boxesTensor.dimension(2),
          //                numDetections);
          
          const float* boxes = boxesTensor.data();
          
          int numReturned = 0;
          for(int i=0; i<numDetections; ++i)
          {
            if(scores[i] >= minScore)
            {
              const float* box = boxes + (4*i);
              
              //(xmax > xmin, "ObjectDetector.Model.Run.InvalidDetectionBoxWidth",
              //               "xmin=%d xmax=%d", xmin, xmax);
              //DEV_ASSERT_MSG(ymax > ymin, "ObjectDetector.Model.Run.InvalidDetectionBoxHeight",
              //               "ymin=%d ymax=%d", ymin, ymax);
              
              // DetectedObject object{
              //   .timestamp = 0, // TODO: Fill in timestamp!  img.GetTimestamp(),
              //   .score     = scores[i],
              //   .name      = _labels[(size_t)classes[i]],
              //   .rect      = Anki::Rectangle<s32>(xmin, ymin, xmax-xmin, ymax-ymin),
              // };

              const int labelIndex = (int)classes[i];

              Json::Value& object = objects[numReturned++];
              object["timestamp"] = 0;
              object["score"] = scores[i];
              if(labelIndex >= 0 && labelIndex < labels.size())
              {
                object["name"] = labels[(size_t)classes[i]];
              }
              else 
              {
                std::cerr << "Invalid label labelIndex=" << labelIndex << " (have " << labels.size() << " labels)" << std::endl;
                object["name"] = "<UNKNOWN>";
              }
              object["xmin"] = box[0];
              object["ymin"] = box[1];
              object["xmax"] = box[2];
              object["ymax"] = box[3];
              

              //std::cout << "ObjectDetector.Model.Run.ObjectDetected: Name:" << object.name << " Score:" << object.score << " Box:[" << 
              //               object.rect.GetX() << " " << object.rect.GetY() << " " << object.rect.GetWidth()<< " " << object.rect.GetHeight() << std::endl;
              
              //objects.push_back(std::move(object));
            }
          }

          if(verbose)
          {
            std::cout << "Returning " << numReturned << " detections with score above " << minScore << ":";
            for(auto const& object : objects)
            {
              std::cout << object["name"].asString() << " ";
            }
            std::cout << std::endl;
          }
        }        
      }

      // Write out the Json
      {
        const std::string jsonFilename = FullFilePath(cachePath, "objectDetectionResults.json");
        if(verbose)
        {
          std::cout << "Writing results to JSON: " << jsonFilename << std::endl;
        }

        Json::StyledStreamWriter writer;
        std::fstream fs;
        fs.open(jsonFilename, std::ios::out);
        if (!fs.is_open()) {
          std::cerr << "Failed to open output file: " << jsonFilename << std::endl;
          return -1;
        }
        writer.write(fs, detectionResult);
        fs.close();
      }

      // Remove the image file we were working with
      if(verbose)
      {
        std::cout << "Deleting image file: " << imageFilename << std::endl;
      }
      remove(imageFilename.c_str());
    }
    else 
    {
      if(verbose)
      {
        const int kVerbosePrintFreq_ms = 1000;
        static int count = 0;
        if(count++ * kPollFrequency_ms >= kVerbosePrintFreq_ms)
        {
          std::cout << "Waiting for image..." << std::endl;
          count = 0;
        }
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(kPollFrequency_ms));  
    }
  }

  return 0;
}
