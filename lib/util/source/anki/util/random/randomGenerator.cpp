/**
 * File: randomGenerator.cpp
 *
 * Author: bsofman (boris) (yes he did, Damjan cleaned it up for util namespace)
 * Created: 6/11/2012
 *
 * Description: Random number generator class
 *
 **/
#include "util/random/randomGenerator.h"

#include "util/logging/logging.h"

namespace Anki{ namespace Util {


RandomGenerator::RandomGenerator(uint32_t seed)
: uniDbl(0, 1)
{
  SetSeed("", seed);
}

void RandomGenerator::SetSeed(const std::string& who, uint32_t seed)
{
  if ( 0 == seed ) {
    std::random_device rd;
    seed = rd();
  }

  rng.seed(seed);

  if(!who.empty())
  {
    // Log the actual random seed used and who set it
    Util::sInfoF("app.random_seed", {{DDATA, who.c_str()}}, "%u", seed);
  }
}

double RandomGenerator::GetNextDbl() const
{
  double r = uniDbl(rng);

  return r;
}

// Return a random floating point number in the range [0,maxVal).  This is much
// better than any sort of mod-based rand because that only focuses on the lower
// bits which are not as random as the higher bits.
double RandomGenerator::RandDbl(double maxVal) const
{
  return maxVal * GetNextDbl();
}


// Returns a random floating point number in the range [minVal, maxVal]
double RandomGenerator::RandDblInRange(double minVal, double maxVal) const
{
  return RandDbl(maxVal-minVal) + minVal;
}


// Returns a random integer in the range [0,numVals-1]
int RandomGenerator::RandInt(int numVals) const
{
  return (int)(RandDbl() * numVals);
}


// Generate a random integer in range [minVal, maxVal]
int RandomGenerator::RandIntInRange(int minVal, int maxVal) const
{
  return RandInt(maxVal-minVal+1) + minVal;
}


bool RandomGenerator::RandBool() const
{
  return (RandDbl(1.0) < 0.5);
}

} // namespace BaseStation
} // namespace
