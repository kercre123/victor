/* 
 * rfir_f.h
 * Author:  Caleb Crome
 * Description:
 * floating point rfir implementation
 */
#ifndef _RFIR_F_H
#define _RFIR_F_H

#include "ext_interface.h"
#ifdef __cplusplus
extern "C" {
#endif
struct RFirF_t;

struct RFirF_t{
    /* n coeffiicents */
    int n;
    /* INPUT block size.  output block size will be blocksize/decimFactor. */
    int blocksize;
    /* decimation factor.  blocksize must be divisible by decimFactor */
    int decimFactor; 
    /* 
     * This is where the implementation is to to store any data 
     * data that gets configured during init.  Typically this would be
     * the coefficients.  However, in for example, a floating point
     * implementation, it would be the coeficients in frequency domain
     * format. This is to allow multiple rfirf instances to use the
     * same coef structure while having different state structures.
     * 
     * usage would be 
     * RFirF a, b;
     * RFirF->init(&a, blah blah balh); <-- creats all state info in a.
     * RFirF->init_copy(&b, &a);        <-- copys coef info, creates state info for b.
     */
    const void *coefs;
    /* The per instance vraible data. */
    void *state;
} ;

#ifdef __cplusplus
}
#endif
#endif
