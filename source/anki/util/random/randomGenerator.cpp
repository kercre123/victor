/**
 * File: randomGenerator.cpp
 *
 * Author: bsofman (boris) (yes he did, Damjan cleaned it up for util namespace)
 * Created: 6/11/2012
 * 
 * Description: Random number generator class
 *
 **/
#include "randomGenerator.h"

namespace Anki{ namespace Util {


RandomGenerator::RandomGenerator(uint32_t seed)
: uniDbl(0, 1)
{
  if ( 0 == seed ) {
    std::random_device rd;
    rng.seed(rd());
  } else{
    rng.seed(seed);
  }
}


double RandomGenerator::GetNextDbl()
{
  double r = uniDbl(rng);

  return r;
}

// Return a random floating point number in the range [0,maxVal).  This is much
// better than any sort of mod-based rand because that only focuses on the lower
// bits which are not as random as the higher bits.
double RandomGenerator::RandDbl(double maxVal) 
{
  return maxVal * GetNextDbl();
}

 
// Returns a random floating point number in the range [minVal, maxVal]
double RandomGenerator::RandDblInRange(double minVal, double maxVal)
{
  return RandDbl(maxVal-minVal) + minVal;
}


// Returns a random integer in the range [0,numVals-1]
int RandomGenerator::RandInt(int numVals) 
{
  return (int)(RandDbl() * numVals); 
}


// Generate a random integer in range [minVal, maxVal]
int RandomGenerator::RandIntInRange(int minVal, int maxVal) 
{
  return RandInt(maxVal-minVal+1) + minVal;
}


} // namespace BaseStation
} // namespace


