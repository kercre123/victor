#include <mex.h>

#include <vector>

#include "anki/common/matlab/mexWrappers.h"
#include "anki/common/shared/utilities_shared.h"

void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
  Anki::SetCoreTechPrintFunctionPtr(mexPrintf);

  if(nrhs != 2 || nlhs != 4) {
    mexErrMsgTxt("\nUsage:\n\t[area, indexList, boundingBox, centroid] = mexRegionprops(regionMap, numRegions)");
  }

  if(mxGetClassID(prhs[0]) != mxUINT32_CLASS) {
    mexErrMsgTxt("Region map should be UINT32!");
  }

  const int *regionMap = (const int *)mxGetData(prhs[0]);

  mwSize nrows = mxGetM(prhs[0]);
  mwSize ncols = mxGetN(prhs[0]);

  int numRegions = (int) mxGetScalar(prhs[1]);

  plhs[0] = mxCreateNumericMatrix(numRegions, 1, mxUINT32_CLASS, mxREAL);
  int *area = (int *) mxGetData(plhs[0]);

  plhs[2] = mxCreateDoubleMatrix(numRegions,4, mxREAL);
  double *bbLeft   = mxGetPr(plhs[2]);
  double *bbTop    = bbLeft + numRegions;
  double *bbWidth  = bbTop + numRegions;
  double *bbHeight = bbWidth + numRegions;

  plhs[3] = mxCreateDoubleMatrix(numRegions, 2, mxREAL);
  double *xcen = mxGetPr(plhs[3]);
  double *ycen = xcen + numRegions;

  for(int i=0; i<numRegions; ++i) {
    area[i] = 0;
    xcen[i] = 0.0;
    ycen[i] = 0.0;

    bbLeft[i]   = double(ncols+1);
    bbTop[i]    = double(nrows+1);
    bbWidth[i]  = 0.0;
    bbHeight[i] = 0.0;
  }

  for(mwSize i=0; i<(nrows*ncols); ++i)
  {
    if(regionMap[i] > 0)
    {
      area[regionMap[i]-1]++;
    }
  }

  plhs[1] = mxCreateCellMatrix(numRegions,1);
  std::vector<int*> indexList(numRegions);
  for(int i=0; i<numRegions; ++i) {
    mxSetCell(plhs[1], i, mxCreateNumericMatrix(area[i], 1, mxUINT32_CLASS, mxREAL));
    indexList[i] = (int *) mxGetData(mxGetCell(plhs[1], i));
  }

  int i=0;
  for(mwSize c=0; c<ncols; ++c)
  {
    const int x = static_cast<int>(c)+1; // +1 for Matlab indexing

    for(mwSize r=0; r<nrows; ++r, ++i)
    {
      const int y = static_cast<int>(r)+1; // +1 for Matlab indexing
      int region = regionMap[i];
      if(region > 0)
      {
        region -= 1; // -1 for C indexing of regions

        *(indexList[region]) = (i+1); // +1 for Matlab indexing
        indexList[region]++;

        xcen[region] += x;
        ycen[region] += y;

        if((x-0.5) < bbLeft[region]) {
          bbLeft[region] = x - 0.5;
        }

        if ((x+0.5) > bbWidth[region]) {
          // Initially just store rightmost coordinate here:
          bbWidth[region] = x + 0.5;
        }

        if((y-0.5) < bbTop[region]) {
          bbTop[region] = y - 0.5;
        }

        if((y+0.5) > bbHeight[region]) {
          // Initially just store bottommost coordinate here:
          bbHeight[region] = y + 0.5;
        }
      }
    }
  }

  for(int i=0; i<numRegions; ++i) {
    xcen[i] /= double(area[i]);
    ycen[i] /= double(area[i]);

    // Turn previously-stored right/bottom into width/height:
    bbWidth[i]  -= bbLeft[i];
    bbHeight[i] -= bbTop[i];
  }
}