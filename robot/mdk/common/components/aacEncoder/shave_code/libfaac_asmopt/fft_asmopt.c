/*
 * FAAC - Freeware Advanced Audio Coder
 * $Id: fft.c,v 1.12 2005/02/02 07:49:55 sur Exp $
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
 */

#include "fft.h"
#include "util.h"

#define MAXLOGM 9
#define MAXLOGR 8

#if defined DRM && !defined DRM_1024

//andreil: removed the code...

#else /* !defined DRM || defined DRM_1024 */

fftfloat g_fft_tables_costbl    [MAXLOGM+1];
fftfloat g_fft_tables_negsintbl [MAXLOGM+1];
fftfloat g_fft_tables_reordertbl[MAXLOGM+1];

#include "reordertbl_6.hh"
#include "reordertbl_8.hh"
#include "reordertbl_9.hh"


void fft_initialize( FFT_Tables *fft_tables )
{
	int i;
	fft_tables->costbl		= (fftfloat**)g_fft_tables_costbl;    //AllocMemory( (MAXLOGM+1) * sizeof( fft_tables->costbl[0] ) ,   "fft_tables->costbl");
	fft_tables->negsintbl	= (fftfloat**)g_fft_tables_negsintbl; //AllocMemory( (MAXLOGM+1) * sizeof( fft_tables->negsintbl[0]),  "fft_tables->negsintbl");
	fft_tables->reordertbl	= (unsigned short**)g_fft_tables_reordertbl;//AllocMemory( (MAXLOGM+1) * sizeof( fft_tables->reordertbl[0]), "fft_tables->reordertbl");
	
	for( i = 0; i< MAXLOGM+1; i++ )
	{
		fft_tables->costbl[i]		= NULL;
		fft_tables->negsintbl[i]	= NULL;
		fft_tables->reordertbl[i]	= NULL;
	}
    
  //andreil: inits just once here:    
    fft_tables->reordertbl[6] = reordertbl_6;
    fft_tables->reordertbl[8] = reordertbl_8;
    fft_tables->reordertbl[9] = reordertbl_9;
}


void hand_fft_proc(
		float *xr, 
		float *xi,
		fftfloat *refac, 
		fftfloat *imfac, 
		int size);
        
void asmopt_reorder( 
        unsigned short *r, 
        float *x, 
        int logm);



#if 0
static void fft_proc(
		float *xr, 
		float *xi,
		fftfloat *refac, 
		fftfloat *imfac, 
		int size)	
{
	int step, shift, pos;
	int exp, estep;

    //printf("fft_proc: size=%d, refac@%x, imfac@%x", size, refac, imfac);
    
    
    // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
      // int i;
      // for(i=0;i<size;i++)
       // if ( ((unsigned int*)xi)[i] == 0xFF800000)
       // {
         // printf("fft_proc bad input: xi[%d] = 0xFF80_0000 !", i);
         // SHAVE_HALT;
       // }
    // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    
    
    estep = size;
	for (step = 1; step < size; step *= 2)
	{
		int x1;
		int x2 = 0;
		estep >>= 1;
		for (pos = 0; pos < size; pos += (2 * step))
		{
			x1 = x2;
			x2 += step;
			exp = 0;
            //printf("_______________pos=%d",pos);
            
			for (shift = 0; shift < step; shift++)
			{
				float v2r, v2i;

				v2r = xr[x2] * refac[exp] - xi[x2] * imfac[exp];
                
                //xi[x2] = -inf !!!! sometimes !!! this generates some NAN later
                
                // //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                // if(*((unsigned int*)&v2r)==0xFF800000)
                // {
                   // printf("issue here: v2r = -inf");
                   // printf("xr[x2]=%x, refac[exp]=%x, xi[x2]=%x imfac[exp]=%x, exp=%d, x2=%d",
                     // ((unsigned int*)xr)[x2],
                     // ((unsigned int*)refac)[exp],
                     // ((unsigned int*)xi)[x2],
                     // ((unsigned int*)imfac)[exp],
                     // exp,
                     // x2
                     // );
                     // SHAVE_HALT;
                // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                
                
                
				v2i = xr[x2] * imfac[exp] + xi[x2] * refac[exp];
                // //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                // if(*((unsigned int*)&v2i)==0xFF800000)
                // {
                   // printf("%%%issue here: v2i = -inf");
                   // printf("refac at %x, imfac at %x", refac, imfac);
                   // printf("xr[x2]=%x, imfac[exp]=%x, xi[x2]=%x refac[exp]=%x, exp=%d, x2=%d",
                     // ((unsigned int*)xr)[x2],
                     // ((unsigned int*)imfac)[exp],
                     // ((unsigned int*)xi)[x2],
                     // ((unsigned int*)refac)[exp],
                     // exp,
                     // x2
                     // );
                     // SHAVE_HALT;
                // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                


                
                
				xr[x2] = xr[x1] - v2r;
                // //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                // //some checks:
                // if( ((unsigned int*)xr)[x2] == 0x7FC00000) {
                  // printf("err_A ## here x2=%d, xr[x2]=0x%x ", x2, ((unsigned int*)xr)[x2]);
                  // printf("err_A    v2r = %x, xr[x1]=%x, x1=%d ", *((unsigned int*)&v2r), ((unsigned int*)xr)[x1], x1);
                  // SHAVE_HALT;
                // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                
                
                
				xr[x1] += v2r;
                // //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                // //some checks:
                // if( ((unsigned int*)xr)[x1] == 0x7FC00000) {
                  // printf("err_A ## here x1=%d, xr[x1]=0x%x ", x1, ((unsigned int*)xr)[x1]);
                  // printf("err_A    v2r = %x", *((unsigned int*)&v2r));
                  // SHAVE_HALT;
                // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                
                
                
                
                
                
                
                xi[x2] = xi[x1] - v2i;
                // //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                // //some checks:
                // if( ((unsigned int*)xi)[x2] == 0xFF800000) {
                  // printf("err_I ## here x2=%d, xi[x1]=0x%x (x1=%d) ", x2, ((unsigned int*)xi)[x1], x1);
                  // printf("err_I    v2i = %x", *((unsigned int*)&v2i));
                  // SHAVE_HALT;
                // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                
				xi[x1] += v2i;
                // //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                // //some checks:
                // if( ((unsigned int*)xi)[x1] == 0xFF800000) {
                  // printf("err_I ## here x1=%d, xi[x1]=0x%x ", x1, ((unsigned int*)xi)[x1]);
                  // printf("err_I    v2i = %x", *((unsigned int*)&v2i));
                  // SHAVE_HALT;
                // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
                
                ///*DBG*/printf("Im(%d):%x",x1,((unsigned int*)xi)[x1]);

				exp += estep;

				x1++;
				x2++;
			}
		}
	}
    

 //   printf("fft_proc: look at xr:%x xi:%x", xr, xi);
}

#endif


#include "fft_tables_costbl_9.hh"
#include "fft_tables_negsintbl_9.hh"

#include "fft_tables_costbl_8.hh"
#include "fft_tables_negsintbl_8.hh"

#include "fft_tables_costbl_6.hh"
#include "fft_tables_negsintbl_6.hh"



static void check_tables( FFT_Tables *fft_tables, int logm)
{

	if( fft_tables->costbl[logm] == NULL )
	{
	 switch(logm)
	 {
	  case 6:
		fft_tables->costbl[logm]	= (float*)fft_tables_costbl_6;
		fft_tables->negsintbl[logm]	= (float*)fft_tables_negsintbl_6;
        break;

	  case 8:
		fft_tables->costbl[logm]	= (float*)fft_tables_costbl_8;
		fft_tables->negsintbl[logm]	= (float*)fft_tables_negsintbl_8;
        break;

      case 9:
		fft_tables->costbl[logm]	= (float*)fft_tables_costbl_9;
		fft_tables->negsintbl[logm]	= (float*)fft_tables_negsintbl_9;
        break;
	  
	  default:
		//ERROR
	    return;
	 }
	}
	//if( fft_tables->costbl[logm] == NULL )
	//{
	//	int i;
	//	int size = 1 << logm;

	//	if( fft_tables->negsintbl[logm] != NULL )
	//		FreeMemory( fft_tables->negsintbl[logm] );

	//	fft_tables->costbl[logm]	= AllocMemory((size / 2) * sizeof(*(fft_tables->costbl[0])),    "fft_tables->costbl[logm]");
	//	fft_tables->negsintbl[logm]	= AllocMemory((size / 2) * sizeof(*(fft_tables->negsintbl[0])), "fft_tables->negsintbl[logm]");

	//	for (i = 0; i < (size >> 1); i++)
	//	{
	//		float theta = 2.0 * M_PI * ((float) i) / (float) size;
	//		fft_tables->costbl[logm][i]		= cos(theta);
	//		fft_tables->negsintbl[logm][i]	= -sin(theta);
	//	}


	//	//switch(logm)
	//	//{
	//	//case 9:
	//	// DUMP_f32_tbl(fft_tables->costbl[logm],    size/2, "fft_tables_costbl_9",    "fft_tables_costbl_9.hh");
	//	// DUMP_f32_tbl(fft_tables->negsintbl[logm], size/2, "fft_tables_negsintbl_9", "fft_tables_negsintbl_9.hh");
	//	// break;

	//	//case 8:
	//	// DUMP_f32_tbl(fft_tables->costbl[logm],    size/2, "fft_tables_costbl_8",    "fft_tables_costbl_8.hh");
	//	// DUMP_f32_tbl(fft_tables->negsintbl[logm], size/2, "fft_tables_negsintbl_8", "fft_tables_negsintbl_8.hh");
	//	// break;

	//	//case 6:
	//    // DUMP_f32_tbl(fft_tables->costbl[logm],    size/2, "fft_tables_costbl_6",    "fft_tables_costbl_6.hh");
	//	// DUMP_f32_tbl(fft_tables->negsintbl[logm], size/2, "fft_tables_negsintbl_6", "fft_tables_negsintbl_6.hh");
	//	// break;

	//	//default:
 //       //  printf("ERR");
	//	//  break;
	//	//}
	//}

}

int xtrace=0;

void fft( FFT_Tables *fft_tables, float *xr, float *xi, int logm)
{

   //andreil: logm = 6, or 9, can skip these tests...

	//if (logm > MAXLOGM)
	//{
	//	fprintf(stderr, "fft size too big\n");
	//	exit(1);
	//}

	//if (logm < 1)
	//{
	//	//printf("logm < 1\n");
	//	return;
	//}

	//andreil: do some inits if not already done
	check_tables( fft_tables, logm);

    //andriel: jsut pass the TABLE, as we're going to do this in ASM !!!
      asmopt_reorder( fft_tables->reordertbl[logm], xr, logm);
	  asmopt_reorder( fft_tables->reordertbl[logm], xi, logm);
    
	//reorder( fft_tables, xr, logm);
	//reorder( fft_tables, xi, logm);
    
  
    
    //andreil: some checks:
    // {//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
     // int i;
     // for(i=0;i<256;i++)
      // if( ((unsigned int *)&xi)[i] == 0xFF800000)
      // {
       // printf("after reorder: xi[%d] = -inf", i);
       // SHAVE_HALT;
      // }
    // }//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    
    
    
  
    //printf(">> fft_proc; logm=%d, costbl at %x, negsintbl at %x", logm, fft_tables->costbl[logm], fft_tables->negsintbl[logm]);
     hand_fft_proc( xr, xi, fft_tables->costbl[logm], fft_tables->negsintbl[logm], 1 << logm );


    // //@@@@@@@@@@@
     // if(xtrace)
     // {
       // printf("after fft_proc: xr:%x xi:%x (logm=%d)", xr, xi, logm);
       // SHAVE_HALT;
     // }
    // //@@@@@@@@@@@
    
    
    // //@@@@@@@@@@@@@@@@@@
      // printf("after fft_proc: xr:%x xi:%x (logm=%d)", xr, xi, logm);
      // SHAVE_HALT;
    // //@@@@@@@@@@@@@@@@@@
}

void rfft( FFT_Tables *fft_tables, float *x, int logm)
{
	float xi[1 << MAXLOGR];

	//if (logm > MAXLOGR)
	//{
		//fprintf(stderr, "rfft size too big\n");
		//exit(1);
	//}

	memset(xi, 0, (1 << logm) * sizeof(xi[0]));

    //printf("rfft:");
	fft( fft_tables, x, xi, logm);
    
    //@@@@@@@@@@@@@
    //printf("after fft: x:%x xi:%x (logm=%d)",x, xi, logm);
    //SHAVE_HALT;
    //@@@@@@@@@@@@@

	memcpy(x + (1 << (logm - 1)), xi, (1 << (logm - 1)) * sizeof(*x));
}

//void ffti( FFT_Tables *fft_tables, float *xr, float *xi, int logm)
//{
//	int i, size;
//	float fac;
//	float *xrp, *xip;
//
//	fft( fft_tables, xi, xr, logm);
//
//	size = 1 << logm;
//	fac = 1.0 / size;
//	xrp = xr;
//	xip = xi;
//
//	for (i = 0; i < size; i++)
//	{
//		*xrp++ *= fac;
//		*xip++ *= fac;
//	}
//}

#endif /* defined DRM && !defined DRM_1024 */

/*
$Log: fft.c,v $
Revision 1.12  2005/02/02 07:49:55  sur
Added interface to kiss_fft library to implement FFT for 960 transform length.

Revision 1.11  2004/04/02 14:56:17  danchr
fix name clash w/ libavcodec: fft_init -> fft_initialize
bump version number to 1.24 beta

Revision 1.10  2003/11/16 05:02:51  stux
moved global tables from fft.c into hEncoder FFT_Tables. Add fft_init and fft_terminate, flowed through all necessary changes. This should remove at least one instance of a memory leak, and fix some thread-safety problems. Version update to 1.23.3

Revision 1.9  2003/09/07 16:48:01  knik
reduced arrays size

Revision 1.8  2002/11/23 17:32:54  knik
rfft: made xi a local variable

Revision 1.7  2002/08/21 16:52:25  knik
new simplier and faster fft routine and correct real fft
new real fft is just a complex fft wrapper so it is slower than optimal but
by surprise it seems to be at least as fast as the old buggy function

*/
