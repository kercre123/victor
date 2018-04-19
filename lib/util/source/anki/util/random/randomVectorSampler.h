/**
 * File: randomVectorSampler.h
 *
 * Author: Brad Neuman
 * Created: 2017-10-31
 *
 * Description: Utility for sampling a random vector with given weights, with replacement
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#ifndef __Util_Random_RandomVectorSampler_H__
#define __Util_Random_RandomVectorSampler_H__

#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/random/randomGenerator.h"

namespace Anki{
namespace Util {

template<typename T>
class RandomVectorSampler {
public:

  RandomVectorSampler();
  RandomVectorSampler(const std::vector<T>& values, const std::vector<float>& weights);

  void PushBack(const T& val, float weight);
  void PushBack(T&& val, float weight);

  void Clear();

  // is considered empty if there are no entries, or if the sum of the weights is near zero
  bool Empty() const;

  // Erases the (first instance of the ) given element, uses ==
  // operator to find val
  void Remove(const T& val);

  const T& Sample(RandomGenerator& rng) const;

  // returns true if the value is contained in the vector, even if it
  // has 0 probability of being sampled
  bool Contains( const T& val ) const;

protected:

  void ReWeight();

  std::vector<T> _v;
  std::vector<float> _weights;
  float _weightSum;

  // these will be updated during Sample calls if needed, contains
  // cumulative probabilities
  std::vector<float> _cumlProbs;
};

///////////////////////////////////////////////////////////////////////////////

template<typename T>
RandomVectorSampler<T>::RandomVectorSampler() :
  _weightSum(0.0)
{
}

template<typename T>
  RandomVectorSampler<T>::RandomVectorSampler(const std::vector<T>& values, const std::vector<float>& weights) :
  _v(values),
  _weights(weights),
  _weightSum(0.0)
{
  if(values.size() != weights.size()) {
    PRINT_NAMED_ERROR("RandomVectorSampler.InvalidConstruction",
                      "sizes of values and weights must match!");
    Clear();
  }

  for(std::vector<float>::const_iterator it = _weights.begin();
      it != _weights.end();
      ++it) {
    _weightSum += *it;
  }

  ReWeight();
}

template<typename T>
void RandomVectorSampler<T>::PushBack(const T& val, float weight)
{
  _v.push_back(val);
  _weights.push_back(weight);
  _weightSum += weight;

  ReWeight();
}

template<typename T>
void RandomVectorSampler<T>::PushBack(T&& val, float weight)
{
  _v.emplace_back(val);
  _weights.push_back(weight);
  _weightSum += weight;

  ReWeight();
}

template<typename T>
void RandomVectorSampler<T>::Clear()
{
  _v.clear();
  _weights.clear();
  _cumlProbs.clear();
  _weightSum = 0.0;
}

template<typename T>
bool RandomVectorSampler<T>::Empty() const
{
  if( _v.empty() ) {
    return true;
  }

  for( const auto& w : _weights ) {
    if( !IsNearZero(w) ) {
      // has a non-zero weight, not empty
      return false;
    }
  }

  // has weights, but they are all zero! consider this empty
  return true;
}

template<typename T>
void RandomVectorSampler<T>::Remove(const T& val)
{
  // find val
  bool found = false;
  size_t end = _v.size();
  for(size_t i=0; i<end; ++i) {
    if(_v[i] == val) {
      found = true;

      _v.erase(_v.begin() + i);
      _weightSum -= _weights[i];
      _weights.erase(_weights.begin() + i);

      ReWeight();
      return;
    }
  }

  if(!found) {
    PRINT_NAMED_WARNING("RandomVectorSampler.CannotRemove",
                            "element not found!");
  }
}

template<typename T>
const T& RandomVectorSampler<T>::Sample(RandomGenerator& rng) const
{
  if(_cumlProbs.empty()) {
    PRINT_NAMED_ERROR("RandomVectorSampler.CannotSample",
                          "The vector is empty!");
    // TODO:(bn) return something here? Since T is a template argument, it's hard to know what to return, so
    // this just allows undefined behavior for now
  }

  float r = (float)rng.RandDbl();
  size_t i = 0;
  size_t end = _cumlProbs.size() - 1;
  while(i < end && r >= _cumlProbs[i])
    ++i;

  if(IsNearZero(_weights[i])) {
    PRINT_NAMED_WARNING("RandomVectorSampler.SampledZeroProbability",
                        "sampled an element with weight near zero!");
  }

  assert(i < _v.size());
  return _v[i];
}


template<typename T>
bool RandomVectorSampler<T>::Contains( const T& val ) const
{
  return std::count(_v.begin(), _v.end(), val) > 0;
}


template<typename T>
void RandomVectorSampler<T>::ReWeight()
{
  _cumlProbs.resize(_weights.size(), 0.0f);

  if( !IsNearZero(_weightSum) ) {

    float last = 0.0;
    float recip = 1.0/_weightSum;

    size_t size = _cumlProbs.size();
    for(size_t i=0; i<size; ++i) {
      last += recip * _weights[i];
      _cumlProbs[i] = last;
    }

    assert(FLT_NEAR(_cumlProbs.back(), 1.0));
  }
}

// TODO:(bn) port unit tests from OD repo!!

}
}

#endif
