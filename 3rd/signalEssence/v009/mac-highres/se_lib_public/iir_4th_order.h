/***************************************************************************
   (C) Copyright 2012 SignalEssence; All Rights Reserved

    Module Name: iir_4th_order

    Description:
    Implement a 4th order IIR filter, as described in "High-Order Digital Parametric Equalizer Design"
    which is available in ../devtools/high-order-filters/

    There are several implementations possible of the 4th order
    sections, which were implemented in the paper (also in
    ../devtools/high-order-filters), with different numeric
    properties.:
    
    * cascfilt.m   : cascaded filters using the matlab (octave) 'filter' function.
    * decoupfilt.m : filtering in 'frequency-shifted 2nd-order
                     cascaeded normalized lattice sections'.  Whatever
                     that means :-)
    * df2filt.m    : filtering in frequency-shifted 2nd-order cascaded
                     direct form II 
    * nlattfilt.m  : filtering in frequency-shifted 2nd-order cascaded
                     normalized lattice sections 
    * statefilt.m  : filtering in frequency-shifted cascaded
                     state-space form 
    * transpfilt.m : filtering in frequency-transformed 2nd-order
                     cascaded transposed direct-form-II 

    For simplicity, I'll probably just stick with the direct
    cascfilt.m form.		 

    History:
    2012-06-26 Initial rev

    Machine/Compiler:
    (ANSI C)
**************************************************************************/

#ifdef __cplusplus
extern "C" { 
#endif
#ifndef _IIR_4TH_ORDER_H_
#define _IIR_4TH_ORDER_H_
#include "se_types.h"

typedef double iir_float;

  typedef struct {
    iir_float b[5];
    iir_float a[5];
  } iir_4th_coefs_t;
  
  typedef struct {
      const iir_4th_coefs_t *coefs;
      iir_float z[4];
  } iir_4th_t;

  /** Initialze a blank iir_4th_t structure.  
   *  @param iir the iir struct to initialize.
   *  @param coefs pointer to the coefficients for this filter.  This
   *               pointer will be used directly and the data will not
   *               be copied, so the data must stick around for the
   *               'run' call. 
   *  @param n     the number of iir filters and coefficients.  these
   *               must be the same.  The initialization happens such
   *               that iir[0].coefs = coefs[0]
   *               that iir[1].coefs = coefs[1], etc.
   */
void iir_4th_init(iir_4th_t *iir, const iir_4th_coefs_t *coefs, int n);
  /** Run the 4th order filter, on floating point data.
   *  @param iir  a pointer to one or more iir filter structures.
   *              This will run num_iir filters, cascaded.
   *  @param num_irr the number of iir filters pointed to by the iir
   *                 parameter. 
   *  @param source  the source data
   *  @param dest    the destination data.
   *  @param blocksize the size of both source and dest.
   */
void iir_4th_run_float(iir_4th_t *iir, int num_iir, const iir_float *source,
                       iir_float *dest, int blocksize, iir_float gain);
void iir_4th_run_int(iir_4th_t *iir, int num_iir, const int16 *source,
                     int16 *dest, int blocksize, iir_float gain);
#endif
#ifdef __cplusplus
}
#endif
