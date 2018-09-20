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

#include "coretech/common/engine/array2d_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/rect_impl.h"
#include "coretech/neuralnets/neuralNetModel_interface.h"
#include "coretech/vision/engine/image_impl.h"

#include "util/console/consoleInterface.h"
#include "util/fileUtils/fileUtils.h"
#include "util/logging/logging.h"

#include "opencv2/imgcodecs/imgcodecs.hpp"

#include <fstream>

namespace Anki {
namespace NeuralNets {
  
#define LOG_CHANNEL "NeuralNets"

static const bool kINeuralNetModel_SaveImages = false;

int GetCVTypeHelper(const uint8_t* intputType)
{
  return CV_8UC2;
}
int GetCVTypeHelper(const float* intputType)
{
  return CV_32FC2;
}

// Inline functions will call proper helper based on output type of network
inline static float QuantizedScalingHelper(const float output, const float scale, const int zero_point)
{
  return output;
}

inline static float QuantizedScalingHelper(const uint8_t output, const float scale, const int zero_point)
{
  return scale * (output - zero_point);
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
INeuralNetModel::INeuralNetModel(const std::string& cachePath)
: _cachePath(cachePath)
{

}
  
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
  
  LOG_DEBUG("NeuralNetModel.ReadLabelsFile.Success", "Read %d labels", (int)labels_out.size());
  
  return RESULT_OK;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
void INeuralNetModel::ClassificationOutputHelper(const T* outputData, TimeStamp_t timestamp,
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
      LOG_INFO("INeuralNetModel.ClassificationOutputHelper.ObjectFound",
               "Name: %s, Score: %f", salientPoint.description.c_str(), salientPoint.score);
    }
    
    salientPoints.push_back(std::move(salientPoint));
  }
  else if(_params.verbose)
  {
    LOG_INFO("INeuralNetModel.ClassificationOutputHelper.NoObjects", "MinScore: %f", _params.minScore);
  }
}
  
// Explicitly instantiate for float and uint8
template void INeuralNetModel::ClassificationOutputHelper(const float*,   TimeStamp_t, std::list<Vision::SalientPoint>&);
template void INeuralNetModel::ClassificationOutputHelper(const uint8_t*, TimeStamp_t, std::list<Vision::SalientPoint>&);
  
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
void INeuralNetModel::LocalizedBinaryOutputHelper(const T* outputData, TimeStamp_t timestamp,
                                                  const float scale, const int zero_point,
                                                  std::list<Vision::SalientPoint>& salientPoints)
{
  // Create a detection box for each grid cell that is above threshold

  // This has been removed because the outputTensor.tensor options are always
  // 0000 regardless of acutally row or column major and Eigen::RowMajor is
  // 0001 and thus this check isn't really checking anything. VIC-4386
  // DEV_ASSERT( !(outputTensor.tensor<float, 2>().Options & Eigen::RowMajor),
  //           "NeuralNetModel.GetLocalizedBinaryClassification.OutputNotRowMajor");
  
  _detectionGrid.Allocate(_params.numGridRows, _params.numGridCols);
  
  bool anyDetections = false;
  for(int i=0; i<_detectionGrid.GetNumRows(); ++i)
  {
    uint8_t* detectionGrid_i = _detectionGrid.GetRow(i);
    for(int j=0; j<_detectionGrid.GetNumCols(); ++j)
    {
      // Compute the column-major index to get data from the output tensor
      const int outputIndex = j*_params.numGridRows + i;
      const float score = QuantizedScalingHelper(outputData[outputIndex], scale, zero_point);
      
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
    // Because we want to get average score, we'll do our own "stats" computation below instead of using
    // ConnectedComponentsWithStats()
    const s32 count = _detectionGrid.GetConnectedComponents(_labelsGrid);
    DEV_ASSERT((_detectionGrid.GetNumRows() == _labelsGrid.GetNumRows()) &&
               (_detectionGrid.GetNumCols() == _labelsGrid.GetNumCols()),
               "INeuralNetModel.LocalizedBinaryOutputHelper.MismatchedLabelsGridSize");
    
    if(_params.verbose)
    {
      LOG_INFO("INeuralNetModel.LocalizedBinaryOutputHelper.FoundConnectedComponents",
               "NumComponents: %d", count);
    }
    
    // Compute stats for each connected component
    struct Stat {
      int32_t area; // NOTE: area will be number of grid squares, not area of bounding box shape
      int32_t scoreSum;
      Point2f centroid;
      int xmin, xmax, ymin, ymax;
    };
    std::vector<Stat> stats(count, {0,0,{0.f,0.f},_detectionGrid.GetNumCols(), -1, _detectionGrid.GetNumRows(), -1});
    
    for(int i=0; i<_detectionGrid.GetNumRows(); ++i)
    {
      const uint8_t* detectionGrid_i = _detectionGrid.GetRow(i);
      const int32_t* labelsGrid_i    = _labelsGrid.GetRow(i);
      for(int j=0; j<_detectionGrid.GetNumCols(); ++j)
      {
        const int32_t label = labelsGrid_i[j];
        if(label > 0) // zero is background (not part of any connected component)
        {
          DEV_ASSERT(label < count, "INeuralNetModel.LocalizedBinaryOutputHelper.BadLabel");
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
    const float widthScale  = 1.f / static_cast<float>(_detectionGrid.GetNumCols());
    const float heightScale = 1.f / static_cast<float>(_detectionGrid.GetNumRows());
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
      DEV_ASSERT(_labels.size()==1, "INeuralNetModel.LocalizedBinaryOutputHelper.NotBinary");
      Vision::SalientPointType type = Vision::SalientPointType::Unknown;
      const bool success = SalientPointTypeFromString(_labels[0], type);
      DEV_ASSERT(success, "INeuralNetModel.LocalizedBinaryOutputHelper.NoSalientPointTypeForLabel");
      
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
        LOG_INFO("INeuralNetModel.LocalizedBinaryOutputHelper.SalientPoint",
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
  template void INeuralNetModel::LocalizedBinaryOutputHelper(const float* outputData,   TimeStamp_t timestamp,
                                                             const float scale, const int zero_point,
                                                             std::list<Vision::SalientPoint>& salientPoints);
  template void INeuralNetModel::LocalizedBinaryOutputHelper(const uint8_t* outputData, TimeStamp_t timestamp,
                                                             const float scale, const int zero_point,
                                                             std::list<Vision::SalientPoint>& salientPoints);

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
template<typename T>
void INeuralNetModel::ResponseMapOutputHelper(const T* outputData, TimeStamp_t timestamp,
                                              const int numberOfChannels,
                                              std::list<Vision::SalientPoint>& salientPoints)
{

  // Note: The Eigen tensor we get here is row major. An assert checking for
  // row major here doesn't really make sense because the Eigen tensor we get
  // always reports it is in column major when in fact it is not. VIC-4386

  // TODO is this really what I want to do, with the raw pointer
  cv::Mat responseMap(_params.inputHeight, _params.inputWidth, GetCVTypeHelper(outputData),
                      const_cast<T*>(reinterpret_cast<const T*>(outputData)));
  std::vector<cv::Mat> channels;
  split(responseMap, channels);

  const int kObjectnessIndex = 1;
  double min(0), max(0);
  cv::Point2i minLoc(0, 0), maxLoc(0, 0);
  cv::minMaxLoc(channels[kObjectnessIndex], &min, &max, &minLoc, &maxLoc);

  if (kINeuralNetModel_SaveImages)
  {
    SaveResponseMaps(channels, numberOfChannels, timestamp);
  }

  // Create a SalientPoint to return
  const float kWidthScale  = 1.f / static_cast<float>(responseMap.cols);
  const float kHeightScale = 1.f / static_cast<float>(responseMap.rows);
  const float x = Util::Clamp(maxLoc.x * kWidthScale,  0.f, 1.f);
  const float y = Util::Clamp(maxLoc.y * kHeightScale, 0.f, 1.f);
  const Vision::SalientPointType type = Vision::SalientPointType::Object;

  // TODO right now objectness doesn't have an area associated with it,
  // thus the shape part of the salient point is empty, and the area
  // fraction has a placeholder.
  Vision::SalientPoint salientPoint(timestamp,
                                    x, y, max,
                                    1.f * (kWidthScale*kHeightScale),
                                    type, EnumToString(type),
                                    Poly2f{}.ToCladPoint2dVector());

  salientPoints.push_back(std::move(salientPoint));
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void INeuralNetModel::SaveResponseMaps(const std::vector<cv::Mat>& channels, const int numberOfChannels,
                                       const TimeStamp_t timestamp)
{
    for (int channel = 0; channel < numberOfChannels; ++channel)
    {
      double channelMin(0), channelMax(0);
      cv::Point2i channelMinLoc(0, 0), channelMaxLoc(0, 0);
      cv::minMaxLoc(channels[channel], &channelMin, &channelMax, &channelMinLoc, &channelMaxLoc);

      const std::string saveFilename = Util::FileUtils::FullFilePath({_cachePath,
        _params.visualizationDirectory, std::to_string(timestamp) + "_" +
        std::to_string(channel) + ".png"});

      cv::Mat imageToSave;
      channels[channel].copyTo(imageToSave);
      imageToSave = 255 * (imageToSave - channelMin) / (channelMax - channelMin);
      imageToSave.convertTo(imageToSave, CV_8UC1, 1.f/(channelMax - channelMin),  -channelMin / (channelMax - channelMin));
      cv::imwrite(saveFilename, imageToSave);

      const std::string salientPointFilename = Util::FileUtils::FullFilePath({_cachePath,
        "objectnessResponseMap", std::to_string(timestamp) + ".txt"});
      std::ofstream salientPointFile(salientPointFilename);
      salientPointFile << channelMaxLoc.x << " " << channelMaxLoc.y << " " << channelMax << + " "
        << channelMinLoc.x << " " << channelMinLoc.y << " " << channelMin;
      salientPointFile.close();
    }
}
  // Explicitly instantiate for float and uint8
  template void INeuralNetModel::ResponseMapOutputHelper(const float* outputData,   TimeStamp_t timestamp,
                                                         const int numberOfChannels,
                                                         std::list<Vision::SalientPoint>& salientPoints);
  template void INeuralNetModel::ResponseMapOutputHelper(const uint8_t* outputData, TimeStamp_t timestamp,
                                                         const int numberOfChannels,
                                                         std::list<Vision::SalientPoint>& salientPoints); 
} // namespace NeuralNets
} // namespace Anki
