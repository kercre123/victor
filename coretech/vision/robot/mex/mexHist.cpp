#include <mex.h>
#include <cmath>

#define ConditionalErrorAndReturn(expression, eventName, eventValue) if(!(expression)) { printf("%s - %s\n", (eventName), (eventValue)); return;}

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  // ASSUMES data is scaled between 0 and 1
  const double *data = mxGetPr(prhs[0]);
  const int numData = (int) mxGetNumberOfElements(prhs[0]);
  const int numBins = (int) mxGetScalar(prhs[1]);

  plhs[0] = mxCreateNumericMatrix(numBins, 1, mxUINT32_CLASS, mxREAL);
  unsigned int *counts = (unsigned int *) mxGetData(plhs[0]);

  for(int i=0; i<numBins; ++i) {
    counts[i] = 0;
  }

  for(int i=0; i<numData; ++i, ++data) {
    const unsigned int bin = (unsigned int) (double(numBins-1)*(*data) + 0.5);
    mxAssert(bin >= 0 && bin < numBins, "Data must not be in range [0,1].");
    counts[bin]++;
  }

  return;
}