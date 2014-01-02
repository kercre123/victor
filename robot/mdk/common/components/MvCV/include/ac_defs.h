///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     This header is really really old, like 3 years ago, and it was used by the first version
///            of the Auto-Convergence algorithm which is not used anymore.
///            This file should be deleted !
///

#ifndef AUTO_CONV_DEFS_H
#define AUTO_CONV_DEFS_H

//#ifdef __VECTORCSHAVE__
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
//#endif

typedef unsigned char MVIplImage;

//================================================================
typedef struct
{
  //! Coordinates of the detected interest point
   unsigned short xL, yL;
   unsigned short xR, yR;

  //andrei: range where search makes sense...
   int match_found; //1 if has reference in LEFT image
   int sad;         //current matching SAD for current value 
   
   float slope;
   
   //vesao: for debug; can be deleted
   int prlx;
}sMatch ;


//================================================================
#define MATCH_MAX_DIST 3	/// max distance between corresponding points in the left image and in the right
//#define MATCH_RIGHT_LEFT	// enable matching from right to left

//================================================================
#define SQUEEZED	1		 ///1 for squeezed images, 0 otherwise
#define SEPB        12       ///Separation border size

//================================================================
#define SEARCH_RANGE_X   50  ///default: 50
#define SEARCH_RANGE_Y   2   ///default: 30
#define FILTER_LOBE      12  ///default: 12; BETTER results than small lobe !!!
#define FILTER_SIZE     (2*FILTER_LOBE)
#define MAX_SAD          99999999

#define CORNER_FILTER_SIZE		9	  //default: 9
//default: 0.002f for Harris
//default: 0.02f for KLT & Shi-Tomasi
#define CORNER_THRESH 			0.001f

//#define PYRAMID_LEVELS			1 //obsolete

//================================================================
#define AC_MAX_WIDTH  640
#define AC_MAX_HEIGHT 720

//If every exponent's bit is 1 it is a NaN
#define NAN 0x7F800000

#define GRID_CELLS_W 20 //default: 20
#define GRID_CELLS_H 10 //default: 20
#define MAX_POINTS (GRID_CELLS_H * GRID_CELLS_W) //can have max 1 BEST per cell

#define MIN_POINTS_FILTER 5

//================================================================
//Maximum number of samples buffered
#ifndef MAX_VR_VALS
#define MAX_VR_VALS	64
#endif

#ifndef MAX_AC_VALS
#define MAX_AC_VALS	8
#endif

#ifndef MAX_RR_VALS
#define MAX_RR_VALS 64
#endif

//================================================================
typedef struct
{
	sMatch pts[MAX_POINTS];
	int    size;
}IpVec;

//###########################################################
typedef struct 
{
  int    filter;  
  float*  responses;
} ResponseLayer;

//###########################################################
typedef struct  {
  
    //! Pointer to the integral Image, and its attributes 
    MVIplImage *img;
    //int i_width, i_height;

    //! Reference to vector of features passed from outside 
    IpVec *ipts;

    //! Response stack of determinant of hessian values
    ResponseLayer responseMap;

    //! Threshold value for blob resonses
    float thresh;
} FastHessian;

#ifndef NULL
#define NULL 0
#endif

#endif
