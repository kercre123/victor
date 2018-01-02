/**
 * File: convolutionalNeuralNet.cpp
 *
 * Author: Andrew Stein
 * Created: 08/13/15
 *
 * Description: Wrapper for Convolutional Neural Net recognition.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#include "coretech/vision/engine/convolutionalNeuralNet.h"

// TODO: Remove this once everyone has matConvNet in coretech-external
#if ENABLE_CNN

#include "coretech/vision/engine/image.h"

#include "coretech/common/engine/jsonTools.h"

#include "util/logging/logging.h"
#include "json/json.h"

#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>

// Includes for vl CNN stuff
#include <nnconv.hpp>
#include <nnfullyconnected.hpp>
#include <nnsubsample.hpp>
#include <nnpooling.hpp>
#include <nnnormalize.hpp>

#include <assert.h>
#include <fstream>

namespace Anki {
namespace Vision {

  
  static vl::Context context;
  
  
#pragma mark -
#pragma mark CNN Layer Definitions
  
  class ICNNLayer
  {
  public:
    ICNNLayer() : verbosity(0) { }
    virtual ~ICNNLayer() { }
    virtual Result Load(const std::string& dirname, const Json::Value& jsonNode) = 0;
    virtual Result Run(const vl::Tensor& inputData) = 0;
    
    vl::Tensor const& GetOutput() const { return _output; }
    
    static ICNNLayer* Factory(const std::string& layerName);
    
  protected:
    
    vl::Tensor _output;
    
    int verbosity;
    
    Result InitOutput(vl::TensorGeometry& outputGeom);
    
  private:

    vl::TensorGeometry _outputGeometry;
    std::vector<float> _outputBuffer;
    
    
  }; // class ICNNLayer
  
  
  class ConvLayer : public ICNNLayer
  {
  public:
    virtual ~ConvLayer() { }
    virtual Result Load(const std::string& dirname, const Json::Value& jsonNode) override;
    virtual Result Run(const vl::Tensor& inputData) override;
    
  private:
    vl::TensorGeometry _inputGeometry;
    
    vl::Tensor _filters;
    vl::Tensor _biases;
    
    std::vector<float> _filterData;
    std::vector<float> _biasData;

    int strideX ;
    int strideY ;
    int padLeft ;
    int padRight;
    int padTop  ;
    int padBottom;
  };


  class NormLayer : public ICNNLayer
  {
  public:
    virtual ~NormLayer() { }
    virtual Result Load(const std::string& dirname, const Json::Value& jsonNode) override;
    virtual Result Run(const vl::Tensor& inputData) override;
    
  private:
    size_t normDepth ;
    double normAlpha ;
    double normKappa ;
    double normBeta ;
    
  };

  class PoolLayer : public ICNNLayer
  {
  public:
    virtual ~PoolLayer() { }
    virtual Result Load(const std::string& dirname, const Json::Value& jsonNode) override;
    virtual Result Run(const vl::Tensor& inputData) override;
    
  private:
    int poolWidth;
    int poolHeight;
    int strideX;
    int strideY;
    int padLeft;
    int padRight;
    int padTop;
    int padBottom;
    vl::PoolingMethod poolMethod;
  };

  class ReLULayer : public ICNNLayer
  {
  public:
    ~ReLULayer() { }
    virtual Result Load(const std::string& dirname, const Json::Value& jsonNode) override;
    virtual Result Run(const vl::Tensor& inputData) override;
    
  };
  
  
#pragma mark -
#pragma mark CNN Implementations

  ICNNLayer* ICNNLayer::Factory(const std::string& layerType)
  {
    if(layerType == "norm") {
      return new NormLayer();
    }
    else if(layerType == "conv") {
      return new ConvLayer();
    }
    else if(layerType == "pool") {
      return new PoolLayer();
    }
    else if(layerType == "relu") {
      return new ReLULayer();
    }
    else {
      PRINT_NAMED_ERROR("ICNNLayer.Factory.BadLayerType",
                        "Unrecognized layer type '%s'", layerType.c_str());
      return nullptr;
    }
  }
  
  Result ICNNLayer::InitOutput(vl::TensorGeometry &outputGeom)
  {
    _outputBuffer.resize(outputGeom.getNumElements());
    _output = vl::Tensor(&(_outputBuffer[0]), _outputBuffer.size()*sizeof(float), vl::CPU, outputGeom);

    return RESULT_OK;
  }
  
  ConvolutionalNeuralNet::ConvolutionalNeuralNet()
  : _isLoaded(false)
  {
    
  }

  ConvolutionalNeuralNet::~ConvolutionalNeuralNet()
  {
    for(auto layer : _layers) {
      assert(layer != nullptr);
      delete layer;
    }
    
    context.clear();
  }

  Result ConvolutionalNeuralNet::Load(std::string dirname)
  {
    Json::Reader reader;
    Json::Value  jsonRoot;
    
    const std::string jsonFilename(dirname + "/definition.json");
    std::ifstream jsonFile(jsonFilename);
    if(false == reader.parse(jsonFile, jsonRoot)) {
      PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Load",
                        "Failed to parse CNN Json file %s",
                        jsonFilename.c_str());
      return RESULT_FAIL;
    } else {
      PRINT_NAMED_INFO("ConvolutionalNeuralNet.Load",
                       "Loaded CNN Json file %s",
                       jsonFilename.c_str());
    }
    jsonFile.close();
    
    if(!jsonRoot.isMember("CNN")) {
      PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Load",
                        "Root Json node should be named 'CNN'");
      return RESULT_FAIL;
    }
    
    Json::Value& jsonCNN = jsonRoot["CNN"];
    
    if(!jsonCNN.isMember("Layers") || !jsonCNN["Layers"].isArray()) {
      PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Load",
                        "Json 'CNN' node should have a 'Layers' array node");
      return RESULT_FAIL;
    }
    
    Json::Value& jsonLayers = jsonCNN["Layers"];
    
    for(s32 i=0; i<jsonLayers.size(); ++i) {
      Json::Value& jsonLayer = jsonLayers[i];
      
      if(!jsonLayer.isMember("Type")) {
        PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Load",
                          "Layer %d is missing a type string", i);
        return RESULT_FAIL;
      }
      
      std::string layerType = JsonTools::GetValue<std::string>(jsonLayer["Type"]);
      
      ICNNLayer* newLayer = ICNNLayer::Factory(layerType);
      
      if(newLayer == nullptr) {
        return RESULT_FAIL;
      }
      
      Result loadResult = newLayer->Load(dirname, jsonLayer);
      
      if(loadResult != RESULT_OK) {
        return RESULT_FAIL;
      }
      
      _layers.push_back(newLayer);
    }
    
    if(jsonCNN.isMember("Classes"))
    {
      Json::Value& jsonClasses = jsonCNN["Classes"];
      if(!jsonClasses.isArray()) {
        PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Load",
                          "Found 'Classes' node, but it is not an array");
      }
      
      for(s32 i=0; i<jsonClasses.size(); ++i) {
        _classNames.push_back(jsonClasses[i].asString());
      }
    }

    _name = dirname.substr(dirname.find_last_of("/")+1, std::string::npos);
    
    PRINT_NAMED_INFO("ConvolutionalNeuralNet.Load",
                     "Loaded %lu CNN layers & %lu class names",
                     _layers.size(), _classNames.size());
    
    _isLoaded = true;
    
    return RESULT_OK;
  } // Load()

  Result ConvolutionalNeuralNet::Run(const cv::Mat& img, size_t& classLabel, std::string& className)
  {
    classLabel = 0;
    className = "UNKNOWN";
    
    std::vector<float> output;
    Result runResult = Run(img, output);
    if(runResult != RESULT_OK) {
      // No error message? Just rely on the other Run() to print its own...
      return runResult;
    }
    
    // Find max output and return its index
    float maxVal = std::numeric_limits<float>::lowest();
    for(s32 i=0; i<output.size(); ++i) {
      if(output[i] > maxVal) {
        classLabel = i;
        maxVal = output[i];
      }
    }
    
    // If we also have classnames, return the corresponding classname as well
    if(!_classNames.empty())
    {
      if(classLabel >= _classNames.size()) {
        PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Run.BadLabel",
                          "Max classLabel out of range for given class names");
        return RESULT_FAIL;
      }
      className = _classNames[classLabel];
    }
    
    return RESULT_OK;
  }
  
  
  Result ConvolutionalNeuralNet::Run(const cv::Mat &img, std::vector<float> &output)
  {
    if(!_isLoaded) {
      PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Run.NotLoaded", "CNN not yet loaded");
      return RESULT_FAIL;
    }
    
    if(img.channels() > 1) {
      PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Run", "Color image data not yet supported");
      return RESULT_FAIL;
    }
    
    vl::TensorGeometry inputGeom(img.rows, img.cols, 1, 1);
    
    std::vector<float> inputData;
    inputData.reserve(img.rows*img.cols);

    s32 nrows = img.rows;
    s32 ncols = img.cols;
    if(img.isContinuous()) {
      ncols *= nrows;
      nrows = 1;
    }
    
    for(s32 i=0; i<nrows; ++i) {
      const u8* img_i = img.ptr<u8>(i);
      for(s32 j=0; j<ncols; ++j) {
        inputData.push_back(static_cast<float>(img_i[j]));
      }
    }
    
    vl::Tensor input(&(inputData[0]), inputData.size()*sizeof(float), vl::CPU, inputGeom);
    
    // Use the image as the "output" of the "last layer" and feed it in as
    // input to the first layer
    const vl::Tensor* lastOutput = &input;
    
    for(auto layer : _layers) {
      vl::Tensor output;
      Result layerResult = layer->Run(*lastOutput);
      
      if(layerResult != RESULT_OK) {
        PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Run.LayerFail", "Layer returned failure");
        return layerResult;
      }
      
      // Use the output of this layer as input to next layer
      lastOutput = &layer->GetOutput();
    }

    // Using const_cast here because vl library doesn't provide a const getMemory() method. :-/
    const float* outputData = const_cast<vl::Tensor*>(lastOutput)->getMemory();
    
    // Copy the result out
    std::copy(outputData, outputData + lastOutput->getNumElements(), std::back_inserter(output));

    return RESULT_OK;
  } // ConvolutionalNeuralNet::Run(grayscale)
  
  

#pragma mark -
#pragma mark ConvLayer Implementation

  
# define READ(__NAME__) do { file.read((char*)&__NAME__, sizeof(__NAME__)); \
if(file.bad()) { PRINT_NAMED_ERROR("FileREAD.Failure", "%s", QUOTE(__NAME__)); return RESULT_FAIL; } } while(0)
  
  Result ReadTensor(std::ifstream& file, vl::Tensor& tensor, std::vector<float>& memory,
                    vl::index_t height, vl::index_t width, vl::index_t depth, vl::index_t size)
  {
/*
    READ(height);
    READ(width);
    READ(depth);
    READ(size);
  */
    vl::TensorGeometry geom(height, width, depth, size);
    
    memory.resize(geom.getNumElements());
    
    const size_t memorySize = memory.size()*sizeof(float);
    file.read((char*)&(memory[0]), memorySize);
    if(file.bad()) {
      return RESULT_FAIL;
    }
    
    tensor = vl::Tensor(&(memory[0]), memorySize, vl::CPU, geom);
    
    return RESULT_OK;
  }
  
#define GET_FROM_JSON(__NAME__) do { \
if(!JsonTools::GetValueOptional(jsonNode, QUOTE(__NAME__), __NAME__)) { \
  PRINT_NAMED_ERROR("ICNNLayer.Load.MissingField", "Field '%s' not found", QUOTE(__NAME__)); \
  return RESULT_FAIL; \
} } while(0);
  
  Result ConvLayer::Load(const std::string& dirname, const Json::Value& jsonNode)
  {
    GET_FROM_JSON(strideX);
    GET_FROM_JSON(strideY);
    GET_FROM_JSON(padLeft);
    GET_FROM_JSON(padRight);
    GET_FROM_JSON(padTop);
    GET_FROM_JSON(padBottom);
    
    /* basic argument checks */
    if (strideX < 1 || strideY < 1) {
      PRINT_NAMED_ERROR("ConvLayer.Load.BadStride", "At least one element of STRIDE is smaller than one") ;
      return RESULT_FAIL;
    }
    if (padLeft < 0 ||
        padRight < 0 ||
        padTop < 0 ||
        padBottom < 0) {
      PRINT_NAMED_ERROR("ConvLayer.Load.BadPadding", "An element of PAD is negative") ;
      return RESULT_FAIL;
    }
    
    vl::index_t weightHeight, weightWidth, weightDepth, weightSize;
    
    GET_FROM_JSON(weightHeight);
    GET_FROM_JSON(weightWidth);
    GET_FROM_JSON(weightDepth);
    GET_FROM_JSON(weightSize);
    
    Result lastResult = RESULT_OK;
    
    std::string DataFilename;
    GET_FROM_JSON(DataFilename);
    std::ifstream file(dirname + "/" + DataFilename, std::ios::binary);
    
    if(!file.is_open()) {
      PRINT_NAMED_ERROR("ConvLayer.Load.FileOpenFailure",
                        "Could not open file %s", DataFilename.c_str());
      return RESULT_FAIL;
    }
    
    lastResult = ReadTensor(file, _filters, _filterData, weightHeight, weightWidth, weightDepth, weightSize);
    if(lastResult != RESULT_OK) {
      return lastResult;
    }
    
    vl::index_t biasHeight, biasWidth;
    
    GET_FROM_JSON(biasHeight);
    GET_FROM_JSON(biasWidth);
    
    lastResult = ReadTensor(file, _biases, _biasData, biasHeight, biasWidth, 1, 1);
    if(lastResult != RESULT_OK) {
      return lastResult;
    }
   
    file.close();
    
    return RESULT_OK;
  }
  
  Result ConvLayer::Run(const vl::Tensor &inputData)
  {
    
    const bool hasFilters = !_filters.isEmpty() ;
    const bool hasBiases = !_biases.isEmpty() ;
    
    /* check for GPU/data class consistency */
    if (hasFilters && ! vl::areCompatible(inputData, _filters)) {
      PRINT_NAMED_ERROR("vl_cnn_wrapper", "DATA and FILTERS are not both CPU or GPU arrays.") ;
      return RESULT_FAIL;
    }
    if (hasBiases && ! vl::areCompatible(inputData, _biases)) {
      PRINT_NAMED_ERROR("vl_cnn_wrapper", "DATA and BIASES are not both CPU or GPU arrays.") ;
      return RESULT_FAIL;
    }
    
    /* Get the filter geometry */
    vl::TensorGeometry filtersGeom(_filters) ;
    int equivalentNumFilters = 0, numFilterGroups = 0;
    if (hasFilters) {
      if (filtersGeom.getHeight() == 0 || filtersGeom.getWidth() == 0 || filtersGeom.getDepth() == 0) {
        PRINT_NAMED_ERROR("vl_cnn_wrapper", "A dimension of FILTERS is void.") ;
        return RESULT_FAIL;
      }
      if (inputData.getHeight() + (padTop+padBottom) < _filters.getHeight() ||
          inputData.getWidth() + (padLeft+padRight) < _filters.getWidth()) {
        PRINT_NAMED_ERROR("vl_cnn_wrapper", "FILTERS are larger than the DATA (including padding).") ;
        return RESULT_FAIL;
      }
      /* grouped filters */
      numFilterGroups = inputData.getDepth() / _filters.getDepth() ;
      if (numFilterGroups * _filters.getDepth() != inputData.getDepth()) {
        PRINT_NAMED_ERROR("vl_cnn_wrapper", "The FILTERS depth does not divide the DATA depth.") ;
        return RESULT_FAIL;
      }
      if (_filters.getSize() % numFilterGroups != 0) {
        PRINT_NAMED_ERROR("vl_cnn_wrapper", "The number of filter groups does not divide the number of filters.") ;
        return RESULT_FAIL;
      }
      equivalentNumFilters = _filters.getSize() ;
    } else {
      /* empty filters -> pretend the identity filter bank */
      filtersGeom = vl::TensorGeometry(1, 1, inputData.getDepth(), inputData.getDepth()) ;
      numFilterGroups = 1 ;
      equivalentNumFilters = inputData.getDepth() ;
    }
    
    assert(numFilterGroups != 0);
    assert(equivalentNumFilters != 0);
    
    /* Get the output geometry */
    const vl::index_t height_out = (inputData.getHeight() + (padTop+padBottom) - filtersGeom.getHeight())/strideY + 1;
    const vl::index_t width_out  = (inputData.getWidth()  + (padLeft+padRight) - filtersGeom.getWidth())/strideX + 1;
    const vl::index_t depth_out  = equivalentNumFilters;
    const vl::index_t size_out   = inputData.getSize();
    vl::TensorGeometry outputGeom(height_out, width_out, depth_out, size_out);
    
    /* Check the biases sizes */
    if (hasBiases) {
      if (_biases.getNumElements() != filtersGeom.getSize()) {
        PRINT_NAMED_ERROR("vl_cnn_wrapper", "The number of elements of BIASES is not the same as the number of filters.") ;
        return RESULT_FAIL;
      }
    }
    
    /*
     Detect fully connected mode (further optimisations):
     the output is 1 x 1 pixels,
     no padding,
     one filter group,
     stride of one pixel
     */
    const bool fullyConnectedMode = (outputGeom.getHeight() == 1 &&
                                     outputGeom.getWidth() == 1 &&
                                     strideY == 1 &&
                                     strideX == 1 &&
                                     padTop == 0 &&
                                     padBottom == 0 &&
                                     padLeft == 0 &&
                                     padRight == 0 &&
                                     numFilterGroups == 1) ;
    
    /* create output buffers */
    Result outResult = InitOutput(outputGeom);
    if(outResult != RESULT_OK) {
      return outResult;
    }
    
    if (verbosity > 0) {
      PRINT_NAMED_INFO("vl_nnconv", "forward: %s", (inputData.getMemoryType()==vl::GPU) ? "GPU" : "CPU") ;
      PRINT_NAMED_INFO("vl_nnconv", "stride: [%d %d], pad: [%d %d %d %d], "
                       "num filter groups: %d, has bias: %d, has filters: %d, is fully connected: %d",
                       strideY, strideX,
                       padTop, padBottom, padLeft, padRight,
                       numFilterGroups, hasBiases, hasFilters, fullyConnectedMode) ;
      //vl::print("vl_nnconv: data: ", data) ;
      //if (hasFilters) { vl::print("vl_nnconv: filters: ", _filters) ; }
      //if (hasBiases) { vl::print("vl_nnconv: biases: ", _biases) ; }
      //vl::print("vl_nnconv: output: ", output) ;
    }
    
    /* -------------------------------------------------------------- */
    /*                                                    Do the work */
    /* -------------------------------------------------------------- */
    
    vl::Error error ;
    
    /*
     special case: fully connected
     (could be done as a regular case, but it is faster this way)
     */
    if (fullyConnectedMode) {
      
      error = vl::nnfullyconnected_forward(context,
                                           _output,
                                           inputData,
                                           _filters,
                                           _biases) ;
    }
    
    /* special case: no filters = identity filter bank (subsample + bias) */
    else if (!hasFilters) {
      
      error = vl::nnsubsample_forward(context,
                                      _output,
                                      inputData,
                                      _biases,
                                      strideY, strideX,
                                      padTop, padBottom, padLeft, padRight) ;
    }
    
    /* regular case */
    else {
      error = vl::nnconv_forward(context,
                                 _output, 0,
                                 inputData, 1,
                                 _filters,
                                 _biases,
                                 strideY, strideX,
                                 padTop, padBottom, padLeft, padRight) ;
    }
    
    /* -------------------------------------------------------------- */
    /*                                                        Cleanup */
    /* -------------------------------------------------------------- */
    
    if (error != vl::vlSuccess) {
      PRINT_NAMED_ERROR("ConvLayer.Run", "nnconv_forward failed: %s", context.getLastErrorMessage().c_str()) ;
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  } // ConvLayer::Run()
  
  
#pragma mark -
#pragma mark PoolLayer Implementation
  
  Result PoolLayer::Load(const std::string& dirname, const Json::Value& jsonNode)
  {
    GET_FROM_JSON(poolWidth);
    GET_FROM_JSON(poolHeight);
    GET_FROM_JSON(strideX);
    GET_FROM_JSON(strideY);
    GET_FROM_JSON(padLeft);
    GET_FROM_JSON(padRight);
    GET_FROM_JSON(padTop);
    GET_FROM_JSON(padBottom);
    
    std::string method;
    GET_FROM_JSON(method);
    if(method == "max") {
      poolMethod = vl::vlPoolingMax;
    } else if(method == "avg" || method == "mean") {
      poolMethod = vl::vlPoolingAverage;
    } else {
      PRINT_NAMED_ERROR("PoolLayer.Load.BadMethod", "Unrecognized pooling method '%s'", method.c_str());
      return RESULT_FAIL;
    }
    
    if (strideX < 1 || strideY < 1) {
      PRINT_NAMED_ERROR("PoolLayer.Load.BadStride", "At least one element of STRIDE is smaller than one") ;
      return RESULT_FAIL;
    }
    if (poolHeight == 0 || poolWidth == 0) {
      PRINT_NAMED_ERROR("PoolLayer.Load.BadPoolDims", "A dimension of the pooling SIZE is void") ;
      return RESULT_FAIL;
    }
    if (padLeft < 0 ||
        padRight < 0 ||
        padTop < 0 ||
        padBottom < 0) {
      PRINT_NAMED_ERROR("PoolLayer.Load.BadPadding", "An element of PAD is negative") ;
      return RESULT_FAIL;
    }
    if (padLeft >= poolWidth ||
        padRight >= poolWidth ||
        padTop >= poolHeight  ||
        padBottom >= poolHeight) {
      PRINT_NAMED_ERROR("PoolLayer.Load.IncompatiblePadAndPool", "A padding value is larger or equal than the size of the pooling window") ;
      return RESULT_FAIL;
    }
    
    return RESULT_OK;

  } // PoolLayer::Load()
  
  Result PoolLayer::Run(const vl::Tensor &inputData)
  {
    if (inputData.getHeight() + (padTop+padBottom) < poolHeight ||
        inputData.getWidth() + (padLeft+padRight) < poolWidth)
    {
      PRINT_NAMED_ERROR("PoolLayer.Run.InputDimMismatch", "The pooling window is larger than the DATA (including padding).") ;
      return RESULT_FAIL;
    }
    
    /* Get the output geometry */
    vl::TensorGeometry outputGeom((inputData.getHeight() + (padTop+padBottom) - poolHeight)/strideY + 1,
                                  (inputData.getWidth()  + (padLeft+padRight) - poolWidth)/strideX + 1,
                                  inputData.getDepth(),
                                  inputData.getSize()) ;
    
    
    Result outResult = InitOutput(outputGeom);
    if(outResult != RESULT_OK) {
      return outResult;
    }
    
    if (verbosity > 0) {
      PRINT_NAMED_INFO("vl_nnpool:", "forward; %s", (inputData.getMemoryType()==vl::GPU) ? "GPU" : "CPU") ;
      PRINT_NAMED_INFO("vl_nnpool:", "stride: [%d %d], pad: [%d %d %d %d]",
                strideY, strideX,
                padTop, padBottom, padLeft, padRight) ;
//      vl::print("vl_nnpool: data: ", data) ;
//      mexPrintf("vl_nnpool: pooling: %d x %d\n", poolHeight, poolWidth);
//      mexPrintf("vl_nnpool: method: %s\n", (method == vl::vlPoolingMax) ? "max" : "avg") ;
//      if (backMode) {
//        vl::print("vl_nnpool: derOutput: ", derOutput) ;
//        vl::print("vl_nnpool: derData: ", derData) ;
//      } else {
//        vl::print("vl_nnpool: output: ", output) ;
//      }
    }
    
    vl::Error error ;
    
    error = vl::nnpooling_forward(context,
                                  _output, inputData,
                                  poolMethod,
                                  poolHeight, poolWidth,
                                  strideY, strideX,
                                  padTop, padBottom, padLeft, padRight) ;
    
    if (error != vl::vlSuccess) {
      PRINT_NAMED_ERROR("PoolLayer.Run", "nnpooling_forward failed: %s", context.getLastErrorMessage().c_str()) ;
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  } // PoolLayer::Run()
  
  
  
#pragma mark -
#pragma mark NormLayer Implementation

  
  Result NormLayer::Load(const std::string& dirname, const Json::Value& jsonNode)
  {
    GET_FROM_JSON(normDepth);
    GET_FROM_JSON(normAlpha);
    GET_FROM_JSON(normKappa);
    GET_FROM_JSON(normBeta);
    
    if (normDepth < 1) {
      PRINT_NAMED_ERROR("NormLayer.Load.BadDepth", "The normalization depth is smaller than 1.") ;
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  } // NormLayer::Load()
  
  
  Result NormLayer::Run(const vl::Tensor &inputData)
  {
    vl::TensorGeometry outputGeom = inputData.getGeometry();
    Result outResult = InitOutput(outputGeom);
    if(outResult != RESULT_OK) {
      return outResult;
    }
    
    if (verbosity > 0) {
      PRINT_NAMED_INFO("vl_nnnormalize:", "mode %s; %s",  (inputData.getMemoryType()==vl::GPU)?"gpu":"cpu", "forward") ;
      PRINT_NAMED_INFO("vl_nnnormalize:", "(depth,kappa,alpha,beta): (%lu,%g,%g,%g)",
                normDepth, normKappa, normAlpha, normBeta) ;
//      vl::print("vl_nnnormalize: data: ", data) ;
//      if (backMode) {
//        vl::print("vl_nnnormalize: derOutput: ", derOutput) ;
//        vl::print("vl_nnnormalize: derData: ", derData) ;
//      } else {
//        vl::print("vl_nnnormalize: output: ", output) ;
//      }
    }
    
    vl::Error error = vl::nnnormalize_forward(context,
                                              _output, inputData,
                                              normDepth, normKappa, normAlpha, normBeta) ;
    
    
    /* -------------------------------------------------------------- */
    /*                                                         Finish */
    /* -------------------------------------------------------------- */
    
    if (error != vl::vlSuccess) {
      PRINT_NAMED_ERROR("NormLayer.Run", "nnnormalize_forward failed: %s", context.getLastErrorMessage().c_str()) ;
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  } // NormLayer::Run()
  
  
#pragma mark -
#pragma mark Rectified Linear Unit (ReLU) Layer Implementation
  
  Result ReLULayer::Load(const std::string& dirname, const Json::Value& jsonNode)
  {
    return RESULT_OK;
  }
  
  Result ReLULayer::Run(const vl::Tensor &inputData)
  {
    vl::TensorGeometry outputGeom = inputData.getGeometry();
    Result lastResult = InitOutput(outputGeom);
    if(lastResult != RESULT_OK) {
      return lastResult;
    }
    
    // Using const_cast here because vl::Tensor does not provide a const getMemory() method
    const float* input = const_cast<vl::Tensor&>(inputData).getMemory();
    float *output = _output.getMemory();
    for(s32 i=0; i<inputData.getNumElements(); ++i) {
      output[i] = std::max(0.f, input[i]);
    }
    
    return lastResult;
  }
  
  
} // namespace Vision
} // namespace Anki

#endif // ENABLE_CNN
