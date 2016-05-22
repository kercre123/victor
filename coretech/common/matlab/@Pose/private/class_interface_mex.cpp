#include "mex.h"
#include "class_handle.hpp"

// The class we are interfacing to:
#include "anki/math/pose.h"

using namespace Anki;

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{	
    // Get the command string
    char cmd[64];
	if (nrhs < 1 || mxGetString(prhs[0], cmd, sizeof(cmd)))
		mexErrMsgTxt("First input should be a command string less than 64 characters long.");
        
    // New
    if (!strcmp("new", cmd)) {
        // Check parameters
        if (nlhs != 1)
            mexErrMsgTxt("New: One output expected.");
        // Return a handle to a new C++ instance
        plhs[0] = convertPtr2Mat<Pose3d>(new Pose3d);
        return;
    }
    
    // Check there is a second input, which should be the class instance handle
    if (nrhs < 2) {
		mexErrMsgTxt("Second input should be a class instance handle.");
    }
        
    // Delete
    if (!strcmp("delete", cmd)) {
        // Destroy the C++ object
        destroyObject<Pose3d>(prhs[1]);
        // Warn if other commands were ignored
        if (nlhs != 0 || nrhs != 2)
            mexWarnMsgTxt("Delete: Unexpected arguments ignored.");
        return;
    }
    
    // Get the class instance pointer from the second input
    Pose3d *thisPose = convertMat2Ptr<Pose3d>(prhs[1]);
    
    // Call the various class methods
    
    // getWithRespectTo
    if (!strcmp("getWithRespectTo", cmd)) {
        // Check parameters
        if (nlhs != 1 || nrhs != 3)
            mexErrMsgTxt("getWithRespectTo: Unexpected arguments.");
        
        Pose3d *otherPose = convertMat2Ptr<Pose3d>(prhs[2]);
        
        // Call the method and return the new Pose3d ptr
        Pose3d *newPose = new Pose3d(thisPose->getWithRespectTo(otherPose));
        plhs[0] = convertPtr2Mat<Pose3d>(newPose);
        
        return;
    }
    
    // 
    // Got here, so command not recognized
    mexErrMsgTxt("Command not recognized.");
}
