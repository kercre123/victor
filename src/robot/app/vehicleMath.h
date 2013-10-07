/**
 * File: vehicleMath.h
 *
 * Author: Kevin Yoon
 * Created: 22-OCT-2012
 * 
 *
 **/
#ifndef _VEHICLEMATH_H
#define _VEHICLEMATH_H

// When defined, uses smaller tables to save space at the cost of more computation (and higher accuracy)
#define USE_INTERPOLATION  

#define PI 3.1415926535898
#define PI_DIV2 1.5707963267949


// Arctangent function based on lookup table
float atan_fast(float x);

// Arcsine function based on lookup table
float asin_fast(float x);

#endif
