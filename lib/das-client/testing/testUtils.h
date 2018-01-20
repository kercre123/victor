/**
 * File: testUtils
 *
 * Author: seichert
 * Created: 01/27/15
 *
 * Description: Simple utilities to help with unit tests
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

#ifndef __TestUtils_H__
#define __TestUtils_H__

#include <ftw.h>
#include <string>

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);
int rmrf(const char *path);
int truncate_to_zero(const char* fpath);
int write_string_to_file(const char* fpath, const char* data);
void copy_file(std::string source, std::string destination);

#endif // __TestUtils_H__
