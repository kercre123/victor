#include "mclDmaTransfer.h"
#include "svuCommonShave.h"

dmaHandle shvHandle;

void mclDmaInit(void *handle, u8 shaveNum)
{
	shvHandle.svu_nr = shaveNum;
	shvHandle.lastTask = 0;
	
	*(u32 *)handle = (u32)&shvHandle;
}

void mclDmaAddTask(void *handle, u32 *dst , u32 *src, u32 len)
{
	dmaHandle *myHandle = (dmaHandle*)handle;

	if(myHandle->lastTask <= DMA_TASK_3)
	{
		scDmaSetupFull(myHandle->lastTask, DMA_ENABLE, src, dst, len, len, 0);
		myHandle->lastTask++;
	}
}

void mclDmaAddTaskStride(void *handle, u32 *dst , u32 *src, u32 len, u32 src_line_size, u32 src_stride, u32 dst_line_size, u32 dst_stride)
{
	dmaHandle *myHandle = (dmaHandle*)handle;

	if(myHandle->lastTask <= DMA_TASK_3)
	{

		if(len>0)
		{
			if(src_stride!=0) scDmaSetupFull(myHandle->lastTask, 
											 DMA_ENABLE | DMA_DST_USE_STRIDE,
											 src, 
											 dst, 
											 len, 
											 src_line_size, 
											 src_stride);
												   
			else if(dst_stride!=0) scDmaSetupFull(myHandle->lastTask, 
														DMA_ENABLE | DMA_SRC_USE_STRIDE,
														src,
														dst, 
														len, 
														dst_line_size,
														dst_stride);
															
					else mclDmaAddTask(handle, dst, src, len);
			
			myHandle->lastTask++;
		}
	}
}

void mclDmaStartTasks(void *handle)
{
	dmaHandle *myHandle = (dmaHandle*)handle;

	scDmaStart(myHandle->lastTask-1);

}

void mclDmaWaitAllTasksCompleted(void *handle)
{
	dmaHandle *myHandle = (dmaHandle*)handle;
	
	scDmaWaitFinished();
	myHandle->lastTask=0;
}

void mclDmaMemcpy(void *handle, u32 *dst, u32 *src, u32 len)
{
	u8 i;
	for(i=0;i<len>>2;i++)
		dst[i]=src[i];
}

void mclDmaMemcpyStride(void *handle, u32 *dst , u32 *src, u32 len, u32 src_line_size, u32 src_stride, u32 dst_line_size, u32 dst_stride)
{
	if(src_stride!=0)
	{
		while(len>=src_line_size)
		{
			mclDmaMemcpy(handle, dst, src, src_line_size);
			src+=(src_line_size+src_stride)>>2;
 			dst+=src_line_size>>2;
			len-=src_line_size;
		}
		mclDmaMemcpy(handle, dst, src, len);
	}
	else if(dst_stride!=0)
	{
		while(len>=dst_line_size)
		{
			mclDmaMemcpy(handle, dst, src, dst_line_size);
			src+=dst_line_size>>2;
			dst+=(dst_line_size+dst_stride)>>2;
			len-=dst_line_size;
		}
		mclDmaMemcpy(handle, dst, src, len);
	}
	else mclDmaMemcpy(handle, dst, src, len);//if there is no src or dst stride, make a simple memcpy
}

void mclDmaMemset32(void *handle, u32 *dst, u32 len, u32 value)
{
	while(len-- > 0)
		*dst++ = value;
}

void mclDmaMemset64(void *handle, u32 *dst, u32 len, u64 value)
{
	while(len-- > 0)
	{
		*dst++ = value>>32;
		*dst++ = value;
	}
}
