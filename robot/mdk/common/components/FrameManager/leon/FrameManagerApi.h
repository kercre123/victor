///  
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved 
///            For License Warranty see: common/license.txt   
///
/// @defgroup FrameManagerApi Simple Leds Functionality API.
/// @{
/// @brief     Simple Leds Functionality API.
/// 
/// This is the API to a simple Maths library implementing basic math operations.
/// We can continue this brief description of the module to add more detail
/// This can actually include quite a long description if necessary.
/// 

#ifndef _FRAMEMANAGER_API_H_
#define _FRAMEMANAGER_API_H_

// 1: Includes
// ----------------------------------------------------------------------------
#include <swcFrameTypes.h>
#include "FrameManagerApiDefines.h"

// 2:  Exported Global Data (generally better to avoid)
// ----------------------------------------------------------------------------

// 3:  Exported Functions (non-inline)
// ----------------------------------------------------------------------------

// For very simple function parameters the following simple style of documentation is sufficent
// The @a parameter highlights the following word as deserving of highlighting

/// Initializes the Frame Manager Configuration Structure
/// @param[in] memory_pool  Frame memory pool address
/// @param[in] mem_size     Frame memory pool size
/// @return    void
///
void FMInitPool(unsigned char* memory_pool, unsigned int mem_size);

/// Adds a new frame Creator to the configuration
/// @param[in] frmt     A frame type as declared by frame_type
/// @param[in] width    Frame width
/// @param[in] height   Frame height
/// @param[in] req_frm  Number of frames requested by this frame creator
/// @return    unsigned int USER_ID - Returns a user ID which may be used later to extract
///
unsigned int FMAddCreator(frameType frmt,unsigned int width, unsigned int height,unsigned int req_frm);

/// Tries to make all configured assignments for all frames
/// @return unsigned int RET_CODE - returns FM_FAIL or FM_SUCCESS
FMRes_type FMAllocateFrames(void);

/// Returns the frame array pointer associated to a certain ID
/// @param[in] CreatorID  ID to which we need to return the frames pointer
/// @return frame_buffer[MAX_FRAMES_PER_CREATOR] array - returns a pointer to the list of frames allocated for this creator
frameBuffer* FMGetFrameArray(unsigned int CreatorID);
/// @}
#endif //_FRAMEMANAGER_API_H_
