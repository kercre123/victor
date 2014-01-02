/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     leon and shave shared header
///

#ifndef SHARED_H_
#define SHARED_H_

enum AACSTATUSFLAG
{
	SUCCESS,
	FAILURE,
	IDLE,
	RUNNING,
	CONFIG_FAILURE
};

#define AAC_BUFF_NR 3
#define PCM_BUFF_NR 3

#endif /* SHARED_H_ */
