// Show a set of voxels
// mexShowVoxels(voxels, <pauseSeconds>);
// voxels is a 6xn array of [x,y,z,red,green,blue]
//
// Example:
// mexShowVoxels(single([0,0,0,1,0,0;0,0,1,0,1,0;0,1,0,0,0,1]'), 1);

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
  
  f64 pauseSeconds = 0;
  
  AnkiConditionalErrorAndReturn((nrhs >= 1 && nrhs <= 2) && nlhs == 0, "mexShowVoxels", "Call this function as follows: mexShowVoxels(voxels, <pauseSeconds>);\nExample:\nmexShowVoxels(single([0,0,0,1,0,0;0,0,1,0,1,0;0,1,0,0,0,1]'), 1);");

  if(nrhs > 1) {
    pauseSeconds = mxGetScalar(prhs[1]);
  }

  AnkiConditionalErrorAndReturn(mxGetClassID(prhs[0]) == mxSINGLE_CLASS && mxGetM(prhs[0]) == 6,
    "mexShowVoxels", "voxels must be a 6xn single matrix");
  
	//Start up SDL and create window
	if(init(WINDOW_WIDTH, WINDOW_HEIGHT) < 0) {
    return;
	}
  
  VoxelBuffer newVoxels;
  newVoxels.numVoxels = mxGetN(prhs[0]);
  newVoxels.voxels = reinterpret_cast<Voxel*>( mxGetData(prhs[0]) );
  
  updateVoxels(&newVoxels);
  
  gameMain(pauseSeconds);
  
//  close();
} // mexFunction()
