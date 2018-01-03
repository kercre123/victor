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

#include "testUtils.h"
#include <stdio.h>
#include <string.h>
#include <fstream>

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
  int rv = remove(fpath);

  if (rv) {
    perror(fpath);
  }

  return rv;
}

int rmrf(const char *path)
{
  return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

int truncate_to_zero(const char* fpath)
{
  return write_string_to_file(fpath, NULL);
}

int write_string_to_file(const char* fpath, const char* data)
{
  int rv = 0;
  FILE* f = fopen(fpath, "w");
  if ((FILE *) 0 == f) {
    return -1;
  }
  if (data && *data) {
    size_t len = strlen(data);
    size_t ret = fwrite(data, sizeof(*data), len, f);
    if (ret == len) {
      rv = 0;
    } else {
      rv = -1;
    }
  }
  rv = fclose(f) | rv; f = (FILE *) 0;
  return rv;
}

void copy_file(std::string source, std::string destination)
{
  std::ifstream src(source);
  std::ofstream dst(destination);
  dst << src.rdbuf();
}
