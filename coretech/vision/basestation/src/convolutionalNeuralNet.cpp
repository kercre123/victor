#include "anki/vision/basestation/convolutionalNeuralNet.h"
#include "anki/vision/basestation/image.h"

#include "anki/common/basestation/jsonTools.h"

#include "util/logging/logging.h"
#include "json/json.h"

#include <opencv2/core/core.hpp>

// Includes for vl CNN stuff
// TODO: Add matconvnet/matlab/src/ to include path
//#include "/Users/andrew/Code/matconvnet/matlab/src/bits/mexutils.h"
//#include "/Users/andrew/Code/matconvnet/matlab/src/bits/datamex.hpp"
#include "/Users/andrew/Code/matconvnet/matlab/src/bits/nnconv.hpp"
#include "/Users/andrew/Code/matconvnet/matlab/src/bits/nnfullyconnected.hpp"
#include "/Users/andrew/Code/matconvnet/matlab/src/bits/nnsubsample.hpp"
#include "/Users/andrew/Code/matconvnet/matlab/src/bits/nnpooling.hpp"
#include "/Users/andrew/Code/matconvnet/matlab/src/bits/nnnormalize.hpp"

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
    virtual ~ICNNLayer() { }
    virtual Result Load(std::string filename) = 0;
    virtual Result Run(const vl::Tensor& inputData) = 0;
    
    vl::Tensor const& GetOutput() const { return _output; }
    
    static ICNNLayer* Factory(const std::string& layerName);
    
  protected:
    
    vl::Tensor _output;
    
    int verbosity = 0 ;
    
    Result InitOutput(vl::TensorGeometry& outputGeom);
    
  private:

    vl::TensorGeometry _outputGeometry;
    std::vector<float> _outputBuffer;
    
    
  }; // class ICNNLayer
  
  
  class ConvLayer : public ICNNLayer
  {
  public:
    virtual ~ConvLayer() { }
    virtual Result Load(std::string filename) override;
    virtual Result Run(const vl::Tensor& inputData) override;
    
  private:
    vl::TensorGeometry _inputGeometry;
    
    vl::Tensor _filters;
    vl::Tensor _biases;
    
    std::vector<float> _filterData;
    std::vector<float> _biasData;

    int strideX = 1 ;
    int strideY = 1 ;
    int padLeft = 0 ;
    int padRight = 0 ;
    int padTop = 0 ;
    int padBottom = 0 ;
  };


  class NormLayer : public ICNNLayer
  {
  public:
    virtual ~NormLayer() { }
    virtual Result Load(std::string filename) override;
    virtual Result Run(const vl::Tensor& inputData) override;
    
  private:
    size_t _normDepth ;
    double _normAlpha ;
    double _normKappa ;
    double _normBeta ;
    
    
  };

  class PoolLayer : public ICNNLayer
  {
  public:
    virtual ~PoolLayer() { }
    virtual Result Load(std::string filename) override;
    virtual Result Run(const vl::Tensor& inputData) override;
    
  private:
    int poolWidth ;
    int poolHeight ;
    int strideX = 1 ;
    int strideY = 1 ;
    int padLeft = 0 ;
    int padRight = 0 ;
    int padTop = 0 ;
    int padBottom = 0 ;
    vl::PoolingMethod method = vl::vlPoolingMax ;
  };

  class ReLULayer : public ICNNLayer
  {
  public:
    ~ReLULayer() { }
    virtual Result Load(std::string filename) override;
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
                        "Unrecognaized layer type '%s'.\n", layerType.c_str());
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
  }

  Result ConvolutionalNeuralNet::Load(std::string jsonFilename)
  {
    Json::Reader reader;
    Json::Value  jsonRoot;
    
    std::ifstream jsonFile(jsonFilename);
    if(false == reader.parse(jsonFile, jsonRoot)) {
      PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Load",
                        "Failed to parse CNN Json file %s.\n",
                        jsonFilename.c_str());
      return RESULT_FAIL;
    } else {
      PRINT_NAMED_INFO("ConvolutionalNeuralNet.Load",
                       "Loaded CNN Json file %s.\n",
                       jsonFilename.c_str());
    }
    jsonFile.close();
    
    if(!jsonRoot.isMember("CNN")) {
      PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Load",
                        "Root Json node should be named 'CNN'.\n");
      return RESULT_FAIL;
    }
    
    Json::Value& jsonCNN = jsonRoot["CNN"];
    
    if(!jsonCNN.isArray()) {
      PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Load",
                        "Json 'CNN' node should be an array of layers.\n");
      return RESULT_FAIL;
    }
    
    for(s32 i=0; i<jsonCNN.size(); ++i) {
      Json::Value& jsonLayer = jsonCNN[i];
      
      if(!jsonLayer.isMember("Type")) {
        PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Load",
                          "Layer %d is missing a type string.\n", i);
        return RESULT_FAIL;
      }
      if(!jsonLayer.isMember("DataFilename")) {
        PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Load",
                          "Layer %d is missing a data filename string.\n", i);
        return RESULT_FAIL;
      }
      
      std::string layerType = JsonTools::GetValue<std::string>(jsonLayer["Type"]);
      
      ICNNLayer* newLayer = ICNNLayer::Factory(layerType);
      
      if(newLayer == nullptr) {
        return RESULT_FAIL;
      }
      
      Result loadResult = newLayer->Load(jsonLayer["DataFilename"].asString());
      
      if(loadResult != RESULT_OK) {
        return RESULT_FAIL;
      }
      
      _layers.push_back(newLayer);
    }
    
    jsonFile.close();
    
    PRINT_NAMED_INFO("ConvolutionalNeuralNet.Load", "Loaded %lu CNN layers.\n", _layers.size());
    
    _isLoaded = true;
    
    return RESULT_OK;
  } // Load()

  Result ConvolutionalNeuralNet::Run(const cv::Mat &img, std::vector<float> &output)
  {
    if(img.channels() > 1) {
      PRINT_NAMED_ERROR("ConvolutionalNeuralNet.Run", "Color image data not yet supported.\n");
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
if(file.bad()) { PRINT_NAMED_ERROR("FileREAD.Failure", "%s.\n", QUOTE(__NAME__)); return RESULT_FAIL; } } while(0)
  
  Result ReadTensor(std::ifstream& file, vl::Tensor& tensor, std::vector<float>& memory)
  {
    vl::index_t height, width, depth, size;
    
    READ(height);
    READ(width);
    READ(depth);
    READ(size);
    
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
  
  
  Result ConvLayer::Load(std::string filename)
  {
    std::ifstream file(filename, std::ios::binary);
    
    if(!file.is_open()) {
      PRINT_NAMED_ERROR("ConvLayer.Load.FileOpenFailure",
                        "Could not open file %s.\n", filename.c_str());
      return RESULT_FAIL;
    }
    
    READ(strideX);
    READ(strideY);
    READ(padLeft);
    READ(padRight);
    READ(padTop);
    READ(padBottom);
    
    /* basic argument checks */
    if (strideX < 1 || strideY < 1) {
      PRINT_NAMED_ERROR("ConvLayer.Load.BadStride", "At least one element of STRIDE is smaller than one.") ;
      return RESULT_FAIL;
    }
    if (padLeft < 0 ||
        padRight < 0 ||
        padTop < 0 ||
        padBottom < 0) {
      PRINT_NAMED_ERROR("ConvLayer.Load.BadPadding", "An element of PAD is negative.") ;
      return RESULT_FAIL;
    }
    
    Result lastResult = RESULT_OK;
    
    lastResult = ReadTensor(file, _filters, _filterData);
    if(lastResult != RESULT_OK) {
      return lastResult;
    }
    
    lastResult = ReadTensor(file, _biases, _biasData);
    if(lastResult != RESULT_OK) {
      return lastResult;
    }
   
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
      PRINT_NAMED_INFO("vl_nnconv", "forward; %s", (inputData.getMemoryType()==vl::GPU) ? "GPU" : "CPU") ;
      PRINT_NAMED_INFO("vl_nnconv", "stride: [%d %d], pad: [%d %d %d %d]\n"
                       "vl_nnconv: num filter groups: %d, has bias: %d, has filters: %d, is fully connected: %d\n",
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
      PRINT_NAMED_ERROR("ConvLayer.Run", "nnconv_forward failed: %s\n", context.getLastErrorMessage().c_str()) ;
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  } // ConvLayer::Run()
  
  
#pragma mark -
#pragma mark PoolLayer Implementation
  
  Result PoolLayer::Load(std::string filename)
  {
    std::ifstream file(filename, std::ios::binary);
    
    if(!file.is_open()) {
      PRINT_NAMED_ERROR("PoolLayer.Load.FileOpenFailure",
                        "Could not open file %s.\n", filename.c_str());
      return RESULT_FAIL;
    }
    
    READ(poolWidth);
    READ(poolHeight);
    READ(strideX);
    READ(strideY);
    READ(padLeft);
    READ(padRight);
    READ(padTop);
    READ(padBottom);
    READ(method);
    
    if (strideX < 1 || strideY < 1) {
      PRINT_NAMED_ERROR("PoolLayer.Load.BadStride", "At least one element of STRIDE is smaller than one.") ;
      return RESULT_FAIL;
    }
    if (poolHeight == 0 || poolWidth == 0) {
      PRINT_NAMED_ERROR("PoolLayer.Load.BadPoolDims", "A dimension of the pooling SIZE is void.") ;
      return RESULT_FAIL;
    }
    if (padLeft < 0 ||
        padRight < 0 ||
        padTop < 0 ||
        padBottom < 0) {
      PRINT_NAMED_ERROR("PoolLayer.Load.BadPading", "An element of PAD is negative.") ;
      return RESULT_FAIL;
    }
    if (padLeft >= poolWidth ||
        padRight >= poolWidth ||
        padTop >= poolHeight  ||
        padBottom >= poolHeight) {
      PRINT_NAMED_ERROR("PoolLayer.Load.IncompatiblePadAndPool", "A padding value is larger or equal than the size of the pooling window.") ;
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
      PRINT_NAMED_INFO("vl_nnpool:", "stride: [%d %d], pad: [%d %d %d %d]\n",
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
                                  method,
                                  poolHeight, poolWidth,
                                  strideY, strideX,
                                  padTop, padBottom, padLeft, padRight) ;
    
    if (error != vl::vlSuccess) {
      PRINT_NAMED_ERROR("PoolLayer.Run", "nnpooling_forward failed: %s\n", context.getLastErrorMessage().c_str()) ;
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  } // PoolLayer::Run()
  
  
  
#pragma mark -
#pragma mark NormLayer Implementation

  
  Result NormLayer::Load(std::string filename)
  {
    std::ifstream file(filename, std::ios::binary);
    
    if(!file.is_open()) {
      PRINT_NAMED_ERROR("NormLayer.Load.FileOpenFailure",
                        "Could not open file %s.\n", filename.c_str());
      return RESULT_FAIL;
    }
    
    READ(_normDepth);
    READ(_normAlpha);
    READ(_normKappa);
    READ(_normBeta);
    
    if (_normDepth < 1) {
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
      PRINT_NAMED_INFO("vl_nnnormalize:", "mode %s; %s\n",  (inputData.getMemoryType()==vl::GPU)?"gpu":"cpu", "forward") ;
      PRINT_NAMED_INFO("vl_nnnormalize:", "(depth,kappa,alpha,beta): (%lu,%g,%g,%g)\n",
                _normDepth, _normKappa, _normAlpha, _normBeta) ;
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
                                              _normDepth, _normKappa, _normAlpha, _normBeta) ;
    
    
    /* -------------------------------------------------------------- */
    /*                                                         Finish */
    /* -------------------------------------------------------------- */
    
    if (error != vl::vlSuccess) {
      PRINT_NAMED_ERROR("NormLayer.Run", "nnnormalize_forward failed: %s\n", context.getLastErrorMessage().c_str()) ;
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  } // NormLayer::Run()
  
  
#pragma mark -
#pragma mark Rectified Linear Unit (ReLU) Layer Implementation
  
  Result ReLULayer::Load(std::string filename)
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
