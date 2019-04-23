#ifndef _FIRR_H
#define _FIRR_H

#define MAX_ORDER 8

typedef struct {
	double b[MAX_ORDER+1];
	double a[MAX_ORDER];
	int order;
	
	double z_ff[MAX_ORDER];
	double z_fb[MAX_ORDER];
} fiir_t;

// b must have order + 1 coefficients
// a must have order coefficients.
void fiir_init(fiir_t *firr, int order, const double *b, const double *a);
double fiir_run(fiir_t *fiir, double sample);
#endif
