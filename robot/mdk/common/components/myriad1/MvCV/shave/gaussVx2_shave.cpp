/******************************************************************************
 @File         : gaussVx2_shave.cpp
 @Author       : Marius Truica
 @Brief        : Containd a function that applay downscale 2x vertical with a gaussian filters with kernel 5x5. 
                 Have to be use in combination with GaussHx2 for obtain correct output. 
                 Gaussian 5x5 filter was decomposed in liniar 1x5, applayed horizontal and vertical
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
#include <gaussVx2.h>
#include <mvcv_internal.h>

#include <math.h>
#include <alloc.h>
#include <mvstl_utils.h>

namespace mvcv
{

void dummyGaussVx2()
{
}

#ifdef __PC__
void GaussVx2(u8 **inLine,u8 *outLine,int width)
{
    unsigned int gaus_matrix[3] = {16, 64,96 };
    int i;
    for (i = 0; i < width; i++)
    {
        outLine[i] = (((inLine[0][i] + inLine[4][i]) * gaus_matrix[0]) + ((inLine[1][i] + inLine[3][i]) * gaus_matrix[1]) + (inLine[2][i]  * gaus_matrix[2]))>>8;
    }
}
#endif

}

