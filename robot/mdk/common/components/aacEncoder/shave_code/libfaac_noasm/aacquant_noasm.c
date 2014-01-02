/*
 * FAAC - Freeware Advanced Audio Coder
 * Copyright (C) 2001 Menno Bakker
 * Copyright (C) 2002, 2003 Krzysztof Nikiel
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
 * $Id: aacquant.c,v 1.32 2008/03/23 23:00:25 menno Exp $
 */

//#include <math.h>
//#include <stdlib.h>

#include "shaveMatopts.h"

#include "frame.h"
#include "aacquant.h"
#include "coder.h"
#include "huffman.h"
#include "psych.h"
#include "util.h"

#define TAKEHIRO_IEEE754_HACK 1

#define XRPOW_FTOI(src,dest) ((dest) = (int)(src))
//#define QUANTFAC(rx)  adj43[rx]
#define ROUNDFAC 0.4054


static void FixNoise(
            short *sfb_offset,
            short *scale_factor,
            int nr_of_sfb,
            
		    const float *xr,
		    float *xr_pow,
		    int *xi,
		    float *xmin
		    //float *pow43,
		    //float *adj43
            );



static void CalcAllowedDist(CoderInfo *coderInfo, PsyInfo *psyInfo,
			    float *xr, float *xmin, int quality);


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
static inline float aacquantCfg_pow43(int i)
{
	if(i==0) return 0.0f;
	return FAAC_pow((float)i, 4.0/3.0);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//andreil: pe Sabre... FAAC_pow ^ 0.75 trebuie sa dea < 65536.. deci ar trebui sa mearga...
//         de spart iarasi in puteri ale lui 2....
/*
float aacquantCfg_adj43(int i)
{
	if(i==0) return 0.0f;
	return (i - 0.5 - FAAC_pow(0.5 * (aacquantCfg_pow43(i - 1) + aacquantCfg_pow43(i)), 0.75));
}
*/
float lookup_adj43[] = 
{
  0,
 -0.094604,
 -0.027988,
 -0.016711,
 -0.011921,
 -0.009267,
 -0.007580,
 -0.006413,
 -0.005557,
 -0.004903,
  -0.004387,
  -0.003969,
  -0.003624,
  -0.003334,
  -0.003087,
  -0.002874,
  -0.002688,
  -0.002526,
  -0.002381,
  -0.002252,
  -0.002137,
  -0.002033,
  -0.001938,
  -0.001852,
  -0.001774,
  -0.001701,
  -0.001633,
  -0.001572,
  -0.001515,
  -0.001462,
  -0.001412,
  -0.001365
};

#if 0
static inline float aacquantCfg_adj43(int i)
{
	//if(i==0) return 0.0f;
	if(i<32)
		return lookup_adj43[i];
	else
	    return (i - 0.5 - FAAC_pow(0.5 * (aacquantCfg_pow43(i - 1) + aacquantCfg_pow43(i)), 0.75));
}
#endif


typedef union {
    float f;
    int i;
} fi_union;

#define MAGIC_FLOAT (65536*(128))
#define MAGIC_INT 0x4b000000

static void QuantizeBand(const float *xp, int *pi, float istep,
             int offset, int end/*, float *adj43*/)
{
  int j;
  fi_union *fi;

  fi = (fi_union *)pi;
  for (j = offset; j < end; j++)
  {
    float x0 = istep * xp[j];

    x0 += MAGIC_FLOAT; fi[j].f = (float)x0;
    //andreil: "adj43" array is allways addressed at location 0, which is zero anyway

    //andreil: change this addressing with a more clear one:
    //fi[j].f = x0 + (adj43 - MAGIC_INT)[fi[j].i];

    //andreil: more clear:
    //fi[j].f = x0 + adj43[fi[j].i-MAGIC_INT];

    //andreil: online compute:
    //fi[j].f = x0 + aacquantCfg_adj43(fi[j].i-MAGIC_INT);

    //andreil: faster
    fi[j].f = x0 + lookup_adj43[fi[j].i-MAGIC_INT];

    fi[j].i -= MAGIC_INT;
  }
}

//void AACQuantizeInit(CoderInfo *coderInfo, unsigned int numChannels,
//		     AACQuantCfg *aacquantCfg)
//{
//    unsigned int channel, i;
//
//	//andreil: do online computation:
//    //aacquantCfg->pow43 = (float*)AllocMemory(PRECALC_SIZE*sizeof(float), "aacquantCfg->pow43");
//    //aacquantCfg->adj43 = (float*)AllocMemory(PRECALC_SIZE*sizeof(float), "aacquantCfg->adj43");
//
//    //aacquantCfg->pow43[0] = 0.0;
//    //for(i=1;i<PRECALC_SIZE;i++)
//    //    aacquantCfg->pow43[i] = FAAC_pow((float)i, 4.0/3.0);
//
//#if TAKEHIRO_IEEE754_HACK
//    //aacquantCfg->adj43[0] = 0.0;
//    //for (i = 1; i < PRECALC_SIZE; i++)
//    //  aacquantCfg->adj43[i] = i - 0.5 - FAAC_pow(0.5 * (aacquantCfg_pow43(i - 1) + aacquantCfg_pow43(i)),0.75);
//#else // !TAKEHIRO_IEEE754_HACK
//    for (i = 0; i < PRECALC_SIZE-1; i++)
//        aacquantCfg->adj43[i] = (i + 1) - FAAC_pow(0.5 * (aacquantCfg->pow43[i] + aacquantCfg->pow43[i + 1]), 0.75);
//    aacquantCfg->adj43[i] = 0.5;
//#endif
//
//	//andreil: "requantFreq" doesn't seem to be used at all in current config
//    //for (channel = 0; channel < numChannels; channel++) {
//    //    coderInfo[channel].requantFreq = (float*)AllocMemory(BLOCK_LEN_LONG*sizeof(float), "coderInfo[channel].requantFreq");
//    //}
//}

//void AACQuantizeEnd(CoderInfo *coderInfo, unsigned int numChannels,
//		   AACQuantCfg *aacquantCfg)
//{
//    unsigned int channel;
//
//    if (aacquantCfg->pow43)
//	{
//      FreeMemory(aacquantCfg->pow43);
//      aacquantCfg->pow43 = NULL;
//	}
//    if (aacquantCfg->adj43)
//	{
//      FreeMemory(aacquantCfg->adj43);
//      aacquantCfg->adj43 = NULL;
//	}
//
//    for (channel = 0; channel < numChannels; channel++) {
//        if (coderInfo[channel].requantFreq) FreeMemory(coderInfo[channel].requantFreq);
//    }
//}

static void BalanceEnergy(CoderInfo *coderInfo,
			  const float *xr, const int *xi,
			  float *pow43)
{
  const float ifqstep   = 1.1892071;//FAAC_pow(2.0, 0.25);//constant... can replace it 
  const float logstep_1 = 5.7707810;//1.0 / FAAC_log(ifqstep);

  int sb;
  int nsfb = coderInfo->nr_of_sfb;
  int start, end;
  int l;
  float en0, enq;
  int shift;

  for (sb = 0; sb < nsfb; sb++)
  {
    float qfac_1;

    start = coderInfo->sfb_offset[sb];
    end   = coderInfo->sfb_offset[sb+1];

    qfac_1 = FAAC_pow(2.0, -0.25*(coderInfo->scale_factor[sb] - coderInfo->global_gain));

    en0 = 0.0;
    enq = 0.0;
    for (l = start; l < end; l++)
    {
      float xq;

      if (!sb && !xi[l])
	continue;

      //xq = pow43[xi[l]];
	  xq = aacquantCfg_pow43(xi[l]);

      en0 += xr[l] * xr[l];
      enq += xq * xq;
    }

    if (enq == 0.0)
      continue;

    enq *= qfac_1 * qfac_1;
//continue;
    shift = (int)(FAAC_log(FAAC_sqrt(enq / en0)) * logstep_1 + 1000.5);
//continue;	
    shift -= 1000;

    shift += coderInfo->scale_factor[sb];
    coderInfo->scale_factor[sb] = shift;
  }
}

int AACQuantize(CoderInfo *coderInfo,
                PsyInfo *psyInfo,
                ChannelInfo *channelInfo,
                //int *cb_width,
                //int num_cb,
				char *cb_width,
                char num_cb,
                float *xr,
		AACQuantCfg *aacquantCfg)
{
    int sb, i, do_q = 0;
    int bits = 0, sign;
    float xr_pow[FRAME_LEN];
    float xmin[MAX_SCFAC_BANDS];
    int xi[FRAME_LEN];

    /* Use local copy's */
    //int *scale_factor = coderInfo->scale_factor;
	short *scale_factor = coderInfo->scale_factor;

    /* Set all scalefactors to 0 */
    coderInfo->global_gain = 0;
    for (sb = 0; sb < coderInfo->nr_of_sfb; sb++)
        scale_factor[sb] = 0;

     //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
     // printf("AACQuantize: look at input xr: %x", xr); //this contains NANs !!!!  
     // SHAVE_HALT;
     //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        
    /* Compute xr_pow */
    for (i = 0; i < FRAME_LEN; i++) {
        float temp = fabs(xr[i]);
        xr_pow[i] = FAAC_sqrt(temp * FAAC_sqrt(temp));
        do_q += (temp > 1E-20);
    }
    
    // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // printf("xr     (4096B) at : %x\n", xr);
        // printf("xr_pow (4096B) at : %x\n", xr_pow);
        // //SHAVE_HALT;
    // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    

    if (do_q) {
        CalcAllowedDist(coderInfo, psyInfo, xr, xmin, aacquantCfg->quality);
         // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
           // static int call=0;
           // printf("look 512B at xmin @ %x\n", xmin);
           // call++;
           // SHAVE_HALT;
           // //andreil: seamana destul de mult, e ok !!!!
        // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    
	    coderInfo->global_gain = 0;
        
        
        
        
        // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
            // printf("before fixNoise xr_pow(4096)  at  %x \n", xr_pow);
            // printf("before fixNoise xmin   (512)  at  %x \n", xmin);
            // //SHAVE_HALT;
        // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        FixNoise(coderInfo->sfb_offset,
                 coderInfo->scale_factor,
                 coderInfo->nr_of_sfb,
                 xr, 
                 xr_pow, 
                 xi, 
                 xmin);//, aacquantCfg->pow43, aacquantCfg->adj43);
        
        //return 0;
        // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
           // printf("after fixNoise coderInfo->scale_factor(256)   =  %x \n", coderInfo->scale_factor);
           // printf("after fixNoise xi                             =  %x \n", xi);
           // SHAVE_HALT;
        // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        
        
        
        
        BalanceEnergy(coderInfo, xr, xi, aacquantCfg->pow43);
		//return 0;		
        //UpdateRequant(coderInfo, xi,     aacquantCfg->pow43);
        for ( i = 0; i < FRAME_LEN; i++ )  {
            sign = (xr[i] < 0) ? -1 : 1;
            xi[i] *= sign;
            //coderInfo->requantFreq[i] *= sign;
        }
    } else {
        coderInfo->global_gain = 0;
        SetMemory(xi, 0, FRAME_LEN*sizeof(int));
    }
    //return 0;
    // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
       // static int call=0;
       // printf("look 4096 at xi @ %x\n", xi);
       // call++;
       // SHAVE_HALT;
    // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    BitSearch(coderInfo, xi);
    
    
    // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
     // printf("Check xi at                     %x \n", xi);
     // printf("Check coderInfo->book_vector at %x \n", coderInfo->book_vector);
     // SHAVE_HALT;
    // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    
    
    
    // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        // printf("before coderInfo->global_gain    =  %x \n", coderInfo->global_gain);
        // printf("before coderInfo->book_vector    =  %x \n", coderInfo->book_vector);
        // printf("before coderInfo->scale_factor   =  %x \n", coderInfo->scale_factor);
        // SHAVE_HALT;
    // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    

    /* offset the difference of common_scalefac and scalefactors by SF_OFFSET  */
    for (i = 0; i < coderInfo->nr_of_sfb; i++) {
        if ((coderInfo->book_vector[i]!=INTENSITY_HCB)&&(coderInfo->book_vector[i]!=INTENSITY_HCB2)) {
            scale_factor[i] = coderInfo->global_gain - scale_factor[i] + SF_OFFSET;
        }
    }
    coderInfo->global_gain = scale_factor[0];
    
    // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    // printf("Check coderInfo->scale_factor at  %x \n", coderInfo->scale_factor);
    // printf("Check coderInfo->global_gain   =  %x \n", coderInfo->global_gain);
    // SHAVE_HALT;
    // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    
    
#if 0
	printf("global gain: %d\n", coderInfo->global_gain);
	for (i = 0; i < coderInfo->nr_of_sfb; i++)
	  printf("sf %d: %d\n", i, coderInfo->scale_factor[i]);
#endif
    // clamp to valid diff range
    {
      int previous_scale_factor = coderInfo->global_gain;
      int previous_is_factor = 0;
      for (i = 0; i < coderInfo->nr_of_sfb; i++) {
        if ((coderInfo->book_vector[i]==INTENSITY_HCB) ||
            (coderInfo->book_vector[i]==INTENSITY_HCB2)) {
            const int diff = scale_factor[i] - previous_is_factor;
            if (diff < -60) scale_factor[i] = previous_is_factor - 60;
            else if (diff > 59) scale_factor[i] = previous_is_factor + 59;
            previous_is_factor = scale_factor[i];
//            printf("sf %d: %d diff=%d **\n", i, coderInfo->scale_factor[i], diff);
        } else if (coderInfo->book_vector[i]) {
            const int diff = scale_factor[i] - previous_scale_factor;
            if (diff < -60) scale_factor[i] = previous_scale_factor - 60;
            else if (diff > 59) scale_factor[i] = previous_scale_factor + 59;
            previous_scale_factor = scale_factor[i];
//            printf("sf %d: %d diff=%d\n", i, coderInfo->scale_factor[i], diff);
        }
      }
    }
    
    // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
     // printf("Check coderInfo->scale_factor %x \n", coderInfo->scale_factor);
     // SHAVE_HALT;
    // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    

    /* place the codewords and their respective lengths in arrays data[] and len[] respectively */
    /* there are 'counter' elements in each array, and these are variable length arrays depending on the input */
#ifdef DRM
    coderInfo->iLenReordSpData = 0; /* init length of reordered spectral data */
    coderInfo->iLenLongestCW = 0; /* init length of longest codeword */
    coderInfo->cur_cw = 0; /* init codeword counter */
#endif
    coderInfo->spectral_count = 0;
    sb = 0;

	//andreil: just check coderInfo->data persistency:
	//memset(coderInfo->data, 0, 4096);
	//memset(coderInfo->len,  0, 2048);

    for(i = 0; i < coderInfo->nr_of_sfb; i++) {
        OutputBits(
            coderInfo,
#ifdef DRM
            &coderInfo->book_vector[i], /* needed for VCB11 */
#else
            coderInfo->book_vector[i],
#endif
            xi,
            coderInfo->sfb_offset[i],
            coderInfo->sfb_offset[i+1]-coderInfo->sfb_offset[i]);

        if (coderInfo->book_vector[i])
              sb = i;
    }

    // FIXME: Check those max_sfb/nr_of_sfb. Isn't it the same?
    coderInfo->max_sfb = coderInfo->nr_of_sfb = sb + 1;

    return bits;
}


static void CalcAllowedDist(CoderInfo *coderInfo, PsyInfo *psyInfo,
                            float *xr, float *xmin, int quality)
{
  int sfb, start, end, l;
  const float globalthr = 132.0 / (float)quality;
  int last = coderInfo->lastx;
  int lastsb = 0;

  //andreil:
  //int *cb_offset = coderInfo->sfb_offset;
  short *cb_offset = coderInfo->sfb_offset;

  int num_cb = coderInfo->nr_of_sfb;
  float avgenrg = coderInfo->avgenrg;

  for (sfb = 0; sfb < num_cb; sfb++)
  {
    if (last > cb_offset[sfb])
      lastsb = sfb;
  }

  for (sfb = 0; sfb < num_cb; sfb++)
  {
    float thr, tmp;
    float enrg = 0.0;

    start = cb_offset[sfb];
    end = cb_offset[sfb + 1];

    if (sfb > lastsb)
    {
      xmin[sfb] = 0;
      continue;
    }

    if (coderInfo->block_type != ONLY_SHORT_WINDOW)
    {
      float enmax = -1.0;
      float lmax;

      lmax = start;
      for (l = start; l < end; l++)
      {
	if (enmax < (xr[l] * xr[l]))
	{
	  enmax = xr[l] * xr[l];
	  lmax = l;
	}
      }

      start = lmax - 2;
      end = lmax + 3;
      if (start < 0)
	start = 0;
      if (end > last)
	end = last;
    }

    for (l = start; l < end; l++)
    {
      enrg += xr[l]*xr[l];
    }

    thr = enrg/((float)(end-start)*avgenrg);
    thr = FAAC_pow(thr, 0.1*(lastsb-sfb)/lastsb + 0.3);

    tmp = 1.0 - ((float)start / (float)last);
    tmp = tmp * tmp * tmp + 0.075;

    thr = 1.0 / (1.4*thr + tmp);

    xmin[sfb] = ((coderInfo->block_type == ONLY_SHORT_WINDOW) ? 0.65 : 1.12)
      * globalthr * thr;
  }
}


//andreil: FixNoise doesn't use "pow43" array at all...
//andreil: ret type = void... ret arg is not used...
#define FIX_NOISE_UNROLL 1
/* TODO: improvve performance !!!! ---- port to asm
 *
 *
 */
static void FixNoise(
            short *sfb_offset,
            short *scale_factor,
            int nr_of_sfb,
		    const float *xr,
		    float *xr_pow,
		    int   *xi,
		    float *xmin
		    //float *pow43,
		    //float *adj43
            )
            
{
    int i, sb;
    int start, end;
    float diffvol;
    float tmp;

	//ANDREIL: replace with CT now...
    const float ifqstep     = 1.1387886;//FAAC_pow(2.0, 0.1875);
    const float log_ifqstep = 7.6943765;//1.0 / FAAC_log(ifqstep);
    const float maxstep = 0.05;

    for (sb = 0; sb < nr_of_sfb; sb++)
    {
      float sfacfix;
      float fixstep = 0.25;
      int sfac;
      float fac;
      int dist;
	  //andreil: changed dist0 to match Float32 precision
      float sfacfix0 = 1.0, dist0 = 1e30;//dist0 = 1e50;
      float maxx;

      start = sfb_offset[sb];
      end   = sfb_offset[sb+1];

      if (!xmin[sb])
	   goto nullsfb;

      maxx = 0.0;
      
      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      #if FIX_NOISE_UNROLL
      for (i = start; i < end;)
      {
        if (xr_pow[i] > maxx) maxx = xr_pow[i]; i++;
        if (xr_pow[i] > maxx) maxx = xr_pow[i]; i++;
        if (xr_pow[i] > maxx) maxx = xr_pow[i]; i++;
        if (xr_pow[i] > maxx) maxx = xr_pow[i]; i++;
      }
      #else //@@@@@@@@@@@@@@@@@@
       //original code:
       for (i = start; i < end; i++)
       {
         if (xr_pow[i] > maxx)
         maxx = xr_pow[i];
        }
      #endif
      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      

      
      
      //printf("band %d: maxx: %f\n", sb, maxx);
      if (maxx < 10.0)
      {
      nullsfb:
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        #if FIX_NOISE_UNROLL
        for (i = start; i < end;){
          xi[i++] = 0;
          xi[i++] = 0;
          xi[i++] = 0;
          xi[i++] = 0;
        }
        #else //@@@@@@@@@@@@@@@@@
        for (i = start; i < end; i++)
          xi[i] = 0;
        #endif
        //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        
	    scale_factor[sb] = 10;
     	continue;
      }

      sfacfix = 1.0 / maxx;
      sfac = (int)(FAAC_log(sfacfix) * log_ifqstep - 0.5);
      
      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      #if FIX_NOISE_UNROLL
      for (i = start; i < end;){
	     xr_pow[i++] *= sfacfix;
         xr_pow[i++] *= sfacfix;
         xr_pow[i++] *= sfacfix;
         xr_pow[i++] *= sfacfix;
      }
      #else //@@@@@@@@@@@@@@@@@@
      for (i = start; i < end; i++)
	     xr_pow[i] *= sfacfix;
      #endif
      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
         
         
      maxx *= sfacfix;
      scale_factor[sb] = sfac;
    
       QuantizeBand(xr_pow, xi, 1.0f, start, end/*, adj43*/);

    calcdist:
      diffvol = 0.0;
      
      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      #if FIX_NOISE_UNROLL
      for (i = start; i < end;)
      {
        tmp = xi[i++]; diffvol += tmp * tmp;  // ~x^(3/2)
        tmp = xi[i++]; diffvol += tmp * tmp;  // ~x^(3/2)
        tmp = xi[i++]; diffvol += tmp * tmp;  // ~x^(3/2)
        tmp = xi[i++]; diffvol += tmp * tmp;  // ~x^(3/2)
      }
      #else //@@@@@@@@@@@@@@@@@
      for (i = start; i < end; i++)
      {
        tmp = xi[i];
        diffvol += tmp * tmp;  // ~x^(3/2)
      }
      #endif
      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

      if (diffvol < 1e-6)
	    diffvol = 1e-6;
        tmp = FAAC_pow(diffvol / (float)(end - start), -0.666);

      if (fabs(fixstep) > maxstep)
      {
	float dd = 0.5*(tmp / xmin[sb] - 1.0);

	if (fabs(dd) < fabs(fixstep))
	{
	  fixstep = dd;

	  if (fabs(fixstep) < maxstep)
	    fixstep = maxstep * ((fixstep > 0) ? 1 : -1);
	}
      }

      if (fixstep > 0)
      {
	if (tmp < dist0)
	{
	  dist0 = tmp;
	  sfacfix0 = sfacfix;
	}
	else
	{
	  if (fixstep > .1)
	    fixstep = .1;
	}
      }
      else
      {
	dist0 = tmp;
	sfacfix0 = sfacfix;
      }

      dist = (tmp > xmin[sb]);
      fac = 0.0;
      if (fabs(fixstep) >= maxstep)
      {
	if ((dist && (fixstep < 0))
	    || (!dist && (fixstep > 0)))
	{
	  fixstep = -0.5 * fixstep;
	}

	fac = 1.0 + fixstep;
      }
      else if (dist)
      {
	fac = 1.0 + fabs(fixstep);
      }

      
      
      
    if (fac != 0.0)
    {
	if (maxx * fac >= IXMAX_VAL)
	{
	  // restore best noise
	  fac = sfacfix0 / sfacfix;
	  
      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      #if FIX_NOISE_UNROLL
       for (i = start; i < end;)
       {
	     xr_pow[i++] *= fac;
         xr_pow[i++] *= fac;
         xr_pow[i++] *= fac;
         xr_pow[i++] *= fac;
       }
      #else //@@@@@@@@@@@@@@@
	  for (i = start; i < end; i++)
	    xr_pow[i] *= fac;
      #endif
      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

	  maxx    *= fac;
	  sfacfix *= fac;

	  scale_factor[sb] = FAAC_log(sfacfix) * log_ifqstep - 0.5;
	  QuantizeBand(xr_pow, xi, 1.0f, start, end/*,adj43*/);
	  continue;
	}

	if (scale_factor[sb] < -10)
	{
     //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      #if FIX_NOISE_UNROLL
      for (i = start; i < end;)
      {
	      xr_pow[i++] *= fac;
          xr_pow[i++] *= fac;
          xr_pow[i++] *= fac;
          xr_pow[i++] *= fac;
       }
      #else //@@@@@@@@@@@@@@@@@
	  for (i = start; i < end; i++)
	    xr_pow[i] *= fac;
      #endif
      //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
        
          maxx *= fac;
          sfacfix *= fac;
	  scale_factor[sb] = FAAC_log(sfacfix) * log_ifqstep - 0.5;
	  QuantizeBand(xr_pow, xi, 1.0f, start, end/*,adj43*/);
	  goto calcdist;
	}
      }
    }
    
}



int SortForGrouping(CoderInfo* coderInfo,
                           PsyInfo *psyInfo,
                           ChannelInfo *channelInfo,
                           //int *sfb_width_table,
						   char *sfb_width_table,
                           float *xr)
{
    int i,j,ii;
    int index = 0;
    float xr_tmp[FRAME_LEN];
    int group_offset=0;
    int k=0;
    int windowOffset = 0;


    /* set up local variables for used quantInfo elements */
    //int* sfb_offset = coderInfo->sfb_offset;
	short* sfb_offset = coderInfo->sfb_offset;

    int* nr_of_sfb = &(coderInfo->nr_of_sfb);
    int* window_group_length;
    int num_window_groups;
    *nr_of_sfb = coderInfo->max_sfb;              /* Init to max_sfb */
    window_group_length = coderInfo->window_group_length;
    num_window_groups = coderInfo->num_window_groups;

    /* calc org sfb_offset just for shortblock */
    sfb_offset[k]=0;
    for (k=1 ; k <*nr_of_sfb+1; k++) {
        sfb_offset[k] = sfb_offset[k-1] + sfb_width_table[k-1];
    }

    /* sort the input spectral coefficients */
    index = 0;
    group_offset=0;
    for (i=0; i< num_window_groups; i++) {
        for (k=0; k<*nr_of_sfb; k++) {
            for (j=0; j < window_group_length[i]; j++) {
                for (ii=0;ii< sfb_width_table[k];ii++)
                    xr_tmp[index++] = xr[ii+ sfb_offset[k] + BLOCK_LEN_SHORT*j +group_offset];
            }
        }
        group_offset +=  BLOCK_LEN_SHORT*window_group_length[i];
    }

    for (k=0; k<FRAME_LEN; k++){
        xr[k] = xr_tmp[k];
    }


    /* now calc the new sfb_offset table for the whole p_spectrum vector*/
    index = 0;
    sfb_offset[index++] = 0;
    windowOffset = 0;
    for (i=0; i < num_window_groups; i++) {
        for (k=0 ; k <*nr_of_sfb; k++) {
            sfb_offset[index] = sfb_offset[index-1] + sfb_width_table[k]*window_group_length[i] ;
            index++;
        }
        windowOffset += window_group_length[i];
    }

    *nr_of_sfb = *nr_of_sfb * num_window_groups;  /* Number interleaved bands. */

    return 0;
}

void CalcAvgEnrg(CoderInfo *coderInfo,
		 const float *xr)
{
  int end, l;
  int last = 0;
  float totenrg = 0.0;

  end = coderInfo->sfb_offset[coderInfo->nr_of_sfb];
  for (l = 0; l < end; l++)
  {
    if (xr[l])
    {
      last = l;
      totenrg += xr[l] * xr[l];
    }
    }
  last++;

  coderInfo->lastx = last;
  coderInfo->avgenrg = totenrg / last;
}
