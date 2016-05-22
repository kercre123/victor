#include <mex.h>

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  const double *x1 = mxGetPr(prhs[0]);
  const int     N1 = (int) mxGetM(prhs[0]);
  const double *y1 = x1 + N1;

  const double *x2 = mxGetPr(prhs[1]);
  const int     N2 = static_cast<int>(mxGetM(prhs[1]));
  const double *y2 = x2 + N2;

  plhs[0] = mxCreateNumericMatrix(N1,1, mxINT32_CLASS, mxREAL);
  int *closestIndex = (int *) mxGetData(plhs[0]);

  for(int i = 0; i < N1; ++i)
  {
    closestIndex[i] = 0;
    double xdiff = x1[i] - x2[0];
    double ydiff = y1[i] - y2[0];
    double minDist = xdiff*xdiff + ydiff*ydiff;

    for(int j = 1; j < N2; ++j)
    {
      double xdiff = x1[i] - x2[j];
      xdiff *= xdiff;

      if(xdiff < minDist)
      {
        double d = y1[i] - y2[j];
        d *= d;
        d += xdiff;

        if(d < minDist)
        {
          minDist = d;
          closestIndex[i] = j;
        }
      }
    }
  }

  // Add one for Matlab indexing:
  for(int i=0; i<N1; ++i) {
    closestIndex[i] += 1;
  }

  return;
}