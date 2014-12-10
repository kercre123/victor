// Show a set of voxels

// Example:
// TODO

#include "mex.h"

#include "anki/common/robot/matlabInterface.h"
#include "anki/common/robot/serialize.h"

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/matlab/mexUtilities.h"
#include "anki/common/shared/utilities_shared.h"

#include "showVoxels.h"

#include <stdlib.h>

using namespace Anki;
using namespace Anki::Embedded;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);
  
  const int WINDOW_WIDTH = 640;
  const int WINDOW_HEIGHT = 480;

	//Start up SDL and create window
	if(init(WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
    return;
	}
  
  VoxelBuffer newVoxels;
  newVoxels.numVoxels = 3;
  newVoxels.voxels = reinterpret_cast<Voxel*>( calloc(newVoxels.numVoxels*sizeof(Voxel), 1) );
  
  newVoxels.voxels[0] = Voxel(0, 0, 0, 1, 0, 0);
  newVoxels.voxels[1] = Voxel(0, 1, 0, 0, 1, 0);
  newVoxels.voxels[2] = Voxel(1, 0, 0, 0, 0, 1);
  
  updateVoxels(&newVoxels);
  
  gameMain();
  
//  close();
} // mexFunction()
