/**
 * File: testLogisticRegression.cpp
 *
 * Author: Lorenzo Riano
 * Created: 11/14/17
 *
 * Description: 
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "util/helpers/includeGTest.h" // Used in place of gTest/gTest.h directly to suppress warnings in the header
#include "coretech/common/engine/math/logisticRegression.h"

#include <random>

using namespace Anki;

inline float sigmoid(float x) {
  return float(1. / ( 1. + std::exp(-x)));
}

GTEST_TEST(TestLogisticRegression, Simplefitting) {

  const int nsamples = 1000;

  // Create the classifier
  cv::Ptr<cv::ml::LogisticRegression> clf = cv::makePtr<WeightedLogisticRegression>();
  clf->setLearningRate(0.1 * nsamples);
  clf->setIterations(1000);
  clf->setRegularization(cv::ml::LogisticRegression::REG_DISABLE);
  clf->setTrainMethod(cv::ml::LogisticRegression::BATCH);

  cv::Mat X(nsamples, 1, CV_32FC1);
  cv::Mat labels(nsamples, 1, CV_32FC1);
  // Generate data
  const float intercept = -1.0f;
  const float coeff = 3.0f;
  {
    // random number generator with known seed
    std::random_device rd;
    std::mt19937 mt(0);
    std::uniform_real_distribution<float> distRng(0.0, 1.0);
    std::normal_distribution<float> normalRng(0, 1);

    float* X_row = X.ptr<float>(0);
    float* label_row = labels.ptr<float>(0);
    for (int i = 0; i < nsamples; ++i) {
      const float x = normalRng(mt);
      const float linepred = intercept + coeff * x;
      const float prob = sigmoid(linepred);
      const float randomNumber = distRng(mt);
      const float y = float(randomNumber < prob);
      label_row[i] = y;
      X_row[i] = x;
    }
  }
  cv::Ptr<cv::ml::TrainData> trainData = cv::ml::TrainData::create(X,
                                                                   cv::ml::SampleTypes::ROW_SAMPLE,
                                                                   labels);
  const bool trainingSuccess = clf->train(trainData);
  ASSERT_TRUE(trainingSuccess);

  cv::Mat thetas = clf->get_learnt_thetas();
  const float learnedIntercept = thetas.at<float>(0, 0);
  const float learnedCoeff = thetas.at<float>(0, 1);
  EXPECT_PRED_FORMAT2(::testing::FloatLE, std::fabs(learnedCoeff - coeff), 0.1)
            << "Learned coeff: "<<learnedCoeff<<"\tReal: "<<coeff;
  EXPECT_PRED_FORMAT2(::testing::FloatLE, std::fabs(learnedIntercept - intercept), 0.1)
            << "Learned intercept: "<<learnedIntercept<<"\tReal: "<<intercept;

  // calculate error
  cv::Mat responses;
  clf->predict(trainData->getSamples(), responses);
  const float error = calculateError(responses, trainData->getResponses());
  EXPECT_PRED_FORMAT2(::testing::FloatLE, error, 0.15);
}

GTEST_TEST(TestLogisticRegression, AmbiguousFittingUseWeights) {
  const int nsamples = 1000;
  const float threshold = 8.0;
  const float positiveClassWeight = 5.0;

  // Create the classifier
  cv::Ptr<cv::ml::LogisticRegression> clf = cv::makePtr<WeightedLogisticRegression>();
  clf->setLearningRate(0.2 * nsamples);
  clf->setIterations(5000);
  clf->setRegularization(cv::ml::LogisticRegression::REG_DISABLE);
  clf->setTrainMethod(cv::ml::LogisticRegression::BATCH);

  cv::Mat X(nsamples, 1, CV_32FC1);
  cv::Mat labels(nsamples, 1, CV_32FC1);
  cv::Mat classWeights(nsamples, 1, CV_32FC1);

  float numberOfPositives = 0;
  // generate the data
  {
    // random number generator with known seed
    std::random_device rd;
    std::mt19937 mt(0);
    std::uniform_real_distribution<float> distRng(-10.5, 10.5);
    std::normal_distribution<float> normalRng(0, 1);

    float* X_row = X.ptr<float>(0);
    float* label_row = labels.ptr<float>(0);
    float* weight_row = classWeights.ptr<float>(0);
    for (int i = 0; i < nsamples; ++i) {

      const float x = distRng(mt);
      const float r = normalRng(mt);
      const float y = std::sin(0.75f*x) * 7.0f * x + 5.0f * r;

      // swap x and y
      X_row[i] = y;
      const float label = float(x > threshold);
      label_row[i] = label;
      if (label == 1.0) {
        weight_row[i] = positiveClassWeight;
        numberOfPositives++;
      }
      else {
        weight_row[i] = 1.0;
      }

    }
  }
  cv::Ptr<cv::ml::TrainData> trainData = cv::ml::TrainData::create(X,
                                                                   cv::ml::SampleTypes::ROW_SAMPLE,
                                                                   labels,
                                                                   cv::noArray(),
                                                                   cv::noArray(),
                                                                   classWeights,
                                                                   cv::noArray());
  const bool trainingSuccess = clf->train(trainData);
  ASSERT_TRUE(trainingSuccess);
  cv::Mat thetas = clf->get_learnt_thetas();

  // calculate error
  cv::Mat responses;
  clf->predict(trainData->getSamples(), responses);
  const float error = calculateError(responses, trainData->getResponses());
  EXPECT_PRED_FORMAT2(::testing::FloatLE, error, 0.15);

  // calculate number of positives
  const float predictedNumberOfPositives = float(cv::sum(responses)[0]);
  const float realPositivesRatio = numberOfPositives / float(nsamples);
  const float predictedPositivesRatio = predictedNumberOfPositives / float(nsamples);

  EXPECT_PRED_FORMAT2(::testing::FloatLE, std::fabs(realPositivesRatio - predictedPositivesRatio), 0.01)
            <<"Real ratio of positives: "<<realPositivesRatio<<", predicted: "<<predictedPositivesRatio;
}
