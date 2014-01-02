///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     
///

#include "Detection.h"

#include <alloc.h>
#include <assert.h>
#include <mvcv.h>
#include <mvstl.h>

#include "../include/ObjectDetection.h"
#include <Defines.h>
#include <HaarDetector.h>

using namespace mvcv;
using namespace mvstl;

///////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION CALLED BY EXTERNAL MODULES
///////////////////////////////////////////////////////////////////////////////

extern "C" void Run(ObjectDetectionConfig* const cfg)
{
    SHAVE_START;

    // Garbage collection - no more memory leaks :)
    // TODO: get rid of this and find the source of memory leak(s)
    clear_heap();

	// Check input validity
	assert(IS_CMX(cfg));
	assert(cfg->width > 0 && cfg->width <= MAX_WIDTH);
	assert(cfg->height > 0 && cfg->height <= MAX_HEIGHT);
	assert(IS_DDR(cfg->cascade_ptr) || IS_CMX(cfg->cascade_ptr));
	assert(IS_DDR(cfg->cascade_hid_ptr) || IS_CMX(cfg->cascade_hid_ptr));
	assert(cfg->min_neighbors >= 0 && cfg->min_neighbors < 10);
	assert(cfg->scale_factor > 1.0f && cfg->scale_factor <= 5.0f);
	assert(cfg->min_size_width >= 0 && cfg->min_size_width <= MAX_WIDTH);
	assert(cfg->min_size_height >= 0 && cfg->min_size_height <= MAX_HEIGHT);
	assert(cfg->max_size_width >= 0 && cfg->max_size_width <= MAX_WIDTH);
	assert(cfg->max_size_height >= 0 && cfg->max_size_height <= MAX_HEIGHT);
	for (u32 i = 0; i < MAX_HID_CASCADES; i++)
		assert(IS_DDR(cfg->all_cascade_hid_ptr[i]));

    // Create the image objects we need
#ifndef DOWNSAMPLE_IMAGE
    assert(IS_DDR(cfg->input_image.p1) || IS_CMX(cfg->input_image.p1));
    Mat img(cfg->input_image.spec.height, cfg->input_image.spec.width, CV_8UC1, cfg->input_image.p1);
#else
    assert(IS_DDR(cfg->sum_image.p1) || IS_CMX(cfg->sum_image.p1));
    Mat img(cfg->input_image.spec.height/DOWNSAMPLE_FACTOR, cfg->input_image.spec.width/DOWNSAMPLE_FACTOR, CV_8UC1, cfg->small_image.p1);
#endif

    assert(IS_DDR(cfg->sqsum_image.p1) || IS_CMX(cfg->sqsum_image.p1));
    assert(IS_DDR(cfg->small_image.p1) || IS_CMX(cfg->small_image.p1));

    Mat sum(cfg->sum_image.spec.height, cfg->sum_image.spec.width, CV_32SC1, cfg->sum_image.p1);
    Mat sqsum(cfg->sqsum_image.spec.height, cfg->sqsum_image.spec.width, CV_32FC1, cfg->sqsum_image.p1);

    // Run the algorithm and get a vector of faces
	const vector<Rect>& objects = HaarDetectObjectsForROC(img, sum, sqsum, cfg->cascade_ptr, cfg->cascade_hid_ptr,
		cfg->all_cascade_hid_ptr, cfg->scale_factor, cfg->min_neighbors, cvSize(cfg->max_size_width, cfg->max_size_height),
		cvSize(cfg->min_size_width, cfg->min_size_height), cfg->windowSizes, 0, cfg->usePrecomputedHid);

    // Pass the results out to the application
    cfg->out_objects = (Object*)objects.data();
    cfg->out_objects_count = objects.size();

#ifdef PROFILING
    for (u32 i = 0; i < profileManager::getProfileNumber(); i++)
    {
        profile& prof = profileManager::getProfile(i);
        printf("%s: %.2f ms\n", prof.name(), prof.mtime());
    }
#endif

    // Check the heap for overflows. Useful for debugging.
    //check_heap();

    SHAVE_STOP;
}