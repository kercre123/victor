#include <sipp.h>
#include <sippMacros.h>
#include <filters/randNoise/randNoise.h>

#define ONE_OVER_UINT32_MAX (1.0f/4294967295.0f)

//=============================================================================
//Algo from: http://en.wikipedia.org/wiki/Random_number_generation
//Another interesting link: http://www.rgba.org/articles/sfrand/sfrand.htm
//Use different seeds for each shave so we don't get repeating patterns
//Also use this storage to keep track from line to line.
UInt32 seeds[12][2] = 
{
  {0x39f4d2fa, 0x2d9cbf10},
  {0x126eb1d1, 0x32a39600},
  {0x1475525d, 0x80f7f9b5},
  {0x0427cf25, 0x51f28cb3},
  {0x29d6a2bc, 0x2b1bc9cd},
  {0x1a1dbe0a, 0x77b33da8},
  {0x241e7fbb, 0x2e95a583},
  {0x44066b1d, 0x4c5d77df},
  {0x09fee756, 0x035a1244},
  {0x3c3420b5, 0x042e1913},
  {0x334fee6b, 0x89315fc8},
  {0x2ceb7dca, 0x6faf23ad}
};

//=============================================================================
//Generate a random number betwen [0 and 1]
float getRandom(UInt32 *m_w, UInt32 *m_z)
{
    float ret;
    *m_z = 36969 * (*m_z & 65535) + (*m_z >> 16);
    *m_w = 18000 * (*m_w & 65535) + (*m_w >> 16);
    ret = ((*m_z << 16) + *m_w) * ONE_OVER_UINT32_MAX;
    return ret;
}

//#############################################################################
void svuGenNoise(SippFilter *fptr, int svuNo, int runNo)
{
    UInt32 i;

    UInt32 *mW = &seeds[svuNo][0];
    UInt32 *mZ = &seeds[svuNo][1];

    RandNoiseParam *param = (RandNoiseParam *)fptr->params;

  //Input Y line
    half  *inY  = (half *)WRESOLVE((void*)fptr->dbLinesIn[0][runNo&1][0], svuNo);
  //Output Y line
    half  *outY = (half *)WRESOLVE(fptr->dbLineOut[runNo&1],       svuNo);

  //Noise 
    for(i=0; i<fptr->sliceWidth; i++)
    {
        outY[i] = clamp01((inY[i] + (getRandom(mW, mZ) - 0.5f)*inY[i] * param->strength));
    }
} 
