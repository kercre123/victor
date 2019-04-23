#ifndef _EXT_DFT_H
#define _EXT_DFT_H

#include "se_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int16 (*VDotProductQ15_i16_i16_t)(const int16 *xptr, 
                                       const int16 *hptr, 
                                       uint16 N);

typedef void (*ComputeRFir_t)(const int16 *pInput,
                                   const int16 *pCoef,
                                   int16 *pOutput,
                                   uint16 ntapsminus1,
                                   uint16 blocksize,
                                   uint16 decimation_factor);

typedef void (*AecConvolution_t)(const int16 *coeffs,
                                      const int16 *input,
                                      int32 *output,
                                      int rshifts,
                                      int lenChanModel,
                                      int blockSize,
                                      int lshifts);

typedef void   (*Interp1toNCore_t)  (uint16 inBlockSize, uint16 interpFactor, int16 **ppInSamps, int16 *pOut0, const int16 *coefAddr, uint16 numCoefsPerPhase);


typedef void   (*VCMul_i16_t)       (const int16 *xp, const int16 *yp, int16 *dest, uint16 numComplexOut);


/*
 * RFirF_t
 * floating point FIR filter interface
 * 
 * Example useage:
 * {
 *    struct RFirF_t rfirf;
 *    RFirF->init(&rfirf, n_taps, blocksize, deci_factor, coefficients); // init
 *    RFirF->run(&rfirf, source, dest); // call many times.
 *    RFirf->free(&rfirf); // free up allocated space in init.
 * }
 * 
 * To explicitely use the ANSI only verion, you simply call:
 *   RFirF_ansi->init, run, free, etc.
 */
struct RFirF_t;
struct  RFirF_ops_t {
    void (*init)(struct RFirF_t *rfirf, int coef_len,
		 int blocksize, int deciFactor, const float32 *pBackwardsH);
    void (*init_copy)(struct RFirF_t *rf_new, struct RFirF_t *rf_old);
    void (*run)(struct RFirF_t *rfirf, const float32 *src, float32 *dest);
    void (*free)(struct RFirF_t *rfif);
    void (*free_copy)(struct RFirF_t *rfif);
};
extern struct RFirF_ops_t        *RFirF;
extern const struct RFirF_ops_t  *RFirF_ansi;

extern VDotProductQ15_i16_i16_t  VDotProductQ15_i16_i16;
extern ComputeRFir_t             ComputeRFir;             //compute_rfir_fct;
extern AecConvolution_t          AecConvolution;          //aec_convolution_fct;
extern VCMul_i16_t               VCMul_i16;               //vcmul_i16_fct;
extern Interp1toNCore_t          Interp1toNCore;


    
// FrMul_Q15_i32;
// GetNormExp_i32
// VAdd_i16
// VCRMulAdd_i16_i32;
// VSub_i16;
// VPower_i16_i32
// VPowerWithExponent_i16_i32
// VFill_i16
// VLeftShifts_i16
// VMove_i32
// VMove_i16
// VShiftRtRnd_i32_i16
// VMul_i16_q15_i16
// VDotProductRtShifts_i16_i32
// VDotProdDualRtShifts_i16_i32
// VDotProdDualQ15_i16_i16
// VScale_i32_q15_i32
// VScale_i16_q12_i16
// VScale_i16_q15_i32
// VScale_q31_q15_q15
// VScale_q31_q14_q15
// VScaleAdd_i16_q15_i16_i16
// VFlipFloatToQ15
// Interp1to2Core42_t Interp1to2Core42 = Interp1to2Core42_Win32;
// Interp1toNCore_t   Interp1toNCore   = Interp1toNCore_Win32;

/// // #define ComputeRFir       ComputeRFirARM
/// #define ComputeRFir       ComputeRFirARM
/// #define AecAdapt          AecAdapt_DualMac
/// 
#ifdef __cplusplus
}
#endif

#endif  // _EXT_DFT_H



