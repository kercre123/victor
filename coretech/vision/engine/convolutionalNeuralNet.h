/**
 * File: convolutionalNeuralNet.h
 *
 * Author: Andrew Stein
 * Created: 08/13/15
 *
 * Description: Wrapper for Convolutional Neural Net recognition.
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __Anki_Vision_Basestation_CNN_H__
#define __Anki_Vision_Basestation_CNN_H__

#include "coretech/common/shared/types.h"

#include <string>
#include <vector>

// TODO: Remove this once everyone has matConvNet in coretech-external
#define ENABLE_CNN 0

// Forward declaration
namespace cv {
  class Mat;
}

namespace Anki {
namespace Vision {
  
  // Forward declarations:
  class ICNNLayer;
  
  class ConvolutionalNeuralNet
  {
  public:
    ConvolutionalNeuralNet();
    ~ConvolutionalNeuralNet();
   
    bool IsLoaded() const { return _isLoaded; }
    
    const std::string& GetName() const { return _name; }
    
    Result Load(std::string filename);
    
    Result Run(const cv::Mat& img, std::vector<float>& output);
    
    Result Run(const cv::Mat& img, size_t& classLabel, std::string& className);
    
  protected:
    
    bool _isLoaded;
    std::string _name;
    
    std::vector<ICNNLayer*>  _layers;
    std::vector<std::string> _classNames;
    
  }; // class ConvolutionalNeuralNet
  
  
  
} // namespace Vision
} // namespace Anki

#endif // __Anki_Vision_Basestation_CNN_H__
