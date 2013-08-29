///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     Shave hello world source code
///

#include <stdio.h>
#include <math.h>
#include <vectorext.h>

int4 add_simd(int4 input1,int4 input2);

volatile int myint1[4], myint2[4], myintrez[4];
int4 myval1, myval2, myrez;


//unsigned char image4[30000];

//#pragma DATA_SECTION (".images")
unsigned char image1[30000] __attribute__ ((section (".images")));

//#pragma DATA_SECTION (".images")
unsigned char image2[30000] __attribute__ ((section (".images")));

//#pragma sDATA_SECTION (".images")
//unsigned char image3[30000] __attribute__ ((section (".images2")));

/*void nothing(unsigned char *image1, unsigned char *image2, unsigned char *image3)
{
	for(int i=0; i<(320*240); i++) {
        image2[i] += image1[i] - image3[i];
    }
}
*/

int main( void )
{


	myval1[0] = myint1[0];
	myval1[1] = myint1[1];
	myval1[2] = myint1[2];
	myval1[3] = myint1[3];

	myval2[0] = myint2[0];
	myval2[1] = myint2[1];
	myval2[2] = myint2[2];
	myval2[3] = myint2[3];

	myrez=add_simd(myval1,myval2);

	myintrez[0] = myrez[0];
	myintrez[1] = myrez[1];
	myintrez[2] = myrez[2];
	myintrez[3] = myrez[3];

/*
	for(int i=0; i<(320*240); i++) {
	    image1[i] = i;
	    image2[i] = i;
  	    image3[i] = i;
	}

	nothing(image1, image2, image3);

    myintrez[3] = myrez[3] + image2[500];
*/
printf("versions %d %d %d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);

	return 0;
}
