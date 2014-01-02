///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     encoder run functions
///

#include <string.h>

#include "faac.h"
#include "faaccfg.h"

#include "shared.h"

#define SHAVE_HALT __asm("BRU.swih 0x001F \n\t nop 5");

//input stream info
unsigned long     sampleRate;
unsigned long     channelNumber;
unsigned long 	  byteRate;

//DDR buffers flags
volatile int      used_pcmbuf[PCM_BUFF_NR];
volatile int      used_aacbuf[AAC_BUFF_NR];

//DDR buffers pointers
unsigned char    *aacbuf[AAC_BUFF_NR];
int              *pcmbuf[PCM_BUFF_NR];

//CMX AAC buffers
unsigned char     cmx_aacbuf_mem[2048];
unsigned char     cmx_pcmbuf_mem[4096];

//informations for leon
unsigned long  totalBytesAAC;
int            currentFrame;
unsigned long  pcmFrameSize;

//encoder handle
faacEncHandle hEncoder;

//encoder status flag
enum AACSTATUSFLAG aacStatusFlag = IDLE;

unsigned long maxBytesOutput;


int initConfigureEncoder()
{
	unsigned long samplesInput;
	int i;

	// open the encoder library ; get handler
	hEncoder = faacEncOpen(sampleRate, channelNumber,
			&samplesInput, &maxBytesOutput);

	// set the input frame size for stream fetcher
	pcmFrameSize = channelNumber * samplesInput;

	// reset counters and flags
	currentFrame  = 0;
	totalBytesAAC = 0;

	for(i = 0; i < PCM_BUFF_NR; ++i)
	{
		used_pcmbuf[i] = -1;
	}
	for(i = 0; i < AAC_BUFF_NR; ++i)
	{
		used_aacbuf[i] = -1;
	}

	aacStatusFlag = IDLE;

	faacEncConfigurationPtr myFormat;
	unsigned int mpegVersion = MPEG4;
	unsigned int objectType = LOW;
	unsigned int useMidSide = 1;
	static unsigned int useTns = 1;
	unsigned int outputStream = 1; // ADTS_STREAM
	int shortctl = SHORTCTL_NORMAL;
	int cutOff = -1;
	unsigned long quantqual = 0;

	// get the encoder configuration handler
	myFormat = faacEncGetCurrentConfiguration(hEncoder);


	myFormat->aacObjectType = objectType;
	myFormat->mpegVersion = mpegVersion;
	myFormat->useTns = useTns;
	switch (shortctl) {
	case SHORTCTL_NOSHORT:
		//fprintf(stderr, "disabling short blocks\n");
		myFormat->shortctl = shortctl;
		break;
	case SHORTCTL_NOLONG:
		//fprintf(stderr, "disabling long blocks\n");
		myFormat->shortctl = shortctl;
		break;
	}
	myFormat->allowMidside = useMidSide;

	//myFormat->bitRate = (byteRate * 8) / channelNumber;

	if (cutOff <= 0) {
		if (cutOff < 0)
			// default
			cutOff = 0;
		else
			// disabled
			cutOff = sampleRate / 2;
	}
	if (cutOff > (sampleRate / 2))
		cutOff = sampleRate / 2;

	myFormat->bandWidth = cutOff;
	if (quantqual > 0)
		myFormat->quantqual = quantqual;

	myFormat->outputFormat = outputStream;
	myFormat->inputFormat = FAAC_INPUT_16BIT;


	/* write the configuration handler in the encoder instance */
	if (!faacEncSetConfiguration(hEncoder, myFormat)) {
		// Unsupported output format
		aacStatusFlag = CONFIG_FAILURE;
		SHAVE_HALT;
		return 0;
	}
	return 1;
}

int startEncoder()
{
	// configure the encoder library
	if( ! initConfigureEncoder() )
		return 0;

	// encoding loop
	aacStatusFlag = RUNNING;
	for(;;)	{
		int bytesWritten;
		int bytesRead;

		// wait for a full input memory slot
		while ( used_pcmbuf[currentFrame%PCM_BUFF_NR] < 0 );
		memcpy(cmx_pcmbuf_mem,
				pcmbuf[currentFrame%PCM_BUFF_NR],
				used_pcmbuf[currentFrame%PCM_BUFF_NR]);
		bytesRead = used_pcmbuf[currentFrame%PCM_BUFF_NR];
		// mark input buffer free
		used_pcmbuf[currentFrame%PCM_BUFF_NR] = -1;

		// call the actual encoding routine
		bytesWritten = faacEncEncode(hEncoder,
				(int *)cmx_pcmbuf_mem,
				bytesRead / 2,
				cmx_aacbuf_mem,
				maxBytesOutput);

		/* frame encoded succesfully */
		if (bytesWritten > 0)
		{
			//wait for a free output memory slot
			while ( used_aacbuf[currentFrame%AAC_BUFF_NR]  > 0);
			//copy from CMX memory to DRAM buffer
			memcpy(aacbuf[currentFrame%AAC_BUFF_NR],
					cmx_aacbuf_mem,
					bytesWritten);
			// mark aac buffer length
			used_aacbuf[currentFrame%AAC_BUFF_NR] = bytesWritten;

			totalBytesAAC += bytesWritten;
			//goto next frame
			currentFrame++;
		}
		else if (!bytesRead && bytesWritten == 0)
		{
			/* all done, bail out */
			aacStatusFlag = SUCCESS;
			SHAVE_HALT;
			return 1;
		}
		else if (bytesWritten < 0)
		{
			/* faacEncEncode() failed */
			aacStatusFlag = FAILURE;
			SHAVE_HALT;
			return 0;
			break ;
		}
	}
	SHAVE_HALT;
	return 0;
}
