///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     encoder math functions
///

#ifndef FAAC_MATOPS_H_
#define FAAC_MATOPS_H_

#include "stddef.h"

#define QUAKE_FLOAT 1

static inline half log2(half n)
{
	half result;

	asm volatile (
			"SAU.log2.f16.l %0, %1\n\t"
			"NOP 4\n\t"
			: "=r" (result) : "r" (n)
	);

	return result;
}

static inline half exp2(half n)
{
	half result;

	asm volatile (
			"SAU.exp2.f16.l %0, %1\n\t"
			"NOP 4\n\t"
			: "=r" (result) : "r" (n)
	);

	return result;
}



static inline float FAAC_sqrt(float number)
{
#if QUAKE_FLOAT
	long i;
	float x, y;
	const float f = 1.5F;

	x = number * 0.5F;
	y  = number;
	i  = * ( long * ) &y;
	i  = 0x5f3759df - ( i >> 1 );
	y  = * ( float * ) &i;;
	y  = y * ( f - ( x * y * y ) );
	y  = y * ( f - ( x * y * y ) );
	return number * y;
#else
	float result;

	__asm volatile (
			"NOP  \n\t"
			: "=r" (result)
			: "r"  (number)
			: "s0", "s1"
	);
#endif
}
#if 0
// float SQRT(float x)
// {
// float result;

// asm volatile (
// /*
// ;Constantin Papandonatos's addition
// ;float SquareRootFloat(float number) {
// ;    long i;
// ;    float x, y;
// ;    const float f = 1.5F;
// ;
// ;    x = number * 0.5F;
// ;    y  = number;
// ;    i  = * ( long * ) &y;
// ;    i  = 0x5f3759df - ( i >> 1 );
// ;    y  = * ( float * ) &i;;
// ;    y  = y * ( f - ( x * y * y ) );
// ;    y  = y * ( f - ( x * y * y ) );
// ;    return number * y;
// ;}
// ;uses s0-s3, i0-i1
// */

// "LSU0.ldil i0, 0x0000 || LSU1.ldih i0, 0x3f00  \n\t"
// "CMU.cpis s1, i0  \n\t"
// "CMU.cpsi i1, %1 || LSU0.ldil i0, 0x59df || LSU1.ldih i0, 0x5f37 || SAU.mul.f32 s0, %1, s1  \n\t"
// "IAU.shr.u32 i1, i1, 1  \n\t"
// "IAU.sub i0, i0, i1  \n\t"
// "CMU.cpis s1, i0  \n\t"
// "SAU.mul.f32 s2, s1, s1  \n\t"
// "LSU0.ldil i0, 0x0000 || LSU1.ldih i0, 0x3fc0  \n\t"
// "CMU.cpis s3, i0  \n\t"
// "SAU.mul.f32 s2, s2, s0  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.sub.f32 s2, s3, s2  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.mul.f32 s1, s1, s2  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.mul.f32 s2, s1, s1  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.mul.f32 s2, s2, s0  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.sub.f32 s2, s3, s2  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.mul.f32 s1, s1, s2  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.mul.f32 s2, s1, s1  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.mul.f32 s2, s2, s0  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.sub.f32 s2, s3, s2  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.mul.f32 s1, s1, s2  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// "SAU.mul.f32 %0, %1, s1  \n\t"
// "NOP  \n\t"
// "NOP  \n\t"
// : "=r" (result) : "r" (x)
// : "s0", "s1", "s2", "s3", "i0", "i1"
// );

// return result;
// }





//#########################################################
//#define F16_MARGIN      65530.0f
//#define F16_REV_MARGIN (1/65530.0f)
//#define F16_SQRT_MARGIN 255.98828098176681962966021701123f

//#####################################
//andreil: spec says: fp16 max FP = 65504
//        so make our SW minimum smaller !!!
//#define F16_MARGIN      65500.0f
//#define F16_REV_MARGIN (1/65500.0f)
//#define F16_SQRT_MARGIN 255.92967784139454896443137575823


//float faac_sqrt_in;
static inline float ____FAAC_sqrt(float in)
{
	float resf32, err;
	int i,pows;
	//static int call=0;
	//unsigned int warn; /*DBG*/

	pows   = 0;
	resf32 = in;

	//copy to a global for debug...
	//faac_sqrt_in = in;

	// /*DBG*/warn = *((unsigned int*)&in);
	// /*DBG*/if((warn & 0x7F800000) == 0x7F800000)//if fp32-exp is max, we got a warning situation !!!
	// /*DBG*/   warn = 1;
	// /*DBG*/else
	// /*DBG*/   warn = 0;


	// /*DBG*/if(warn) {
	// /*DBG*/   printf("__faac_sqrt(%d): %x", call, *((unsigned int*)&in));
	// /*DBG*/   SHAVE_HALT;
	// /*DBG*/}


	while(resf32 > F16_MARGIN){
		resf32 = resf32 * F16_REV_MARGIN;
		pows++;
	}

	resf32 = FAAC_sqrt(resf32);//this can be f16, as number is surely < F16_MARGIN !!!
	for(i=0;i<pows;i++)
		resf32 *= F16_SQRT_MARGIN;//this is f32

	// /*DBG*/if(warn) printf("  faac_sqrt res = %x", *((unsigned int*)&resf32));

	//call++;//calls of faac_sqrt
	return resf32;
}
#endif

//#########################################################
/*
float FAAC_log(float in)
{
    float fres;
	half hin = in;

    //1/ log2(E) = 0.6931471805599469464935774385452f
    fres  = 0.6931471805599469464935774385452f;
    fres *=log2(hin);
	return fres;
}
 */
#define LOG_CT  10000.0f
#define LOG2_CT 13.287712379f //log2(LOG_CT)

static inline float FAAC_log(float in)
{
	float fres;
	half hin = in;

	if(hin < 0.0002f) //if dangerously close to denormal area...
	{//then split operation... so that I don't get -INFINITY !!!!
		fres  = log2(in*LOG_CT);
		fres -= LOG2_CT;
	}else
		fres = log2(hin);

	fres *= 0.6931471805599469464935774385452f;

	return fres;
}


//#########################################################
#define DIV           2.0f
#define LOG2_DIV      1.0f
#define ONE_OVER_DIV  (1.0f/DIV)
#define SUBF          8
#define POW2_2_SUBF   (65536.0f) //(2^SUBF) * (2^SUBF)

static inline float FAAC_pow(float x, float y)
{
	float fres;
	half  z, r1, r2;

	z  = x * ONE_OVER_DIV;
	r1 = exp2(y*log2(z)  - SUBF);  //f16 res
	r2 = exp2(y*LOG2_DIV - SUBF);  //f16 res

	fres = r1;
	fres *= (r2 * POW2_2_SUBF);
	return fres;
}


//#########################################################
#define fabs(x) (x>0 ? x : -x)

//
// //#########################################################
// static inline void * memset ( void * ptr, int value, size_t num )
// {
//   int i;
//
//   //andrei: fast mode, I see all memsets are 4B multiples... so: all memsets do ZERO !!!
//   int len = num>>2;
//   unsigned int *ptr32 = (unsigned int *)ptr;
//
//   for(i=0; i<len; i++)
//     ptr32[i] = 0;
//
//   return ptr;
// }


#endif /* FAAC_MATOPS_H_ */
