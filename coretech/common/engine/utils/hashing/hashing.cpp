/**
 * File: hashing
 *
 * Author: damjan
 * Created: 5/6/14
 * 
 * Description: 
 * 
 *
 * Copyright: Anki, Inc. 2014
 *
 **/
#include "coretech/common/engine/utils/hashing/hashing.h"
#include <stdio.h>

namespace AnkiUtil
{

// used to create hashes for reproducibility.
#define HASHING_VALUE 19

/*
// TODO: (DS)(BN) timer should move to ankiutil namespace
// damjan commented out this function so that we don't have to create
// cyclical namespace dependency
bool _DEBUG_REPRO_HAS_ENABLED;
bool _checkReproHash()
{
  if(_DEBUG_REPRO_HAS_ENABLED)
    return true;
  return (_DEBUG_REPRO_HAS_ENABLED =
              BaseStationTimer::getInstance()->GetCurrentTimeInSeconds() >= DEBUG_REPRO_HASH_ENABLE_TIME_S);
}
*/

//#define CHECK_REPRO_HASH (DEBUG_REPRO_HASH && _checkReproHash())
#define CHECK_REPRO_HASH (DEBUG_REPRO_HASH)

void _AddHash(unsigned int& value, const unsigned int newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    static unsigned long long int numHashes;
    printf("hv: %08X [hash#%8llu] item \"%s\"\n", newValue, numHashes, str);
    numHashes++;
  }

  value = value * HASHING_VALUE + newValue;
}


void AddHash(unsigned int& value, const unsigned int newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %35u ", newValue);
  }
  _AddHash(value, newValue, str);
}

void AddHash(unsigned int& value, const int newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %+35d ", newValue);
  }
  _AddHash(value, newValue, str);
}

void AddHash(unsigned int& value, const short newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %+35d ", newValue);
  }
  _AddHash(value, newValue, str);
}

void AddHash(unsigned int& value, const unsigned short newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %35d ", newValue);
  }
  _AddHash(value, newValue, str);
}

void AddHash(unsigned int& value, const bool newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("bv: %s ", newValue ? "true" : "false");
  }
  _AddHash(value, newValue ? 1 : 0, str);
}

void AddHash(unsigned int& value, const char newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %+35d ", newValue);
  }
  _AddHash(value, newValue, str);
}

void AddHash(unsigned int& value, const unsigned char newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("iv: %35u ", newValue);
  }
  _AddHash(value, newValue, str);
}


void AddHash(unsigned int& value, const float newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("fv: %+35.26e ", newValue);
  }

  union {
    float floatValue;
    unsigned int uintValue;
  } floatConverter;

  floatConverter.floatValue = newValue;
  _AddHash(value, floatConverter.uintValue, str);
}

void AddHash(unsigned int& value, const double newValue, const char* str)
{
  if(CHECK_REPRO_HASH) {
    printf("dv: %+35.26e ", newValue);
  }

  union {
    double doubleValue;
    unsigned int uintValue;
  } doubleConverter;

  doubleConverter.doubleValue = newValue;
  _AddHash(value, doubleConverter.uintValue, str);
}

}// end namespace AnkiUtil
