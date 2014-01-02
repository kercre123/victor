///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @brief     ObjectDetection API
/// 

#include <mv_types.h>

#include <stdio.h>

#include "DrvTimer.h"
#include "DrvSvu.h"
#include <DrvL2Cache.h>
#include "swcShaveLoader.h"
#include "swcSliceUtils.h"
#include "swcMemoryTransfer.h"

#include "../include/ObjectDetection.h"
#include <HaarDetectorFrontalFaceCascade.h>

// Profiling
#include "swcTestUtilsDefines.h"
#include "swcTestUtils.h"

// Flag that turns on search skipping based on last face location
//#define PARTIAL_IMPLEMENTATION
#define I_FRAME_PERIOD  7
#define RANGE_FACTOR    20

// Shave entry point symbols
extern u32 ObjectDetection0_Initialize;
extern u32 ObjectDetection0_Preprocessing;
extern u32 ObjectDetection0_Run;

static u32 FnInitialize = (u32)&ObjectDetection0_Initialize;
static u32 FnPreprocessing = (u32)&ObjectDetection0_Preprocessing;
static u32 FnRun = (u32)&ObjectDetection0_Run;

static u32 stackTopAddr;

static u32 IframeAvailable;
static u32 PframeAvailable;

static Object oldObjects[10];
static u32  oldObjectsCount = 0;

static void swcSetStackPointer(u32 shave_num)
{
    DrvSvutIrfWrite(shave_num, 19, stackTopAddr);
}

static void checkObjectsSanity(ObjectDetectionConfig* cfg, Object* objects, u32 objectsCount)
{
    u32 i;
    u32 good = 1;
    
    for (i = 0; i < objectsCount; i++)
        if (objects[i].x > cfg->width || objects[i].y > cfg->height ||
            objects[i].width > cfg->width || objects[i].height > cfg->height)
        {
            printf("%d. %d %d %d %d\n", i, objects[i].x, objects[i].y,
                objects[i].width, objects[i].height);
            good = 0;
        }
        
    if (!good)
        swcLeonHalt();
}

static void adjustWindowSize(ObjectDetectionConfig* cfg)
{
#ifdef PARTIAL_IMPLEMENTATION
    u32 min_width, min_height, max_width, max_height, i;

    if((cfg->out_faces_nr > 0) && (IframeAvailable == 1))
    {
        min_width  = 640;
        min_height = 480;
        max_width  = 0;
        max_height = 0;
        for (i = 0; i < cfg->out_objects_count; i++)
        {
            min_width  = min(min_width,  cfg->out_faces[i].width);
            min_height = min(min_height, cfg->out_faces[i].height);
            max_width  = max(max_width,  cfg->out_faces[i].width);
            max_height = max(max_height, cfg->out_faces[i].height);
        }
        cfg->min_size_width  = min_width - RANGE_FACTOR*min_width/100;
        cfg->min_size_height = min_height - RANGE_FACTOR*min_height/100;
        cfg->max_size_width = max_width + RANGE_FACTOR*max_width/100;
        cfg->max_size_height= max_height + RANGE_FACTOR*max_height/100;
        IframeAvailable = 0;
        PframeAvailable = 0;
    }

    if(IframeAvailable == 0)
    {
        PframeAvailable ++;
    }
    if((PframeAvailable % I_FRAME_PERIOD) == (I_FRAME_PERIOD - 1))
    {
        IframeAvailable = 1;
    }

    if((cfg->out_faces_nr == 0) || (IframeAvailable == 1))
    {
        cfg->min_size_width  = 40;
        cfg->min_size_height = 40;
        cfg->max_size_width = 0;
        cfg->max_size_height = 0;
        IframeAvailable = 1;
    }
#endif
}

void ObjectDetectionInit(ObjectDetectionConfig* cfg, u32 shaveStackTopAddr)
{
    IframeAvailable = 1;
    PframeAvailable = 0;
        
    stackTopAddr = shaveStackTopAddr;
    
    cfg->cascade_ptr = (u8*)&cascade_aux;
    
    swcResetShave(SVU_OBJECTDETECT);    
    swcSetStackPointer(SVU_OBJECTDETECT);
    swcStartShaveCC(SVU_OBJECTDETECT, FnInitialize, "i", (u32)cfg);
    swcWaitShave(SVU_OBJECTDETECT);
}

void ObjectDetectionPreprocessing(ObjectDetectionConfig* objectDetectCfg, frameBuffer* inputSensorFrame,
    frameBuffer** outputFrame)
{
    // Initialize the Shave
	swcResetShave(SVU_OBJECTDETECT);
	swcSetStackPointer(SVU_OBJECTDETECT);
        
    DrvL2CachePartitionInvalidate(SVU_OBJECTDETECT);
        
	// Compute 8bpp, integral and squared integral images,
    // providing the image from the sensor as input
	swcStartShaveCC(SVU_OBJECTDETECT, FnPreprocessing, "ii", (u32)inputSensorFrame, (u32)objectDetectCfg);
        
    swcWaitShave(SVU_OBJECTDETECT);
    
    // Output can be the same buffer as input for 1bpp
    *outputFrame = (inputSensorFrame->spec.bytesPP == 2) ? &objectDetectCfg->input_image : inputSensorFrame;
}

int ObjectDetectionRun(ObjectDetectionConfig* cfg)
{
    u32 i;
    static tyTimeStamp executionTimer;
    Object* objects;
    u32 lastExecTime;
    
	#ifdef PROFILING
		performanceStruct perfStr;
		swcShaveProfInit(&perfStr);
	#endif
	
    DrvL2CachePartitionInvalidate(SVU_OBJECTDETECT);
 
    DrvTimerStart(&executionTimer);
 
    // Initialize the Shave
    swcResetShave(SVU_OBJECTDETECT);
	#ifdef PROFILING
		swcShaveProfStartGathering(SVU_OBJECTDETECT, &perfStr);
	#endif
    swcSetStackPointer(SVU_OBJECTDETECT);
    
    // Start the Shave with run-time parameters
    swcStartShaveCC(SVU_OBJECTDETECT, FnRun, "i", (u32)cfg);
    
    // Wait for all Shaves to finish
    swcWaitShave(SVU_OBJECTDETECT);
    lastExecTime = DrvTimerTicksToMs(DrvTimerElapsedTicks(&executionTimer));
    
    #ifdef PROFILING
		swcShaveProfStopGathering(SVU_OBJECTDETECT, &perfStr);
		swcShaveProfPrint(SVU_OBJECTDETECT, &perfStr);
	#endif
    
    adjustWindowSize(cfg);
    
    // Translate Shave's virtual address to Leon's address space
    objects = (Object*)swcSolveShaveRelAddrAHB((u32)cfg->out_objects, 1);
    
    swcU32memcpy((u32*)oldObjects, (u32*)objects, cfg->out_objects_count * sizeof(Object));
    oldObjectsCount = cfg->out_objects_count;
    
    checkObjectsSanity(cfg, oldObjects, oldObjectsCount);
    
    return lastExecTime;
}