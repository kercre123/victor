#ifndef ANKI_CORETECH_VISION_BASESTATION_CNN_H
#define ANKI_CORETECH_VISION_BASESTATION_CNN_H

#include "anki/common/types.h"

#include <string>
#include <vector>

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
    
    Result Load(std::string filename);
    
    Result Run(const cv::Mat& img, std::vector<float>& output);
    
  protected:
    
    bool _isLoaded;
    
    std::vector<ICNNLayer*> _layers;
    
  }; // class ConvolutionalNeuralNet
  
  
  
} // namespace Vision
} // namespace Anki

#endif // ANKI_CORETECH_VISION_BASESTATION_CNN_H
