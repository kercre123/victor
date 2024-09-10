//////////////////////////////////////////////////////////////////////
//
// AkFilePackageLowLevelIOBlocking.h
//
// Extends the AudioDefaultIOHookBlocking low level I/O hook with File
// Package handling functionality. 
//
// See AkDefaultIOHookBlocking.h for details on using the blocking 
// low level I/O hook. 
// 
// See AkFilePackageLowLevelIO.h for details on using file packages.
// 
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_FILE_PACKAGE_LOW_LEVEL_IO_BLOCKING_H_
#define _AK_FILE_PACKAGE_LOW_LEVEL_IO_BLOCKING_H_

#include "audioFilePackageLowLevelIO.h"
#include "audioDefaultIOHookBlocking.h"

class AudioFilePackageLowLevelIOBlocking
	: public AudioFilePackageLowLevelIO<AudioDefaultIOHookBlocking>
{
public:
	AudioFilePackageLowLevelIOBlocking() { }
	virtual ~AudioFilePackageLowLevelIOBlocking() { }
};

#endif //_AK_FILE_PACKAGE_LOW_LEVEL_IO_BLOCKING_H_
