#include <stdio.h>
#include <string.h>
#include "fiir.h"

void fiir_init(fiir_t *fiir, int order, const double *b, const double *a)
{
	int i;
	fiir->order = order;
        memset (fiir->b, 0, sizeof(fiir->b));
        memset (fiir->a, 0, sizeof(fiir->a));
        memset (fiir->z_ff, 0, sizeof(fiir->z_ff));
        memset (fiir->z_fb, 0, sizeof(fiir->z_fb));
        
	for (i = 0; i < order+1; i++) {
		fiir->b[i] = b[i];
	}
	for (i = 0; i < order; i++) {
		fiir->a[i] = a[i];
	}
}


// IIR direct form I implementation
double fiir_run(fiir_t *fiir, double sample)
{
	int i;
	double y = sample * fiir->b[0];
	for (i = 0; i < fiir->order; i++) { // feed forward
		y += fiir->z_ff[i] * fiir->b[i+1];
	}
	for (i = 0; i < fiir->order; i++) { // feed back
		y -= fiir->z_fb[i] * fiir->a[i];
	}
	for (i = fiir->order-1; i > 0; i--) {
		fiir->z_fb[i] = fiir->z_fb[i-1];
		fiir->z_ff[i] = fiir->z_ff[i-1];
	}
	fiir->z_fb[0] = y;
	fiir->z_ff[0] = sample;
	return y;
}


#ifdef FIIR_TEST



double tweeter_1_b[] =  { 7.867584e-01,  -3.147034e+00,  4.720551e+00,  -3.147034e+00,  7.867584e-01};
double tweeter_1_a[] =  { -3.521440e+00,  4.675527e+00,  -2.772178e+00,  6.189888e-01};
double tweeter_2_b[] =  { 7.867584e-01,  -3.147034e+00,  4.720551e+00,  -3.147034e+00,  7.867584e-01};
double tweeter_2_a[] =  { -3.521440e+00,  4.675527e+00,  -2.772178e+00,  6.189888e-01};
double tweeter_3_b[] =  { 9.063465e-01,  -1.983114e+00,  1.110540e+00};
double tweeter_3_a[] =  { -2.000000e+00,  1.000000e+00};
double woofer_1_b[]  =  { 5.608664e-05,  2.243466e-04,  3.365198e-04,  2.243466e-04,  5.608664e-05};
double woofer_1_a[]  =  { -3.521440e+00,  4.675527e+00,  -2.772178e+00,  6.189888e-01};
double woofer_2_b[]  =  { 5.608664e-05,  2.243466e-04,  3.365198e-04,  2.243466e-04,  5.608664e-05};
double woofer_2_a[]  =  { -3.521440e+00,  4.675527e+00,  -2.772178e+00,  6.189888e-01};


int main(int argc, char *argv[]) 
{
	fiir_t tweeter_1, tweeter_2, tweeter_3, woofer_1, woofer_2;
	int impulse_length = 1000;
	double r;
	int i;
	fiir_init(&tweeter_1, 4, tweeter_1_b, tweeter_1_a);
	fiir_init(&tweeter_2, 4, tweeter_2_b, tweeter_2_a);
	fiir_init(&tweeter_3, 2, tweeter_3_b, tweeter_3_a);
	fiir_init(&woofer_1 , 4, woofer_1_b ,  woofer_1_a);
	fiir_init(&woofer_2 , 4, woofer_2_b ,  woofer_2_a);
	
	r = fiir_run(&woofer_1, 1);
	printf("%e\n", r);
	for (i = 0; i < impulse_length; i++) {
		r = fiir_run(&woofer_1, 0);
		//r = fiir_run(&tweeter_2, r);
		//r = fiir_run(&tweeter_3, r);
		printf("%e\n", r);
	}
}
#endif
