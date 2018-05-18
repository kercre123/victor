/**
 * File: randomGenerator.h
 *
 * Author: bsofman (boris) (yes he did, Damjan cleaned it up for util namespace)
 * Created: 6/11/2012
 * 
 * Description: Random number generator class
 *
 **/
#ifndef __Util_Random_RandomGenerator_H__
#define __Util_Random_RandomGenerator_H__

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-register"
#if __has_warning("-Wextern-c-compat")
#pragma GCC diagnostic ignored "-Wextern-c-compat"
#endif
#include <random>
#pragma GCC diagnostic pop
#include <cstdint>

namespace Anki {
namespace Util {

class RandomGenerator
{
public:

  RandomGenerator(uint32_t seed = 0);

  // Updates the seed and logs who did it as a DAS event. Use seed=0 to get a
  // randomly chosen new seed.
  void SetSeed(const std::string& who, uint32_t seed);
  
  // Return a random floating point number in the range [0,maxVal).  This is much
  // better than any sort of mod-based rand because that only focuses on the lower
  // bits which are not as random as the higher bits.
  double RandDbl(double maxVal = 1.0) const;

  // Returns a random floating point number in the range [minVal, maxVal]
  double RandDblInRange(double minVal, double maxVal) const;

  // Returns a random integer in the range [0,numVals-1]
  int RandInt(int numVals) const;

  // Generates number in the whole range of given type
  template <typename T>
  T RandT();
  
  // Generate a random integer in range [minVal, maxVal]
  int RandIntInRange(int minVal, int maxVal) const;

private:

  // Returns the next double in [0,1)
  double GetNextDbl() const;

  // Instance of random number generator
  mutable std::mt19937 rng;

  // Define a random variate generator using our base generator and distribution
  mutable std::uniform_real_distribution<> uniDbl;

};



// Generates number in the whole range of given type
template <typename T>
T RandomGenerator::RandT() {
  double maxVal = std::numeric_limits<T>::max();
  double minVal = std::numeric_limits<T>::min();
  double range = (maxVal-minVal+1);
  return (T)((GetNextDbl() * range) + minVal);
}


} // namespace Util
} // namespace Anki

#endif // __Util_Random_RandomGenerator_H__

