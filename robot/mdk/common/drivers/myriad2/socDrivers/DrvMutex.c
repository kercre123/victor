#include "DrvMutex.h"
#include <registersMyriad2.h>
#include <DrvRegUtils.h>
#include "swcWhoAmI.h"
#include <assert.h>


u32 DrvMutexLock(u32 mutex)
{
	int waitIdx = 20;
	if (PROCESS_LEON_OS == swcWhoAmI())
	{
		SET_REG_HALF(CMX_MTX_BASE + CMX_MTX_LOS_GET, (mutex & MTX_NUM_MASK) | MTX_REQ_ON_RETRY);
		//return a status, leon will continue to wait for get this mutexs
		while(waitIdx--);
		if ((GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_0 + (mutex & MTX_NUM_MASK) * 4) & MTX_STAT_MASK) == (MTX_STAT_IN_USE | MTX_GARANTED_BY_LOS))
		{
			return 1;
		}
	}
	else
	{
		SET_REG_HALF(CMX_MTX_BASE + CMX_MTX_LRT_GET, (mutex & MTX_NUM_MASK) | MTX_REQ_ON_RETRY);
		while(waitIdx--);
		if( (GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_0 + (mutex & MTX_NUM_MASK) * 4) & MTX_STAT_MASK) == (MTX_STAT_IN_USE | MTX_GARANTED_BY_LRT))
		{
			return 1;
		}
	}
	return 0;
}

void DrvMutexWaitToKetIt(u32 mutex)
{
	if (PROCESS_LEON_OS == swcWhoAmI())
	{
		//wait for mutex to be get by LOS
		while ( (GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_0 + (mutex & MTX_NUM_MASK) * 4) & MTX_STAT_MASK) != (MTX_STAT_IN_USE | MTX_GARANTED_BY_LOS));
	}
	else
	{
		while ( (GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_0 + (mutex & MTX_NUM_MASK) * 4) & MTX_STAT_MASK) != (MTX_STAT_IN_USE | MTX_GARANTED_BY_LRT));
	}
}

void DrvMutexUnlock(u32 mutex)
{
	if (PROCESS_LEON_OS == swcWhoAmI())
	{
		SET_REG_HALF(CMX_MTX_BASE + CMX_MTX_LOS_GET, (mutex & MTX_NUM_MASK) | MTX_RELEASE);
		//wait for mutex to be releaed by LOS
		while ( (GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_0 + (mutex & MTX_NUM_MASK) * 4) & MTX_STAT_MASK) == (MTX_STAT_IN_USE | MTX_GARANTED_BY_LOS));
	}
	else
	{
		SET_REG_HALF(CMX_MTX_BASE + CMX_MTX_LRT_GET, (mutex & MTX_NUM_MASK) | MTX_RELEASE);
		while ( (GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_0 + (mutex & MTX_NUM_MASK) * 4) & MTX_STAT_MASK) == (MTX_STAT_IN_USE | MTX_GARANTED_BY_LRT));
	}

}

u32 DrvMutexLockIfAvailable(u32 mutex)
{
	int waitIdx = 20;
	if (PROCESS_LEON_OS == swcWhoAmI())
	{
		SET_REG_HALF(CMX_MTX_BASE + CMX_MTX_LOS_GET, (mutex & MTX_NUM_MASK) | MTX_REQ_OFF_RETRY);
		while(waitIdx--);
		if ((GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_0 + (mutex & MTX_NUM_MASK) * 4) & MTX_STAT_MASK) == (MTX_STAT_IN_USE | MTX_GARANTED_BY_LOS))
		{
			return 1;
		}
	}
	else
	{
		SET_REG_HALF(CMX_MTX_BASE + CMX_MTX_LRT_GET, (mutex & MTX_NUM_MASK) | MTX_REQ_OFF_RETRY);
		while(waitIdx--);
		if( (GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_0 + (mutex & MTX_NUM_MASK) * 4) & MTX_STAT_MASK) == (MTX_STAT_IN_USE | MTX_GARANTED_BY_LRT))
		{
			return 1;
		}
	}
	return 0;
}

void DrvMutexClearOwnPendingRequest(u32 mutex)
{
	if (PROCESS_LEON_OS == swcWhoAmI())
	{
		SET_REG_HALF(CMX_MTX_BASE + CMX_MTX_LOS_GET, (mutex & MTX_NUM_MASK) | MTX_CLEAR_OWN_PENDING);
	}
	else
	{
		SET_REG_HALF(CMX_MTX_BASE + CMX_MTX_LRT_GET, (mutex & MTX_NUM_MASK) | MTX_CLEAR_OWN_PENDING);
	}
}

void DrvMutexClearAllPendingRequest(void)
{
	if (PROCESS_LEON_OS == swcWhoAmI())
	{
		SET_REG_HALF(CMX_MTX_BASE + CMX_MTX_LOS_GET, MTX_CLEAR_ALL_PENDING);
	}
	else
	{
		SET_REG_HALF(CMX_MTX_BASE + CMX_MTX_LRT_GET, MTX_CLEAR_ALL_PENDING);
	}
}

u32 DrvMutexGetStatus(u32 mutex)
{
	return (GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_0 + (mutex & MTX_NUM_MASK) * 4) & MTX_STAT_MASK);
}

u32 DrvMutexGetAllStatus(void)
{
	return (GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_STATUS));
}


void DrvMutexClearInterupts(u32 intMaskToClear)
{
	if (PROCESS_LEON_OS == swcWhoAmI())
	{
		SET_REG_WORD(CMX_MTX_BASE + CMX_MTX_LOS_INT_CLR, intMaskToClear);
	}
	else
	{
		SET_REG_WORD(CMX_MTX_BASE + CMX_MTX_LRT_INT_CLR, intMaskToClear);
	}
}

void DrvMutexConfigInterupts(u32 intEnableMask)
{
	if (PROCESS_LEON_OS == swcWhoAmI())
	{
		SET_REG_WORD(CMX_MTX_BASE + CMX_MTX_LOS_INT_EN, intEnableMask);
	}
	else
	{
		SET_REG_WORD(CMX_MTX_BASE + CMX_MTX_LRT_INT_EN, intEnableMask);
	}
}

u32 DrvMutexGetInteruptsStatus(void)
{
	if (PROCESS_LEON_OS == swcWhoAmI())
	{
		return (GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_LOS_INT_STAT));
	}
	else
	{
		return (GET_REG_WORD_VAL(CMX_MTX_BASE + CMX_MTX_LRT_INT_STAT));
	}
}

