/**
File: showVoxels.cpp
Author: Peter Barnum
Created: 2014-12-09

Show a set of voxels

Based on tips at
http://lazyfoo.net/tutorials/SDL/50_SDL_and_opengl_2/index.php
https://www3.ntu.edu.sg/home/ehchua/programming/opengl/CG_Examples.html

Copyright Anki, Inc. 2014
For internal use only. No part of this code may be used without a signed non-disclosure agreement with Anki, inc.
**/

#include "showVoxels.h"

#include <stdlib.h>

using namespace Anki;

int main(int argc, char* args[])
{
  const int WINDOW_WIDTH = 640;
  const int WINDOW_HEIGHT = 480;

	//Start up SDL and create window
	if(init(WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
    return -1;
	}
  
  VoxelBuffer newVoxels;
  newVoxels.numVoxels = 3;
  newVoxels.voxels = reinterpret_cast<Voxel*>( calloc(newVoxels.numVoxels*sizeof(Voxel), 1) );
  
  newVoxels.voxels[0] = Voxel(0, 0, 0, 1, 0, 0);
  newVoxels.voxels[1] = Voxel(0, 1, 0, 0, 1, 0);
  newVoxels.voxels[2] = Voxel(1, 0, 0, 0, 0, 1);
  
  updateVoxels(&newVoxels);
  
  gameMain(0);
  
	return close();
} // int main(int argc, char* args[])

