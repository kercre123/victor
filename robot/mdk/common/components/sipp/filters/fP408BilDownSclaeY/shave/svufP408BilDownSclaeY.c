#include <sipp.h>
#include <sippMacros.h>
#include <filters/fP408BilDownSclaeY/fP408BilDownSclaeY.h>


//##########################################################################################
void svufP408BilDownSclaeY(SippFilter *fptr, int svuNo, int runNo)
{
   UInt32  p, x;
   float   ratioH = (float)fptr->parents[0]->outputW / fptr->outputW;
   float   ratioV = (float)fptr->parents[0]->outputH / fptr->outputH;
   //float   shiftH = (float)(ratioH-1.0f)/2.0f;
   //float   shiftV = (float)(ratioV-1.0f)/2.0f;

   //I/O line pointers
   half *in[2];  
   half *out;    

   //The two input lines needed (bilinear)
   in[0] = (half *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo); 
   in[1] = (half *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
  
   //Initially, point to first plane, will add fptr->planeStride to go to next plane
   out   = (half *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);

   //Compute in advance yFrac, as it's constant for entire line
   float dyFrac = runNo * ratioV + 0.25f;
   dyFrac       = dyFrac - (int)dyFrac;

    //================================================================================================
   //The low level implementation
   for(p=0; p<fptr->nPlanes; p++)
   {
      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //1) compute current plane
      for(x=0; x<fptr->sliceWidth; x++)
      {
         float xCoord = (float)x * ratioH + 0.25f;
         int   xWhole = (int)xCoord;
         float xFrac  = xCoord-(float)xWhole;

         //The weights
         float BW[4] = { (1-xFrac)*(1-dyFrac),
            (  xFrac)*(1-dyFrac),
            (1-xFrac)*(  dyFrac),
            (  xFrac)*(  dyFrac)};

         //Bilinear interp
         out[x+0] = in[0][xWhole] * BW[0] + in[0][xWhole+1] * BW[1]+ //line 0
         in[1][xWhole] * BW[2] + in[1][xWhole+1] * BW[3]; //line 1
      }

      //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      //2) Move to next plane    
      in[0] += fptr->parents[0]->planeStride;
      in[1] += fptr->parents[0]->planeStride;
      out   += fptr->planeStride;
   }
} 
