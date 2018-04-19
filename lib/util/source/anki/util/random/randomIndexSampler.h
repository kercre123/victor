/**
 * File: randomIndexSampler.h
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

#ifndef __Util_Random_RandomIndexSampler_H__
#define __Util_Random_RandomIndexSampler_H__

#include <unordered_set>
#include <unordered_map>
#include <cstddef>

namespace Anki{
namespace Util{
  
class RandomGenerator;
  
class RandomIndexSampler {
public:

  enum OrderType {
    ORDER_DOESNT_MATTER,
    ORDER_SHOULD_BE_RANDOM,
  };
  
  // create a sampler to sample sampleCount indices (without replacement) from [0, populationSize).
  // If order==ORDER_DOESNT_MATTER, you'll get indices generated from an efficient method that may
  // cause initial samples to be consecutive or nearly consecutive, especially if sampleCount == populationSize,
  // although the full sample will of course be uniformly distributed. Use this if you care about
  // a random subset (i.e., random values) but don't care about the order.
  // If order=ORDER_SHOULD_BE_RANDOM, you'll get indices that are randomly chosen (i.e.,
  // one with both random order and random values)
  RandomIndexSampler( int populationSize, int sampleCount, OrderType order=ORDER_DOESNT_MATTER );
  
  // You can either get all samples all at once, or request them individually. The latter may be
  // useful if you're using this class for rejection sampling. Either method will assert if both are
  // called without a Reset()
  
  // get all samples (the order may/may not be random based on your constructor parameters!)
  std::vector<int> GetAll( RandomGenerator& rng );
  
  // get the next sample (the order may/may not be random based on your constructor parameters!).
  // returns negative if the number of calls is > the supplied population size.
  int GetNext( RandomGenerator& rng );
  
  // start over
  void Reset();
  
  // start over with a new populationSize and sampleCount and possibly ordering
  void Reset( int populationSize, int sampleCount, OrderType order=ORDER_DOESNT_MATTER );
  
  // shortcut for getting a vector of all indices [0, 1, ..., length-1] shuffled randomly
  static std::vector<int> CreateFullShuffledVector( RandomGenerator& rng, int length );
  
  
private:
  
  // get the next sample from floyd's algorithm
  int GetNextFloydSample( RandomGenerator& rng );
  // get the next sample for the case of random order
  int GetNextRandomOrderSample( RandomGenerator& rng );
  
  // methods to get a full vector all at once (which may call the incremental versions)
  
  // run floyd's algorithm to completion and return it
  std::vector<int> CreateFullFloydSample( RandomGenerator& rng );
  // run the other algorithm for random order samples and return it
  std::vector<int> CreateFullRandomOrderSample( RandomGenerator& rng );
  
  
  // used for ORDER_DOESNT_MATTER
  std::unordered_set<int> _floydSamples;
  int _floydIdx;
  
  // used for ORDER_SHOULD_BE_RANDOM
  std::unordered_map<int,int> _sampleSwaps;
  int _otherIdx;
  
  // use for both
  int _populationSize;
  int _sampleCount;
  OrderType _order;
  
};

}
}

#endif
