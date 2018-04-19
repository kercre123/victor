/**
 * File: randomIndexSampler.cpp
 *
 * Author: ross
 * Created: 2018 Apr 3
 *
 * Description: Utility for sampling from the indices [0,N) without replacement.
 *              The results are NOT CONSECUTIVE (check out Vitter JS (1987) for consecutive).
 *              There are 3 related algorithms at work here, based on whether you care if the results are randomly
 *              shuffled.
 *              If you just want a shuffled vector of indices, use CreateFullShuffledVector
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "util/random/randomIndexSampler.h"
#include "util/logging/logging.h"
#include "util/math/math.h"
#include "util/random/randomGenerator.h"


namespace Anki{
namespace Util{

using OrderType = RandomIndexSampler::OrderType;
  
RandomIndexSampler::RandomIndexSampler( int populationSize, int sampleCount, OrderType order )
{
  Reset( populationSize, sampleCount, order );
}
  
void RandomIndexSampler::Reset( int populationSize, int sampleCount, OrderType order )
{
  _populationSize = populationSize;
  _sampleCount = sampleCount;
  _order = order;
  
  _sampleSwaps.clear();
  _otherIdx = _populationSize - 1;
  
  _floydSamples.clear();
  _floydIdx = _populationSize - _sampleCount;
  
  ANKI_VERIFY( _sampleCount <= _populationSize,
               "RandomIndexSampler.Reset.TooManySamples",
               "Requested [%d] samples in a population of [%d]",
               _sampleCount,
               _populationSize );
}
  
void RandomIndexSampler::Reset()
{
  Reset( _populationSize, _sampleCount, _order );
}

int RandomIndexSampler::GetNext( RandomGenerator& rng )
{
  if( _order == ORDER_SHOULD_BE_RANDOM ) {
    return GetNextRandomOrderSample( rng );
  } else if( _order == ORDER_DOESNT_MATTER ) {
    return GetNextFloydSample( rng );
  } else {
    DEV_ASSERT( false, "unsupported order type" );
    return -1;
  }
}
  
std::vector<int> RandomIndexSampler::GetAll( RandomGenerator& rng )
{
  if( _order == ORDER_SHOULD_BE_RANDOM ) {
    DEV_ASSERT( _floydSamples.empty(), "Don't interchange order parameter usage" );
    if( _populationSize == _sampleCount ) {
      // todo: test when this one is prefered based on _populationSize and _sampleCount (nlogn vs mlogn)
      // The RandomOrder sampler is probably best when _populationSize >> _sampleCount
      return CreateFullShuffledVector( rng, _populationSize );
    } else {
      return CreateFullRandomOrderSample( rng );
    }
  } else if( _order == ORDER_DOESNT_MATTER ) {
    DEV_ASSERT( _sampleSwaps.empty(), "Don't interchange order parameter usage" );
    return CreateFullFloydSample( rng );
  } else {
    DEV_ASSERT( false, "unsupported order type" );
    return {};
  }
}
  
int RandomIndexSampler::GetNextFloydSample( RandomGenerator& rng )
{
  if( !ANKI_VERIFY( _floydIdx < _populationSize,
                    "RandomIndexSampler.GetNextFloydSample.TooManyCalls",
                    "Already called sampler %d times",
                    _floydIdx ) )
  {
    return -1;
  }
  
  // Floyd's algorithm (yes, the same dude who was friends with Warshall, although this is a different algorithm)
  // The reason this one only runs when ORDER_DOESNT_MATTER is because if _populationSize==_sampleCount,
  // the sample will always be [0, 1, ... , _populationSize-1], which is a valid sample without replacement
  // of [0, 1, ... , _populationSize-1] if order doesn't matter! As sampleCount is decremented, more
  // order randomness is seen. In other words, the values' positions are not always random, but the subset is.
  
  int sample = rng.RandIntInRange(0, _floydIdx);
  auto insertPair = _floydSamples.insert( sample );
  if( !insertPair.second ) {
    // already existed. instead, use the index
    _floydSamples.insert( _floydIdx );
    sample = _floydIdx;
  }
  ++_floydIdx;
  return sample;
}
  
int RandomIndexSampler::GetNextRandomOrderSample( RandomGenerator& rng )
{
  if( !ANKI_VERIFY( _otherIdx >= 0,
                    "RandomIndexSampler.GetNextRandomOrderSample.TooManyCalls",
                    "Already alled sampler %d times",
                    _populationSize ) )
  {
    return -1;
  }
  
  // I don't know of a name or reference for this sampling algorithm (although given its simplicity
  // it must exist somewhere), so here's an explanation:
  // Consider the std::random_shuffle implementation (Fisher-Yates shuffle), which would be able to
  // take a full vector [0, 1, ..., _populationSize-1] and shuffle it. The implementation
  // for std::random_shuffle suggests something like (cppreference.com):
  //    template<class RandomIt, class RandomFunc>
  //    void random_shuffle(RandomIt first, RandomIt last, RandomFunc&& r)
  //    {
  //      typename std::iterator_traits<RandomIt>::difference_type i, n;
  //      n = last - first;
  //      for (i = n-1; i > 0; --i) {
  //        using std::swap;
  //        swap(first[i], first[r(i+1)]);
  //      }
  //    }
  // (In fact, this is used in CreateFullShuffledVector). You _could_ obtain a subset of _sampleCount
  // elements in the indices in [0, 1, ..., _populationSize-1] by running this for _sampleCount steps and
  // returning the _sampleCount last elems in the vector. But this would require initializing an entire
  // vector of all the indices in [0, 1, ..., _populationSize-1], which might not be desirable if, for
  // instance, _populationSize>>sampleCount. Instead, the below algorithm only holds a list of the swaps
  // that were made. If _sampleSwaps contains a key of integer i, then i has already been sampled. In
  // that case, the choice of sample is what value would exist in the full vector at that location if
  // we were keeping track of it.
  // You'll probably notice some similarities with Floyd's algorithm (also used in this class), which,
  // if you look up a proof, is much more clever, but its randomly-generated subsets do not have random order.
  
  int sample = rng.RandIntInRange(0, _otherIdx);
  auto it = _sampleSwaps.find( sample );
  if( it != _sampleSwaps.end() ) {
    int tmp = it->second;
    auto it2 = _sampleSwaps.find( _otherIdx );
    if( it2 != _sampleSwaps.end() ) {
      _sampleSwaps[sample] = it2->second;
    } else {
      _sampleSwaps[sample] = _otherIdx;
    }
    sample = tmp;
  } else {
    auto it2 = _sampleSwaps.find( _otherIdx );
    if( it2 != _sampleSwaps.end() ) {
      _sampleSwaps[sample] = it2->second;
    } else {
      _sampleSwaps[sample] = _otherIdx;
    }
  }
  
  // decrement for next call
  --_otherIdx;
  
  return sample;
}

std::vector<int> RandomIndexSampler::CreateFullShuffledVector( RandomGenerator& rng, int length )
{
  std::vector<int> shuffled( length ) ; // vector of size _populationSize
  std::iota(std::begin(shuffled), std::end(shuffled), 0); // fill with 0, 1, ..., length-1
  // a re-implementation of std::random_shuffle (Fisher-yates shuffle) with our rng
  for( int idx=length-1; idx>=0; --idx ) {
    int sample = rng.RandIntInRange(0, idx);
    std::swap( shuffled[idx], shuffled[sample] );
  }
  return shuffled;
}
  
std::vector<int> RandomIndexSampler::CreateFullRandomOrderSample( RandomGenerator& rng )
{
  std::vector<int> ret;
  ret.reserve( _sampleCount );
  for( size_t i=0; i<_sampleCount; ++i ) {
    ret.push_back( GetNextRandomOrderSample( rng ) );
  }
  return ret;
}

std::vector<int> RandomIndexSampler::CreateFullFloydSample( RandomGenerator& rng )
{
  std::vector<int> ret;
  ret.reserve( _sampleCount );
  for( size_t i=0; i<_sampleCount; ++i ) {
    ret.push_back( GetNextFloydSample( rng ) );
  }
  return ret;
}
  
} // namespace Util
} // namespace Anki


