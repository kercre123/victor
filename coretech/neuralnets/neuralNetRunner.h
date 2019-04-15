/**
 * File: neuralNetRunner.h
 *
 * Author: Andrew Stein
 * Date:   6/29/2017
 *
 * Description: Asynchronous wrapper for running forward inference with a deep neural network on an image.
 *              Currently supports detection of SalientPoints, but API could be extended to get other
 *              types of outputs later.
 *
 *              Abstracts away the private implementation around what kind of inference engine is used
 *              and runs asynchronously since forward inference through deep networks is generally "slow".
 *
 * Copyright: Anki, Inc. 2017
 **/

#ifndef __Anki_NeuralNets_NeuralNetRunner_H__
#define __Anki_NeuralNets_NeuralNetRunner_H__

#include "coretech/common/shared/types.h"

#include "coretech/vision/engine/asyncRunnerInterface.h"

#include <list>

// Forward declaration:
namespace Json {
  class Value;
}

namespace Anki {

// Forward declarations:
namespace Vision {
  class ImageRGB;
  class ImageCache;
  struct SalientPoint;
}

namespace NeuralNets {

// Forward declaration:
class INeuralNetModel;
  
class NeuralNetRunner : public Vision::IAsyncRunner
{
public:
  
  explicit NeuralNetRunner(const std::string& modelPath);
  virtual ~NeuralNetRunner();
  
protected:
  
  virtual Result InitInternal(const std::string& cachePath, const Json::Value& config) override;
  virtual bool IsVerbose() const override;
  virtual std::list<Vision::SalientPoint> Run(Vision::ImageRGB& img) override;
  
private:
  std::string _modelPath;
  std::string _visualizationDirectory;
  std::unique_ptr<INeuralNetModel> _model;
};
  
} // namespace NeuralNets
} // namespace Anki

#endif /* __Anki_NeuralNets_NeuralNetRunner_H__ */
