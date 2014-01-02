///   
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved. 
///            For License Warranty see: common/license.txt   
///
/// @brief     Simple Led Functionality.
/// 
/// This is the implementation of a simple Maths library implementing basic math operations.
/// We can continue this brief description of the module to add more detail
/// This can actually include quite a long description if necessary.
/// 
/// @todo It can be useful to keep a list of outstanding tasks for your module
/// @todo You can have more than one item on your todo list
/// 
/// 

// 1: Includes
// ----------------------------------------------------------------------------
#include <DrvGpioDefines.h>
#include <DrvGpio.h>
#include <stdio.h>
#include <swcFrameTypes.h>
#include "FrameManagerApi.h"

// 2:  Source Specific #defines and types  (typedef,enum,struct)
// ----------------------------------------------------------------------------

//Structure to be initialized and used to generate each buffer address for each user
typedef struct frame_users{
	//Frame memory pool address
	unsigned char* frame_pool;
	//Frame memory pool size in bytes
	unsigned int frame_pool_size;
	//Number of active users
	unsigned int active_creators;
	//Widths for each of the active users
	unsigned int width[MAX_FRAME_CREATORS];
	//Heights for each of the active users
	unsigned int height[MAX_FRAME_CREATORS];
	//Types for each of the active users
	frameType frmtypes[MAX_FRAME_CREATORS];
	//Number of frames requested by each user
	unsigned int frm_req[MAX_FRAME_CREATORS];
}frame_users_cfg;

// 3: Global Data (Only if absolutely necessary)
// ----------------------------------------------------------------------------
                               
// 4: Static Local Data 
// ----------------------------------------------------------------------------
//Variable to hold the configuration information
static frame_users_cfg frame_configuration;
//Structure to hold the generated frames
static frameBuffer creators_buffers[MAX_FRAME_CREATORS][MAX_FRAMES_PER_CREATOR];

// 5: Static Function Prototypes
// ----------------------------------------------------------------------------
 
// 6: Functions Implementation
// ----------------------------------------------------------------------------

//Start Configuring the memory Pool
void FRAMEMANAGER_CODE_SECTION(FMInitPool)(unsigned char* memory_pool, unsigned int mem_size)
{
	//Initialize an empty structure
	frame_configuration.frame_pool=memory_pool;
	frame_configuration.frame_pool_size=mem_size;
	frame_configuration.active_creators=0;
	return;
}

//Allocate one new user for the manager and return it's ID
unsigned int FRAMEMANAGER_CODE_SECTION(FMAddCreator)(frameType frmt,unsigned int width, unsigned int height,unsigned int req_frm)
{
	unsigned int RETID;
	RETID=frame_configuration.active_creators;
	frame_configuration.width[RETID]=width;
	frame_configuration.height[RETID]=height;
	frame_configuration.frmtypes[RETID]=frmt;
	frame_configuration.frm_req[RETID]=req_frm;
	frame_configuration.active_creators++;
	return RETID;
}

//Try to allocate all configured frames within the given frame pool
FMRes_type FRAMEMANAGER_CODE_SECTION(FMAllocateFrames)(void)
{
	FMRes_type aux=FM_SUCCESS;
	int mem_left;
	unsigned char *crt_address;
	unsigned int i;
	//Starting to decrease from the whole pool
	mem_left=frame_configuration.frame_pool_size;
	//Starting with mem_pool address
	crt_address=frame_configuration.frame_pool;
	//Take each producer one by one and try to allocate the frames according to the requested type
	i=0;
	while ( (i<frame_configuration.active_creators) && (aux==FM_SUCCESS) )
	{
		//Check there is enough space left to allocate the frames for the current producer
		int frame_size,needed_space;
		switch (frame_configuration.frmtypes[i]){
		case YUV422i:
			frame_size=frame_configuration.width[i]*frame_configuration.height[i]*2;
			break;
		case YUV420p:
			frame_size=(frame_configuration.width[i]*frame_configuration.height[i]*3)/2;
			break;
		case YUV422p:
			frame_size=frame_configuration.width[i]*frame_configuration.height[i]*2;
			break;
		case NONE:
			frame_size=0;
			break;
		default:
			frame_size=0;
			break;
		}
		needed_space=frame_size*frame_configuration.frm_req[i];
		if (mem_left<needed_space){
			aux=FM_FAIL;
		}else{
			unsigned int j;
			//Allocate each frame
			for (j=0;j<frame_configuration.frm_req[i];j++)
			{
				creators_buffers[i][j].spec.type=frame_configuration.frmtypes[i];
				creators_buffers[i][j].spec.width=frame_configuration.width[i];
				creators_buffers[i][j].spec.height=frame_configuration.height[i];
				switch (frame_configuration.frmtypes[i]){
				case YUV422i:
					creators_buffers[i][j].p1=crt_address;
					crt_address=crt_address+creators_buffers[i][j].spec.width*creators_buffers[i][j].spec.height*2;
					creators_buffers[i][j].spec.bytesPP=2;
					creators_buffers[i][j].spec.stride=frame_configuration.width[i]*2;
					//Decrease allocated frame space from memory pool
					mem_left-=creators_buffers[i][j].spec.width*creators_buffers[i][j].spec.height*2;
					break;
				case YUV420p:
					creators_buffers[i][j].p1=crt_address;
					creators_buffers[i][j].p2=crt_address+creators_buffers[i][j].spec.width*creators_buffers[i][j].spec.height;
					creators_buffers[i][j].p3=crt_address+(creators_buffers[i][j].spec.width*creators_buffers[i][j].spec.height*5)/4;
					crt_address=crt_address+(creators_buffers[i][j].spec.width*creators_buffers[i][j].spec.height*3)/2;
					creators_buffers[i][j].spec.bytesPP=1;
					creators_buffers[i][j].spec.stride=frame_configuration.width[i];
					//Decrease allocated frame space from memory pool
					mem_left-=(creators_buffers[i][j].spec.width*creators_buffers[i][j].spec.height*3)/2;
					break;
				case YUV422p:
					creators_buffers[i][j].p1=crt_address;
					creators_buffers[i][j].p2=crt_address+creators_buffers[i][j].spec.width*creators_buffers[i][j].spec.height;
					creators_buffers[i][j].p3=crt_address+(creators_buffers[i][j].spec.width*creators_buffers[i][j].spec.height*3)/2;
					crt_address=crt_address+(creators_buffers[i][j].spec.width*creators_buffers[i][j].spec.height*2);
					creators_buffers[i][j].spec.bytesPP=1;
					creators_buffers[i][j].spec.stride=frame_configuration.width[i];
					//Decrease allocated frame space from memory pool
					mem_left-=creators_buffers[i][j].spec.width*creators_buffers[i][j].spec.height*2;
					break;
				case NONE:
					creators_buffers[i][j].p1=0x0;
					aux=FM_FAIL;
					break;
				default:
					creators_buffers[i][j].p1=0x0;
					aux=FM_FAIL;
					break;
				}
			}
		}
		i++;
	}
	return aux;
}

frameBuffer* FRAMEMANAGER_CODE_SECTION(FMGetFrameArray)(unsigned int CreatorID)
{
	return creators_buffers[CreatorID];
}
