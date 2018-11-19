/**
 * File: colorClassifer.h
 *
 * Author: Patrick Doran
 * Date: 2018-11-08
 */

#ifndef __Anki_Vision_ColorClassifier_H__
#define __Anki_Vision_ColorClassifier_H__

#include "coretech/common/shared/types.h"

#include <vector>
#include <opencv2/core/mat.hpp>

#include "json/json.h"

namespace Anki
{
namespace Vision
{

class ColorClassifier
{
public:
  ColorClassifier ();
  ColorClassifier (const Json::Value& config);
  virtual ~ColorClassifier ();

  /**
   * @brief Train a model based on samples and labels
   * @details Train a model based on the samples and labels. Samples are expected to be NxM where N is the number of
   * samples and M is the dimensionality of the feature. Labels is expected to be Nx1 applying the label index to the
   * given sample.
   * @param[in] samples - Vector filled with NxM feature vector matrix for each class K
   * @return True if training completed.
   */
  bool Train(const std::vector<cv::Mat>& samples);

  /**
   * @brief Predict the probability that a sample belongs to a class
   * @param[in] samples - NxM matrix of samples
   * @param[out] labels - Output NxK matrix (float) of probabilities where K is the number of classes and
   */
  void Predict(const cv::Mat& samples, cv::Mat& labels) const;

  /**
   * @brief Label a sample as belonging to a class
   * @param samples - NxM matrix of samples; N is the number of samples and M is the length of the feature
   * @param labels - Nx1 matrix (int) of labels for each sample
   * @param threshold - Distance threshold
   * @param unknown - Label id for samples belonging to no class
   */
  void Classify(const cv::Mat& samples, cv::Mat& labels, f32 threshold, s32 unknown = -1) const;

  bool Save(const std::string& filename);
  bool Load(const std::string& filename);

  bool Save(Json::Value& config);
  bool Load(const Json::Value& config);

private:
  struct LabelStats
  {
    cv::Mat samples;
    cv::Mat mean;
    cv::Mat covariance;
    cv::Mat covariance_inv;
    float determinant;
    float determinant_log;
    cv::Mat eigenvectors;
    cv::Mat eigenvalues;

    //! Denominator for multivariate normal distribution calculation
    float denominator;
  };

  void Reset();

  std::vector<LabelStats> _stats;
  s32 _dimensionality;

};

} /* namespace Vision */
} /* namespace Anki */

#endif /* __Anki_Vision_ColorClassifier_H__ */
