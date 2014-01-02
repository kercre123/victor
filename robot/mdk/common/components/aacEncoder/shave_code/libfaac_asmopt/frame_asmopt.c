/*
 * FAAC - Freeware Advanced Audio Coder
 * Copyright (C) 2001 Menno Bakker
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: frame.c,v 1.67 2004/11/17 14:26:06 menno Exp $
 */

/*
 * CHANGES:
 *  2001/01/17: menno: Added frequency cut off filter.
 *  2001/02/28: menno: Added Temporal Noise Shaping.
 *  2001/03/05: menno: Added Long Term Prediction.
 *  2001/05/01: menno: Added backward prediction.
 *
 */

#include "shaveMatopts.h"

#include "frame.h"
#include "coder.h"
#include "midside.h"
#include "channels.h"
#include "bitstream.h"
#include "filtbank.h"
#include "aacquant.h"
#include "util.h"
#include "huffman.h"
#include "psych.h"
#include "tns.h"

#define FAAC_RELEASE 1
#define FAAC_VERSION "1.28"

static const psymodellist_t psymodellist[] = {
		{&psymodel2, "knipsycho psychoacoustic"},
		{NULL}
};

static SR_INFO srInfo[12+1];

// base bandwidth for q=100
static const int bwbase = 16000;
// bandwidth multiplier (for quantiser)
static const int bwmult = 120;
// max bandwidth/samplerate ratio
static const float bwfac = 0.45;

faacEncConfigurationPtr FAACAPI faacEncGetCurrentConfiguration(faacEncHandle hEncoder)
{
	faacEncConfigurationPtr config = &(hEncoder->config);

	return config;
}

int FAACAPI faacEncSetConfiguration(faacEncHandle hEncoder,
		faacEncConfigurationPtr config)
{
	int i;

	hEncoder->config.allowMidside = config->allowMidside;
	hEncoder->config.useLfe = config->useLfe;
	hEncoder->config.useTns = config->useTns;
	hEncoder->config.aacObjectType = config->aacObjectType;
	hEncoder->config.mpegVersion = config->mpegVersion;
	hEncoder->config.outputFormat = config->outputFormat;
	hEncoder->config.inputFormat = config->inputFormat;
	hEncoder->config.shortctl = config->shortctl;

	/* Check for correct bitrate */
	if (config->bitRate > MaxBitrate(hEncoder->sampleRate))
		return 0;


	if (config->bitRate && !config->bandWidth)
	{
		static struct {
			int rate; // per channel at 44100 sampling frequency
			int cutoff;
		}	rates[] = {
				{29500, 5000},
				{37500, 7000},
				{47000, 10000},
				{64000, 16000},
				{76000, 20000},
				{0, 0}
		};

		int f0, f1;
		int r0, r1;

		float tmpbitRate = (float)config->bitRate * 44100 / hEncoder->sampleRate;

		config->quantqual = 100;

		f0 = f1 = rates[0].cutoff;
		r0 = r1 = rates[0].rate;

		for (i = 0; rates[i].rate; i++)
		{
			f0 = f1;
			f1 = rates[i].cutoff;
			r0 = r1;
			r1 = rates[i].rate;
			if (rates[i].rate >= tmpbitRate)
				break;
		}

		if (tmpbitRate > r1)
			tmpbitRate = r1;
		if (tmpbitRate < r0)
			tmpbitRate = r0;

		if (f1 > f0)
			config->bandWidth =
					FAAC_pow((float)tmpbitRate / r1,
							FAAC_log((float)f1 / f0) / FAAC_log ((float)r1 / r0)) * (float)f1;
		else
			config->bandWidth = f1;

		config->bandWidth =
				(float)config->bandWidth * hEncoder->sampleRate / 44100;
		config->bitRate = tmpbitRate * hEncoder->sampleRate / 44100;

		if (config->bandWidth > bwbase)
			config->bandWidth = bwbase;
	}

	hEncoder->config.bitRate = config->bitRate;

	if (!config->bandWidth)
	{
		config->bandWidth = (config->quantqual - 100) * bwmult + bwbase;
	}

	hEncoder->config.bandWidth = config->bandWidth;

	// check bandwidth
	if (hEncoder->config.bandWidth < 100)
		hEncoder->config.bandWidth = 100;
	if (hEncoder->config.bandWidth > (hEncoder->sampleRate / 2))
		hEncoder->config.bandWidth = hEncoder->sampleRate / 2;

	if (config->quantqual > 500)
		config->quantqual = 500;
	if (config->quantqual < 10)
		config->quantqual = 10;

	hEncoder->config.quantqual = config->quantqual;

	/* set quantization quality */
	hEncoder->aacquantCfg.quality = config->quantqual;

	// reset psymodel
	hEncoder->psymodel->PsyEnd(&hEncoder->gpsyInfo, hEncoder->psyInfo, hEncoder->numChannels);

	if (config->psymodelidx >= (sizeof(psymodellist) / sizeof(psymodellist[0]) - 1))
		config->psymodelidx = (sizeof(psymodellist) / sizeof(psymodellist[0])) - 2;

	hEncoder->config.psymodelidx = config->psymodelidx;
	hEncoder->psymodel = psymodellist[hEncoder->config.psymodelidx].model;


	hEncoder->psymodel->PsyInit(&hEncoder->gpsyInfo, hEncoder->psyInfo, hEncoder->numChannels,
			hEncoder->sampleRate, hEncoder->srInfo->cb_width_long,
			hEncoder->srInfo->num_cb_long, hEncoder->srInfo->cb_width_short,
			hEncoder->srInfo->num_cb_short);

	/* load channel_map */
	//for( i = 0; i < 64; i++ )
	for( i = 0; i < 2; i++ )
		hEncoder->config.channel_map[i] = config->channel_map[i];

	/* OK */
	return 1;
}




faacEncStruct g_hEncoder;

faacEncHandle FAACAPI faacEncOpen(unsigned long sampleRate,
		unsigned int numChannels,
		unsigned long *inputSamples,
		unsigned long *maxOutputBytes)
{
	unsigned int channel;
	faacEncHandle hEncoder;

	*inputSamples = FRAME_LEN*numChannels;
	*maxOutputBytes = (6144/8)*numChannels;

	//hEncoder = (faacEncStruct*)AllocMemory(sizeof(faacEncStruct), "hEncoder");
	hEncoder = &g_hEncoder;//static alloc

	SetMemory(hEncoder, 0, sizeof(faacEncStruct));

	hEncoder->numChannels = numChannels;
	hEncoder->sampleRate = sampleRate;
	hEncoder->sampleRateIdx = GetSRIndex(sampleRate);

	/* Initialize variables to default values */
	hEncoder->frameNum = 0;
	hEncoder->flushFrame = 0;

	/* Default configuration */
	hEncoder->config.mpegVersion = MPEG4;
	hEncoder->config.aacObjectType = LTP;
	hEncoder->config.allowMidside = 1;
	hEncoder->config.useLfe = 0; //andreil: was 1
	hEncoder->config.useTns = 0;
	hEncoder->config.bitRate = 0; /* default bitrate / channel */
	hEncoder->config.bandWidth = bwfac * hEncoder->sampleRate;
	if (hEncoder->config.bandWidth > bwbase)
		hEncoder->config.bandWidth = bwbase;
	hEncoder->config.quantqual = 100;
	hEncoder->config.psymodellist = (psymodellist_t *)psymodellist;
	hEncoder->config.psymodelidx = 0;
	hEncoder->psymodel =
			hEncoder->config.psymodellist[hEncoder->config.psymodelidx].model;
	hEncoder->config.shortctl = SHORTCTL_NORMAL;

	/* default channel map is straight-through */
	//for( channel = 0; channel < 64; channel++ )
	for( channel = 0; channel < 2; channel++ )//only doing stereo :)
		hEncoder->config.channel_map[channel] = channel;

	/*
        by default we have to be compatible with all previous software
        which assumes that we will generate ADTS
        /AV
	 */
	hEncoder->config.outputFormat = 1;

	/*
        be compatible with software which assumes 24bit in 32bit PCM
	 */
	//hEncoder->config.inputFormat = FAAC_INPUT_32BIT;
	hEncoder->config.inputFormat = FAAC_INPUT_16BIT;//andreil: as if we get data from i2s !!!!


	/* find correct sampling rate depending parameters */
	hEncoder->srInfo = &srInfo[hEncoder->sampleRateIdx];

	for (channel = 0; channel < numChannels; channel++)
	{
		hEncoder->coderInfo[channel].prev_window_shape = SINE_WINDOW;
		hEncoder->coderInfo[channel].window_shape = SINE_WINDOW;
		hEncoder->coderInfo[channel].block_type = ONLY_LONG_WINDOW;
		hEncoder->coderInfo[channel].num_window_groups = 1;
		hEncoder->coderInfo[channel].window_group_length[0] = 1;

		/* FIXME: Use sr_idx here */
		hEncoder->coderInfo[channel].max_pred_sfb = GetMaxPredSfb(hEncoder->sampleRateIdx);

		hEncoder->sampleBuff[channel] = NULL;
		hEncoder->nextSampleBuff[channel] = NULL;
		hEncoder->next2SampleBuff[channel] = NULL;
		//hEncoder->ltpTimeBuff[channel] = (float*)AllocMemory(2*BLOCK_LEN_LONG*sizeof(float), "hEncoder->ltpTimeBuff[channel]");
		//SetMemory(hEncoder->ltpTimeBuff[channel], 0, 2*BLOCK_LEN_LONG*sizeof(float));
	}

	/* Initialize coder functions */
	fft_initialize( &hEncoder->fft_tables );

	FilterBankInit(hEncoder);

	HuffmanInit(hEncoder->coderInfo, hEncoder->numChannels);

	/* Return handle */
	return hEncoder;
}


int FAACAPI faacEncClose(faacEncHandle hEncoder)
{
	// placeholder function for compatibility
	// mem statically allocated ; no need for this function
	return 0;
}

short g_sampleBuff[MAX_CHANNELS][FRAME_LEN];//static alloc

int FAACAPI faacEncEncode(faacEncHandle hEncoder,
		int *inputBuffer,
		unsigned int samplesInput,
		unsigned char *outputBuffer,
		unsigned int bufferSize
)
{
	unsigned int channel, i;
	int sb, frameBytes;
	unsigned int offset;
	BitStream *bitStream; /* bitstream used for writing the frame to */
	//TnsInfo *tnsInfo_for_LTP;
	TnsInfo *tnsDecInfo;
#ifdef DRM
	int desbits, diff;
	float fix;
#endif

	/* local copy's of parameters */
	ChannelInfo *channelInfo = hEncoder->channelInfo;
	CoderInfo *coderInfo = hEncoder->coderInfo;
	unsigned int numChannels = hEncoder->numChannels;
	unsigned int sampleRate = hEncoder->sampleRate;
	unsigned int aacObjectType = hEncoder->config.aacObjectType;
	unsigned int mpegVersion = hEncoder->config.mpegVersion;
	unsigned int useLfe = hEncoder->config.useLfe;
	unsigned int useTns = hEncoder->config.useTns;
	unsigned int allowMidside = hEncoder->config.allowMidside;
	unsigned int bandWidth = hEncoder->config.bandWidth;
	unsigned int shortctl = hEncoder->config.shortctl;

	/* Increase frame number */
	hEncoder->frameNum++;

	if (samplesInput == 0)
		hEncoder->flushFrame++;

	/* After 4 flush frames all samples have been encoded,
       return 0 bytes written */
	if (hEncoder->flushFrame > 4)
		return 0;

	/* Determine the channel configuration */
	GetChannelInfo(channelInfo, numChannels, useLfe);

	/* Update current sample buffers */
	for (channel = 0; channel < numChannels; channel++)
	{
		//float *tmp;
		short *tmp;

		if (!hEncoder->sampleBuff[channel])
		{
			//hEncoder->sampleBuff[channel] = (short*)AllocMemory(FRAME_LEN*sizeof(short), "hEncoder->sampleBuff[channel]");
			hEncoder->sampleBuff[channel] = g_sampleBuff[channel];//(short*)AllocMemory(FRAME_LEN*sizeof(short), "hEncoder->sampleBuff[channel]");
		}

		//andreil: buffer circular aici...
		tmp = hEncoder->sampleBuff[channel];
		//andreil: it seems to me that this is just a delay buffer...
		//         don't see why need to keep it...
		hEncoder->nextSampleBuff [channel] = tmp;
		hEncoder->next2SampleBuff[channel] = tmp;
		hEncoder->next3SampleBuff[channel] = tmp;


		// if (samplesInput == 0)
		// {
		// /* start flushing*/
		// for (i = 0; i < FRAME_LEN; i++)
		// hEncoder->next3SampleBuff[channel][i] = 0;//0.0;
		// }
		// else
		{
			int samples_per_channel = samplesInput/numChannels;

			/* handle the various input formats and channel remapping */
			// switch( hEncoder->config.inputFormat )
			// {
			// //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			// case FAAC_INPUT_16BIT:
			{
				short *input_channel = (short*)inputBuffer + hEncoder->config.channel_map[channel];

				//printf("samples_per_channel = %d (%d/%d)", samples_per_channel, samplesInput, numChannels);

				for (i = 0; i < samples_per_channel; i++)
				{
					//andreil: if datatype = short, the "(float)" doesn't matter...
					hEncoder->next3SampleBuff[channel][i] = *input_channel;
					//printf("%d (%x)-> [%x]-> (%x)",i, input_channel, *input_channel, hEncoder->next3SampleBuff[channel][i]);
					input_channel += numChannels;
				}
			}
			// break;
			// }

			//WARNING andreil: se completeaza cu zero... inutil....
			for (i = (int)(samplesInput/numChannels); i < FRAME_LEN; i++)
			{
				//printf("z");
				hEncoder->next3SampleBuff[channel][i] = 0;//0.0;
			}
		}
		//##############################


		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@
		//   printf("look at samples %x and %x !", inputBuffer, hEncoder->next3SampleBuff[channel]);
		//   SHAVE_HALT;
		//@@@@@@@@@@@@@@@@@@@@@@@@@@@@


		/* Psychoacoustics */
		/* Update buffers and run FFT on new samples */
		/* LFE psychoacoustic can run without it */
		//printf("Got samples for %d", channel);

		if (!channelInfo[channel].lfe || channelInfo[channel].cpe)
		{
			hEncoder->psymodel->PsyBufferUpdate( 
					&hEncoder->fft_tables, 
					&hEncoder->gpsyInfo, 
					&hEncoder->psyInfo[channel],
					hEncoder->next3SampleBuff[channel], //andreil: PCM-urile ca float
					bandWidth,
					hEncoder->srInfo->cb_width_short,
					hEncoder->srInfo->num_cb_short);
		}
	}

	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// FINE till here: checked gpsyInfo->fftEnrgNext2S[], data is ok... small mantissa differences
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	//if (hEncoder->frameNum <= 3) /* Still filling up the buffers */
	//    return 0;



	/* Psychoacoustics */
	hEncoder->psymodel->PsyCalculate(
			channelInfo,
			&hEncoder->gpsyInfo,
			hEncoder->psyInfo,
			hEncoder->srInfo->cb_width_long,
			hEncoder->srInfo->num_cb_long,
			hEncoder->srInfo->cb_width_short,
			hEncoder->srInfo->num_cb_short,
			numChannels);
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	//   printf("psyInfo[0].block_type = %d\n", hEncoder->psyInfo[0].block_type);
	//   printf("psyInfo[1].block_type = %d\n", hEncoder->psyInfo[1].block_type);    
	//   SHAVE_HALT;
	// //FINE TILL NOW !!! block type = 0,0 !!!!
	//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@




	hEncoder->psymodel->BlockSwitch(coderInfo, hEncoder->psyInfo, numChannels);
	// //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// printf(" coderInfo[0].desired_block_type = %d\n", coderInfo[0].desired_block_type);
	// printf(" coderInfo[1].desired_block_type = %d\n", coderInfo[1].desired_block_type);
	// SHAVE_HALT;
	// //FINE till now, desired_block_type = 0 !!!!
	// //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	/* force block type */
	if (shortctl == SHORTCTL_NOSHORT)
	{
		for (channel = 0; channel < numChannels; channel++)
		{
			coderInfo[channel].block_type = ONLY_LONG_WINDOW;
		}
	}
	if (shortctl == SHORTCTL_NOLONG)
	{
		for (channel = 0; channel < numChannels; channel++)
		{
			coderInfo[channel].block_type = ONLY_SHORT_WINDOW;
		}
	}

	/* AAC Filterbank, MDCT with overlap and add */
	for (channel = 0; channel < numChannels; channel++) {
		int k;

		//andreil: check freqBuff persistency
		//memset(hEncoder->freqBuff[channel],0, 2048*4);

		//andreil: oberlapBuff = doar buffer intermediar.... e folosit doar pt. niste mutari...
		FilterBank(hEncoder,
				&coderInfo[channel],
				hEncoder->sampleBuff[channel],
				hEncoder->freqBuff[channel],
				hEncoder->overlapBuff[channel],
				MOVERLAPPED);

		if (coderInfo[channel].block_type == ONLY_SHORT_WINDOW) {
			for (k = 0; k < 8; k++) {
				specFilter(hEncoder->freqBuff[channel]+k*BLOCK_LEN_SHORT,
						sampleRate, bandWidth, BLOCK_LEN_SHORT);
			}
		} else {
			specFilter(hEncoder->freqBuff[channel], sampleRate,
					bandWidth, BLOCK_LEN_LONG);
		}
	}


	// {//@@@@@@@@@@@@@@@@@@@@@@@@@
	// //dump 4096B from here...
	// printf("freqBuff[0] at %x\n", hEncoder->freqBuff[0]);
	// printf("freqBuff[1] at %x\n", hEncoder->freqBuff[1]);
	// SHAVE_HALT;
	// //OK till now... very small differences !!!
	// }//@@@@@@@@@@@@@@@@@@@@@@@@@



	/* TMP: Build sfb offset table and other stuff */
	for (channel = 0; channel < numChannels; channel++) {
		channelInfo[channel].msInfo.is_present = 0;

		if (coderInfo[channel].block_type == ONLY_SHORT_WINDOW) {
			coderInfo[channel].max_sfb = hEncoder->srInfo->num_cb_short;
			coderInfo[channel].nr_of_sfb = hEncoder->srInfo->num_cb_short;

			coderInfo[channel].num_window_groups = 1;
			coderInfo[channel].window_group_length[0] = 8;
			coderInfo[channel].window_group_length[1] = 0;
			coderInfo[channel].window_group_length[2] = 0;
			coderInfo[channel].window_group_length[3] = 0;
			coderInfo[channel].window_group_length[4] = 0;
			coderInfo[channel].window_group_length[5] = 0;
			coderInfo[channel].window_group_length[6] = 0;
			coderInfo[channel].window_group_length[7] = 0;

			offset = 0;
			for (sb = 0; sb < coderInfo[channel].nr_of_sfb; sb++) {
				coderInfo[channel].sfb_offset[sb] = offset;
				offset += hEncoder->srInfo->cb_width_short[sb];
			}
			coderInfo[channel].sfb_offset[coderInfo[channel].nr_of_sfb] = offset;
		} else {
			coderInfo[channel].max_sfb = hEncoder->srInfo->num_cb_long;
			coderInfo[channel].nr_of_sfb = hEncoder->srInfo->num_cb_long;

			coderInfo[channel].num_window_groups = 1;
			coderInfo[channel].window_group_length[0] = 1;

			offset = 0;
			for (sb = 0; sb < coderInfo[channel].nr_of_sfb; sb++) {
				coderInfo[channel].sfb_offset[sb] = offset;
				offset += hEncoder->srInfo->cb_width_long[sb];
			}
			coderInfo[channel].sfb_offset[coderInfo[channel].nr_of_sfb] = offset;
		}
	}


	// {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// printf(" coderInfo[0].sfb_offset at   %x\n",  coderInfo[0].sfb_offset);
	// printf(" coderInfo[0].max_sfb     =   %x\n",  coderInfo[0].max_sfb);
	// printf(" coderInfo[0].nr_of_sfb   =   %x\n",  coderInfo[0].nr_of_sfb);

	// printf(" coderInfo[1].sfb_offset at   %x\n",  coderInfo[1].sfb_offset);
	// printf(" coderInfo[1].max_sfb     =   %x\n",  coderInfo[1].max_sfb);
	// printf(" coderInfo[1].nr_of_sfb   =   %x\n",  coderInfo[1].nr_of_sfb);
	// SHAVE_HALT;
	// //OK till now !!!!
	// }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	/* Perform TNS analysis and filtering */
	for (channel = 0; channel < numChannels; channel++) {
		if ((!channelInfo[channel].lfe) && (useTns)) {
			TnsEncode(&(coderInfo[channel].tnsInfo),
					coderInfo[channel].max_sfb,
					coderInfo[channel].max_sfb,
					coderInfo[channel].block_type,
					(int*)coderInfo[channel].sfb_offset,
					hEncoder->freqBuff[channel]);
		} else {
			coderInfo[channel].tnsInfo.tnsDataPresent = 0;      /* TNS not used for LFE */
		}
	}


	for(channel = 0; channel < numChannels; channel++)
	{
		coderInfo[channel].pred_global_flag = 0;
	}

	for (channel = 0; channel < numChannels; channel++) {
		if (coderInfo[channel].block_type == ONLY_SHORT_WINDOW) {
			SortForGrouping(&coderInfo[channel],
					&hEncoder->psyInfo[channel],
					&channelInfo[channel],
					hEncoder->srInfo->cb_width_short,
					hEncoder->freqBuff[channel]);
		}
		CalcAvgEnrg(&coderInfo[channel], hEncoder->freqBuff[channel]);
	}
	// {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// printf("coderInfo[0].lastx = %d\n", coderInfo[0].lastx);
	// printf("coderInfo[0].lastx = %x\n", *((unsigned int*)&coderInfo[0].avgenrg));

	// printf("coderInfo[1].lastx = %d\n", coderInfo[1].lastx);
	// printf("coderInfo[1].lastx = %x\n", *((unsigned int*)&coderInfo[1].avgenrg));
	// SHAVE_HALT;
	// //works till here... mantissa differences... acceptable I'd say...
	// }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@





	MSEncode(coderInfo, channelInfo, hEncoder->freqBuff, numChannels, allowMidside);
	// {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// printf("hEncoder->freqBuff[0]         at %x\n", hEncoder->freqBuff[0]);
	// printf("channelInfo[0].msInfo.ms_used at %x\n", channelInfo[0].msInfo.ms_used);
	// printf("hEncoder->freqBuff[1]         at %x\n", hEncoder->freqBuff[1]);
	// printf("channelInfo[1].msInfo.ms_used at %x\n", channelInfo[1].msInfo.ms_used);
	// SHAVE_HALT;
	// //Works till here... freqBuff there are some mantissa differences.... but it's ok I'd say
	// // ms_used are identical
	// }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@



	for (channel = 0; channel < numChannels; channel++)
	{
		CalcAvgEnrg(&coderInfo[channel], hEncoder->freqBuff[channel]);
	}
	// {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
	// printf("coderInfo[0].lastx    = %d\n", coderInfo[0].lastx);
	// printf("coderInfo[0].avgenrg  = %x\n", *((unsigned int*)&coderInfo[0].avgenrg));

	// printf("coderInfo[1].lastx    = %d\n", coderInfo[1].lastx);
	// printf("coderInfo[1].avgenrg  = %x\n", *((unsigned int*)&coderInfo[1].avgenrg));
	// SHAVE_HALT;
	// //works till here... mantissa differences... acceptable I'd say...
	// }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


#ifdef DRM
	/* loop the quantization until the desired bit-rate is reached */
	diff = 1; /* to enter while loop */
	hEncoder->aacquantCfg.quality = 120; /* init quality setting */
	while (diff > 0) { /* if too many bits, do it again */
#endif
		//return 0;
		/* Quantize and code the signal */
		for (channel = 0; channel < numChannels; channel++) {
			if (coderInfo[channel].block_type == ONLY_SHORT_WINDOW) {
				//aici nu intra
				AACQuantize(&coderInfo[channel], &hEncoder->psyInfo[channel],
						&channelInfo[channel], hEncoder->srInfo->cb_width_short,
						hEncoder->srInfo->num_cb_short, hEncoder->freqBuff[channel],
						&(hEncoder->aacquantCfg));
			} else {
				//_printf("b");
				AACQuantize(&coderInfo[channel], &hEncoder->psyInfo[channel],
						&channelInfo[channel], hEncoder->srInfo->cb_width_long,
						hEncoder->srInfo->num_cb_long, hEncoder->freqBuff[channel],
						&(hEncoder->aacquantCfg));
			}
		}
		//return 0;
#ifdef DRM
		//andreil: removed code
#endif

		// fix max_sfb in CPE mode
		for (channel = 0; channel < numChannels; channel++)
		{
			if (channelInfo[channel].present
					&& (channelInfo[channel].cpe)
					&& (channelInfo[channel].ch_is_left))
			{
				CoderInfo *cil, *cir;

				cil = &coderInfo[channel];
				cir = &coderInfo[channelInfo[channel].paired_ch];

				cil->max_sfb = cir->max_sfb = max(cil->max_sfb, cir->max_sfb);
				cil->nr_of_sfb = cir->nr_of_sfb = cil->max_sfb;
			}
		}

		MSReconstruct(coderInfo, channelInfo, numChannels);

		for (channel = 0; channel < numChannels; channel++)
		{
			/* If short window, reconstruction not needed for prediction */
			if (coderInfo[channel].block_type == ONLY_SHORT_WINDOW) {
				//int sind;
				//for (sind = 0; sind < BLOCK_LEN_LONG; sind++) {
				//	coderInfo[channel].requantFreq[sind] = 0.0;
				//}
			} else {

				if((coderInfo[channel].tnsInfo.tnsDataPresent != 0) && (useTns))
					tnsDecInfo = &(coderInfo[channel].tnsInfo);
				else
					tnsDecInfo = NULL;
			}
		}
		//return 0;
		/* Write the AAC bitstream */
		bitStream = OpenBitStream(bufferSize, outputBuffer);

		WriteBitstream(hEncoder, coderInfo, channelInfo, bitStream, numChannels);

		/* Close the bitstream and return the number of bytes written */
		frameBytes = CloseBitStream(bitStream);

		/* Adjust quality to get correct average bitrate */
		if (hEncoder->config.bitRate)
		{
			float fix;
			int desbits = numChannels * (hEncoder->config.bitRate * FRAME_LEN)
						/ hEncoder->sampleRate;
			int diff = (frameBytes * 8) - desbits;

			hEncoder->bitDiff += diff;
			fix = (float)hEncoder->bitDiff / desbits;
			fix *= 0.01;
			fix = max(fix, -0.2);
			fix = min(fix, 0.2);

			if (((diff > 0) && (fix > 0.0)) || ((diff < 0) && (fix < 0.0)))
			{
				hEncoder->aacquantCfg.quality *= (1.0 - fix);
				if (hEncoder->aacquantCfg.quality > 300)
					hEncoder->aacquantCfg.quality = 300;
				if (hEncoder->aacquantCfg.quality < 50)
					hEncoder->aacquantCfg.quality = 50;
			}
		}
		return frameBytes;
	}

	/* Scalefactorband data table for 1024 transform length */
	static SR_INFO srInfo[12+1] =
	{
			{ 96000, 41, 12,
					{
							4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
							8, 8, 8, 8, 8, 12, 12, 12, 12, 12, 16, 16, 24, 28,
							36, 44, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
					},{
							4, 4, 4, 4, 4, 4, 8, 8, 8, 16, 28, 36
			}
			}, { 88200, 41, 12,
					{
							4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
							8, 8, 8, 8, 8, 12, 12, 12, 12, 12, 16, 16, 24, 28,
							36, 44, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
					},{
							4, 4, 4, 4, 4, 4, 8, 8, 8, 16, 28, 36
			}
			}, { 64000, 47, 12,
					{
							4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
							8, 8, 8, 8, 12, 12, 12, 16, 16, 16, 20, 24, 24, 28,
							36, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
							40, 40, 40, 40, 40
					},{
							4, 4, 4, 4, 4, 4, 8, 8, 8, 16, 28, 32
			}
			}, { 48000, 49, 14,
					{
							4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  8,
							12, 12, 12, 12, 16, 16, 20, 20, 24, 24, 28, 28, 32, 32, 32, 32, 32, 32,
							32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 96
					}, {
							4,  4,  4,  4,  4,  8,  8,  8, 12, 12, 12, 16, 16, 16
			}
			}, { 44100, 49, 14,
					{
							4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  8,
							12, 12, 12, 12, 16, 16, 20, 20, 24, 24, 28, 28, 32, 32, 32, 32, 32, 32,
							32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 96
					}, {
							4,  4,  4,  4,  4,  8,  8,  8, 12, 12, 12, 16, 16, 16
			}
			}, { 32000, 51, 14,
					{
							4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,
							8,  8,  8,  12, 12, 12, 12, 16, 16, 20, 20, 24, 24, 28,
							28, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32,
							32, 32, 32, 32, 32, 32, 32, 32, 32
					},{
							4,  4,  4,  4,  4,  8,  8,  8,  12, 12, 12, 16, 16, 16
			}
			}, { 24000, 47, 15,
					{
							4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  8,
							8,  8,  8,  12, 12, 12, 12, 16, 16, 16, 20, 20, 24, 24, 28, 28, 32,
							36, 36, 40, 44, 48, 52, 52, 64, 64, 64, 64, 64
					}, {
							4,  4,  4,  4,  4,  4,  4,  8,  8,  8, 12, 12, 16, 16, 20
			}
			}, { 22050, 47, 15,
					{
							4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  4,  8,  8,  8,  8,  8,  8,  8,
							8,  8,  8,  12, 12, 12, 12, 16, 16, 16, 20, 20, 24, 24, 28, 28, 32,
							36, 36, 40, 44, 48, 52, 52, 64, 64, 64, 64, 64
					}, {
							4,  4,  4,  4,  4,  4,  4,  8,  8,  8, 12, 12, 16, 16, 20
			}
			}, { 16000, 43, 15,
					{
							8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 12, 12, 12,
							12, 12, 12, 12, 12, 12, 16, 16, 16, 16, 20, 20, 20, 24,
							24, 28, 28, 32, 36, 40, 40, 44, 48, 52, 56, 60, 64, 64, 64
					}, {
							4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 12, 12, 16, 20, 20
			}
			}, { 12000, 43, 15,
					{
							8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 12, 12, 12,
							12, 12, 12, 12, 12, 12, 16, 16, 16, 16, 20, 20, 20, 24,
							24, 28, 28, 32, 36, 40, 40, 44, 48, 52, 56, 60, 64, 64, 64
					}, {
							4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 12, 12, 16, 20, 20
			}
			}, { 11025, 43, 15,
					{
							8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 12, 12, 12,
							12, 12, 12, 12, 12, 12, 16, 16, 16, 16, 20, 20, 20, 24,
							24, 28, 28, 32, 36, 40, 40, 44, 48, 52, 56, 60, 64, 64, 64
					}, {
							4, 4, 4, 4, 4, 4, 4, 4, 8, 8, 12, 12, 16, 20, 20
			}
			}, { 8000, 40, 15,
					{
							12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 16,
							16, 16, 16, 16, 16, 16, 20, 20, 20, 20, 24, 24, 24, 28,
							28, 32, 36, 36, 40, 44, 48, 52, 56, 60, 64, 80
					}, {
							4, 4, 4, 4, 4, 4, 4, 8, 8, 8, 8, 12, 16, 20, 20
			}
			},
			{ -1 }
	};

/*
$Log: frame.c,v $
Revision 1.67  2004/11/17 14:26:06  menno
Infinite loop fix
dunno if this is good, encoder might be tuned to use energies from before MS encoding. But since the MS encoded samples are used in quantisation this might actually be better. Please test.

Revision 1.66  2004/11/04 12:51:09  aforanna
version number updated to 1.24.1 due to changes in Winamp and CoolEdit plugins

Revision 1.65  2004/07/18 09:34:24  corrados
New bandwidth settings for DRM, improved quantization quality adaptation (almost constant bit-rate now)

Revision 1.64  2004/07/13 17:56:37  corrados
bug fix with new object type definitions

Revision 1.63  2004/07/08 14:01:25  corrados
New scalefactorband table for 960 transform length, bug fix in HCR

Revision 1.62  2004/07/04 12:10:52  corrados
made faac compliant with Digital Radio Mondiale (DRM) (DRM macro must be set)
implemented HCR tool, VCB11, CRC, scalable bitstream order
note: VCB11 only uses codebook 11! TODO: implement codebooks 16-32
960 transform length is not yet implemented (TODO)! Use 1024 for encoding and 960 for decoding, resulting in a lot of artefacts

Revision 1.61  2004/05/03 11:37:16  danchr
bump version to unstable 1.24+

Revision 1.60  2004/04/13 13:47:33  danchr
clarify release <> unstable status

Revision 1.59  2004/04/02 14:56:17  danchr
fix name clash w/ libavcodec: fft_init -> fft_initialize
bump version number to 1.24 beta

Revision 1.58  2004/03/17 13:34:20  danchr
Automatic, untuned setting of lowpass for VBR.

Revision 1.57  2004/03/15 20:16:42  knik
fixed copyright notice

Revision 1.56  2004/01/23 10:22:26  stux
 *** empty FAAC_log message ***

Revision 1.55  2003/12/17 20:59:55  knik
changed default cutoff to 16k

Revision 1.54  2003/11/24 18:09:12  knik
A safe version of faacEncGetVersion() without string length problem.
Removed Stux from copyright notice. I don't think he contributed something very
substantial to faac and this is not the right place to list all contributors.

Revision 1.53  2003/11/16 05:02:52  stux
moved global tables from fft.c into hEncoder FFT_Tables. Add fft_init and fft_terminate, flowed through all necessary changes. This should remove at least one instance of a memory leak, and fix some thread-safety problems. Version update to 1.23.3

Revision 1.52  2003/11/15 08:13:42  stux
added FaacEncGetVersion(), version 1.23.2, added myself to faacCopyright :-P, does vanity know no bound ;)

Revision 1.51  2003/11/10 17:48:00  knik
Allowed independent bitRate and bandWidth setting.
Small fixes.

Revision 1.50  2003/10/29 10:31:25  stux
Added channel_map to FaacEncHandle, facilitates free generalised channel remapping in the faac core. Default is straight-through, should be *zero* performance hit... and even probably an immeasurable performance gain, updated FAAC_CFG_VERSION to 104 and FAAC_VERSION to 1.22.0

Revision 1.49  2003/10/12 16:43:39  knik
average bitrate control made more stable

Revision 1.48  2003/10/12 14:29:53  knik
more accurate average bitrate control

Revision 1.47  2003/09/24 16:26:54  knik
faacEncStruct: quantizer specific data enclosed in AACQuantCfg structure.
Added config option to enforce block type.

Revision 1.46  2003/09/07 16:48:31  knik
Updated psymodel call. Updated bitrate/cutoff mapping table.

Revision 1.45  2003/08/23 15:02:13  knik
last frame moved back to the library

Revision 1.44  2003/08/15 11:42:08  knik
removed single silent flush frame

Revision 1.43  2003/08/11 09:43:47  menno
thread safety, some tables added to the encoder context

Revision 1.42  2003/08/09 11:39:30  knik
LFE support enabled by default

Revision 1.41  2003/08/08 10:02:09  menno
Small fix

Revision 1.40  2003/08/07 08:17:00  knik
Better LFE support (reduced bandwidth)

Revision 1.39  2003/08/02 11:32:10  stux
added config.inputFormat, and associated defines and code, faac now handles native endian 16bit, 24bit and float input. Added faacEncGetDecoderSpecificInfo to the dll exports, needed for MP4. Updated DLL .dsp to compile without error. Updated CFG_VERSION to 102. Version number might need to be updated as the API has technically changed. Did not update libfaac.pdf

Revision 1.38  2003/07/10 19:17:01  knik
24-bit input

Revision 1.37  2003/06/26 19:20:09  knik
Mid/Side support.
Copyright info moved from frontend.
Fixed memory leak.

Revision 1.36  2003/05/12 17:53:16  knik
updated ABR table

Revision 1.35  2003/05/10 09:39:55  knik
added approximate ABR setting
modified default cutoff

Revision 1.34  2003/05/01 09:31:39  knik
removed ISO psyodel
disabled m/s coding
fixed default bandwidth
reduced max_sfb check

Revision 1.33  2003/04/13 08:37:23  knik
version number moved to version.h

Revision 1.32  2003/03/27 17:08:23  knik
added quantizer quality and bandwidth setting

Revision 1.31  2002/10/11 18:00:15  menno
small bugfix

Revision 1.30  2002/10/08 18:53:01  menno
Fixed some memory leakage

Revision 1.29  2002/08/19 16:34:43  knik
added one additional flush frame
fixed sample buffer memory allocation

 */
