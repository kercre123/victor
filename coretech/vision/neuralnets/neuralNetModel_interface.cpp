/**
 * File: neuralNetModel_interface.cpp
 *
 * Author: Andrew Stein
 * Date:   7/2/2018
 *
 * Description: Defines interface and shared helpers for various NeuralNetModel implementations.
 *
 * Copyright: Anki, Inc. 2018
 **/

#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/vision/neuralnets/neuralNetModel_interface.h"
#include "util/logging/logging.h"
#include <fstream>

namespace Anki {
namespace Vision {
  
#define LOG_CHANNEL "NeuralNets"
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
Result INeuralNetModel::ReadLabelsFile(const std::string& fileName, std::vector<std::string>& labels_out)
{
  std::ifstream file(fileName);
  if (!file)
  {
    PRINT_NAMED_ERROR("NeuralNetModel.ReadLabelsFile.LabelsFileNotFound", "%s", fileName.c_str());
    return RESULT_FAIL;
  }
  
  labels_out.clear();
  std::string line;
  while (std::getline(file, line)) {
    labels_out.push_back(line);
  }
  
  LOG_INFO("NeuralNetModel.ReadLabelsFile.Success", "Read %d labels", (int)labels_out.size());
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
void INeuralNetModel::GetClassification(const T* outputData, TimeStamp_t timestamp,
                                        std::list<Vision::SalientPoint>& salientPoints)
{
  T maxScore = _params.minScore;
  int labelIndex = -1;
  for(int i=0; i<_labels.size(); ++i)
  {
    if(outputData[i] > maxScore)
    {
      maxScore = outputData[i];
      labelIndex = i;
    }
  }
  
  const Rectangle<int32_t> imgRect(0.f,0.f,1.f,1.f);
  const Poly2i imgPoly(imgRect);
  
  if(labelIndex >= 0)
  {
    Vision::SalientPoint salientPoint(timestamp, 0.5f, 0.5f, maxScore, 1.f,
                                      Vision::SalientPointType::Object,
                                      (labelIndex < _labels.size() ? _labels.at((size_t)labelIndex) : "<UNKNOWN>"),
                                      imgPoly.ToCladPoint2dVector());
    
    if(_params.verbose)
    {
      LOG_INFO("NeuralNetModel.GetClassification.ObjectFound", "Name: %s, Score: %f", salientPoint.description.c_str(), salientPoint.score);
    }
    
    salientPoints.push_back(std::move(salientPoint));
  }
  else if(_params.verbose)
  {
    LOG_INFO("NeuralNetModel.GetClassification.NoObjects", "MinScore: %f", _params.minScore);
  }
}
  
// Explicitly instantiate for float and uint8
template void INeuralNetModel::GetClassification(const float*,   TimeStamp_t, std::list<Vision::SalientPoint>&);
template void INeuralNetModel::GetClassification(const uint8_t*, TimeStamp_t, std::list<Vision::SalientPoint>&);
  
} // namespace Vision
} // namespace Anki
