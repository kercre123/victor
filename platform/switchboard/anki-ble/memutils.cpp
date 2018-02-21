/**
 * File: memutils.cpp
 *
 * Author: seichert
 * Created: 2/8/2018
 *
 * Description: Memory utilities
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#include "memutils.h"
#include <stdlib.h>
#include <string.h>

void *malloc_zero(size_t size)
{
  void* p = malloc(size);
  if (p) {
    memset(p, 0, size);
  }
  return p;
}