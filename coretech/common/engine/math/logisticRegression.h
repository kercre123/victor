/**
 * File: logisticRegression.h
 *
 * Author: Lorenzo Riano
 * Created: 11/13/17
 *
 * Description: Implementation of the Logistic Regression algorithm in C++ in OpenCV. Modified
 * from the original to add weighted samples
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

/**
 * Original Author: Rahul Kavi rahulkavi[at]live[at]com
                    License Agreement

               For Open Source Computer Vision Library
  Copyright (C) 2000-2008, Intel Corporation, all rights reserved.

  Copyright (C) 2008-2011, Willow Garage Inc., all rights reserved.

  Third party copyrights are property of their respective owners.
*/

#ifndef _ANKICORETECH_COMMON_LOGISTICREGRESSION_H_
#define _ANKICORETECH_COMMON_LOGISTICREGRESSION_H_

#if ANKICORETECH_USE_OPENCV

#include "opencv2/core/core.hpp"
#include "opencv2/ml.hpp"


#define CV_IMPL_PROPERTY_RO(type, name, member) \
    inline type get##name() const { return member; }
#define CV_HELP_IMPL_PROPERTY(r_type, w_type, name, member) \
    CV_IMPL_PROPERTY_RO(r_type, name, member) \
    inline void set##name(w_type val) { member = val; }
#define CV_IMPL_PROPERTY(type, name, member) CV_HELP_IMPL_PROPERTY(type, type, name, member)

namespace Anki {

class LrParams
{
public:
  LrParams()
  {
    alpha = 0.001;
    num_iters = 1000;
    norm = cv::ml::LogisticRegression::REG_L2;
    train_method = cv::ml::LogisticRegression::BATCH;
    mini_batch_size = 1;
    term_crit = cv::TermCriteria(cv::TermCriteria::COUNT + cv::TermCriteria::EPS, num_iters, alpha);
  }

  double alpha; //!< learning rate.
  int num_iters; //!< number of iterations.
  int norm;
  int train_method;
  int mini_batch_size;
  cv::TermCriteria term_crit;
};

float calculateError( const cv::Mat& _p_labels, const cv::Mat& _o_labels);

class WeightedLogisticRegression : public cv::ml::LogisticRegression
{
public:

  WeightedLogisticRegression() = default;
  virtual ~WeightedLogisticRegression()  = default;

  CV_IMPL_PROPERTY(double, LearningRate, params.alpha)
  CV_IMPL_PROPERTY(int, Iterations, params.num_iters)
  CV_IMPL_PROPERTY(int, Regularization, params.norm)
  CV_IMPL_PROPERTY(int, TrainMethod, params.train_method)
  CV_IMPL_PROPERTY(int, MiniBatchSize, params.mini_batch_size)
  CV_IMPL_PROPERTY(cv::TermCriteria, TermCriteria, params.term_crit)

  virtual bool train( const cv::Ptr<cv::ml::TrainData>& trainData, int=0 );
  virtual float predict(cv::InputArray samples, cv::OutputArray results, int flags=0) const;
  virtual void clear();
  virtual void write(cv::FileStorage& fs) const;
  virtual void read(const cv::FileNode& fn);
  virtual cv::Mat get_learnt_thetas() const { return learnt_thetas; }
  virtual int getVarCount() const { return learnt_thetas.cols; }
  virtual bool isTrained() const { return !learnt_thetas.empty(); }
  virtual bool isClassifier() const { return true; }
  virtual cv::String getDefaultName() const { return "opencv_ml_lr"; }
protected:
  cv::Mat calc_sigmoid(const cv::Mat& data) const;
  double compute_cost(const cv::Mat& _data, const cv::Mat& _labels, const cv::Mat& _init_theta);
  void compute_gradient(const cv::Mat& _data, const cv::Mat& _labels, const cv::Mat& _weights,
                          const cv::Mat& _theta, const double _lambda, cv::Mat& _gradient);
  cv::Mat batch_gradient_descent(const cv::Mat& _data, const cv::Mat& _labels,
                                   const cv::Mat& _init_theta, const cv::Mat& _weights);
  cv::Mat mini_batch_gradient_descent(const cv::Mat& _data, const cv::Mat& _labels,
                                        const cv::Mat& _init_theta, const cv::Mat& _weights);
  bool set_label_map(const cv::Mat& _labels_i);
  cv::Mat remap_labels(const cv::Mat& _labels_i, const std::map<int, int>& lmap) const;
protected:
  LrParams params;
  cv::Mat learnt_thetas;
  std::map<int, int> forward_mapper;
  std::map<int, int> reverse_mapper;
  cv::Mat labels_o;
  cv::Mat labels_n;
};

} // namespace Anki

#endif //ANKICORETECH_USE_OPENCV
#endif //COZMO_LOGISTICREGRESSION_H
