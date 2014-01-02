/************************* MPEG-2 NBC Audio Decoder **************************
 *                                                                           *
"This software module was originally developed by
AT&T, Dolby Laboratories, Fraunhofer Gesellschaft IIS in the course of
development of the MPEG-2 NBC/MPEG-4 Audio standard ISO/IEC 13818-7,
14496-1,2 and 3. This software module is an implementation of a part of one or more
MPEG-2 NBC/MPEG-4 Audio tools as specified by the MPEG-2 NBC/MPEG-4
Audio standard. ISO/IEC  gives users of the MPEG-2 NBC/MPEG-4 Audio
standards free license to this software module or modifications thereof for use in
hardware or software products claiming conformance to the MPEG-2 NBC/MPEG-4
Audio  standards. Those intending to use this software module in hardware or
software products are advised that this use may infringe existing patents.
The original developer of this software module and his/her company, the subsequent
editors and their companies, and ISO/IEC have no liability for use of this software
module or modifications thereof in an implementation. Copyright is not released for
non MPEG-2 NBC/MPEG-4 Audio conforming products.The original developer
retains full right to use the code for his/her  own purpose, assign or donate the
code to a third party and to inhibit third party from using the code for non
MPEG-2 NBC/MPEG-4 Audio conforming products. This copyright notice must
be included in all copies or derivative works."
Copyright(c)1996.
 *                                                                           *
 ****************************************************************************/
/*
 * $Id: filtbank.c,v 1.13 2005/02/02 07:51:12 sur Exp $
 */

/*
 * CHANGES:
 *  2001/01/17: menno: Added frequency cut off filter.
 *
 */

//#include <math.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

#include "coder.h"
#include "filtbank.h"
#include "frame.h"
#include "fft.h"
#include "util.h"

#include "f16.h"

#define  TWOPI       2*M_PI


//static void	CalculateKBDWindow	( float* win, float alpha, int length );
//static float	Izero				( float x);
static void		MDCT				( FFT_Tables *fft_tables, float *data, int N );
//static void		IMDCT				( FFT_Tables *fft_tables, float *data, int N );


extern unsigned char g_WBUFF[1024*16];


//####################################################################
//#include "f16.h"
//void DUMP_f16_tbl(float *tab, int len, char *tab_name, char *file_name)
//{
//   FILE *f = fopen(file_name, "w");
//   int i;
//
//   fprintf(f, "//andreil: Automatically generated file. Do NOT modify !\n");
//   fprintf(f, "\nFloat16 %s[] = {\n", tab_name);
//
//   for(i=0;i<len;i++)
//    fprintf(f, "  0x%04x,\n", f32_to_f16(tab[i]));  
// 
//   fprintf(f, "\n};\n");
//   fclose(f);
//}



//####################################################################
#include "f16_sin_win_long.hh"
#include "f16_sin_win_short.hh"
#include "f16_kbd_win_long.hh"
#include "f16_kbd_win_short.hh"

short g_overlapBuff[2][FRAME_LEN];

void FilterBankInit(faacEncHandle hEncoder)
{
    unsigned int i, channel;

    for (channel = 0; channel < hEncoder->numChannels; channel++) {
        
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//andreil: map the freqBuff on g_WBUFF
		//hEncoder->freqBuff[channel]    = (float*)AllocMemory(2*FRAME_LEN*sizeof(float),  "hEncoder->freqBuff[channel]");
		hEncoder->freqBuff[channel]    = (float*)&g_WBUFF[channel*2*FRAME_LEN*sizeof(float)];

        //hEncoder->overlapBuff[channel] = (float*)AllocMemory(FRAME_LEN*sizeof(float), "hEncoder->overlapBuff[channel]");
		//SetMemory(hEncoder->overlapBuff[channel], 0, FRAME_LEN*sizeof(float));
		//hEncoder->overlapBuff[channel] = (short*)AllocMemory(FRAME_LEN*sizeof(short), "hEncoder->overlapBuff[channel]");
		hEncoder->overlapBuff[channel] = g_overlapBuff[channel];
		SetMemory(hEncoder->overlapBuff[channel], 0, FRAME_LEN*sizeof(short));
        
    }

	//andreil: assign ptr's to constant tables !!!
	hEncoder->sin_window_long  = (Float16 *)f16_sin_win_long;
	hEncoder->sin_window_short = (Float16 *)f16_sin_win_short;
	hEncoder->kbd_window_long  = (Float16 *)f16_kbd_win_long;
	hEncoder->kbd_window_short = (Float16 *)f16_kbd_win_short;

	/*
    hEncoder->sin_window_long  = (Float16*)AllocMemory(BLOCK_LEN_LONG *sizeof(Float16),  "hEncoder->sin_window_long");
    hEncoder->sin_window_short = (Float16*)AllocMemory(BLOCK_LEN_SHORT*sizeof(Float16), "hEncoder->sin_window_short");
    hEncoder->kbd_window_long  = (Float16*)AllocMemory(BLOCK_LEN_LONG *sizeof(Float16),  "hEncoder->kbd_window_long");
    hEncoder->kbd_window_short = (Float16*)AllocMemory(BLOCK_LEN_SHORT*sizeof(Float16), "hEncoder->kbd_window_short");

	
    for( i=0; i<BLOCK_LEN_LONG; i++ )
        hEncoder->sin_window_long[i] = sin((M_PI/(2*BLOCK_LEN_LONG)) * (i + 0.5));

    for( i=0; i<BLOCK_LEN_SHORT; i++ )
        hEncoder->sin_window_short[i] = sin((M_PI/(2*BLOCK_LEN_SHORT)) * (i + 0.5));

    CalculateKBDWindow(hEncoder->kbd_window_long,  4, BLOCK_LEN_LONG*2);
    CalculateKBDWindow(hEncoder->kbd_window_short, 6, BLOCK_LEN_SHORT*2);

 //~~~~~~~~~~~~~~~~~~~~~~~
 //dump as f16!!!
	DUMP_f16_tbl(hEncoder->sin_window_long,  BLOCK_LEN_LONG,  "f16_sin_win_long",  "f16_sin_win_long.hh");
	DUMP_f16_tbl(hEncoder->sin_window_short, BLOCK_LEN_SHORT, "f16_sin_win_short", "f16_sin_win_short.hh");
	DUMP_f16_tbl(hEncoder->kbd_window_long,  BLOCK_LEN_LONG,  "f16_kbd_win_long",  "f16_kbd_win_long.hh");
	DUMP_f16_tbl(hEncoder->kbd_window_short, BLOCK_LEN_SHORT, "f16_kbd_win_short", "f16_kbd_win_short.hh");
*/
}

void FilterBankEnd(faacEncHandle hEncoder)
{
    //unsigned int channel;

    //for (channel = 0; channel < hEncoder->numChannels; channel++) {
    //    //if (hEncoder->freqBuff[channel]) FreeMemory(hEncoder->freqBuff[channel]);
    //    if (hEncoder->overlapBuff[channel]) FreeMemory(hEncoder->overlapBuff[channel]);
    //}

    //if (hEncoder->sin_window_long) FreeMemory(hEncoder->sin_window_long);
    //if (hEncoder->sin_window_short) FreeMemory(hEncoder->sin_window_short);
    //if (hEncoder->kbd_window_long) FreeMemory(hEncoder->kbd_window_long);
    //if (hEncoder->kbd_window_short) FreeMemory(hEncoder->kbd_window_short);
}


void FilterBank(faacEncHandle hEncoder,
                CoderInfo *coderInfo,
                
				//float *p_in_data,
				short *p_in_data,

                float *p_out_mdct,
                //float *p_overlap,
				short *p_overlap,
                int overlap_select)
{
    float *p_o_buf;//, *first_window, *second_window;
	Float16 *first_window, *second_window;
    float *transf_buf;
    int k, i, j;
    int block_type = coderInfo->block_type;

   //andreil: "transf_buf" and "p_out_mdct" can concide:
   //      there is 1:1 addressing (can overwrite)
	transf_buf = p_out_mdct;

    /* create / shift old values */
    /* We use p_overlap here as buffer holding the last frame time signal*/
    if(overlap_select != MNON_OVERLAPPED) {

		//andreil: do conversions, not just copy !!!
        //memcpy(transf_buf, p_overlap, FRAME_LEN*sizeof(float));
		for(j=0;j<FRAME_LEN;j++) transf_buf[j] = (float) p_overlap[j];

        //memcpy(transf_buf+BLOCK_LEN_LONG, p_in_data, FRAME_LEN*sizeof(float));
		for(j=0;j<FRAME_LEN;j++)
			transf_buf[BLOCK_LEN_LONG+j] = p_in_data[j];

		//WARNING: can put "memcpy" back....
        //andreil: do conversions, not just copy !!!
        //memcpy(p_overlap, p_in_data, FRAME_LEN*sizeof(float));
		for(j=0;j<FRAME_LEN;j++) p_overlap[j] = p_in_data[j];
    }
	//else {
    //    memcpy(transf_buf, p_in_data, 2*FRAME_LEN*sizeof(float));
    //}

    /*  Window shape processing */
    if(overlap_select != MNON_OVERLAPPED) {
        switch (coderInfo->prev_window_shape) {
        case SINE_WINDOW:
            if ( (block_type == ONLY_LONG_WINDOW) || (block_type == LONG_SHORT_WINDOW))
                first_window = hEncoder->sin_window_long;
            else
                first_window = hEncoder->sin_window_short;
            break;
        case KBD_WINDOW:
            if ( (block_type == ONLY_LONG_WINDOW) || (block_type == LONG_SHORT_WINDOW))
                first_window = hEncoder->kbd_window_long;
            else
                first_window = hEncoder->kbd_window_short;
            break;
        }

        switch (coderInfo->window_shape){
        case SINE_WINDOW:
            if ( (block_type == ONLY_LONG_WINDOW) || (block_type == SHORT_LONG_WINDOW))
                second_window = hEncoder->sin_window_long;
            else
                second_window = hEncoder->sin_window_short;
            break;
        case KBD_WINDOW:
            if ( (block_type == ONLY_LONG_WINDOW) || (block_type == SHORT_LONG_WINDOW))
                second_window = hEncoder->kbd_window_long;
            else
                second_window = hEncoder->kbd_window_short;
            break;
        }
    } else {
        /* Always long block and sine window for LTP */
        first_window  = hEncoder->sin_window_long;
        second_window = hEncoder->sin_window_long;
    }

    /* Set ptr to transf-Buffer */
    p_o_buf = transf_buf;

    /* Separate action for each Block Type */
    switch (block_type) {

	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    case ONLY_LONG_WINDOW :
        for ( i = 0 ; i < BLOCK_LEN_LONG ; i++){
            p_out_mdct[i] = p_o_buf[i] * f16_to_f32(first_window[i]);
            p_out_mdct[i+BLOCK_LEN_LONG] = p_o_buf[i+BLOCK_LEN_LONG] * f16_to_f32(second_window[BLOCK_LEN_LONG-i-1]);
        }
        MDCT( &hEncoder->fft_tables, p_out_mdct, 2*BLOCK_LEN_LONG );
        break;

    case LONG_SHORT_WINDOW :
        for ( i = 0 ; i < BLOCK_LEN_LONG ; i++)
            p_out_mdct[i] = p_o_buf[i] * f16_to_f32(first_window[i]);
        memcpy(p_out_mdct+BLOCK_LEN_LONG,p_o_buf+BLOCK_LEN_LONG,NFLAT_LS*sizeof(float));
        for ( i = 0 ; i < BLOCK_LEN_SHORT ; i++)
            p_out_mdct[i+BLOCK_LEN_LONG+NFLAT_LS] = p_o_buf[i+BLOCK_LEN_LONG+NFLAT_LS] * f16_to_f32(second_window[BLOCK_LEN_SHORT-i-1]);
        SetMemory(p_out_mdct+BLOCK_LEN_LONG+NFLAT_LS+BLOCK_LEN_SHORT,0,NFLAT_LS*sizeof(float));
        MDCT( &hEncoder->fft_tables, p_out_mdct, 2*BLOCK_LEN_LONG );
        break;

    case SHORT_LONG_WINDOW :
        SetMemory(p_out_mdct,0,NFLAT_LS*sizeof(float));
        for ( i = 0 ; i < BLOCK_LEN_SHORT ; i++)
            p_out_mdct[i+NFLAT_LS] = p_o_buf[i+NFLAT_LS] * f16_to_f32(first_window[i]);
        memcpy(p_out_mdct+NFLAT_LS+BLOCK_LEN_SHORT,p_o_buf+NFLAT_LS+BLOCK_LEN_SHORT,NFLAT_LS*sizeof(float));
        for ( i = 0 ; i < BLOCK_LEN_LONG ; i++)
            p_out_mdct[i+BLOCK_LEN_LONG] = p_o_buf[i+BLOCK_LEN_LONG] * f16_to_f32(second_window[BLOCK_LEN_LONG-i-1]);
        MDCT( &hEncoder->fft_tables, p_out_mdct, 2*BLOCK_LEN_LONG );
        break;

    case ONLY_SHORT_WINDOW :
        p_o_buf += NFLAT_LS;
        for ( k=0; k < MAX_SHORT_WINDOWS; k++ ) {
            for ( i = 0 ; i < BLOCK_LEN_SHORT ; i++ ){
                p_out_mdct[i] = p_o_buf[i] * f16_to_f32(first_window[i]);
                p_out_mdct[i+BLOCK_LEN_SHORT] = p_o_buf[i+BLOCK_LEN_SHORT] * f16_to_f32(second_window[BLOCK_LEN_SHORT-i-1]);
            }
            MDCT( &hEncoder->fft_tables, p_out_mdct, 2*BLOCK_LEN_SHORT );
            p_out_mdct += BLOCK_LEN_SHORT;
            p_o_buf += BLOCK_LEN_SHORT;
            first_window = second_window;
        }
        break;
    }
    
    
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
     //printf("fiterbank 1; check %x, %x, %x", p_out_mdct, p_overlap, p_in_data);
     // SHAVE_HALT;
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    //if (transf_buf) FreeMemory(transf_buf);
}

//######################################################################
void specFilter(float *freqBuff,
                int sampleRate,
                int lowpassFreq,
                int specLen
                )
{
    int lowpass,xlowpass;

    /* calculate the last line which is not zero */
    lowpass = (lowpassFreq * specLen) / (sampleRate>>1) + 1;
    xlowpass = (lowpass < specLen) ? lowpass : specLen ;

    SetMemory(freqBuff+xlowpass,0,(specLen-xlowpass)*sizeof(float));
}

//static float Izero(float x)
//{
//    const float IzeroEPSILON = 1E-41;  /* Max error acceptable in Izero */
//    float sum, u, halfx, temp;
//    int n;
//
//    sum = u = n = 1;
//    halfx = x/2.0;
//    do {
//        temp = halfx/(float)n;
//        n += 1;
//        temp *= temp;
//        u *= temp;
//        sum += u;
//    } while (u >= IzeroEPSILON*sum);
//
//    return(sum);
//}
//
//static void CalculateKBDWindow(float* win, float alpha, int length)
//{
//    int i;
//    float IBeta;
//    float tmp;
//    float sum = 0.0;
//
//    alpha *= M_PI;
//    IBeta = 1.0/Izero(alpha);
//
//    /* calculate lower half of Kaiser Bessel window */
//    for(i=0; i<(length>>1); i++) {
//        tmp = 4.0*(float)i/(float)length - 1.0;
//        win[i] = Izero(alpha*faac_sqrt(1.0-tmp*tmp))*IBeta;
//        sum += win[i];
//    }
//
//    sum = 1.0/sum;
//    tmp = 0.0;
//
//    /* calculate lower half of window */
//    for(i=0; i<(length>>1); i++) {
//        tmp += win[i];
//        win[i] = faac_sqrt(tmp*sum);
//    }
//}

//alexs:  these down here were pointed to pcmbuf and bitbuf ..... which caused a serious bug !!!
//andreil: could reduce here...???
float g_xi[512];//stupid me !!! N>>2 = 2048/4 = 512
float g_xr[512];

extern int xtrace;
static void MDCT( FFT_Tables *fft_tables, float *data, int N )
{
    float *xi, *xr;
    float tempr, tempi, c, s, cold, cfreq, sfreq; /* temps for pre and post twiddle */
    float freq = TWOPI / N;
    float cosfreq8, sinfreq8;
    int i, n;

    //xi = (float*)AllocMemory((N >> 2)*sizeof(float), "xi");
    //xr = (float*)AllocMemory((N >> 2)*sizeof(float), "xr");
	xi = (float*)g_xi;//g_xi;
	xr = (float*)g_xr;
	
	//andreil: freq = CT = TWOPI/N ... there are just 2 possibilities
    /* prepare for recurrence relation in pre-twiddle */
    //cfreq    = cos (freq);
    //sfreq    = sin (freq);
    //cosfreq8 = cos (freq * 0.125);
    //sinfreq8 = sin (freq * 0.125);

	if(N==2048)
	{
       //printf("1");
	   cfreq    = 0.99999529;
	   sfreq    = 0.0030679568;
	   cosfreq8 = 0.99999994;
	   sinfreq8 = 0.00038349521;
	}else //(N==256)
	{
		//printf("2");
		cfreq    = 0.99969882;
		sfreq    = 0.024541229;
		cosfreq8 = 0.99999529;
		sinfreq8 = 0.0030679568;
	}


    c = cosfreq8;
    s = sinfreq8;

    for (i = 0; i < (N >> 2); i++) {
        /* calculate real and imaginary parts of g(n) or G(p) */
        n = (N >> 1) - 1 - 2 * i;

        if (i < (N >> 3))
            tempr = data [(N >> 2) + n] + data [N + (N >> 2) - 1 - n]; /* use second form of e(n) for n = N / 2 - 1 - 2i */
        else
            tempr = data [(N >> 2) + n] - data [(N >> 2) - 1 - n]; /* use first form of e(n) for n = N / 2 - 1 - 2i */

        n = 2 * i;
        if (i < (N >> 3))
            tempi = data [(N >> 2) + n] - data [(N >> 2) - 1 - n]; /* use first form of e(n) for n=2i */
        else
            tempi = data [(N >> 2) + n] + data [N + (N >> 2) - 1 - n]; /* use second form of e(n) for n=2i*/

        /* calculate pre-twiddled FFT input */
        xr[i] = tempr * c + tempi * s;
        xi[i] = tempi * c - tempr * s;

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
	}

    
    // //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      // printf("check xr:%x, xi:%x", xr, xi);
      // SHAVE_HALT;
    // //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    //ok up to here.... small differences in mantissa !!!
    
    
    
    //crashes in fft !!!!
    /* Perform in-place complex FFT of length N/4 */
    
    
    ///*DBG*/printf("MDCT: trace ON");
    /*DBG*/xtrace = 1;//trace ON
    
    switch (N) {

    case BLOCK_LEN_SHORT * 2:
        fft( fft_tables, xr, xi, 6);
        break;

    case BLOCK_LEN_LONG * 2:
        fft( fft_tables, xr, xi, 9);
        break;
    }
    
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    //  printf("fft_check xr:%x, xi:%x", xr, xi);
    //  SHAVE_HALT;
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    

    
    /* prepare for recurrence relations in post-twiddle */
    c = cosfreq8;
    s = sinfreq8;

    /* post-twiddle FFT output and then get output data */
    for (i = 0; i < (N >> 2); i++) {
        /* get post-twiddled FFT output  */
        tempr = 2. * (xr[i] * c + xi[i] * s);
        tempi = 2. * (xi[i] * c - xr[i] * s);

        /* fill in output values */
        data [2 * i] = -tempr;   /* first half even */
        data [(N >> 1) - 1 - 2 * i] = tempi;  /* first half odd */
        data [(N >> 1) + 2 * i] = -tempi;  /* second half even */
        data [N - 1 - 2 * i] = tempr;  /* second half odd */

        /* use recurrence to prepare cosine and sine for next value of i */
        cold = c;
        c = c * cfreq - s * sfreq;
        s = s * cfreq + cold * sfreq;
    }

    //if (xr) FreeMemory(xr);
    //if (xi) FreeMemory(xi);
}

