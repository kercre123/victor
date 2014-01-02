#include <sipp.h>

//##############################################################################
// Clamp input line indices to input height
// this is ok with top/bottom replication concept
// Number of planes doesn't matter, each plane has same line requirement
//##############################################################################
void clampInLines(SippFilter *fptr)
{

  //input height is the parent's output height.
  //all parents of a filter MUST have same output height (otherwise we can't combine the data)
   UInt32 p, i;

   for(p=0; p<fptr->nParents; p++)
   {
    int  in_H = fptr->parents[p]->outputH-1;

    for(i=0; i<fptr->nLinesUsed[p]; i++)
    {
      if(fptr->sch->reqInLns[p][i] < 0)
         fptr->sch->reqInLns[p][i] = 0;
   
      if(fptr->sch->reqInLns[p][i] > in_H) 
         fptr->sch->reqInLns[p][i] = in_H;
    }
   }
}

//##############################################################################
// Function that polupates lists of REQUIRED [INPUT line indices] to generate
// the output line numbered "outLineNo"
// This is the typical function for non-resize filters
// (useful for DMA, convolutions, median, denoise, etc...
//
// Example: 
//  for a 3x3 convolution, to generate the output line N, we need the
//  following input lines
//   req_in_ln[0] = N - 1;
//   req_in_ln[1] = N + 0;
//   req_in_ln[2] = N + 1;
//##############################################################################
void askRegular(SippFilter *fptr, int outLineNo)
{
    UInt32 p, i, topLobe;

    //Lines required for each parent
    for(p=0; p<fptr->nParents; p++)
    {
        topLobe = (fptr->nLinesUsed[p]-1)>>1;

        for(i=0; i<fptr->nLinesUsed[p]; i++)
            fptr->sch->reqInLns[p][i] = outLineNo - topLobe + i;
    }

    clampInLines(fptr);
}

//##############################################################################
void askCrop(SippFilter *fptr, int outLineNo)
{
    //parent 0, first_line
    fptr->sch->reqInLns[0][0] = outLineNo + *((int*)fptr->params);
    //TBD: use proper cast to CropParam... put this in a standard place

    clampInLines(fptr);
}

//##############################################################################
void askHwDebayer(SippFilter *fptr, int outLineNo)
{
    fptr->sch->reqInLns[0][ 0] = outLineNo +  0;
    fptr->sch->reqInLns[0][ 1] = outLineNo +  1;
    fptr->sch->reqInLns[0][ 2] = outLineNo +  2;
    fptr->sch->reqInLns[0][ 3] = outLineNo +  3;
    fptr->sch->reqInLns[0][ 4] = outLineNo +  4;
    fptr->sch->reqInLns[0][ 5] = outLineNo +  5;
    fptr->sch->reqInLns[0][ 6] = outLineNo +  6;
    fptr->sch->reqInLns[0][ 7] = outLineNo +  7;
    fptr->sch->reqInLns[0][ 8] = outLineNo +  8;
    fptr->sch->reqInLns[0][ 9] = outLineNo +  9;
    fptr->sch->reqInLns[0][10] = outLineNo + 10;
    clampInLines(fptr);
}

//##############################################################################
void askHwColorComb(SippFilter *fptr, int outLineNo)
{
  //from Luma parent (i.e. parent 0), we need 1 line
    fptr->sch->reqInLns[0][0] = outLineNo; 

  //from Chroma parent (i.e. parent 1), we need 5 lines
    fptr->sch->reqInLns[1][0] = (outLineNo/2)-2; 
    fptr->sch->reqInLns[1][1] = (outLineNo/2)-1; 
    fptr->sch->reqInLns[1][2] = (outLineNo/2)+0; 
    fptr->sch->reqInLns[1][3] = (outLineNo/2)+1; 
    fptr->sch->reqInLns[1][4] = (outLineNo/2)+2; 

    clampInLines(fptr);
}

//##############################################################################
void askHwHarris (SippFilter *fptr, int outLineNo)
{
#if defined(SIPP_PC) || (defined(__sparc) && defined(MYRIAD2))
    UInt32 i;

    for(i=0; i<fptr->nLinesUsed[0]; i++)
    {
        fptr->sch->reqInLns[0][i] = outLineNo + i;
    }

    clampInLines(fptr);
#endif
}

/*
//##############################################################################
//Example of custom ASK function for 3x3 convolution kernel
//##############################################################################
void ASK_conv3x3(SippFilter *fptr, int line_no)
{
  fptr->sch->reqInLns[0][0] = line_no-1;
  fptr->sch->reqInLns[0][1] = line_no;
  fptr->sch->reqInLns[0][2] = line_no+1;

  clampInLines(fptr);
}
*/

//##############################################################################
// calculates the source co-ordinate for a given row/column zoom 
// factor and centre point for different source and dest sizes
float getCoord2(int in, float factor, float centreIn, float centreOut)
{
    float shifted = (float)in;
    shifted -= centreOut;    
    shifted /= factor;
    shifted += centreIn;
    return shifted;
}

//##############################################################################
void askResizer(SippFilter *fptr, int outLineNo)
{
    UInt32 p, i, topLobe;
  
    int   input_H   = fptr->parents[0]->outputH;
    float z_factor  = (float)fptr->outputH / input_H;

    int   center_ln = (int)getCoord2(outLineNo, 
                                     z_factor, 
                                     input_H /2.0f,        //centered scale
                                     fptr->outputH/2.0f); //centered scale

    //Lines required for each parent
    for(p=0; p<fptr->nParents; p++)
    {
        topLobe = (fptr->nLinesUsed[p]-1)>>1;

        for(i=0; i<fptr->nLinesUsed[p]; i++)
            fptr->sch->reqInLns[p][i] = center_ln - topLobe + i;
    }

    clampInLines(fptr);
}

#if 0
//####################################################
//This is just for 7-tap resizer, TBD: make generic
//####################################################
void ASK_sync_7(SippFilter *fptr, int line_no)
{
  int   p;
  int   input_H   = fptr->parents[0]->outputH;
  float z_factor  = (float)fptr->outputH / input_H;
  int   center_ln = getCoord2(line_no, 
                              z_factor, 
                              input_H /2.0f,        //centered scale
                              fptr->outputH/2.0f); //centered scale

  for(p=0; p<fptr->nParents; p++)
  {
      fptr->sch->reqInLns[p][0] = center_ln - 3;
      fptr->sch->reqInLns[p][1] = center_ln - 2;
      fptr->sch->reqInLns[p][2] = center_ln - 1;
      fptr->sch->reqInLns[p][3] = center_ln + 0;//<<<<<
      fptr->sch->reqInLns[p][4] = center_ln + 1;
      fptr->sch->reqInLns[p][5] = center_ln + 2;
      fptr->sch->reqInLns[p][6] = center_ln + 3;
  }
  
  clampInLines(fptr);
}

//####################################################
//This is just for 7-tap resizer, TBD: make generic
//####################################################
void ASK_sync_4(SippFilter *fptr, int line_no)
{
  int   p;
  int   input_H   = fptr->parents[0]->outputH;
  float z_factor  = (float)fptr->outputH / input_H;

  int   center_ln = getCoord2(line_no, 
                              z_factor, 
                              input_H /2.0f,        //centered scale
                              fptr->outputH/2.0f); //centered scale

  for(p=0; p<fptr->nParents; p++)
  {
      fptr->sch->reqInLns[p][0] = center_ln - 1;
      fptr->sch->reqInLns[p][1] = center_ln + 0; //<<<<< 
      fptr->sch->reqInLns[p][2] = center_ln + 1; //<<<<<
      fptr->sch->reqInLns[p][3] = center_ln + 2;
  }
  
  clampInLines(fptr);
}

//####################################################
void ASK_bilin_gen(SippFilter *fptr, int line_no)
{
  int   input_H   = fptr->parents[0]->outputH;
  float z_factor  = (float)fptr->outputH / input_H;

 //The general case: two succesive lines
  fptr->sch->reqInLns[0][0] = getCoord2(line_no, 
                                          z_factor, //fptr->params.factor, 
                                          input_H /2.0f,
                                          fptr->outputH/2.0f);

  fptr->sch->reqInLns[0][1] = fptr->sch->reqInLns[0][0]+1;
  
  clampInLines(fptr);
}
#endif
