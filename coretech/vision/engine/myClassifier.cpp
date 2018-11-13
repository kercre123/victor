/**
 * File: myClassifier.cpp
 *
 * Author: Patrick Doran
 * Date: 2018-11-08
 */

#include "coretech/vision/engine/myClassifier.h"
#include "util/math/math.h"

#include "json/json.h"

#include <opencv2/core.hpp>

#include <algorithm>
#include <cmath>
#include <random>


// TODO: Remove
#include <iostream>
#include <fstream>
#include <unordered_map>

namespace Anki
{
namespace Vision
{

#if 0
namespace
{
std::ostream& operator<<(std::ostream& os, const MyClassifier::LabelStats& stats)
{
  os<<"Mean:            "<<stats.mean<<std::endl;
  os<<"Covariance: "<<std::endl<<stats.covariance<<std::endl;
  os<<"Covariance_inv: "<<std::endl<<stats.covariance_inv<<std::endl;
  os<<"Determinant:     "<<stats.determinant<<std::endl;
  os<<"Determinant_log: "<<stats.determinant_log<<std::endl;
  os<<"Eigenvalues:     "<<stats.eigenvalues<<std::endl;
  os<<"Eigenvectors:    "<<std::endl<<stats.eigenvectors<<std::endl;
  return os;
}
}
#endif

MyClassifier::MyClassifier ()
{
  Reset();
}

MyClassifier::~MyClassifier ()
{
}

void MyClassifier::Reset()
{
  _stats.clear();
  _dimensionality = 0;
}

bool MyClassifier::Train (const std::vector<cv::Mat>& samples)
{
  if (samples.empty())
    return false;

  _stats.clear();
  _stats.resize(static_cast<std::vector<LabelStats>::size_type>(samples.size()));

  // Sanity check sample matrices
  _dimensionality = samples[0].cols;
  for (auto index = 1; index < samples.size(); ++index){
    assert(_dimensionality == samples[index].cols);
  }

  for (auto index = 0; index < samples.size(); ++index)
  {
    LabelStats& stats = _stats[index];
    stats.samples = samples[index];
    cv::calcCovarMatrix(stats.samples, stats.covariance, stats.mean, CV_COVAR_NORMAL | CV_COVAR_ROWS);
    stats.covariance_inv = stats.covariance.inv();
    stats.determinant = cv::determinant(stats.covariance);
    stats.denominator = std::sqrt(std::pow(2*M_PI, _dimensionality));
    stats.determinant_log = log(stats.determinant);
    cv::eigen(stats.covariance, stats.eigenvalues, stats.eigenvectors);

#if 0
    std::cerr<<" ---- Stats["<<index<<"] ---- "<<std::endl;
    std::cerr<<stats<<std::endl;
#endif
  }

  return false;
}

void MyClassifier::Predict (const cv::Mat& samples, cv::Mat& labels) const
{
  for (auto index = 0; index < _stats.size(); ++index)
  {
    const LabelStats& stats = _stats[index];
    bool denominatorNearZero = Util::IsNearZero(stats.denominator);
    for (auto row = 0; row  < samples.rows; ++row)
    {
      float probability = 0.f;
      if (!denominatorNearZero)
      {
        const cv::Mat& sample = samples.row(row);
        cv::Mat num = (-0.5f * (sample - stats.mean).t() * stats.covariance_inv * (sample - stats.mean));
        float numerator = std::exp(num.at<double>(0));
        probability = numerator / stats.denominator;
      }
      labels.at<double>(row,index) = probability;
    }
  }
}

void MyClassifier::Classify (const cv::Mat& samples, cv::Mat& labelings, s32 unknown) const
{
  // Calculate the Mahalanobis distance to each cluster
  cv::Mat distances = cv::Mat::ones(samples.rows, static_cast<int>(_stats.size()), CV_64FC1);
  for (auto row = 0; row  < samples.rows; ++row)
  {
    const cv::Mat& sample = samples.row(row);
    for (auto index = 0; index < _stats.size(); ++index)
    {
      const LabelStats& stats = _stats[index];
      cv::Mat tmp = ((sample - stats.mean) * stats.covariance_inv * (sample - stats.mean).t());
      float distance = tmp.at<double>(0);
      distances.at<double>(row,index) = distance;
    }
  }

  // TODO: Make this be a  threshold computed from N where N means "N standard deviations away"
  float threshold = 0.000002f; // (distance^2)

  cv::Mat minDistances = cv::Mat::zeros(labelings.rows,labelings.cols,CV_64FC1);
  cv::Mat maxDistances = cv::Mat::zeros(labelings.rows,labelings.cols,CV_64FC1);

  for (auto row = 0; row < distances.rows; ++row)
  {
    const cv::Mat& distance = distances.row(row);

    double minVal;
    double maxVal;
    cv::Point minLoc;
    cv::Point maxLoc;
    cv::minMaxLoc(distance, &minVal, &maxVal, &minLoc, &maxLoc);
    minDistances.at<double>(row) = minVal;
    maxDistances.at<double>(row) = maxVal;
    // Minimum below threshold, useful if trying to find the minimum distance
    if (minVal < threshold){
      labelings.at<s32>(row) = minLoc.x;
    } else {
      labelings.at<s32>(row) = unknown;
    }
  }
}

void MyClassifier::Save (const std::string& filename)
{
  Json::Value root(Json::ValueType::objectValue);
  Json::Value clusters(Json::ValueType::arrayValue);
  for (auto label = 0; label < _stats.size(); ++label)
  {
    const LabelStats& stats = _stats[label];
    Json::Value mean(Json::ValueType::arrayValue);
    for (auto index = 0; index < stats.mean.cols; ++index)
    {
      mean.append(Json::Value(stats.mean.at<double>(index)));
    }
    Json::Value covariance(Json::ValueType::objectValue);
    covariance["rows"] = Json::Value(stats.covariance.rows);
    covariance["cols"] = Json::Value(stats.covariance.cols);
    {
      Json::Value data(Json::ValueType::arrayValue);
      for (auto index = 0; index < stats.covariance.rows*stats.covariance.cols; ++index)
      {
        covariance["data"].append(Json::Value(stats.covariance.at<double>(index)));
      }
    }
    Json::Value cluster(Json::ValueType::objectValue);
    cluster["mean"] = mean;
    cluster["covariance"] = covariance;

    clusters.append(cluster);
  }

  root["dimensionality"] = Json::Value(_dimensionality);
  root["clusters"] = clusters;

  std::ofstream os(filename);
  Json::StyledWriter writer;
  os << writer.write(root);
}

void MyClassifier::Load (const std::string& filename)
{
  Json::CharReaderBuilder rbuilder;
  rbuilder["collectComments"] = false;
  std::string errs;
  std::ifstream is(filename);
  Json::Value root;

  if (!Json::parseFromStream(rbuilder, is, &root, &errs))
  {
    return; // fail
  }

  Json::Value jsonClusters = root.get("clusters",Json::Value::null);
  if (jsonClusters.isNull() || jsonClusters.type() != Json::ValueType::arrayValue)
  {
    return; // fail;
  }

  Json::Value jsonDimensionality = root.get("dimensionality",Json::Value::null);
  if (jsonDimensionality.isNull() || jsonDimensionality.type() != Json::ValueType::arrayValue)
  {
    return; // fail;
  }
  s32 dimensionality = jsonDimensionality.asInt();


  std::vector<LabelStats> allStats;

  for (const Json::Value& jsonCluster : jsonClusters)
  {
    LabelStats stats;

    Json::Value jsonMean = jsonCluster.get("mean",Json::Value::null);
    Json::Value jsonCovariance = jsonCluster.get("covariance",Json::Value::null);

    if (jsonMean.isNull() || jsonMean.type() != Json::ValueType::arrayValue)
    {
      return; // fail
    }
    if (jsonCovariance.isNull() || jsonCovariance.type() != Json::ValueType::objectValue)
    {
      return; // fail
    }
    Json::Value jsonRows = jsonCovariance.get("rows",Json::Value::null);
    if (jsonRows.isNull() || jsonRows.isInt())
    {
      return; //fail
    }
    int rows = jsonRows.asInt();

    Json::Value jsonCols = jsonCovariance.get("cols",Json::Value::null);
    if (jsonCols.isNull() || jsonCols.isInt())
    {
      return; //fail
    }
    int cols = jsonCols.asInt();

    Json::Value jsonData = jsonCovariance.get("rows",Json::Value::null);
    if (jsonData.isNull() || jsonData.type() != Json::ValueType::arrayValue || jsonData.size() != rows*cols)
    {
      return; // fail
    }

    for (const Json::Value& value : jsonMean)
    {
      stats.mean.push_back(value.asDouble());
    }

    for (const Json::Value& value : jsonCovariance)
    {
      stats.covariance.push_back(value.asDouble());
    }

    stats.mean.reshape(1,{1,cols});
    stats.covariance.reshape(1,{rows,cols});

    allStats.push_back(stats);
  }

  // Compute the secondary statistical information
  for (LabelStats& stats : allStats)
  {
    stats.covariance_inv = stats.covariance.inv();
    stats.determinant = cv::determinant(stats.covariance);
    stats.denominator = std::sqrt(std::pow(2*M_PI, dimensionality));
    stats.determinant_log = log(stats.determinant);
    cv::eigen(stats.covariance, stats.eigenvalues, stats.eigenvectors);
  }

  Reset();
  _stats = allStats;
  _dimensionality = dimensionality;
}

} /* namespace Vision */
} /* namespace Anki */
