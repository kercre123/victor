/**
 * File: OnlineGrowingForestClassifier.cpp
 *
 * Author: Lorenzo Riano
 * Created: 2/28/18
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include <numeric>
#include "coretech/common/engine/math/logisticRegression.h" // for calculateError
#include "onlineGrowingForestClassifier.h"

namespace Anki {
namespace Cozmo {

namespace {

// Remove a set of elements from vector specified by the indexes in toRemove.
// IMPORTANT: The indexes in toRemove need to be sorted in ascending order
// Adapted from https://stackoverflow.com/a/20583932/1047543
template<typename T>
void RemoveFromIndexes(std::vector<T>& vector, const std::vector<int>& toRemove,
                        const T& defaultValue = T())
{
  auto vectorBase = vector.begin();
  typename std::vector<T>::size_type down_by = 0;

  for (auto iter = toRemove.cbegin();
       iter < toRemove.cend();
       iter++, down_by++)
  {
    typename std::vector<T>::size_type next = (iter + 1 == toRemove.cend()
                                               ? vector.size()
                                               : *(iter + 1));

    std::move(vectorBase + *iter + 1,
              vectorBase + next,
              vectorBase + *iter - down_by);
  }
  // Resize in this case will always be to shrink, but to make STL happy I have to provide a default value
  vector.resize(vector.size() - toRemove.size(), defaultValue);
}

// Returns the score (1-error) of the classifier with respect to input Xs and labels Ys
template <typename T>
float Score(const T& clf, const cv::Mat& Xs, const cv::Mat& Ys) {
  const Array2d<RawPixelsClassifier::FeatureType> arrayInput((cv::Mat_<RawPixelsClassifier::FeatureType>(Xs)));
  const std::vector<uchar> responses = clf.PredictClass(arrayInput);
  cv::Mat responsesMat(responses);
  const float error = Anki::calculateError(responsesMat, Ys);
  const float score = 1.0f - error;
  return score;
}

template <typename T>
inline constexpr float average(const std::vector<T>& v) {
  return std::accumulate(v.begin(), v.end(), 0.0f) / float(v.size());
}

} // anonymous namespace

OnlineGrowingForestClassifier::OnlineGrowingForestClassifier(const CozmoContext *context, int maxNumberOfTrees,
                                                             int maxDepth,
                                                             float drivableClassWeight, float minScoreToAddATree,
                                                             float weightDecayRate,
                                                             float minWeightToBeDeleted,
                                                             Vision::Profiler *profiler)
    : RawPixelsClassifier(context, profiler)
    , _maxNumberOfTrees(maxNumberOfTrees)
    , _maxDepth(maxDepth)
    , _drivableClassWeight(drivableClassWeight)
    , _minScoreToAddATree(minScoreToAddATree)
    , _weightDecayRate(weightDecayRate)
    , _minWeightToBeDeleted(minWeightToBeDeleted)
{

  // TODO probably needs to be passed in as a sub-config in a json file
  _config["MaxDepth"] = _maxDepth;
  _config["MinSampleCount"] = 10;
  _config["TruncatePrunedTree"] = true;
  _config["Use1SERule"] = true;
  _config["PositiveWeight"] = _drivableClassWeight;
  _config["OnTheFlyTrain"] = false;
  _config["FileOrDirName"] = "";
}

bool OnlineGrowingForestClassifier::Train(const cv::Mat& allInputs, const cv::Mat& allClasses,
                                                       uint)
{
  return UpdateChunk(allInputs, allClasses);
}

uchar OnlineGrowingForestClassifier::PredictClass(
    const std::vector<RawPixelsClassifier::FeatureType>& values) const
{
  // first get the predictions of all the trees
  std::vector<uchar> allTreesPredictions;
  allTreesPredictions.reserve(_trees.size());
  for (auto&& tree : _trees) {
    uchar treePrediction = tree.PredictClass(values);
    allTreesPredictions.push_back(treePrediction);
  }

  const std::vector<float> normalizedWeights = GetNormalizedWeights();
  DEV_ASSERT(allTreesPredictions.size() == normalizedWeights.size(),
             "OnlineGrowingForestClassifier.PredictClass.WrongNormalizedWeightsSize");

  // the result is the inner product of weights times prediction, check if > 0.5
  float sum = 0.0f;
  for (int i = 0; i < allTreesPredictions.size(); ++i) {
    sum += float(allTreesPredictions[i]) * normalizedWeights[i];
  }
  return uchar(sum > 0.5f);

}

std::vector<uchar>
OnlineGrowingForestClassifier::PredictClass(const Array2d<RawPixelsClassifier::FeatureType>& features) const
{

  // first get the predictions of all the trees
  std::vector<std::vector<uchar>> allTreesPredictions;
  allTreesPredictions.reserve(_trees.size());
  for (auto&& tree : _trees) {
    std::vector<uchar> treePrediction = tree.PredictClass(features);
    allTreesPredictions.push_back(std::move(treePrediction));
  }

  // create the normalized weights
  const std::vector<float> normalizedWeights = GetNormalizedWeights();

  std::vector<uchar> predictions;
  predictions.reserve(features.GetNumRows());

  // for each individual prediction, calculate the weighted average. if it's > 0.5 then the class is 1
  // TODO this is literally the Matrix product of the decaying weights times all the Predicitons [n_features * n_trees]
  // TODO could it be faster to convert to cv::Mat and do the matrix multiplication?
  for (int featureIndex = 0; featureIndex < features.GetNumRows(); ++featureIndex) {
    float sum = 0;
    for (int treeIndex = 0; treeIndex < allTreesPredictions.size(); ++treeIndex) {
      sum += allTreesPredictions[treeIndex][featureIndex] * normalizedWeights[treeIndex];
    }
    predictions.push_back(sum > 0.5? uchar(1) : uchar(0));
  }

  return predictions;
}

std::vector<float> OnlineGrowingForestClassifier::GetNormalizedWeights() const
{
  std::vector<float> normalizedWeights;
  {
    normalizedWeights.reserve(_decayingWeights.size());
    const float sum = accumulate(_decayingWeights.begin(), _decayingWeights.end(), 0.0f);
    for (auto&& weight : _decayingWeights) {
      normalizedWeights.push_back(weight / sum);
    }
  }
  return normalizedWeights;
}

bool OnlineGrowingForestClassifier::UpdateChunk(const cv::Mat& allInputs, const cv::Mat& allClasses)
{
  _latestScores.clear();

  if (_trees.size() < _maxNumberOfTrees) {
    CreateNewTree(allInputs, allClasses);
    UpdateScores(allInputs, allClasses);
  }
  else {
    UpdateTrees(allInputs, allClasses);
    // cumulative scores are updated in UpdateTrees
  }

  // find indexes of elements whose normalized weight is too small
  std::vector<int> indexesToRemove;
  indexesToRemove.reserve(_decayingWeights.size());
  {
    float sum = std::accumulate(_decayingWeights.begin(), _decayingWeights.end(), 0.0f);
    for (uint i = 0; i < _decayingWeights.size(); i++) {
      float newValue = _decayingWeights[i] / sum;
      if (newValue < _minWeightToBeDeleted) {
        indexesToRemove.push_back(i);
      }
    }
  }

  RemoveFromIndexes(_trees, indexesToRemove, DTRawPixelsClassifier(_config, _context));
  RemoveFromIndexes(_cumulativeScores, indexesToRemove);
  RemoveFromIndexes(_decayingWeights, indexesToRemove);

  // sanity checks
  DEV_ASSERT(_trees.size() == _cumulativeScores.size(),
             "OnlineGrowingForestClassifier,UpdateChunk.WrongTreesVectorSize");
  DEV_ASSERT(_trees.size() <= _maxNumberOfTrees,
             "OnlineGrowingForestClassifier,UpdateChunk.TooManyTrees");
  DEV_ASSERT(_trees.size() == _decayingWeights.size(),
             "OnlineGrowingForestClassifier,UpdateChunk.WrongDecayingWeightsSize");

  return true;
}

void OnlineGrowingForestClassifier::CreateNewTree(const cv::Mat& allInputs, const cv::Mat& allClasses)
{
  DTRawPixelsClassifier clf(_config, _context);

  // train the new classifier on the data
  clf.Train(allInputs, allClasses, 0);

  // calculate the new classifier score
  const float newClassifierScore = Score(clf, allInputs, allClasses);

  // if there are already trees in the list, check the score to see if adding a new one helps
  if (!_trees.empty()) {
    // calculate the ensemble's error
    const float forestScore = Score(*this, allInputs, allClasses);

    if (forestScore > _minScoreToAddATree) {
      // the current forest is good enough
      return;
    }
    else if (newClassifierScore >  forestScore) {
      // TODO maybe check for |a-b| < eps?

      // new boy in town is good
      _trees.push_back(clf);

      // the new cumulative score is the average of the others
      const float  newComulativeScore =average(_cumulativeScores);

      _cumulativeScores.push_back(newComulativeScore);
      // decay the weights
      for (auto& weight : _decayingWeights) {
        weight *= _weightDecayRate;
      }
      // and add a fresh new member
      _decayingWeights.push_back(1.0f);
    }
  }
  else {
    // no trees, add the new one
    _trees.push_back(clf);
    _cumulativeScores.push_back(newClassifierScore);
    _latestScores.push_back(newClassifierScore);
    _decayingWeights.push_back(1.0);
  }
}

void OnlineGrowingForestClassifier::UpdateTrees(const cv::Mat& allInputs, const cv::Mat& allClasses)
{

  // calculate the error on each individual tree
  UpdateScores(allInputs, allClasses);

  // find the tree with the minimum historical (cumulative) score
  const auto minScoreIt = std::min_element(_cumulativeScores.begin(), _cumulativeScores.end());
  const auto minScoreIndex = std::distance(_cumulativeScores.begin(), minScoreIt);

  DEV_ASSERT(minScoreIndex < _latestScores.size(), "OnlineGrowingForestClassifier.UpdateTreesHighMinScoreIndex");

  // new guy to replace a sad loser
  DTRawPixelsClassifier clf(_config, _context);
  clf.Train(allInputs, allClasses, 0);
  const float newScore = Score(clf, allInputs, allClasses);

  // the tree with lowest score gets kicked out
  if (newScore > _latestScores[minScoreIndex]) {
    // replace the loser
    _trees[minScoreIndex] = clf;
    _latestScores[minScoreIndex] = newScore;
    // the new guy gets the average of the previous scores
    _cumulativeScores[minScoreIndex] = average(_cumulativeScores);
    // decay the weights
    for (auto& weight : _decayingWeights) {
      weight *= _weightDecayRate;
    }
    // the new guy doesn't get 1.0 here, might be too high
    _decayingWeights[minScoreIndex] = _weightDecayRate;
  }
}

void OnlineGrowingForestClassifier::UpdateScores(const cv::Mat& allInputs, const cv::Mat& allClasses)
{
  // if the latest scores haven't been updated, do it
  std::vector<float>& newScores = _latestScores;
  if (newScores.empty()) {
    newScores.reserve(_trees.size());
    for (auto&& tree : _trees) {
      float score = Score(tree, allInputs, allClasses);
      newScores.push_back(score);
    }
  }

  DEV_ASSERT(_cumulativeScores.size() == newScores.size(), "OnlineGrowingForestClassifier.UpdateScores.SizeMismatch");

  // Update the cumulative scores
  for (int i = 0; i < _cumulativeScores.size(); ++i) {
    _cumulativeScores[i] += newScores[i];
  }
}

} // namespace Cozmo
} // namespace Anki