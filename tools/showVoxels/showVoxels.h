/**
File: showVoxels.h
Author: Peter Barnum
Created: 2014-12-09

Show a set of voxels

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#ifndef __SHOW_VOXELS_H__
#define __SHOW_VOXELS_H__
  #include <vector>

  namespace Anki
  {
    typedef struct Voxel
    {
      float x, y, z;
      float red, green, blue;
      
      Voxel(const float x, const float y, const float z, const float red, const float green, const float blue)
        : x(x), y(y), z(z), red(red), green(green), blue(blue)
      {
      }
    } Voxel;

    int init(const int windowWidth, const int windowHeight);

    int gameMain();

    int close();

    int updateVoxels(const std::vector<Voxel> &newVoxels);
  } // namespace Anki

#endif // #ifndef __SHOW_VOXELS_H__
