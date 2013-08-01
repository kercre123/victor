#include <mex.h>

#include <math.h>
#include <cstdlib> 
#include <iostream>

#include "opencv2/core/core.hpp"
#include "opencv2/contrib/contrib.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include "mexWrappers.h"

/*
 * [X_final, R_final, T_final] = mexBundleAdjust(x_obs, X_world, R, T, K, radDistortionCoeffs)
 * 
 * Inputs:
 *   x_obs = cell array with numPoses elements, each containt an Nx2 array 
 *           of observed image points. Unobserved points should have 
 *           x = y = -1.
 *
 *   X     = Nx3 array of 3D world points.
 *
 *   R,T   = numPoses-length cell array of 3x3 and 3x1 rotation and 
 *           translation information, respectively
 *
 * Outputs:
 *  X_final, R_final, T_final - correspondingly-sized arrays of world 
 *    coordinates, rotation matrices, and translation vectors, respectively.
 *
 */  
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
    // TODO: pass this in at some point
    cv::Size  cameraRes(640, 480);
    
    mxAssert(mxIsCell(prhs[0]), //&& mxGetNumberOfElements(prhs[0])==numPoses,
             "Observed image points should be a cell array with numPoses elements.");
    
    int numPoses = (int) mxGetNumberOfElements(prhs[0]);
    
    mxAssert(mxIsCell(prhs[2]) && mxIsCell(prhs[3]) &&
             mxGetNumberOfElements(prhs[2])==numPoses &&
             mxGetNumberOfElements(prhs[3])==numPoses,
             "R and T should be cell arrays of numPoses poses.");

    // See how many poses we're actually dealing with:
    // (Ignore empty cell array elements)
    for(int i = numPoses-1; i >= 0; --i) {
        mxArray *pose = mxGetCell(prhs[0], i);
        if(pose==NULL || mxIsEmpty(pose)) {
            numPoses--;
        }
    }           
    
    const int numPts = (int) mxGetM(prhs[1]);
    mxAssert(mxGetN(prhs[1])==3,
            "World points should be [numPts x 3] array.");
    
    // variables for OpenCV's bundle adjustment call
    std::vector<cv::Point3d>                worldPts(numPts);
    std::vector<std::vector<cv::Point2d> >  imagePts(numPoses);
    std::vector<std::vector<int> >          visibility(numPoses);
    std::vector<cv::Mat>                    camCalibMats(numPoses);
    std::vector<cv::Mat>                    R(numPoses);
    std::vector<cv::Mat>                    T(numPoses);
    std::vector<cv::Mat>                    distCoeffs(numPoses);
    cv::TermCriteria                        criteria(CV_TERMCRIT_ITER+CV_TERMCRIT_EPS, 70, 1e-10);
    
    
    // Copy in 2D image points and set up visibility indicator vector
    for(int i_pose=0; i_pose<numPoses; ++i_pose) {
        const mxArray *ptsArray = mxGetCell(prhs[0], i_pose);
        mxAssert(mxGetM(ptsArray)==numPts && mxGetN(ptsArray)==2,
                "Image points should be [numPts x 2].");
        
        const double *x = mxGetPr(ptsArray);
        const double *y = x + numPts;

        imagePts[i_pose].resize(numPts);
        visibility[i_pose].resize(numPts);
        for(int i=0; i<numPts; ++i) {
            imagePts[i_pose][i].x = x[i];
            imagePts[i_pose][i].y = y[i];
            
            if(x[i] >= 0.0 && y[i] >= 0.0) {
                visibility[i_pose][i] = 1;
            } 
            else {
                visibility[i_pose][i] = 0;
            }
        }
    }
    
    // Copy in 3D points:
    const double *X = mxGetPr(prhs[1]);
    const double *Y = X + numPts;
    const double *Z = Y + numPts;
    
    for(int i=0; i<numPts; ++i) {
        worldPts[i].x = X[i];
        worldPts[i].y = Y[i];
        worldPts[i].z = Z[i];
    }
    
    // Get the poses:
    for(int i_pose=0; i_pose<numPoses; ++i_pose) {
        
        mxArray2cvMat(mxGetCell(prhs[2], i_pose), R[i_pose]);
        mxAssert(R[i_pose].rows==3 && R[i_pose].cols==3,  
                "Expecting 3x3 rotation matrices.");
        
        mxArray2cvMat(mxGetCell(prhs[3], i_pose), T[i_pose]);
        mxAssert(T[i_pose].rows==3 && T[i_pose].cols==1,  
                "Expecting 3x1 translation vectors.");
    }
    
                       
    // Get camera calibration information:
    cv::Mat_<double> camCalibMat;
    mxArray2cvMatScalar(prhs[4], camCalibMat);
    mxAssert(camCalibMat.rows==3 && camCalibMat.cols==3,
            "Expecting camera calibration matrix to be 3x3.");
    cv::Mat_<double> distCoeffVec;
    mxArray2cvMatScalar(prhs[5], distCoeffVec);
    mxAssert(distCoeffVec.rows==4 && distCoeffVec.cols==1,
            "Expecting radial distortion coefficients to be 4x1 vector.");            
    
    // For now, will assume we have one camera in a bunch of poses
    camCalibMats.resize(numPoses, camCalibMat);
    distCoeffs.resize(numPoses, distCoeffVec);
    
  
    // Run bundle adjustment
    cv::LevMarqSparse   lms;
    lms.bundleAdjust(worldPts, imagePts, visibility, camCalibMats, 
            R, T, distCoeffs, criteria);
  
    // Return new world points and poses
    plhs[0] = mxCreateDoubleMatrix(numPts, 3, mxREAL);
    double *Xout = mxGetPr(plhs[0]);
    double *Yout = Xout + numPts;
    double *Zout = Yout + numPts;
    for(int i=0; i<numPts; ++i) {
       Xout[i] = worldPts[i].x;
       Yout[i] = worldPts[i].y;
       Zout[i] = worldPts[i].z;
    }
     
    plhs[1] = mxCreateCellMatrix(1, numPoses);
    plhs[2] = mxCreateCellMatrix(1, numPoses);
    for(int i=0; i<numPoses; ++i) {
        mxSetCell(plhs[1], i, cvMat2mxArray(R[i]));
        mxSetCell(plhs[2], i, cvMat2mxArray(T[i]));
    }
    
    return;
}

