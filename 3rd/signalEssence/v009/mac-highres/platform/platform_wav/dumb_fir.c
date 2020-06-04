#include "dumb_fir.h"
#include <string.h>
#include <assert.h>

#ifdef __MACH__
#include <stdlib.h>
#include <stdio.h>
#else
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>
#endif

int dumb_fir_init(dumb_fir_t *fir, float *taps, int ntaps)
{
	int result = 0;
	int size_bytes = ntaps * sizeof(fir->delay_line[0]);
	memset(fir, 0, sizeof(*fir)); // zero out the struct.
	fir->ntaps = ntaps;
	fir->taps  = taps;
	
	fir->delay_line = malloc(size_bytes);
	if (fir->delay_line == NULL)
		result = -1;
	memset(fir->delay_line, 0, size_bytes);
	return result;
}
/** Shift a sample into the delay line */
void dumb_fir_shift(int16 sample, dumb_fir_t *fir)
{
	memmove(fir->delay_line+1, fir->delay_line, sizeof(fir->delay_line[0])*(fir->ntaps-1));
	fir->delay_line[0] = sample;
}

/** Run the fir filter */
float dumb_fir_fir(dumb_fir_t *fir) {
	float sum = 0;
	int i;
	int16 *d;
	float *t;
	for (i = 0, d = fir->delay_line, t = fir->taps;
	     i < fir->ntaps;
	     i++)
		sum += *d++ * *t++;
	return sum;
}

void dumb_fir_run(dumb_fir_t *fir, int16 *input, int16 *output, int ninput)
{
	if (fir->delay_line) {
		while(ninput-- > 0) {
			dumb_fir_shift(*input++, fir); // shifts 1 word into the delay line.
			*output++ = (int)(dumb_fir_fir(fir)+0.5);
		}
	} else {
        memcpy(output, input, sizeof(*input)*ninput);
	}
}

void dumb_fir_free(dumb_fir_t *fir)
{
	if (fir->delay_line)
		free(fir->delay_line);
	memset(fir, 0, sizeof(*fir)); // zero out the struct.
}


int fir_bank_init(fir_bank_t *bank, int nfilters){
	int bytes = nfilters * sizeof(bank->firs[0]);
        if (bytes == 0)
            return 0;
	bank->firs = malloc(bytes);
	if (bank->firs == NULL)
		return -1;
	memset(bank->firs, 0, bytes);
	bank->nfirs = nfilters;
	return 0;
			    
}

int fir_bank_set_ir(fir_bank_t *bank, int channel, float *ir, int ntaps) {
	int err = 0;
	if (channel < bank->nfirs && channel >= 0) {
		dumb_fir_init(&bank->firs[channel], ir, ntaps);
	} else  {
		fprintf(stderr, "Error, bad channel number\n");
		err = -1;
	} 
	return err;
}

void fir_bank_run(fir_bank_t *bank, int16 *input, int16 *output, int channels, int ninput, int noutput)
{
	// Runs a whole bank of firs on the *mono* input block input.
	int channel;

	for (channel = 0; channel < channels; channel++) {
        //
        // if no FIR bank configured, then nfirs==0
        //
		if (channel < bank->nfirs) 
        {
            assert(noutput==ninput); // apply FIR only if input blocksize==output blocksize
			dumb_fir_run(&(bank->firs[channel]), input, output, ninput);
		} 
        else 
        {
            // default behavior:
            // no fir configured, so fill output with zeros
            // (formerly it copied the input to output)
            memset(output, 0, sizeof(*input) * noutput);
		}
		output += noutput;
	}
}

void fir_bank_free(fir_bank_t *bank) {
	int i;
        if (bank->firs) {
            for (i = 0; i < bank->nfirs; i++) {
		dumb_fir_free(&bank->firs[i]);
            }
            free(bank->firs);
            bank->firs = 0;
        }
}

#ifdef DUMB_FIR_TEST

float taps[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
int ntaps = sizeof(taps)/sizeof(taps[0]);

int ninput = 50;
int16 input[50];
int16 out[50];
int16 check[50] = {1,  2,  3,  4,  5 ,  6,  7,  8,  9,  10,
		   0,  0,  0,  0,  0,   0,  0,  0,  0,   0,
		   0,  0,  0,  0,  0,   1,  3,  6,  9,  12,
		   15, 18, 21, 24, 27,  19, 10, 0,  0,   0, 
		   0,  0,  0,  0,  0,   0,  0,  0,  0,  30};

		   
int check_results(int16 *a, int16 *b, int ninput) {
	int i;
	int errors = 0;
	for (i = 0; i < ninput; i++) {
		if (a[i] != b[i]) {
			errors ++;
			fprintf(stderr, "Error:  a[%d] (%d) != b[%d] (%d)\n",
				i, a[i], i, b[i]);
		}
	}
	return errors;
}

int main(int argc, char *argv[])
{
	dumb_fir_t fir;
	dumb_fir_init(&fir, taps, ntaps);
	input[0] = 1;
	input[25] = 1;
	input[26] = 1;
	input[27] = 1;
	input[49] = 30;
	memset(out, 0, sizeof(out[0])*ninput);
	dumb_fir_run(&fir, input, out, ninput);
	dumb_fir_free(&fir);
	return check_results(out, check, ninput);
}

#endif
