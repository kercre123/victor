 #include "objectDetector_caffe2.h"
#include "helpers.h"

#include "caffe2/core/flags.h"
#include "caffe2/core/init.h"
#include "caffe2/utils/proto_utils.h"

#include <cmath>
#include <fstream>

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
static inline void SetFromConfigHelper(const Json::Value& json, int32_t& value) {
  value = json.asInt();
}

static inline void SetFromConfigHelper(const Json::Value& json, float& value) {
  value = json.asFloat();
}

static inline void SetFromConfigHelper(const Json::Value& json, bool& value) {
  value = json.asBool();
}

static inline void SetFromConfigHelper(const Json::Value& json, std::string& value) {
  value = json.asString();
}

static inline void SetFromConfigHelper(const Json::Value& json, uint8_t& value) {
  value = json.asUInt();
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::ObjectDetector() 
: _params{} 
{

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
ObjectDetector::~ObjectDetector()
{
  std::cout << "Destructing ObjectDetector" << std::endl;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::LoadModel(const std::string& modelPath, const Json::Value& config)
{
 
# define GetFromConfig(keyName) \
  if(!config.isMember(QUOTE(keyName))) \
  { \
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadModel.MissingConfig", QUOTE(keyName)); \
    return RESULT_FAIL; \
  } \
  else \
  { \
    SetFromConfigHelper(config[QUOTE(keyName)], _params.keyName); \
  }

  GetFromConfig(verbose);
  GetFromConfig(labels);
  GetFromConfig(min_score);  
  GetFromConfig(graph);
  GetFromConfig(input_height);
  GetFromConfig(input_width);
  GetFromConfig(architecture);
  GetFromConfig(memoryMapGraph);

  if("ssd_mobilenet" == _params.architecture)
  {
    _inputLayerName = "image_tensor";
    _outputLayerNames = {"detection_scores", "detection_classes", "detection_boxes", "num_detections"};
    _useFloatInput = false;
  }
  else if("mobilenet" == _params.architecture)
  { 
    _inputLayerName = "input";
    _outputLayerNames = {"MobilenetV1/Predictions/Softmax"};
    _useFloatInput = true;
  }
  else
  {
    PRINT_NAMED_ERROR("ObjectDetector.LoadModel.UnrecognizedArchitecture", "%s", 
                      _params.architecture.c_str());
    return RESULT_FAIL;
  }

  if(_params.verbose)
  {
    std::cout << "Input: " << _inputLayerName << ", Outputs: ";
    for(auto const& outputLayerName : _outputLayerNames)
    {
      std::cout << outputLayerName << " ";
    }
    std::cout << std::endl;
  }

  if ("mobilenet" == _params.architecture)
  {
    // Only required in classification mode
    GetFromConfig(input_mean_R);
    GetFromConfig(input_mean_G);
    GetFromConfig(input_mean_B);
    GetFromConfig(input_std);
  }
  
  const std::string init_net_filename = FullFilePath(modelPath, FullFilePath(_params.graph, "init_net.pb"));
  const std::string predict_net_filename = FullFilePath(modelPath, FullFilePath(_params.graph, "predict_net.pb"));

  if (!FileExists(init_net_filename))
                  
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.InitNetFileDoesNotExist", "%s",
                      init_net_filename.c_str());
    return RESULT_FAIL;
  }


  if (!FileExists(predict_net_filename))
                  
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.PredictNetFileDoesNotExist", "%s",
                      predict_net_filename.c_str());
    return RESULT_FAIL;
  }

  if(_params.verbose)
  {
    std::cout << "Found graph files: " << init_net_filename << " + " << predict_net_filename << std::endl;
  }

  caffe2::NetDef init_net, predict_net;
  if(!ReadProtoFromFile(init_net_filename, &init_net))
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.ReadInitNetFailed", "");
    return RESULT_FAIL;
  }
  if(!ReadProtoFromFile(predict_net_filename, &predict_net))
  {
    PRINT_NAMED_ERROR("ObjectDetector.Model.LoadGraph.ReadPredictNetFailed", "");
    return RESULT_FAIL;
  }
  
  std::cout << "ObjectDetector.Model.LoadGraph.ProtoFilesLoaded" << std::endl;

  
  std::cout << "PredictNet has " << predict_net.op_size() << " ops" << std::endl;
  
  // enable NNPACk on any conv layers:
  // See also: https://github.com/caffe2/caffe2/issues/726
  //           https://caffe2.ai/docs/mobile-integration.html#null__performance-considerations
  for(int i=0; i < predict_net.op_size(); ++i)
  {
    if ("Conv" == predict_net.op(i).type())
    {
      std::cout << "Setting Conv op " << i << " to use NNPACK engine" << std::endl;
      predict_net.mutable_op(i)->set_engine("NNPACK");
      //std::cout << "Conv engine used:" << predict_net.op(i).has_engine() << std::endl;
      //std::cout << "Conv engine set:" << predict_net.op(i).engine() << std::endl;
    }
    else if ("ConvTranspose" == predict_net.op(i).type())
    {
      std::cout << "Setting ConvTranspose op " << i << " to use BLOCK engine" << std::endl;
      predict_net.mutable_op(i)->set_engine("BLOCK");
    }
  }
  // Can be large due to constant fills
  _predictor = caffe2::make_unique<caffe2::Predictor>(init_net, predict_net);
  
  std::cout << "ObjectDetector.Model.LoadGraph.PredictorCreated" << std::endl;

  const std::string labels_file_name = FullFilePath(modelPath, _params.labels);
  Result readLabelsResult = ReadLabelsFile(labels_file_name, _labels);
  if (RESULT_OK == readLabelsResult)
  {
    std::cout << "ObjectDetector.Model.LoadGraph.ReadLabelFileSuccess " << labels_file_name.c_str() << std::endl;
  }
  return readLabelsResult;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out)
{
  std::ifstream file(fileName);
  if (!file)
  {
    PRINT_NAMED_ERROR("ObjectDetector.ReadLabelsFile.LabelsFileNotFound: ", "%s", fileName.c_str());
    return RESULT_FAIL;
  }
  
  labels_out.clear();
  std::string line;
  while (std::getline(file, line)) {
    labels_out.push_back(line);
  }
  
  std::cout << "ReadLabelsFile read " << labels_out.size() << " labels" << std::endl;

  return RESULT_OK;
}


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectDetector::GetClassification(const caffe2::Predictor::TensorVector& output_vec, 
                                       TimeStamp_t timestamp, 
                                       std::list<DetectedObject>& objects)
{

  // Find top result above min_score
  float maxScore = _params.min_score;
  int labelIndex = -1;
  
  const float* output = output_vec[0]->template data<float>();  
  for(auto i=0; i < output_vec[0]->size(); ++i)
  {
    if(output[i] > maxScore)
    {
      maxScore = output[i];
      labelIndex = i;
    }
  }
  
  if(labelIndex >= 0)
  {    
    DetectedObject object{
      .timestamp = timestamp,
      .score = maxScore,
      .name = (labelIndex < _labels.size() ? _labels.at((size_t)labelIndex) : "<UNKNOWN>"),
      .xmin = 0, .ymin = 0, .xmax = 1, .ymax = 1,
    };
    
    if(_params.verbose)
    {
      std::cout << "Found " << object.name << "[" << labelIndex << "] with score=" << object.score << std::endl;
    }

    objects.push_back(std::move(object));
  }
  else if(_params.verbose)
  {
    std::cout << "Nothing found above min_score=" << _params.min_score << std::endl;
  }
}

/*
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void ObjectDetector::GetDetectedObjects(const std::vector<tensorflow::Tensor>& output_tensors, TimeStamp_t timestamp,
                                        std::list<DetectedObject>& objects)
{
  assert(output_tensors.size() == 4);

  const int numDetections = (int)output_tensors[3].tensor<float,1>().data()[0];

  if(_params.verbose)
  {
    std::cout << "Got " << numDetections << " raw detections" << std::endl;
  }

  if(numDetections > 0)
  {
    const float* scores  = output_tensors[0].tensor<float, 2>().data();
    const float* classes = output_tensors[1].tensor<float, 2>().data();
    
    auto const& boxesTensor = output_tensors[2].tensor<float, 3>();
    
    const float* boxes = boxesTensor.data();
    
    for(int i=0; i<numDetections; ++i)
    {
      if(scores[i] >= _params.min_score)
      {
        const float* box = boxes + (4*i);
        
        const size_t labelIndex = (size_t)(classes[i]);

        objects.emplace_back(DetectedObject{
          .timestamp = 0, // TODO: Fill in timestamp!  img.GetTimestamp(),
          .score     = scores[i],
          .name      = (labelIndex < _labels.size() ? _labels[labelIndex] : "<UNKNOWN>"),
          .xmin = box[0], .ymin = box[1], .xmax = box[2], .ymax = box[3],
        });
      }
    }

    if(_params.verbose)
    {
      std::cout << "Returning " << objects.size() << " detections with score above " << _params.min_score << ":";
      for(auto const& object : objects)
      {
        std::cout << object.name << " ";
      }
      std::cout << std::endl;
    }
  } 
}
*/


// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result ObjectDetector::Detect(cv::Mat& img, std::list<DetectedObject>& objects)
{
  // TODO: Resize and convert directly into the tensor
  cv::resize(img, img, cv::Size(_params.input_width,_params.input_height), 0, 0, CV_INTER_AREA);
  assert(img.isContinuous());
  assert(img.channels() == 3);

  img.convertTo(img, CV_32FC3, 1.0, -128.f);

  std::vector<caffe2::TIndex> dims({1, img.channels(), img.rows, img.cols});
  caffe2::TensorCPU input(dims);

  float* input_data = input.mutable_data<float>();

  // convert NHWC to NCHW
  std::vector<cv::Mat> channels(3);
  cv::split(img, channels);
  int offset = 0;
  for (auto &c : channels) {
    memcpy(input_data + offset, (float*)c.datastart, c.rows*c.cols*sizeof(float));
    offset += c.rows*c.cols;
  }

  // bool infer_HWC = false;

  
  // if (infer_HWC) {
  //   input.Resize(std::vector<int>({img.rows, img.cols, img.channels()}));
  // }
  // else {
  //   input.Resize(std::vector<int>({1, img.channels(), img.rows, img.cols}));
  // }

  // // Adjust according to mean/std, convert to float, and put in NCHW ordering
  // {
  //   float* input_data = input.mutable_data<float>();

  //   const float divisor = 1.f / _params.input_std;

  //   for(int i=0; i<img.rows; ++i)
  //   {
  //     const uint8_t* img_i = img.ptr<uint8_t>(i);

  //     int col=0;
  //     for(int j=0; j<img.cols*img.channels(); j+=3, ++col)
  //     {
  //       const int baseIndex = col*img.cols + i;
  //       int b_i, g_i, r_i;
        
  //       if (infer_HWC) {
  //         b_i = baseIndex * img.cols;
  //         g_i = baseIndex * img.cols + 1;
  //         r_i = baseIndex * img.cols + 2;
  //       }
  //       else {
  //         b_i = baseIndex;
  //         g_i = baseIndex + (img.rows*img.cols);
  //         r_i = baseIndex + 2*(img.rows*img.cols);
  //       }

  //       // NOTE: Assumes BGR ordering
  //       input_data[b_i] = ((float)img_i[j+0] - _params.input_mean_B) * divisor;
  //       input_data[g_i] = ((float)img_i[j+1] - _params.input_mean_G) * divisor;
  //       input_data[r_i] = ((float)img_i[j+2] - _params.input_mean_R) * divisor;
  //     }
     
  //   }
  // }
   
  caffe2::Predictor::TensorVector input_vec{&input};
  caffe2::Predictor::TensorVector output_vec;
  _predictor->run(input_vec, &output_vec);

  // TODO: Get timestamp somehow
  const TimeStamp_t t = 0;

  GetClassification(output_vec, t, objects);

  if(_params.verbose)
  {
    std::cout << "Session complete" << std::endl;
  }

  return RESULT_OK;
}
