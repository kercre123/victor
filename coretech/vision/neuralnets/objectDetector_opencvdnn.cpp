#include "objectDetector_opencvdnn.h"
#include "helpers.h"

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
  
  // Only required in classification mode
  GetFromConfig(input_mean_R);
  GetFromConfig(input_mean_G);
  GetFromConfig(input_mean_B);
  GetFromConfig(input_std);

  std::string modelType = "UNKNOWN";
  
  if(_params.graph.substr(_params.graph.size()-3,3) == ".pb")
  {
    // Tensorflow models:
    const std::string graphFileName = FullFilePath(modelPath,_params.graph);
    const std::string graphTxtFileName = FullFilePath(modelPath,_params.graph + "txt");
    if(FileExists(graphTxtFileName)) {
      _network = cv::dnn::readNetFromTensorflow(graphFileName, graphTxtFileName);
    }
    else {
      _network = cv::dnn::readNetFromTensorflow(graphFileName);
    }
    modelType = "TensorFlow";
  }
  else if(_params.graph.substr(_params.graph.size()-4,4) == ".cfg")
  {
    // Darknet Model
    const std::string modelConfiguration = FullFilePath(modelPath,_params.graph);
    const std::string modelBinary = FullFilePath(modelPath,_params.graph.substr(0,_params.graph.size()-4) + ".weights");
    _network = cv::dnn::readNetFromDarknet(modelConfiguration, modelBinary);
    modelType = "Darknet";
  }
  else
  {
    // Caffe models:
    const std::string protoFileName = FullFilePath(modelPath,_params.graph + ".prototxt");
    const std::string modelFileName = FullFilePath(modelPath,_params.graph + ".caffemodel");
    _network = cv::dnn::readNetFromCaffe(protoFileName, modelFileName);
    modelType = "Caffe";
  }
  
  if(_network.empty())
  {
    return RESULT_FAIL;
  }
  
  std::cout << "ObjectDetector.Model.OpenCvDNN.LoadedGraph: " << modelType << ": " << _params.graph << std::endl;

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
  if (!file.is_open())
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
void ObjectDetector::GetClassification(const cv::Mat& detections, 
                                       TimeStamp_t timestamp, 
                                       std::list<DetectedObject>& objects)
{
  int labelIndex = -1;
  float maxScore = _params.min_score;
  const float* scores = detections.ptr<float>(0);
  
  // NOTE: this is more general / safer than using cols. (Some architectures seem to output a 4D tensor
  //       and not set cols)
  const int numDetections = detections.size[1];
  
  assert(numDetections == _labels.size());
  
  for(int i=0; i<numDetections; ++i)
  {
    if(scores[i] > maxScore)
    {
      labelIndex = i;
      maxScore = scores[i];
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
    
    objects.emplace_back(std::move(object));
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
  const cv::Size processingSize(_params.input_width, _params.input_height);
  const float scale = 1.f / (float)_params.input_std;
  const cv::Scalar mean(_params.input_mean_R, _params.input_mean_G, _params.input_mean_B);
  const bool kSwapRedBlue = false; // Our image class is already RGB
  cv::Mat dnnBlob = cv::dnn::blobFromImage(img, scale, processingSize, mean, kSwapRedBlue);
  
  _network.setInput(dnnBlob);
  
  cv::Mat detections = _network.forward();
    
  // TODO: Get timestamp somehow
  const TimeStamp_t t = 0;

  GetClassification(detections, t, objects);

  if(_params.verbose)
  {
    std::cout << "Session complete" << std::endl;
  }

  return RESULT_OK;
}
