#ifndef _DUMB_FIR_H
#define _DUMB_FIR_H
#include "se_types.h"

/** Pretty dumb FIR filter implementation.
 */

/** Dumb fir state */
typedef struct {
	int    ntaps;
	int16 *delay_line;
	float *taps;
} dumb_fir_t;

/** Initialize the dumb fir filter.  When done with the dumb_fir_t, be sure to call dumb_fir_free on it.
 * You can specify taps = NULL and ntaps == 0 in order to specify a
 * unity impulse response (i.e. copy in to out).
 * @param fir  an uninitialized FIR filter to be initialized.
 * @param taps  a pointer to a block of filter coefficients.
 * @param ntaps the number of taps pointed to by taps. 
 * @returns 0 on success, nonzero if unable to allocate memory.
 */
int dumb_fir_init(dumb_fir_t *fir, float *taps, int ntaps);

/** Run the dumb fir filter.  Each call takes ninput samples and generates ninput output sampels.
 * @param fir the fir filter
 * @param input pointer to the input samples.
 * @param output pointer to the output samples.
 * @param ninput the number of input samples to process.
 */
void dumb_fir_run(dumb_fir_t *fir, int16 *input, int16 *output, int ninput);

/** cleanup after dumb_fir_init.
 *  @param fir The fir filter to cleanup
 */
void dumb_fir_free(dumb_fir_t *fir);


/** A bank of FIR filters. 
 *  Usage:  1) fir_bank_init(), 
 *          2) then fir_bank_set_ir() on each fir you wish to use.
 *          3) fir_bank_run() for each block
 *          4) fir_bank_free() when done.
 */
typedef struct {
	dumb_fir_t *firs;
	int         nfirs;
} fir_bank_t;

/** Initialize a FIR bank
 *  @param bank      the FIR bank to be initialized.
 *  @param nfilters  how many filters to allocate
 *  @returns         0 on success, nonzero on failure.
 */
int  fir_bank_init(fir_bank_t *bank, int nfilters);

/** Set a single impulse response for an FIR filter.
 * @param bank    The FIR bank
 * @param channel Which channel inside the bank to set
 * @param ir      the impulse response.  not copied -- it needs to stick around.
 * @param ntaps   the number of taps in the impulse response.
 * @returns       0 on success.  nonzero if channel is out of range.
 */
int  fir_bank_set_ir(fir_bank_t *bank, int channel, float *ir, int ntaps);

/** Run the entire FIR bank on a single input.  Creates
 * 'channels*ninput' output samples, in non-linearleaved format. 
 * @param bank   the fir filter.
 * @param input  a *mono* input signal, of ninput samples long.
 * @param output a multi-channel output signal of channels*ninput
 * samples long.
 * @param channels the number of channels in output.
 * @param input    the number of input samples
 */
void fir_bank_run(fir_bank_t *bank, int16 *input, int16 *output, int channels, int ninput, int noutput);

/** cleanup the fir filter bank */
void fir_bank_free(fir_bank_t *bank);

#endif
