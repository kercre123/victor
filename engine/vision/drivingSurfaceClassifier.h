/**
 * File: DrivingSurfaceClassifier.h
 *
 * Author: Lorenzo Riano
 * Created: 11/9/17
 *
 * Description: A set of classes to classify pixels as drivable or non drivable. Works in tandem with OverheadMap.
 *              See testSurfaceClassifier.cpp for examples of use.
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
class DTrees;
}
}

namespace Anki {

class WeightedLogisticRegression;

namespace Cozmo {

class CozmoContext;

/****************************************************************
 *                     DrivingSurfaceClassifier                 *
 ****************************************************************/
/*
 * Generic class for drivable surface classifier. See Individual implementations below. The class can be
 * trained either from data (see Train) or from files (see TrainFromFiles). To classify a (set of) pixel
 * use the Predict methods. The pixels are provided by the OverheadMap class. Training is performed in
 * batches.
 */
class DrivingSurfaceClassifier {

public:

  DrivingSurfaceClassifier(const CozmoContext *context);

  /*
   * Train from a set of drivable and non-drivable pixels. Data might come from OverheadMap
   */
  bool Train(const OverheadMap::PixelSet& drivablePixels, const OverheadMap::PixelSet& nonDrivablePixels);

  /*
   * This function is used mainly for testing the class and debugging
   */
  void GetTrainingData(cv::Mat& trainingSamples, cv::Mat& trainingLabels) const;

  /*
   * Predict the class of a single pixel (1 is drivable, 0 is not)
   */
  virtual uchar PredictClass(const std::vector<u8>& values) const = 0;

  /*
   * Predict the class of a vector of pixels (1 is drivable, 0 is not)
   */
  virtual std::vector<uchar> PredictClass(const std::vector<std::vector<u8>>& pixels) const;

  /*
   * Predict the class of the pixel at image[row, col]. Useful when padding needs to be considered
   */
  virtual uchar PredictClass(const Vision::ImageRGB& image, uint row, uint col) const;

  void ClassifyImage(const Vision::ImageRGB& image, Vision::Image& outputMask) const;

  /*
   * Load data from two files and use it for training
   */
  virtual bool TrainFromFiles(const char* positiveDataFileName, const char* negativeDataFileName);

  /*
   * Serialize/Deserialize the detector
   */
  virtual bool Serialize(const char* filename) = 0;
  virtual bool DeSerialize(const char* filename) = 0;

  virtual void setPadding(uint padding) // base classes can decide to exclude certain levels of padding
  {
    _padding = padding;
  }

protected:
  cv::Mat _trainingSamples;
  cv::Mat _trainingLabels;
  const CozmoContext* _context;
  uint _padding = 0; // Padding = 0 is single pixels classification, > 0 a square of size 2*padding+1 is used

  virtual bool Train(const cv::Mat& allInputs, const cv::Mat& allClasses, uint numberOfPositives) = 0;

  /*
   * Write a cv::Mat to a file
   */
  void WriteMat(const cv::Mat& mat, const char *filename) const;

  template<class T>
  void WriteMat(const cv::Mat& mat, std::ofstream& outputFile) const;
};

/****************************************************************
 *                     GMMDrivingSurfaceClassifier              *
 ****************************************************************/

/*
 * Abstract class for drivable surface classification using Gaussian Mixture Models. See LRDrivingSurfaceClassifier
 * and THrivingSurfaceClassifier for implementations
 */
class GMMDrivingSurfaceClassifier : public DrivingSurfaceClassifier{
public:
  explicit GMMDrivingSurfaceClassifier(const Json::Value& config, const CozmoContext *context);

protected:
  cv::Ptr<cv::ml::EM> _gmm;

  /*
   * For each row in input, calculate the mimimum distance from the kernels in the Gaussian Mixture model and return it
   * If useWeight=true then scale the distance by the weight of the kernel, so lower weight kernels will have a
   * greater distance.
   */
  std::vector<float> MinMahalanobisDistanceFromGMM(const cv::Mat& input, bool useWeight = true) const;

  bool TrainGMM(const cv::Mat& input);

};

/****************************************************************
 *                     LRDrivingSurfaceClassifier               *
 ****************************************************************/
/*
 * This class uses Gaussian Mixture Models (GMM) and Logistic Regression (LR) to classify pixels as drivable or not.
 * To classify/train a set of pixels, a 3-dimensional GMM is created with only the positive (drivable surface) pixels.
 * Then for each pixel its minimum Mahalhanobis distance from all the kernels is computed. This new one dimensional
 * vector is used as training for LR which finally classifies the pixel as drivable or not.
 *
 * Since the OverheadMap only labels pixels the robot has driven on as positive, a higher weight should be given to the
 * positive (1) class during training.
 */
class LRDrivingSurfaceClassifier : public GMMDrivingSurfaceClassifier
{
public:
  explicit LRDrivingSurfaceClassifier(const Json::Value& config, const CozmoContext *context);

  using GMMDrivingSurfaceClassifier::PredictClass;
  uchar PredictClass(const std::vector<u8>& values) const override;

  bool Serialize(const char *filename) override
  {
    PRINT_NAMED_ERROR("LRDrivingSurfaceClassifier.SerializeNotImplemented", "Serialize is not implementd for "
                                                                            "LRDrivingSurfaceClassifier");
    return false;
  }

  bool DeSerialize(const char *filename) override
  {
    PRINT_NAMED_ERROR("LRDrivingSurfaceClassifier.SerializeNotImplemented", "DeSerialize is not implementd for "
                                                                            "LRDrivingSurfaceClassifier");
    return false;
  }

protected:

  bool Train(const cv::Mat& allInputs, const cv::Mat& allClasses, uint numberOfPositives) override;

  float _positiveClassWeight = 1.2; // This parameter should be overwritten by the Json config file
  float _trainingAlpha = 0.5;       // This parameter should be overwritten by the Json config file
  cv::Ptr<Anki::WeightedLogisticRegression> _logisticRegressor;
};

/****************************************************************
 *                     THDrivingSurfaceClassifier               *
 ****************************************************************/

/*
 * Threshold based drivable surface classifier. During training this class builds a Gaussian Mixture Model out of the
 * data. Then during prediction the minimum Mahlanobis distance between the test point and the kernels is computed,
 * and the result is thresholded. The threshold is automatically found as a multiple of the median of the training data.
 */
class THDrivingSurfaceClassifier : public GMMDrivingSurfaceClassifier
{
public:
  explicit THDrivingSurfaceClassifier(const Json::Value& config, const CozmoContext *context);

  using GMMDrivingSurfaceClassifier::PredictClass;
  uchar PredictClass(const std::vector<u8>& values) const override;

  bool TrainFromFiles(const char *positiveDataFileName, const char *negativeDataFileName) override;

  bool TrainFromFile(const char* positiveDataFilename);

  bool Serialize(const char *filename) override
  {
    PRINT_NAMED_ERROR("THDrivingSurfaceClassifier.SerializeNotImplemented", "Serialize is not implementd for "
                                                                            "THDrivingSurfaceClassifier");
    return false;
  }

  bool DeSerialize(const char *filename) override
  {
    PRINT_NAMED_ERROR("THDrivingSurfaceClassifier.SerializeNotImplemented", "DeSerialize is not implementd for "
                                                                            "THDrivingSurfaceClassifier");
    return false;
  }

protected:
  bool Train(const cv::Mat& allInputs, const cv::Mat&, uint) override;

  float FindThreshold(std::vector<float>& distances) const;

  float _threshold;
  float _medianMultiplier = 5.0; // This value might be changed by the Json config
};

/****************************************************************
 *                     DTDrivingSurfaceClassifier               *
 ****************************************************************/

/*
 * This class uses Decision Trees to classify drivable pixels. It does not build a Gaussian Mixture Model. It currently
 * is the fastest among the other classifiers and possibly the most accurate.
 */
class DTDrivingSurfaceClassifier : public DrivingSurfaceClassifier
{
public:
  explicit DTDrivingSurfaceClassifier(const Json::Value& config, const CozmoContext *context);
  DTDrivingSurfaceClassifier(const std::string& serializedFilename, const CozmoContext* context);

  uchar PredictClass(const std::vector<u8>& values) const override;

  bool Serialize(const char *filename) override;

  bool DeSerialize(const char *filename) override;

protected:
  cv::Ptr<cv::ml::DTrees> _dtree;
  bool Train(const cv::Mat& allInputs, const cv::Mat& allClasses, uint numberOfPositives) override;

};

} // namespace Cozmo
} // namespace Anki


#endif //COZMO_DRIVINGSURFACECLASSIFIER_H
