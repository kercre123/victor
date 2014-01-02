///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     ObjectDetection API
///

#ifndef __OBJECT_DETECTION_H__
#define __OBJECT_DETECTION_H__

#include <mv_types.h>
#include <swcFrameTypes.h> 
#include <HaarDetectorShared.h>

#define SVU_OBJECTDETECT	0

/**
 * Bounding box for a face
 */
typedef struct
{
	u32 x;
	u32 y;
	u32 width;
	u32 height;
} Object;

/** 
 * Interface between the application running on the Leon and the
 * algorithm running on Shaves
 */
typedef struct 
{
	// Input frame buffers
	frameBuffer		input_image;	                /**< Input image with data from the sensor */
    frameBuffer     small_image;                    /**< Resized image - downsampled by 2 x 2*/
	frameBuffer		sum_image;		                /**< Integral image buffer (u32 type) */
                                                    /**< width = input_image.width + 1 */
                                                    /**< height = input_image.height + 1 */
	frameBuffer		sqsum_image;	                /**< Squared integral image buffer (fp32 type) */
                                                    /**< width = input_image.width + 1 */
                                                    /**< height = input_image.height + 1 */

	u32		width;					                /**< Input image width (pixels) */
	u32		height;					                /**< Input image height (pixels) */
	
	u8*		cascade_ptr;			                /**< Pointer to the cascade buffer */
	u8*		cascade_hid_ptr;		                /**< Pointer to the hidden cascade buffer */
	u8*		all_cascade_hid_ptr[MAX_HID_CASCADES];  /**< Pointer to the hidden cascade buffers */
	
	fp32	scale_factor;			                /**< Scale factor */
	u32		min_neighbors;			                /**< Minimum number of rectangles for an object */
	u32		min_size_width;			                /**< Minimum width for the search rectangle */
	u32		min_size_height;		                /**< Minimum height for the search rectangle */
	u32		max_size_width;			                /**< Maximum width for the search rectangle */
	u32		max_size_height;		                /**< Maximum height for the search rectangle */

	u32		usePrecomputedHid;		                /**< Flag to enable running on precomputed 
                                                    hidden cascades */
            
	Object*	out_objects;				            /**< Pointer to the array of object bounding boxes found */
	u32		out_objects_count;		                /**< Number of object bounding boxes found */
    u32     windowSizes[MAX_SCALES];                /**< Array of searching windows sizes */

} ObjectDetectionConfig;

void ObjectDetectionInit(ObjectDetectionConfig* cfg, u32 shaveStackTopAddr);

void ObjectDetectionPreprocessing(ObjectDetectionConfig* objectDetectCfg, 
	frameBuffer* inputSensorFrame, frameBuffer** outputFrame);

int ObjectDetectionRun(ObjectDetectionConfig* cfg);

#endif