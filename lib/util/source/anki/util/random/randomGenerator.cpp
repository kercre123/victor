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

#include "util/logging/DAS.h"
#include "util/logging/logging.h"

#define LOG_CHANNEL "RandomGenerator"

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
    DASMSG(random_generator.seed, "random_generator.seed",
           "RandomGenerator::SetSeed was called to set a new random seed");
    DASMSG_SET(i1, seed, "New random seed");
    DASMSG_SET(s1, who, "Identifier for who set the random seed");
    DASMSG_SEND();
    
    LOG_INFO("random_generator.seed", "seed %u, who %s", seed, who.c_str());
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


// Returns a random floating point number in the range [minVal, maxVal)
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


bool RandomGenerator::RandBool(double probTrue) const
{
  return (RandDbl(1.0) < probTrue);
}

} // namespace BaseStation
} // namespace
