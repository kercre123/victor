#include "mclDmaTransfer.h"
#include <stdio.h>
#include <mv_types.h>
#include <string.h>

dmaHandle leonHandle;
dmaTask taskList[10];

void mclDmaInit(void *handle, u8 shaveNum)
{
    leonHandle.svu_nr = shaveNum;
    leonHandle.lastTask = 0;

    *(u32 *)handle = (u32)&leonHandle;
}

void mclDmaAddTask(void *handle, u32 *dst , u32 *src, u32 len)
{
    dmaHandle *myHandle = (dmaHandle*)handle;

    taskList[myHandle->lastTask].src = src;
    taskList[myHandle->lastTask].dst = dst;
    taskList[myHandle->lastTask].len = len;
    taskList[myHandle->lastTask].srcLine = 0;
    taskList[myHandle->lastTask].dstLine = 0;
    taskList[myHandle->lastTask].srcStride = 0;
    taskList[myHandle->lastTask].dstStride = 0;
    taskList[myHandle->lastTask].completed = 0;

    myHandle->lastTask++;
}

void mclDmaAddTaskStride(void *handle, u32 *dst , u32 *src, u32 len, u32 src_line_size, u32 src_stride, u32 dst_line_size, u32 dst_stride)
{
    dmaHandle *myHandle = (dmaHandle*)handle;

    taskList[myHandle->lastTask].src = src;
    taskList[myHandle->lastTask].dst = dst;
    taskList[myHandle->lastTask].len = len;
    taskList[myHandle->lastTask].srcLine = src_line_size;
    taskList[myHandle->lastTask].dstLine = dst_line_size;
    taskList[myHandle->lastTask].srcStride = src_stride;
    taskList[myHandle->lastTask].dstStride = dst_stride;
    taskList[myHandle->lastTask].completed = 0;

    myHandle->lastTask++;
}

void mclDmaStartTasks(void *handle)
{
    //Currently we don't do nothing here. There might be some tasks checking
}

void mclDmaWaitAllTasksCompleted(void *handle)
{

    u8 i;
    dmaHandle *myHandle = (dmaHandle*)handle;

    for(i = 0; i < myHandle->lastTask; i++)
    {
        if(taskList[i].completed == 0)
            mclDmaMemcpyStride(handle, 
                           taskList[i].dst,
                           taskList[i].src,
                           taskList[i].len,
                           taskList[i].srcLine,
                           taskList[i].srcStride,
                           taskList[i].dstLine,
                           taskList[i].dstStride);
        taskList[i].completed = 1;
    }
}



void mclDmaMemcpy(void *handle, u32 *dst, u32 *src, u32 len)
{
    u8 i;
    for(i = 0; i < len>>2; i++)
        dst[i] = src[i];
}

void mclDmaMemcpyStride(void *handle, u32 *dst , u32 *src, u32 len, u32 src_line_size, u32 src_stride, u32 dst_line_size, u32 dst_stride)
{
    if(src_stride != 0 && dst_stride != 0 )
    {
        return;
//Not functional yet
//      u32 srcLnBytesLeft = src_line_size;
//      u32 dstLnBytesLeft = dst_line_size;
//
//      while(len-- > 0 )
//      {
//          if(srcLnBytesLeft == 0)
//          {
//              src+=src_stride;
//              srcLnBytesLeft = src_line_size;
//          }
//          if(dstLnBytesLeft == 0)
//          {
//          dst+=dst_stride;
//          dstLnBytesLeft = dst_line_size;
//          }
//          if(srcLnBytesLeft-- > 0 && dstLnBytesLeft-- > 0) 
//          {
//          *dst++ = *src++;//bad pointer arithmetic, should increase by 1 byte, not sizeof(u32)
//          }
//      }
    }
    else if(src_stride != 0)
    {
        while(len >= src_line_size)
        {
            mclDmaMemcpy(handle, dst, src, src_line_size);
            src += (src_line_size+src_stride) >> 2;
            dst += src_line_size >> 2;
            len -= src_line_size;
        }
        mclDmaMemcpy(handle, dst, src, len);
    }
    else if(dst_stride != 0)
    {
        while(len >= dst_line_size)
        {
            mclDmaMemcpy(handle, dst, src, dst_line_size);
            src += dst_line_size >> 2;
            dst += (dst_line_size + dst_stride) >> 2;
            len -= dst_line_size;
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
