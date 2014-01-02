///
/// @file
/// @copyright All code copyright Movidius Ltd 2013, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief Application Config File
///
#ifndef __HAAR_DETECTOR_SHARED_H__
#define __HAAR_DETECTOR_SHARED_H__

// (De)activate L2 cache for an address
#ifdef __MOVICOMPILE__
#define L2CACHE(x) (u8*)(((u32)(x) & ~0x08000000))
//#define L2CACHE(x) x // L2 Cache bypass
#define L2BYPASS(x) (u8*)(((u32)(x) | 0x08000000))
#else
#define L2CACHE(x) (x)
#define L2BYPASS(x) (x)
#endif

/*-----------Algorithm configuration-----------
-----------------------------------------------*/

#define DOWNSAMPLE_IMAGE            /* If defined, input image is downsampled before starting scaling the classifier */

#ifdef DOWNSAMPLE_IMAGE             /* ----------QVGA -> used for both Desktop and Myriad versions---------- */
    #define DOWNSAMPLE_FACTOR   2   /* Factor for resizing the image */
    #define MAX_SCALES          7   /* For QVGA there are 7 window sizes */
    #define MIN_WINDOW_SIZE     40  /* Minimum feature size */
    #define FIXED_WINDOW_SIZES  1   /* 1 - Window sizes are given in an input array */
                                    /* 0 - Window sizes are computed based on scale factor */

#else                               /* -----------VGA -> used for Desktop version only---------- */
    #define DOWNSAMPLE_FACTOR   1   
    #define MAX_SCALES          13  /* Clasical approach has 13 window sizes */
    #define MIN_WINDOW_SIZE     80  /* Minimum feature size */
    #define FIXED_WINDOW_SIZES  0   /* 1 - Window sizes are given in an input array */
                                    /* 0 - Window sizes are computed based on scale factor */
    #define MAX_SCALES 13
    #define MIN_WINDOW_SIZE 80
    #define fixedWindowSizes  0
#endif

#define HID_CASCADE_SIZE   (160*1024)

/*-----------Algorithm optimizations-----------
-----------------------------------------------*/

#define _ASM_KERNEL_OPT             /* Optimzation by adding one classifier at each stage to remove some conditions */
#define HID_CASCADE_OPT         1   /* 1 - Activate the optimization for SetImagesForHaarClassifierCascade */
                                    /* 0 - Disable optimization */
#define MAX_HID_CASCADES        MAX_SCALES  

/*--------------Debug information-------------
---------------------------------------------*/
#ifndef __MOVICOMPILE__
    #define DUMP_IMG_TO_FILE
    #define DEBUG_TO_FILE
    //#define DEBUG_MAIN
#endif

#ifdef DEBUG_MAIN
 #ifdef DEBUG_TO_FILE
    #define DEBUG_PRINT(_message)							fprintf(fDebug, _message)
    #define DEBUG_PRINT1(_message, _a0)						fprintf(fDebug, _message, _a0)
    #define DEBUG_PRINT2(_message, _a0, _a1)				fprintf(fDebug, _message, _a0, _a1)
    #define DEBUG_PRINT3(_message, _a0, _a1, _a2)			fprintf(fDebug, _message, _a0, _a1, _a2)
    #define DEBUG_PRINT4(_message, _a0, _a1, _a2, _a3)		fprintf(fDebug, _message, _a0, _a1, _a2, _a3)
    #define DEBUG_PRINT5(_message, _a0, _a1, _a2, _a3, _a4)	fprintf(fDebug, _message, _a0, _a1, _a2, _a3, _a4)
 #else
    #define DEBUG_PRINT(_message)							printf(_message)
    #define DEBUG_PRINT1(_message, _a0)						printf(_message, _a0)
    #define DEBUG_PRINT2(_message, _a0, _a1)				printf(_message, _a0, _a1)
    #define DEBUG_PRINT3(_message, _a0, _a1, _a2)			printf(_message, _a0, _a1, _a2)
    #define DEBUG_PRINT4(_message, _a0, _a1, _a2, _a3)		printf(_message, _a0, _a1, _a2, _a3)
    #define DEBUG_PRINT5(_message, _a0, _a1, _a2, _a3, _a4)	printf(_message, _a0, _a1, _a2, _a3, _a4)
 #endif
    #define DEBUG_IMAGE cvShowImage
    #define DEBUG_WAIT cvWaitKey
#else
    #define DEBUG_PRINT(_message)
    #define DEBUG_PRINT1(_message, _a0)
    #define DEBUG_PRINT2(_message, _a0, _a1)
    #define DEBUG_PRINT3(_message, _a0, _a1, _a2)
    #define DEBUG_PRINT4(_message, _a0, _a1, _a2, _a3)
    #define DEBUG_PRINT5(_message, _a0, _a1, _a2, _a3, _a4)
    #define DEBUG_IMAGE
    #define DEBUG_WAIT
#endif

#define FREE_IMAGE(x) {if (NULL != (x)) {cvReleaseImage(&x); x = NULL;}}

/*------Platform abstractization defines-----
---------------------------------------------*/
#ifdef __PC__
    #include <myriadModel.h>
    #define SHAVE_START		mmCreate()
    #define SHAVE_STOP		mmDestroy()
#else
    #include <stdlib.h>
    #define SHAVE_START
    #define SHAVE_STOP		exit(0)
#endif

#endif // SYSTEM_PRAMS
