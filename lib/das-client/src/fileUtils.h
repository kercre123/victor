/**
 * File: fileUtils
 *
 * Author: seichert
 * Created: 01/20/15
 *
 * Description: Utilities for files
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __FileUtils_H__
#define __FileUtils_H__

#include <string>

namespace AnkiUtil
{
bool FileExistsAtPath(const std::string& path);
} // namespace AnkiUtil
#endif // __FileUtils_H__
