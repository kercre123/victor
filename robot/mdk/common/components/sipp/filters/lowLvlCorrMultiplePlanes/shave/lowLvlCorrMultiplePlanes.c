// Low Level Correction contain black level correction and corect bad pixel
#include <sipp.h>
#include <sippMacros.h>
#include <filters/lowLvlCorrMultiplePlanes/lowLvlCorrMultiplePlanes.h>

#define SUB_SATURATE(x,y) (x < y ? 0 : (x - y))
//#define MAX(x,y) (x < y ? y : x)
//#define MIN(x,y) (x < y ? x : y)
//##########################################################################################
void svulowLvlCorrMultiplePlanes(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32         x;
    LowLvlCorrNPlParam *param = (LowLvlCorrNPlParam *)fptr->params;
    UInt8         blackLevel   = param->blackLevel;
    float         alpha        = param->alphaBadPixel;
    int p;
    

  //Input pointers
    UInt8  *in[3]; 
    in[0]= (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
    in[1]= (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][1], svuNo);
    in[2]= (UInt8 *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][2], svuNo);

  //Output pointers
    UInt8  *out = (UInt8 *)WRESOLVE(fptr->dbLineOut[runNo&1], svuNo);


  //The low level implementation
   for(p=0; p<fptr->nPlanes; p++)
   {
        for(x=0; x<fptr->sliceWidth; x++)
        {
            UInt16 maxim = 
            MAX( 
                MAX(
                    MAX(SUB_SATURATE(in[0][x-1],blackLevel),SUB_SATURATE(in[2][x-1],blackLevel)),
                    MAX(SUB_SATURATE(in[0][x-0],blackLevel),SUB_SATURATE(in[2][x-0],blackLevel))
                ),
                MAX(          
                    MAX(SUB_SATURATE(in[0][x+1],blackLevel),SUB_SATURATE(in[2][x+1],blackLevel)),
                    MAX(SUB_SATURATE(in[1][x-1],blackLevel),SUB_SATURATE(in[1][x+1],blackLevel))
                )
            );
        
            UInt16 minim = 
            MIN(
                MIN(
                    MIN(SUB_SATURATE(in[0][x-1],blackLevel),SUB_SATURATE(in[2][x-1],blackLevel)),
                    MIN(SUB_SATURATE(in[0][x-0],blackLevel),SUB_SATURATE(in[2][x-0],blackLevel))
                ),
                MIN(          
                    MIN(SUB_SATURATE(in[0][x+1],blackLevel),SUB_SATURATE(in[2][x+1],blackLevel)),
                    MIN(SUB_SATURATE(in[1][x-1],blackLevel),SUB_SATURATE(in[1][x+1],blackLevel))
                )
            );  

            float fp = (float)SUB_SATURATE(in[1][x+0],blackLevel);
            float mxf = (float)maxim;
            float mnf = (float)minim;
            if (fp > alpha * mxf)
            {
                out[x] = maxim;
            }
            else 
            {
                if (fp < mnf/alpha )
                {
                    out[x] = minim;
                }
                else
                {
                    out[x] = SUB_SATURATE(in[1][x+0],blackLevel);
                }
            }            
        }

        // move to the next plane
        in[0] += fptr->parents[0]->planeStride;
        in[1] += fptr->parents[0]->planeStride;
        in[2] += fptr->parents[0]->planeStride;
        out += fptr->planeStride;
    }
}
