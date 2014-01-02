/*
 * FAAC - Freeware Advanced Audio Coder
 * Copyright (C) 2002 Krzysztof Nikiel
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
 * $Id: psychkni.c,v 1.17 2005/04/24 19:16:14 rjamorim Exp $
 */
 
//#include <stdio.h>
//#include <stdlib.h>
//#include <math.h>

#include "shaveMatopts.h"
#include "psych.h"
#include "coder.h"
#include "fft.h"
#include "util.h"
#include "frame.h"

typedef float psyfloat;

typedef struct
{
  /* bandwidth */
  int bandS;
  int lastband;

  /* SFB energy */
  psyfloat *fftEnrgS[8];
  psyfloat *fftEnrgNextS[8];
  psyfloat *fftEnrgNext2S[8];
  psyfloat *fftEnrgPrevS[8];
}
psydata_t;


static void Hann(GlobalPsyInfo * gpsyInfo, float *inSamples, int size)
{
  int i;

  //~~~~~~~~~~~~~~~
  //Warning !!!
  //~~~~~~~~~~~~~~~

  //andreil: can configure to use just a bit of HannWindow???
  //         I see encoder only uses first 256 entries !!!
  
  //if(size != 256)
  //  printf("__size = %d__\n", size);


  /* Applying Hann window */
  if (size == BLOCK_LEN_LONG * 2)
  {
    
    for (i = 0; i < size; i++)
      inSamples[i] *= gpsyInfo->hannWindow[i];
  }
  else
  {
    for (i = 0; i < size; i++)
      inSamples[i] *= gpsyInfo->hannWindowS[i];
  }
}

static void PsyCheckShort(PsyInfo * psyInfo)
{
  float totvol = 0.0;
  float totchg, totchg2;
  psydata_t *psydata = psyInfo->data;
  int lastband = psydata->lastband;
  int firstband = 1;
  int sfb;

  /* long/short block switch */
  totchg = totchg2 = 0.0;
  for (sfb = 0; sfb < lastband; sfb++)
  {
    int win;
    float volb[16];
    float vavg[13];
    float maxdif = 0.0;
    float totmaxdif = 0.0;
    float e, v;
    
    // previous frame
    for (win = 0; win < 4; win++)
    {
      e = psydata->fftEnrgPrevS[win + 4][sfb];

      volb[win] = FAAC_sqrt(e);
      totvol += e;
    }

    // current frame
    for (win = 0; win < 8; win++)
      {
      e = psydata->fftEnrgS[win][sfb];

      volb[win + 4] = FAAC_sqrt(e);
      totvol += e;
    }
    // next frame
    for (win = 0; win < 4; win++)
    {
      e = psydata->fftEnrgNextS[win][sfb];

      volb[win + 12] = FAAC_sqrt(e);
    totvol += e;
    }
    
    // ignore lowest SFBs
    if (sfb < firstband)
      continue;

    v = 0.0;
    for (win = 0; win < 4; win++)
    {
      v += volb[win];
    }
    vavg[0] = 0.25 * v;

    for (win = 1; win < 13; win++)
      {
      v -= volb[win - 1];
      v += volb[win + 3];
      vavg[win] = 0.25 * v;
    }

    for (win = 0; win < 8; win++)
    {
      int i;
      float mina, maxv;
      float voldif;
      float totvoldif;

      mina = vavg[win];
      for (i = 1; i < 5; i++)
        mina = min(mina, vavg[win + i]);

      maxv = volb[win + 2];//vectoru asta ajunge cu nan-uri
      for (i = 3; i < 6; i++)
        maxv = max(maxv, volb[win + i]);

      if (!maxv || !mina)
        continue;

      voldif = (maxv - mina) / mina;
      totvoldif = (maxv - mina) * (maxv - mina);

	if (voldif > maxdif)
	  maxdif = voldif;

	if (totvoldif > totmaxdif)
	  totmaxdif = totvoldif;
      }//for (win = 0; win < 8; win++)
    totchg += maxdif;
    totchg2 += totmaxdif;
  }

  totvol = FAAC_sqrt(totvol);

  totchg2 = FAAC_sqrt(totchg2);

  totchg = totchg / lastband;
  if (totvol)
    totchg2 /= totvol;
  else
    totchg2 = 0.0;
  psyInfo->block_type = ((totchg > 1.0) && (totchg2 > 0.04))
    ? ONLY_SHORT_WINDOW : ONLY_LONG_WINDOW;
}


//andreil: 
//float g_HANN_WND_LARGE[BLOCK_LEN_LONG  * 2];
//float g_HANN_WND_SHORT[BLOCK_LEN_SHORT * 2];




//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define MAX_CHANNS 2

psyfloat g_fftEnrgPrevS [MAX_CHANNS][8][NSFB_SHORT];
psyfloat g_fftEnrgS     [MAX_CHANNS][8][NSFB_SHORT];
psyfloat g_fftEnrgNextS [MAX_CHANNS][8][NSFB_SHORT];
psyfloat g_fftEnrgNext2S[MAX_CHANNS][8][NSFB_SHORT];


//extern void DUMP_f32_tbl(float *tab, int len, char *tab_name, char *file_name);
#include "hannWindow.hh"
#include "hannWindowS.hh"

short     g_prevSamples[2][BLOCK_LEN_LONG];
psydata_t g_psydata[2];

//###########################################################
static void PsyInit(GlobalPsyInfo * gpsyInfo, PsyInfo * psyInfo, unsigned int numChannels,
		    unsigned int sampleRate, 
			//int *cb_width_long, int num_cb_long,
			char *cb_width_long, char num_cb_long,
		    //int *cb_width_short, int num_cb_short)
			char *cb_width_short, char num_cb_short)
{
  unsigned int channel;
  int i, j, size;

  gpsyInfo->hannWindow  = (float*)hannWindow;
  gpsyInfo->hannWindowS = (float*)hannWindowS;

  /*
  //andreil WARNING: check this buff allocation !!!
  //gpsyInfo->hannWindow =   (float *) AllocMemory(2 * BLOCK_LEN_LONG * sizeof(float),  "gpsyInfo->hannWindow");
  gpsyInfo->hannWindow =   (float *) AllocMemory(256 * sizeof(float),  "gpsyInfo->hannWindow");


  gpsyInfo->hannWindowS =  (float *) AllocMemory(2 * BLOCK_LEN_SHORT * sizeof(float), "gpsyInfo->hannWindowS");

//#error  I guess this is called two times and faac_malloc reports 2x the actual mem requirement\
//        as I con't consider the frees; update also frees.

  //gpsyInfo->hannWindow  = g_HANN_WND_LARGE;
  //gpsyInfo->hannWindowS = g_HANN_WND_SHORT;

 //andreil: check for the size that's really needed !!!!
  //for (i = 0; i < BLOCK_LEN_LONG * 2; i++)
  for (i = 0; i < 256; i++)//????????
    gpsyInfo->hannWindow[i] = 0.5 * (1 - cos(2.0 * M_PI * (i + 0.5) / (BLOCK_LEN_LONG * 2)));

  //DUMP_f16_tbl(gpsyInfo->hannWindow, "hannWindow", "f16_hannWindow.hh");

  for (i = 0; i < BLOCK_LEN_SHORT * 2; i++)
    gpsyInfo->hannWindowS[i] = 0.5 * (1 - cos(2.0 * M_PI * (i + 0.5) / (BLOCK_LEN_SHORT * 2)));

  DUMP_f32_tbl(gpsyInfo->hannWindow,  256, "hannWindow", "hannWindow.hh");
  DUMP_f32_tbl(gpsyInfo->hannWindowS, 256, "hannWindowS", "hannWindowS.hh");
  */

  gpsyInfo->sampleRate = (float) sampleRate;

  for (channel = 0; channel < numChannels; channel++)
  {
    //psydata_t *psydata; = AllocMemory(sizeof(psydata_t), "psydata");
    psyInfo[channel].data = (void*)&g_psydata[channel];
  }

  size = BLOCK_LEN_LONG;
  for (channel = 0; channel < numChannels; channel++)
  {
    psyInfo[channel].size = size;

    //psyInfo[channel].prevSamples =  (float *) AllocMemory(size * sizeof(float), "psyInfo[channel].prevSamples");
    //memset(psyInfo[channel].prevSamples, 0, size * sizeof(float));

	//psyInfo[channel].prevSamples =  (short *) AllocMemory(size * sizeof(short), "psyInfo[channel].prevSamples");
    psyInfo[channel].prevSamples = g_prevSamples[channel];//andreil:static alloc
    memset(psyInfo[channel].prevSamples, 0, size * sizeof(short));
  }




  size = BLOCK_LEN_SHORT;
  for (channel = 0; channel < numChannels; channel++)
  {
    psydata_t *psydata = psyInfo[channel].data;

    psyInfo[channel].sizeS = size;

    //psyInfo[channel].prevSamplesS = (float *) AllocMemory(size * sizeof(float), "psyInfo[channel].prevSamplesS");
    //memset(psyInfo[channel].prevSamplesS, 0, size * sizeof(float));

	//psyInfo[channel].prevSamplesS = (short *) AllocMemory(size * sizeof(short), "psyInfo[channel].prevSamplesS");
    //memset(psyInfo[channel].prevSamplesS, 0, size * sizeof(short));



	//andreil: pentru fiecare din cele 8 SHORT_WINDOWs !!!
    for (j = 0; j < 8; j++)
    {
      //psydata->fftEnrgPrevS[j] = (psyfloat *) AllocMemory(NSFB_SHORT * sizeof(psyfloat), "psydata->fftEnrgPrevS[j]");
	  psydata->fftEnrgPrevS[j] = g_fftEnrgPrevS[channel][j];//andreil: static alloc
      memset(psydata->fftEnrgPrevS[j], 0, NSFB_SHORT * sizeof(psyfloat));

      //psydata->fftEnrgS[j] =  (psyfloat *) AllocMemory(NSFB_SHORT * sizeof(psyfloat), "psydata->fftEnrgS[j]");
	  psydata->fftEnrgS[j] = g_fftEnrgS[channel][j]; //andreil: static alloc
      memset(psydata->fftEnrgS[j], 0, NSFB_SHORT * sizeof(psyfloat));

      //psydata->fftEnrgNextS[j] = (psyfloat *) AllocMemory(NSFB_SHORT * sizeof(psyfloat), "psydata->fftEnrgNextS[j]");
	  psydata->fftEnrgNextS[j] = g_fftEnrgNextS[channel][j]; //andreil: static alloc
      memset(psydata->fftEnrgNextS[j], 0, NSFB_SHORT * sizeof(psyfloat));

      //psydata->fftEnrgNext2S[j] =(psyfloat *) AllocMemory(NSFB_SHORT * sizeof(psyfloat), "psydata->fftEnrgNext2S[j]");
      psydata->fftEnrgNext2S[j] = g_fftEnrgNext2S[channel][j];
      memset(psydata->fftEnrgNext2S[j], 0, NSFB_SHORT * sizeof(psyfloat));
    }
  }
}

static void PsyEnd(GlobalPsyInfo * gpsyInfo, PsyInfo * psyInfo, unsigned int numChannels)
{
  unsigned int channel;
  int j;

  //if (gpsyInfo->hannWindow)    FreeMemory(gpsyInfo->hannWindow);
  //if (gpsyInfo->hannWindowS)    FreeMemory(gpsyInfo->hannWindowS);

  //for (channel = 0; channel < numChannels; channel++)
  //{
  //  if (psyInfo[channel].prevSamples)
  //    FreeMemory(psyInfo[channel].prevSamples);
  //}

  //for (channel = 0; channel < numChannels; channel++)
  //{
  //  psydata_t *psydata = psyInfo[channel].data;

  //  if (psyInfo[channel].prevSamplesS)
  //    FreeMemory(psyInfo[channel].prevSamplesS);
  //  for (j = 0; j < 8; j++)
  //  {
  //    if (psydata->fftEnrgPrevS[j])	FreeMemory(psydata->fftEnrgPrevS[j]);
  //    if (psydata->fftEnrgS[j])   	FreeMemory(psydata->fftEnrgS[j]);
  //    if (psydata->fftEnrgNextS[j]) FreeMemory(psydata->fftEnrgNextS[j]);
  //    if (psydata->fftEnrgNext2S[j])FreeMemory(psydata->fftEnrgNext2S[j]);
  //  }
  //}

  //for (channel = 0; channel < numChannels; channel++)
  //{
  //  if (psyInfo[channel].data)
  //    FreeMemory(psyInfo[channel].data);
  //}
}

/* Do psychoacoustical analysis */
static void PsyCalculate(ChannelInfo * channelInfo, GlobalPsyInfo * gpsyInfo,
			 PsyInfo * psyInfo, 
			 //int *cb_width_long,  int num_cb_long, 
			 //int *cb_width_short, int num_cb_short, 
			 char *cb_width_long,  char num_cb_long, 
			 char *cb_width_short, char num_cb_short, 
			 unsigned int numChannels)
{
  unsigned int channel;

  for (channel = 0; channel < numChannels; channel++)
  {
    if (channelInfo[channel].present)
    {

      if (channelInfo[channel].cpe &&
	  channelInfo[channel].ch_is_left)
      {				/* CPE */

	int leftChan = channel;
	int rightChan = channelInfo[channel].paired_ch;

	PsyCheckShort(&psyInfo[leftChan]);
	PsyCheckShort(&psyInfo[rightChan]);
      }
      else if (!channelInfo[channel].cpe &&
	       channelInfo[channel].lfe)
      {				/* LFE */
        // Only set block type and it should be OK
	psyInfo[channel].block_type = ONLY_LONG_WINDOW;
      }
      else if (!channelInfo[channel].cpe)
      {				/* SCE */
	PsyCheckShort(&psyInfo[channel]);
      }
    }
  }
}


extern unsigned char g_WBUFF[1024*16];

static void PsyBufferUpdate( FFT_Tables *fft_tables, GlobalPsyInfo * gpsyInfo, PsyInfo * psyInfo,
			    //float *newSamples, 
				short *newSamples,
				unsigned int bandwidth,
			    //int *cb_width_short, int num_cb_short)
				char *cb_width_short, char num_cb_short)
{
  int win;
  //float transBuff[2 * BLOCK_LEN_LONG];   //8KB buffer !!! huge 
  //float transBuffS[2 * BLOCK_LEN_SHORT]; //1KB buffer !!!

  float *transBuff;
  float *transBuffS;

  psydata_t *psydata = psyInfo->data;
  psyfloat *tmp;
  int sfb, j;

  //andreil: map volatile transBuff buffers onto g_WBUFF !!!
  //  can use it now as freqBuff usage starts at a later step...
  //  and lifetime of transBuff is only in this func !!!
  transBuff  = (float*)&g_WBUFF[0];
  transBuffS = (float*)&g_WBUFF[8192];

  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //   printf("look at input samples at %x", newSamples);
  //   SHAVE_HALT;
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

  psydata->bandS = psyInfo->sizeS * bandwidth * 2 / gpsyInfo->sampleRate;

  //andreil: de copiat din PCM si facut aici conversia o singura data !!!
  //memcpy(transBuff, psyInfo->prevSamples, psyInfo->size * sizeof(float));
  for(j=0; j<psyInfo->size; j++)
   transBuff[j] = psyInfo->prevSamples[j];

  //memcpy(transBuff, psyInfo->prevSamples, psyInfo->size * sizeof(float));


  //memcpy(transBuff + psyInfo->size, newSamples, psyInfo->size * sizeof(float));
  for(j=0;j<psyInfo->size;j++)
	  transBuff[psyInfo->size+j] = newSamples[j];


  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //  printf("PsyBufferUpdate 2");
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      
  for (win = 0; win < 8; win++)
  {
    int first = 0;
    int last = 0;

    memcpy(transBuffS, transBuff + (win * BLOCK_LEN_SHORT) + (BLOCK_LEN_LONG - BLOCK_LEN_SHORT) / 2,
	   2 * psyInfo->sizeS * sizeof(float));

    Hann(gpsyInfo, transBuffS, 2 * psyInfo->sizeS);
    //printf("after Hann !!!");
    rfft( fft_tables, transBuffS, 8 /*andrei: 'logm' value*/);

   // shift bufs
    tmp = psydata->fftEnrgPrevS[win];
    psydata->fftEnrgPrevS[win]  = psydata->fftEnrgS[win];
    psydata->fftEnrgS[win]      = psydata->fftEnrgNextS[win];
    psydata->fftEnrgNextS[win]  = psydata->fftEnrgNext2S[win];
    psydata->fftEnrgNext2S[win] = tmp;

    for (sfb = 0; sfb < num_cb_short; sfb++)
    {
      float e;
      int l;

      first = last;
      last = first + cb_width_short[sfb];

      if (first < 1)
	first = 1;

      //if (last > psydata->bandS) // band out of range
      if (first >= psydata->bandS) // band out of range
	break;

      e = 0.0;
      for (l = first; l < last; l++)
      {
	float a = transBuffS[l];
	float b = transBuffS[l + psyInfo->sizeS];

	e += a * a + b * b;
      }

      psydata->fftEnrgNext2S[win][sfb] = e;
    }
    psydata->lastband = sfb;
    for (; sfb < num_cb_short; sfb++)
    {
      psydata->fftEnrgNext2S[win][sfb] = 0;
    }
  }

  
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  //  printf("PsyBufferUpdate see: 0x%x\n", psydata->fftEnrgNext2S);
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  
  // for (win = 0; win < 8; win++)
  // {
	// printf("see 60B at %x\n",psydata->fftEnrgNext2S[win]);
  // }
    
  
  //andreil: prevSamples persistent 100%
  memcpy(psyInfo->prevSamples, newSamples, psyInfo->size * sizeof(short));
  //for(j=0;j<psyInfo->size; j++)
  //  psyInfo->prevSamples[j] = newSamples[j];
  
  // //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // printf("Look at %x!", g_fftEnrgPrevS);
    // SHAVE_HALT;
  // //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  
}




static void BlockSwitch(CoderInfo * coderInfo, PsyInfo * psyInfo, unsigned int numChannels)
{
  unsigned int channel;
  int desire = ONLY_LONG_WINDOW;

  /* Use the same block type for all channels
     If there is 1 channel that wants a short block,
     use a short block on all channels.
   */
  for (channel = 0; channel < numChannels; channel++)
  {
    if (psyInfo[channel].block_type == ONLY_SHORT_WINDOW)
      desire = ONLY_SHORT_WINDOW;
  }

  for (channel = 0; channel < numChannels; channel++)
  {
    int lasttype = coderInfo[channel].block_type;

    if (desire == ONLY_SHORT_WINDOW
	|| coderInfo[channel].desired_block_type == ONLY_SHORT_WINDOW)
    {
      if (lasttype == ONLY_LONG_WINDOW || lasttype == SHORT_LONG_WINDOW)
	coderInfo[channel].block_type = LONG_SHORT_WINDOW;
      else
	coderInfo[channel].block_type = ONLY_SHORT_WINDOW;
    }
    else
    {
      if (lasttype == ONLY_SHORT_WINDOW || lasttype == LONG_SHORT_WINDOW)
	coderInfo[channel].block_type = SHORT_LONG_WINDOW;
      else
	coderInfo[channel].block_type = ONLY_LONG_WINDOW;
    }
    coderInfo[channel].desired_block_type = desire;
  }
}

psymodel_t psymodel2 =
{
  PsyInit,
  PsyEnd,
  PsyCalculate,
  PsyBufferUpdate,
  BlockSwitch
};
