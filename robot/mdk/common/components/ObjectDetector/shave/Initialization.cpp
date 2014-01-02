///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     
///

#include <Initialization.h>

#include <alloc.h>
#include <mvcv.h>
#include <Defines.h>
#include <HaarDetector.h>
#include "../include/ObjectDetection.h"
#include <stdio.h>

using namespace mvcv;

// Heap buffer size
#define HEAP_SIZE (20 * 1024)
// Heap buffer in local slice
static u8 heap[HEAP_SIZE];

//extern u32 precomputedHidsCreated[MAX_HID_CASCADES];

u32 dbgInit = 0;

void computeWindowSizes(ObjectDetectionConfig* cfg)
{
    if (FIXED_WINDOW_SIZES == 1)      // For QVGA we use fixed window sizes for better results
    {
        int inputWindowSizes[MAX_SCALES] = {48, 62, 82, 106, 138, 180, 232};
        for (int i = 0; i < MAX_SCALES; i++)
            cfg->windowSizes[i] = inputWindowSizes[i];
    }
    else                            // Classical approach with window sizes computed based on scale factor: can be used for VGA and QVGA
    {
        u8* original_window_size;
        original_window_size = cfg->cascade_ptr + 3*sizeof(int);
        cfg->windowSizes[0] = *original_window_size;
        float scaleFactor = cfg->scale_factor;
        float factor = 1;
        for (int i = 0; i < MAX_SCALES; i++, factor *= scaleFactor)
        {
            cfg->windowSizes[i] = cvRoundLocal(cfg->windowSizes[0]*factor);
        }
    }
}

void BuildHiddenCascades(ObjectDetectionConfig* const cfg)
{
	// Create the image objects we need
#ifndef DOWNSAMPLE_IMAGE
	assert(IS_DDR(cfg->input_image.p1) || IS_CMX(cfg->input_image.p1));
    Mat img(cfg->input_image.spec.height, cfg->input_image.spec.width, CV_8UC1, cfg->input_image.p1);
#else
	assert(IS_DDR(cfg->small_image.p1) || IS_CMX(cfg->small_image.p1));
    Mat img(cfg->input_image.spec.height/DOWNSAMPLE_FACTOR, cfg->input_image.spec.width/DOWNSAMPLE_FACTOR, CV_8UC1, cfg->small_image.p1);
#endif

	dbgInit++;
	//printf("%s\n", __FUNCTION__);

	assert(IS_DDR(cfg->sum_image.p1) || IS_CMX(cfg->sum_image.p1));
	assert(IS_DDR(cfg->sqsum_image.p1) || IS_CMX(cfg->sqsum_image.p1));

	Mat sum(cfg->sum_image.spec.height, cfg->sum_image.spec.width, CV_32SC1, cfg->sum_image.p1);
	Mat sqsum(cfg->sqsum_image.spec.height, cfg->sqsum_image.spec.width, CV_32FC1, cfg->sqsum_image.p1); 

    HaarClassifierCascade*	cascade = (HaarClassifierCascade*)cfg->cascade_ptr;
    int						scale;

	assert(IS_DDR(cascade) || IS_CMX(cascade));
	assert(IS_DDR(cfg->cascade_hid_ptr) || IS_CMX(cfg->cascade_hid_ptr));

	// Force hidden cascade creation
	cascade->hid_cascade = null;
    CreateHidHaarClassifierCascade((HaarClassifierCascade*)L2BYPASS(cascade), cfg->cascade_hid_ptr);
    HidHaarClassifierCascade*	hid_cascade = (HidHaarClassifierCascade*)cfg->cascade_hid_ptr;
	
	dbgInit++;

	// Read integral image thourgh L2 cache
	sqsum.data = L2CACHE(sqsum.data);
    hid_cascade->sum = sum;
    hid_cascade->sqsum = sqsum;

    fp32						scaleFactor = cfg->scale_factor;
    float						factor = 1.0;

    // Max window size by default is the size of the frame
    if (cfg->max_size_height == 0 || cfg->max_size_width == 0)
    {
		cfg->max_size_height = cfg->input_image.spec.height;
        cfg->max_size_width  = cfg->input_image.spec.width;
    }

    scale = 0;

    // For each factor compute hidden cascade
    for (int i = 0; i < MAX_SCALES; i++)
    {
        int current_window_size = cfg->windowSizes[i];
        CvSize winSize = CvSize( current_window_size, current_window_size);
        if (FIXED_WINDOW_SIZES == 1)
        {
            factor = current_window_size/(20.0);
        }
        else
        {
            if (i > 0) // factor of first scale must be 1
                factor *= scaleFactor;
        }

        if (winSize.width < cfg->min_size_width || winSize.height < cfg->min_size_height)
        {
            continue;
        }

        cascade->scale = factor;
        cascade->real_window_size.width = winSize.width;
        cascade->real_window_size.height = winSize.height;

		SetImagesForHaarClassifierCascade((HaarClassifierCascade*)L2BYPASS(cascade), cfg->cascade_hid_ptr, &sum, &sqsum, factor, null/*&precomputedHidsCreated[scale]*/);

		dbgInit++;

	    scDmaSetup(DMA_TASK_0, (u8*)cfg->cascade_hid_ptr, (u8*)L2BYPASS(cfg->all_cascade_hid_ptr[scale]), HID_CASCADE_SIZE);
	    scDmaStart(START_DMA0);
	    scDmaWaitFinished();

        scale++;
    }
}

extern "C" void Initialize(ObjectDetectionConfig* const cfg)
{
	SHAVE_START;

	// Never forget to initialize the heap before using it. 
	// This can be done only once, at initialization time.
	init_heap(heap, HEAP_SIZE);
    clear_heap();

	dbgInit++;

    computeWindowSizes(cfg);
	BuildHiddenCascades(cfg);

	SHAVE_STOP;
}