/**
 * File: OnlineGrowingForestClassifier.h
 *
 * Author: Lorenzo Riano
 * Created: 2/28/18
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#ifndef __Anki_Cozmo_OnlineGrowingForestClassifier_H__
#define __Anki_Cozmo_OnlineGrowingForestClassifier_H__

#include "engine/vision/rawPixelsClassifier.h"

#include <vector>

namespace Anki {
namespace Cozmo {

class OnlineGrowingForestClassifier : public RawPixelsClassifier
{
public:
  OnlineGrowingForestClassifier(const CozmoContext *context,
                                int maxNumberOfTrees,
                                int maxDepth,
                                float drivableClassWeight = 1.0,
                                float minScoreToAddATree=0.85,
                                float weightDecayRate=0.8,
                                float minWeightToBeDeleted=0.05,
                                Vision::Profiler *profiler= nullptr);

  bool Train(const cv::Mat& allInputs, const cv::Mat& allClasses, uint numberOfPositives) override;

  uchar PredictClass(const std::vector<FeatureType>& values) const override;

  std::vector<uchar> PredictClass(const Anki::Array2d<FeatureType>& features) const override;

protected:

  bool UpdateChunk(const cv::Mat& allInputs, const cv::Mat& allClasses);

  void CreateNewTree(const cv::Mat& allInputs, const cv::Mat& allClasses);

  void UpdateTrees(const cv::Mat& allInputs, const cv::Mat& allClasses);

  std::vector<float> GetNormalizedWeights() const;

  /**
   * Update the scores for all the trees
   * @param allInputs
   * @param allClasses
   */
  void UpdateScores(const cv::Mat& allInputs, const cv::Mat& allClasses);


  int _maxNumberOfTrees;
  int _maxDepth;
  float _drivableClassWeight;
  float _minScoreToAddATree;
  float _weightDecayRate;
  float _minWeightToBeDeleted;

  Json::Value _config; // to be passed when creating a DTree

  std::vector<DTRawPixelsClassifier> _trees;
  std::vector<float> _cumulativeScores;
  std::vector<float> _latestScores;
  std::vector<float> _decayingWeights;

};

} // namespace Cozmo
} // namespace Anki


#endif //__Anki_Cozmo_OnlineGrowingForestClassifier_H__
