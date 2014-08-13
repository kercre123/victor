/**
 * File: hashing
 *
 * Author: damjan
 * Created: 5/6/14
 * 
 * Description: helper functions for hashing values together
 * 
 *
 * Copyright: Anki, Inc. 2014
 *
 **/

#ifndef UTIL_HASHING_H_
#define UTIL_HASHING_H_

namespace AnkiUtil
{

// Print the state and position hash each time we do it for reproducibility.
#define DEBUG_REPRO_HASH 0

#define ADD_HASH(v, n) AddHash((v), (n), (#n))

// Functions for hashing. updates value by hashing in the given newValue
void AddHash(unsigned int& value, const unsigned int newValue, const char* str = "");
void AddHash(unsigned int& value, const int newValue, const char* str = "");
void AddHash(unsigned int& value, const short newValue, const char* str = "");
void AddHash(unsigned int& value, const bool newValue, const char* str = "");
void AddHash(unsigned int& value, const unsigned short newValue, const char* str = "");
void AddHash(unsigned int& value, const char newValue, const char* str = "");
void AddHash(unsigned int& value, const unsigned char newValue, const char* str = "");
void AddHash(unsigned int& value, const float newValue, const char* str = "");
void AddHash(unsigned int& value, const double newValue, const char* str = "");



} // end namespace AnkiUtil

#endif //UTIL_HASHING_H_
