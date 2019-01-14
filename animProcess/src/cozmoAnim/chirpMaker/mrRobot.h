/**
 * File: because it had to be done
 *
 */

#ifndef ANIMPROCESS_COZMO_MRROBOTO_H
#define ANIMPROCESS_COZMO_MRROBOTO_H
#pragma once

#include <vector>
#include <cstdint>

namespace Anki {
namespace Vector {
  
struct Chirp;
  
class MrRoboto
{
public:
  
  static void BasePart(std::vector<Chirp>& output, uint64_t startTime_ms);
  
  static void TreblePart(std::vector<Chirp>& output, uint64_t startTime_ms);
};


} // namespace Vector
} // namespace Anki

#endif // ANIMPROCESS_COZMO_CHIRPMAKER_H
