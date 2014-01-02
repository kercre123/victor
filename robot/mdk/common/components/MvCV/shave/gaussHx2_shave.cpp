/******************************************************************************
 @File         : gaussHx2_shave.cpp
 @Author       : Marius Truica
 @Brief        : Containd a function that applay downscale 2x horizontal with a gaussian filters with kernel 5x5. 
                 Have to be use in combination with GaussVx2 for obtain correct output. 
                 Gaussian 5x5 filter was decomposed in liniar 1x5, applayed horizontal and vertical
                 Function not make replicate at margin, assumed the you have 3 more pixels on left and right from input buffer
 Date          : 10 - 01 - 2013
 E-mail        : marius.truica@movidius.com
 Copyright     : C Movidius Srl 2013, C Movidius Ltd 2013

HISTORY
 Version        | Date       | Owner         | Purpose
 ---------------------------------------------------------------------------
 1.0            | 10.09.2012 | Marius Truica    | First implementation
 1.1            | 14.09.2012 | Marius Truica    | in conformity with mail RE: 5x5 gaussian SIPP style - 9/13/2012, split the original file in 3, remove tabs
 2.0            | 10.01.2013 | Marius Truica    | in conformity with mail Gauss x2 unit tests... din 1/9/2013 3:16 PM, change name and integrated in MvCvtests
  ***************************************************************************/

#ifdef _WIN32
#include <iostream>
#endif

#include <mvcv_types.h>
#include <gaussHx2.h>
#include <mvcv_internal.h>

#include <math.h>
#include <alloc.h>
#include <mvstl_utils.h>

namespace mvcv
{

void dummyGaussHx2()
{
}

#ifdef __PC__
void GaussHx2(u8 *inLine,u8 *outLine,int width)
{
    unsigned int gaus_matrix[3] = {16, 64,96 };
    int i;
    for (i = 0; i<(width<<1);i+=2)
    {
        outLine[i>>1] = (((inLine[i-2] + inLine[i+2]) * gaus_matrix[0]) + ((inLine[i-1] + inLine[i+1]) * gaus_matrix[1]) + (inLine[i]  * gaus_matrix[2]))>>8;
    }
}
#endif

}

