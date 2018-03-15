/**
 * File: stringutils.h
 *
 * Author: seichert
 * Created: 2/16/2018
 *
 * Description: Routines for working with strings
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#pragma once

#include <string>
#include <vector>

bool AreCaseInsensitiveStringsEqual(const std::string& s1, const std::string& s2);
std::string byteVectorToHexString(const std::vector<uint8_t>& byteVector,
                                  const int numSpaces = 0,
                                  const bool lowercase = false);