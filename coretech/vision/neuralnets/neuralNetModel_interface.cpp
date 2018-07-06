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
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
void INeuralNetModel::GetLocalizedBinaryClassification(const T* outputData, TimeStamp_t timestamp,
                                                      std::list<Vision::SalientPoint>& salientPoints)
{
  // Create a detection box for each grid cell that is above threshold
  
  _detectionGrid.create(_params.numGridRows, _params.numGridCols);
  
  bool anyDetections = false;
  for(int i=0; i<_detectionGrid.rows; ++i)
  {
    uint8_t* detectionGrid_i = _detectionGrid.ptr(i);
    for(int j=0; j<_detectionGrid.cols; ++j)
    {
      // Compute the column-major index to get data from the output tensor
      const int outputIndex = j*_params.numGridRows + i;
      const float score = outputData[outputIndex];
      
      // Create binary detection image
      if(score > _params.minScore) {
        anyDetections = true;
        detectionGrid_i[j] = static_cast<uint8_t>(255.f * score);
      }
      else {
        detectionGrid_i[j] = 0;
      }
      
    }
  }
  
  if(anyDetections)
  {
    // Someday if we ever link vic-neuralnets against coretech vision, we could use our own connected
    // components API, but for now, rely directly on OpenCV. Because we want to get average score,
    // we'll do our own "stats" computation below instead of using connectedComponentsWithStats()
    const s32 count = cv::connectedComponents(_detectionGrid, _labelsGrid);
    DEV_ASSERT((_detectionGrid.rows == _labelsGrid.rows) && (_detectionGrid.cols == _labelsGrid.cols),
               "NeuralNetModel.GetLocalizedBinaryClassification.MismatchedLabelsGridSize");
    
    if(_params.verbose)
    {
      LOG_INFO("NeuralNetModel.GetLocalizedBinaryClassification.FoundConnectedComponents",
               "NumComponents: %d", count);
    }
    
    // Compute stats for each connected component
    struct Stat {
      int32_t area; // NOTE: area will be number of grid squares, not area of bounding box shape
      int32_t scoreSum;
      Point2f centroid;
      int xmin, xmax, ymin, ymax;
    };
    std::vector<Stat> stats(count, {0,0,{0.f,0.f},_detectionGrid.cols, -1, _detectionGrid.rows, -1});
    
    for(int i=0; i<_detectionGrid.rows; ++i)
    {
      const uint8_t* detectionGrid_i = _detectionGrid.ptr<uint8_t>(i);
      const int32_t* labelsGrid_i    = _labelsGrid.ptr<int32_t>(i);
      for(int j=0; j<_detectionGrid.cols; ++j)
      {
        const int32_t label = labelsGrid_i[j];
        if(label > 0) // zero is background (not part of any connected component)
        {
          DEV_ASSERT(label < count, "NeuralNetModel.GetLocalizedBinaryClassification.BadLabel");
          const int32_t score = detectionGrid_i[j];
          Stat& stat = stats[label];
          stat.scoreSum += score;
          stat.area++;
          
          stat.centroid.x() += j;
          stat.centroid.y() += i;
          
          stat.xmin = std::min(stat.xmin, j);
          stat.xmax = std::max(stat.xmax, j);
          stat.ymin = std::min(stat.ymin, i);
          stat.ymax = std::max(stat.ymax, i);
        }
      }
    }
    
    // Create a SalientPoint to return for each connected component (skipping background component 0)
    const float widthScale  = 1.f / static_cast<float>(_detectionGrid.cols);
    const float heightScale = 1.f / static_cast<float>(_detectionGrid.rows);
    for(s32 iComp=1; iComp < count; ++iComp)
    {
      Stat& stat = stats[iComp];
      
      const float area = static_cast<float>(stat.area);
      const float avgScore = (1.f/255.f) * ((float)stat.scoreSum / area);
      
      // Centroid currently contains a sum. Divide by area to get actual centroid.
      stat.centroid *= 1.f/area;
      
      // Convert centroid to normalized coordinates
      stat.centroid.x() = Util::Clamp(stat.centroid.x()*widthScale,  0.f, 1.f);
      stat.centroid.y() = Util::Clamp(stat.centroid.y()*heightScale, 0.f, 1.f);
      
      // Use the single label (this is supposed to be a binary classifier after all) and
      // convert it to a SalientPointType
      DEV_ASSERT(_labels.size()==1, "ObjectDetector.GetLocalizedBinaryClassification.NotBinary");
      Vision::SalientPointType type = Vision::SalientPointType::Unknown;
      const bool success = SalientPointTypeFromString(_labels[0], type);
      DEV_ASSERT(success, "ObjectDetector.GetLocalizedBinaryClassification.NoSalientPointTypeForLabel");
      
      // Use the bounding box as the shape (in normalized coordinates)
      // TODO: Create a more precise polygon that traces the shape of the connected component (cv::findContours?)
      const float xmin = (static_cast<float>(stat.xmin)-0.5f) * widthScale;
      const float ymin = (static_cast<float>(stat.ymin)-0.5f) * heightScale;
      const float xmax = (static_cast<float>(stat.xmax)+0.5f) * widthScale;
      const float ymax = (static_cast<float>(stat.ymax)+0.5f) * heightScale;
      const Poly2f shape( Rectangle<float>(xmin, ymin, xmax-xmin, ymax-ymin) );
      
      Vision::SalientPoint salientPoint(timestamp,
                                        stat.centroid.x(),
                                        stat.centroid.y(),
                                        avgScore,
                                        area * (widthScale*heightScale), // convert to area fraction
                                        type,
                                        EnumToString(type),
                                        shape.ToCladPoint2dVector());
      
      if(_params.verbose)
      {
        LOG_INFO("NeuralNetModel.GetLocalizedBinaryClassification.SalientPoint",
                 "%d: %s score:%.2f area:%.2f [%s %s %s %s]",
                 iComp, stat.centroid.ToString().c_str(), avgScore, area,
                 shape[0].ToString().c_str(),
                 shape[1].ToString().c_str(),
                 shape[2].ToString().c_str(),
                 shape[3].ToString().c_str());
      }
      
      salientPoints.push_back(std::move(salientPoint));
    }
  }
}
  
  // Explicitly instantiate for float and uint8
  template void INeuralNetModel::GetLocalizedBinaryClassification(const float* outputData,   TimeStamp_t timestamp,
                                                                  std::list<Vision::SalientPoint>& salientPoints);
  template void INeuralNetModel::GetLocalizedBinaryClassification(const uint8_t* outputData, TimeStamp_t timestamp,
                                                                  std::list<Vision::SalientPoint>& salientPoints);

  
} // namespace Vision
} // namespace Anki
