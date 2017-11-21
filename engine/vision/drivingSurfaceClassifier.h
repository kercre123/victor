/**
 * File: DrivingSurfaceClassifier.h
 *
 * Author: Lorenzo Riano
 * Created: 11/9/17
 *
 * Description: A class to classify pixels as drivable or non drivable. Works in tandem with OverheadMap
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Anki_Cozmo_DrivingSurfaceClassifier_H__
#define __Anki_Cozmo_DrivingSurfaceClassifier_H__


#include "basestation/jsonTools.h"
#include "engine/vision/overheadMap.h"

#include <opencv2/core.hpp>

// forward declarations
namespace cv {
namespace ml {
class EM;
class LogisticRegression;
class TrainData;
}
}

namespace Anki {
class WeightedLogisticRegression;
namespace Cozmo {

class CozmoContext;

/*
 * This class uses Gaussian Mixture Models (GMM) and Logistic Regression (LR) to classify pixels as drivable or not.
 * The pixels are provided by the OverheadMap class. Training is performed in batches. To classify/train a set of
 * pixels, a 3-dimensional GMM is created with only the positive (drivable surface) pixels. Then for each pixel its
 * minimum Mahalhanobis distance from all the kernels is computed. This new one dimensional vector is used as training
 * for LR which finally classifies the pixel as drivable or not.
 *
 * Notes:
 * a single GMM won't suffice in this case as the probablilites are not normalized and it's hard to find a threshold for
 * all of them
 *
 * Since the OverheadMap only labels pixels the robot has driven on as positive, a higher weight should be given to the
 * positive (1) class during training.
 */
class DrivingSurfaceClassifier {
public:
  explicit DrivingSurfaceClassifier(const Json::Value& config, const CozmoContext *context);

  /*
   * Train from a set of drivable and non-drivable pixels. Data might come from OverheadMap
   */
  bool Train(const OverheadMap::PixelSet& drivablePixels, const OverheadMap::PixelSet& nonDrivablePixels);
  /*
   * Load data from two files and use it for training
   */
  bool TrainFromFiles(const char* positiveDataFileName, const char* negativeDataFileName);
  /*
   * This function is used mainly for testing the class and debugging
   */
  void GetTrainingData(cv::Mat& trainingSamples, cv::Mat& trainingLabels) const;

  /*
   * Predict the class of a single pixel (1 is drivable, 0 is not)
   */
  uchar PredictClass(const Vision::PixelRGB& pixel) const;
  /*
   * Predict the class of a vector of pixels (1 is drivable, 0 is not)
   */
  std::vector<uchar> PredictClass(const std::vector<Vision::PixelRGB>& pixels) const;

private:
  cv::Ptr<cv::ml::EM> _gmm;
  cv::Ptr<Anki::WeightedLogisticRegression> _logisticRegressor;
  cv::Mat _trainingSamples;
  cv::Mat _trainingLabels;
  float _positiveClassWeight = 1.2; // This parameter should be overwritten by the Json config file
  float _trainingAlpha = 0.5;       // This parameter should be overwritten by the Json config file

  const CozmoContext* _context;

  /*
   * For each row in input, calculate the mimimum distance from the kernels in the Gaussian Mixture model and return it
   * If useWeight=true then scale the distance by the weight of the kernel, so lower weight kernels will have a
   * greater distance.
   */
  std::vector<float> MinMahalanobisDistanceFromGMM(const cv::Mat& input, bool useWeight = true) const;

  bool Train(const cv::Mat& allInputs, const cv::Mat& allClasses, uint numberOfPositives);

  /*
   * Write a cv::Mat to a file
   */
  void WriteMat(const cv::Mat& mat, const char *filename) const;

  template<class T>
  void WriteMat(const cv::Mat& mat, std::ofstream& outputFile) const;
};


} // namespace Cozmo
} // namespace Anki


#endif //COZMO_DRIVINGSURFACECLASSIFIER_H
