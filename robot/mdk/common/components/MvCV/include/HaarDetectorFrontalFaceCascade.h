///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     
///
#ifndef __HAAR_DETECTOR_H
#define __HAAR_DETECTOR_H

#include <HaarCascade.h>

#ifdef __LEON__
 #include <Sections.h>
 #define MEMORY_TYPE DDR_DATA
#else
 #define MEMORY_TYPE
#endif

#define mvcvRect(_x, _y, _width, _height) _x, _y, _width, _height
#define mvcvCvSize(_x, _y)                {_x, _y}

HaarFeature MEMORY_TYPE haar_0_0[2] =
{
0, mvcvRect(2,7,16,4),-1.000000,mvcvRect(2,9,16,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,4,3,14),-1.000000,mvcvRect(8,11,3,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_0_0[2] =
{
0.004327,0.013076,
};
int MEMORY_TYPE feature_left_0_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_0_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_0_0[3] =
{
0.038382,0.896526,0.262931,
};
HaarFeature MEMORY_TYPE haar_0_1[2] =
{
0,mvcvRect(13,6,1,6),-1.000000,mvcvRect(13,9,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,2,12,8),-1.000000,mvcvRect(8,2,4,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_0_1[2] =
{
0.000524,0.004457,
};
int MEMORY_TYPE feature_left_0_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_0_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_0_1[3] =
{
0.102166,0.123840,0.691038,
};
HaarFeature MEMORY_TYPE haar_0_2[2] =
{
0,mvcvRect(6,3,1,9),-1.000000,mvcvRect(6,6,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,7,14,9),-1.000000,mvcvRect(3,10,14,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_0_2[2] =
{
-0.000927,0.000340,
};
int MEMORY_TYPE feature_left_0_2[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_0_2[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_0_2[3] =
{
0.195370,0.210144,0.825867,
};
HaarClassifier MEMORY_TYPE haar_feature_class_0[3] =
{
{2,haar_0_0,feature_thres_0_0,feature_left_0_0,feature_right_0_0,feature_alpha_0_0},{2,haar_0_1,feature_thres_0_1,feature_left_0_1,feature_right_0_1,feature_alpha_0_1},{2,haar_0_2,feature_thres_0_2,feature_left_0_2,feature_right_0_2,feature_alpha_0_2},
};
HaarFeature MEMORY_TYPE haar_1_0[2] =
{
0,mvcvRect(4,7,4,4),-1.000000,mvcvRect(4,9,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,4,2,16),-1.000000,mvcvRect(9,12,2,8),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_1_0[2] =
{
0.002303,0.004417,
};
int MEMORY_TYPE feature_left_1_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_1_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_1_0[3] =
{
0.101838,0.821906,0.195655,
};
HaarFeature MEMORY_TYPE haar_1_1[2] =
{
0,mvcvRect(1,1,18,5),-1.000000,mvcvRect(7,1,6,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,5,13,8),-1.000000,mvcvRect(4,9,13,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_1_1[2] =
{
0.022203,-0.000173,
};
int MEMORY_TYPE feature_left_1_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_1_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_1_1[3] =
{
0.220541,0.073263,0.593148,
};
HaarFeature MEMORY_TYPE haar_1_2[2] =
{
0,mvcvRect(1,7,16,9),-1.000000,mvcvRect(1,10,16,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,0,15,4),-1.000000,mvcvRect(2,2,15,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_1_2[2] =
{
0.004357,-0.002603,
};
int MEMORY_TYPE feature_left_1_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_1_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_1_2[3] =
{
0.184411,0.403221,0.806652,
};
HaarFeature MEMORY_TYPE haar_1_3[2] =
{
0,mvcvRect(7,5,6,4),-1.000000,mvcvRect(9,5,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,3,8,9),-1.000000,mvcvRect(6,6,8,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_1_3[2] =
{
0.001731,-0.007815,
};
int MEMORY_TYPE feature_left_1_3[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_1_3[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_1_3[3] =
{
0.254833,0.605707,0.277906,
};
HaarFeature MEMORY_TYPE haar_1_4[2] =
{
0,mvcvRect(8,12,3,8),-1.000000,mvcvRect(8,16,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,16,2,2),-1.000000,mvcvRect(3,17,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_1_4[2] =
{
-0.008734,0.000945,
};
int MEMORY_TYPE feature_left_1_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_1_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_1_4[3] =
{
0.288998,0.761659,0.349564,
};
HaarFeature MEMORY_TYPE haar_1_5[2] =
{
0,mvcvRect(14,1,6,12),-1.000000,mvcvRect(14,1,3,12),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,4,12,6),-1.000000,mvcvRect(8,4,4,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_1_5[2] =
{
0.049415,0.004489,
};
int MEMORY_TYPE feature_left_1_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_1_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_1_5[3] =
{
0.815165,0.280878,0.602777,
};
HaarFeature MEMORY_TYPE haar_1_6[2] =
{
0,mvcvRect(0,2,6,15),-1.000000,mvcvRect(3,2,3,15),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,9,6),-1.000000,mvcvRect(5,6,9,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_1_6[2] =
{
0.060314,-0.001076,
};
int MEMORY_TYPE feature_left_1_6[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_1_6[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_1_6[3] =
{
0.760750,0.444404,0.143731,
};
HaarFeature MEMORY_TYPE haar_1_7[2] =
{
0,mvcvRect(13,11,6,3),-1.000000,mvcvRect(13,12,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,12,6,4),-1.000000,mvcvRect(12,14,6,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_1_7[2] =
{
-0.009508,0.007660,
};
int MEMORY_TYPE feature_left_1_7[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_1_7[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_1_7[3] =
{
0.531817,0.541105,0.218069,
};
HaarFeature MEMORY_TYPE haar_1_8[2] =
{
0,mvcvRect(1,11,6,3),-1.000000,mvcvRect(1,12,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,5,5,8),-1.000000,mvcvRect(2,9,5,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_1_8[2] =
{
0.007647,-0.000847,
};
int MEMORY_TYPE feature_left_1_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_1_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_1_8[3] =
{
0.115896,0.234068,0.599038,
};
HaarClassifier MEMORY_TYPE haar_feature_class_1[9] =
{
{2,haar_1_0,feature_thres_1_0,feature_left_1_0,feature_right_1_0,feature_alpha_1_0},{2,haar_1_1,feature_thres_1_1,feature_left_1_1,feature_right_1_1,feature_alpha_1_1},{2,haar_1_2,feature_thres_1_2,feature_left_1_2,feature_right_1_2,feature_alpha_1_2},{2,haar_1_3,feature_thres_1_3,feature_left_1_3,feature_right_1_3,feature_alpha_1_3},{2,haar_1_4,feature_thres_1_4,feature_left_1_4,feature_right_1_4,feature_alpha_1_4},{2,haar_1_5,feature_thres_1_5,feature_left_1_5,feature_right_1_5,feature_alpha_1_5},{2,haar_1_6,feature_thres_1_6,feature_left_1_6,feature_right_1_6,feature_alpha_1_6},{2,haar_1_7,feature_thres_1_7,feature_left_1_7,feature_right_1_7,feature_alpha_1_7},{2,haar_1_8,feature_thres_1_8,feature_left_1_8,feature_right_1_8,feature_alpha_1_8},
};
HaarFeature MEMORY_TYPE haar_2_0[2] =
{
0,mvcvRect(5,4,10,4),-1.000000,mvcvRect(5,6,10,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,4,16,12),-1.000000,mvcvRect(2,8,16,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_0[2] =
{
-0.004851,-0.004614,
};
int MEMORY_TYPE feature_left_2_0[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_2_0[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_2_0[3] =
{
0.180550,0.217789,0.801824,
};
HaarFeature MEMORY_TYPE haar_2_1[2] =
{
0,mvcvRect(4,5,12,6),-1.000000,mvcvRect(8,5,4,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,7,2,9),-1.000000,mvcvRect(13,10,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_1[2] =
{
-0.002430,0.000418,
};
int MEMORY_TYPE feature_left_2_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_2_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_2_1[3] =
{
0.114135,0.120309,0.610853,
};
HaarFeature MEMORY_TYPE haar_2_2[2] =
{
0,mvcvRect(5,7,2,9),-1.000000,mvcvRect(5,10,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,1,6,8),-1.000000,mvcvRect(9,1,2,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_2[2] =
{
0.001001,0.001058,
};
int MEMORY_TYPE feature_left_2_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_2_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_2_2[3] =
{
0.207996,0.330205,0.751109,
};
HaarFeature MEMORY_TYPE haar_2_3[2] =
{
0,mvcvRect(12,0,4,12),-1.000000,mvcvRect(14,0,2,6),2.000000,mvcvRect(12,6,2,6),2.000000,0,mvcvRect(5,8,10,2),-1.000000,mvcvRect(5,9,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_3[2] =
{
0.001238,0.000353,
};
int MEMORY_TYPE feature_left_2_3[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_2_3[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_2_3[3] =
{
0.276822,0.166829,0.582948,
};
HaarFeature MEMORY_TYPE haar_2_4[2] =
{
0,mvcvRect(5,1,6,4),-1.000000,mvcvRect(7,1,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,3,9,12),-1.000000,mvcvRect(3,3,3,12),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_4[2] =
{
-0.011954,0.001418,
};
int MEMORY_TYPE feature_left_2_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_2_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_2_4[3] =
{
0.150879,0.439123,0.764660,
};
HaarFeature MEMORY_TYPE haar_2_5[2] =
{
0,mvcvRect(9,8,3,12),-1.000000,mvcvRect(9,12,3,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,5,20,15),-1.000000,mvcvRect(0,10,20,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_5[2] =
{
0.003464,-0.014949,
};
int MEMORY_TYPE feature_left_2_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_2_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_2_5[3] =
{
0.265156,0.229805,0.544217,
};
HaarFeature MEMORY_TYPE haar_2_6[2] =
{
0,mvcvRect(2,2,6,8),-1.000000,mvcvRect(2,2,3,4),2.000000,mvcvRect(5,6,3,4),2.000000,0,mvcvRect(2,1,6,2),-1.000000,mvcvRect(2,2,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_6[2] =
{
-0.001051,-0.004078,
};
int MEMORY_TYPE feature_left_2_6[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_2_6[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_2_6[3] =
{
0.362284,0.260126,0.723366,
};
HaarFeature MEMORY_TYPE haar_2_7[2] =
{
0,mvcvRect(10,15,6,4),-1.000000,mvcvRect(13,15,3,2),2.000000,mvcvRect(10,17,3,2),2.000000,0,mvcvRect(12,14,2,6),-1.000000,mvcvRect(12,16,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_7[2] =
{
0.000542,-0.007320,
};
int MEMORY_TYPE feature_left_2_7[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_2_7[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_2_7[3] =
{
0.384968,0.296551,0.548031,
};
HaarFeature MEMORY_TYPE haar_2_8[2] =
{
0,mvcvRect(5,15,4,4),-1.000000,mvcvRect(5,15,2,2),2.000000,mvcvRect(7,17,2,2),2.000000,0,mvcvRect(7,18,1,2),-1.000000,mvcvRect(7,19,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_8[2] =
{
0.001142,0.001178,
};
int MEMORY_TYPE feature_left_2_8[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_2_8[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_2_8[3] =
{
0.410477,0.723902,0.278728,
};
HaarFeature MEMORY_TYPE haar_2_9[2] =
{
0,mvcvRect(4,5,12,10),-1.000000,mvcvRect(10,5,6,5),2.000000,mvcvRect(4,10,6,5),2.000000,0,mvcvRect(7,4,8,12),-1.000000,mvcvRect(11,4,4,6),2.000000,mvcvRect(7,10,4,6),2.000000,
};
float MEMORY_TYPE feature_thres_2_9[2] =
{
0.044077,0.003790,
};
int MEMORY_TYPE feature_left_2_9[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_2_9[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_2_9[3] =
{
0.564052,0.594755,0.331202,
};
HaarFeature MEMORY_TYPE haar_2_10[2] =
{
0,mvcvRect(9,11,2,3),-1.000000,mvcvRect(9,12,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,3,12,12),-1.000000,mvcvRect(3,3,6,6),2.000000,mvcvRect(9,9,6,6),2.000000,
};
float MEMORY_TYPE feature_thres_2_10[2] =
{
-0.002429,0.009426,
};
int MEMORY_TYPE feature_left_2_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_2_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_2_10[3] =
{
0.660323,0.468067,0.206434,
};
HaarFeature MEMORY_TYPE haar_2_11[2] =
{
0,mvcvRect(15,11,5,3),-1.000000,mvcvRect(15,12,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,18,3,2),-1.000000,mvcvRect(11,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_11[2] =
{
0.008063,0.005224,
};
int MEMORY_TYPE feature_left_2_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_2_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_2_11[3] =
{
0.529885,0.528160,0.190955,
};
HaarFeature MEMORY_TYPE haar_2_12[2] =
{
0,mvcvRect(0,11,5,3),-1.000000,mvcvRect(0,12,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,18,3,2),-1.000000,mvcvRect(8,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_12[2] =
{
-0.007063,0.005690,
};
int MEMORY_TYPE feature_left_2_12[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_2_12[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_2_12[3] =
{
0.138065,0.549064,0.126028,
};
HaarFeature MEMORY_TYPE haar_2_13[2] =
{
0,mvcvRect(2,8,16,2),-1.000000,mvcvRect(2,9,16,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,6,5,12),-1.000000,mvcvRect(9,12,5,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_2_13[2] =
{
0.001247,0.049543,
};
int MEMORY_TYPE feature_left_2_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_2_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_2_13[3] =
{
0.237266,0.524017,0.176922,
};
HaarClassifier MEMORY_TYPE haar_feature_class_2[14] =
{
{2,haar_2_0,feature_thres_2_0,feature_left_2_0,feature_right_2_0,feature_alpha_2_0},{2,haar_2_1,feature_thres_2_1,feature_left_2_1,feature_right_2_1,feature_alpha_2_1},{2,haar_2_2,feature_thres_2_2,feature_left_2_2,feature_right_2_2,feature_alpha_2_2},{2,haar_2_3,feature_thres_2_3,feature_left_2_3,feature_right_2_3,feature_alpha_2_3},{2,haar_2_4,feature_thres_2_4,feature_left_2_4,feature_right_2_4,feature_alpha_2_4},{2,haar_2_5,feature_thres_2_5,feature_left_2_5,feature_right_2_5,feature_alpha_2_5},{2,haar_2_6,feature_thres_2_6,feature_left_2_6,feature_right_2_6,feature_alpha_2_6},{2,haar_2_7,feature_thres_2_7,feature_left_2_7,feature_right_2_7,feature_alpha_2_7},{2,haar_2_8,feature_thres_2_8,feature_left_2_8,feature_right_2_8,feature_alpha_2_8},{2,haar_2_9,feature_thres_2_9,feature_left_2_9,feature_right_2_9,feature_alpha_2_9},{2,haar_2_10,feature_thres_2_10,feature_left_2_10,feature_right_2_10,feature_alpha_2_10},{2,haar_2_11,feature_thres_2_11,feature_left_2_11,feature_right_2_11,feature_alpha_2_11},{2,haar_2_12,feature_thres_2_12,feature_left_2_12,feature_right_2_12,feature_alpha_2_12},{2,haar_2_13,feature_thres_2_13,feature_left_2_13,feature_right_2_13,feature_alpha_2_13},
};
HaarFeature MEMORY_TYPE haar_3_0[2] =
{
0,mvcvRect(6,3,8,6),-1.000000,mvcvRect(6,6,8,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,12,2),-1.000000,mvcvRect(8,7,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_0[2] =
{
-0.004933,0.000028,
};
int MEMORY_TYPE feature_left_3_0[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_3_0[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_3_0[3] =
{
0.199806,0.229938,0.739321,
};
HaarFeature MEMORY_TYPE haar_3_1[2] =
{
0,mvcvRect(10,9,6,8),-1.000000,mvcvRect(10,13,6,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,5,3,10),-1.000000,mvcvRect(12,10,3,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_1[2] =
{
0.003088,0.000007,
};
int MEMORY_TYPE feature_left_3_1[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_3_1[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_3_1[3] =
{
0.153384,0.203686,0.585492,
};
HaarFeature MEMORY_TYPE haar_3_2[2] =
{
0,mvcvRect(4,6,3,9),-1.000000,mvcvRect(4,9,3,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,4,6,4),-1.000000,mvcvRect(9,4,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_2[2] =
{
0.001874,0.000934,
};
int MEMORY_TYPE feature_left_3_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_3_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_3_2[3] =
{
0.204990,0.323420,0.732301,
};
HaarFeature MEMORY_TYPE haar_3_3[2] =
{
0,mvcvRect(12,3,8,3),-1.000000,mvcvRect(12,3,4,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,0,3,6),-1.000000,mvcvRect(15,3,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_3[2] =
{
0.001915,-0.005968,
};
int MEMORY_TYPE feature_left_3_3[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_3_3[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_3_3[3] =
{
0.304515,0.293213,0.562130,
};
HaarFeature MEMORY_TYPE haar_3_4[2] =
{
0,mvcvRect(2,12,10,8),-1.000000,mvcvRect(2,12,5,4),2.000000,mvcvRect(7,16,5,4),2.000000,0,mvcvRect(5,5,6,8),-1.000000,mvcvRect(5,9,6,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_4[2] =
{
-0.000721,-0.005966,
};
int MEMORY_TYPE feature_left_3_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_3_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_3_4[3] =
{
0.365804,0.271216,0.722633,
};
HaarFeature MEMORY_TYPE haar_3_5[2] =
{
0,mvcvRect(12,3,8,3),-1.000000,mvcvRect(12,3,4,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,0,3,6),-1.000000,mvcvRect(15,3,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_5[2] =
{
0.030874,-0.011100,
};
int MEMORY_TYPE feature_left_3_5[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_3_5[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_3_5[3] =
{
0.441984,0.361298,0.525145,
};
HaarFeature MEMORY_TYPE haar_3_6[2] =
{
0,mvcvRect(0,3,8,3),-1.000000,mvcvRect(4,3,4,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,1,4,4),-1.000000,mvcvRect(2,3,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_6[2] =
{
0.002116,-0.009432,
};
int MEMORY_TYPE feature_left_3_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_3_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_3_6[3] =
{
0.362862,0.160110,0.705228,
};
HaarFeature MEMORY_TYPE haar_3_7[2] =
{
0,mvcvRect(10,2,3,2),-1.000000,mvcvRect(11,2,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,3,3,1),-1.000000,mvcvRect(11,3,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_7[2] =
{
-0.003527,-0.001691,
};
int MEMORY_TYPE feature_left_3_7[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_3_7[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_3_7[3] =
{
0.130129,0.178632,0.552153,
};
HaarFeature MEMORY_TYPE haar_3_8[2] =
{
0,mvcvRect(7,15,3,4),-1.000000,mvcvRect(7,17,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,13,3,6),-1.000000,mvcvRect(4,15,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_8[2] =
{
0.000465,-0.010216,
};
int MEMORY_TYPE feature_left_3_8[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_3_8[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_3_8[3] =
{
0.348738,0.267399,0.666792,
};
HaarFeature MEMORY_TYPE haar_3_9[2] =
{
0,mvcvRect(10,5,1,14),-1.000000,mvcvRect(10,12,1,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,10,6),-1.000000,mvcvRect(5,6,10,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_9[2] =
{
0.001263,-0.011875,
};
int MEMORY_TYPE feature_left_3_9[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_3_9[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_3_9[3] =
{
0.343786,0.599534,0.349772,
};
HaarFeature MEMORY_TYPE haar_3_10[2] =
{
0,mvcvRect(5,0,6,3),-1.000000,mvcvRect(7,0,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,0,3,5),-1.000000,mvcvRect(7,0,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_10[2] =
{
-0.010732,0.007184,
};
int MEMORY_TYPE feature_left_3_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_3_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_3_10[3] =
{
0.215049,0.627144,0.251954,
};
HaarFeature MEMORY_TYPE haar_3_11[2] =
{
0,mvcvRect(7,15,6,5),-1.000000,mvcvRect(9,15,2,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,10,2,6),-1.000000,mvcvRect(9,12,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_11[2] =
{
-0.028341,-0.000458,
};
int MEMORY_TYPE feature_left_3_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_3_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_3_11[3] =
{
0.082412,0.591006,0.370520,
};
HaarFeature MEMORY_TYPE haar_3_12[2] =
{
0,mvcvRect(8,17,3,2),-1.000000,mvcvRect(9,17,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,12,7,6),-1.000000,mvcvRect(1,14,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_12[2] =
{
0.004294,0.010751,
};
int MEMORY_TYPE feature_left_3_12[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_3_12[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_3_12[3] =
{
0.159473,0.598048,0.283251,
};
HaarFeature MEMORY_TYPE haar_3_13[2] =
{
0,mvcvRect(9,6,3,7),-1.000000,mvcvRect(10,6,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,3,4,9),-1.000000,mvcvRect(16,6,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_13[2] =
{
0.022465,-0.057989,
};
int MEMORY_TYPE feature_left_3_13[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_3_13[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_3_13[3] =
{
0.787709,0.155574,0.523966,
};
HaarFeature MEMORY_TYPE haar_3_14[2] =
{
0,mvcvRect(8,6,3,7),-1.000000,mvcvRect(9,6,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,5,18,8),-1.000000,mvcvRect(0,5,9,4),2.000000,mvcvRect(9,9,9,4),2.000000,
};
float MEMORY_TYPE feature_thres_3_14[2] =
{
0.007211,-0.048368,
};
int MEMORY_TYPE feature_left_3_14[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_3_14[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_3_14[3] =
{
0.662037,0.142472,0.442983,
};
HaarFeature MEMORY_TYPE haar_3_15[2] =
{
0,mvcvRect(13,5,2,10),-1.000000,mvcvRect(13,10,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,10,2,6),-1.000000,mvcvRect(12,13,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_15[2] =
{
-0.014418,-0.023156,
};
int MEMORY_TYPE feature_left_3_15[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_3_15[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_3_15[3] =
{
0.158854,0.237580,0.521713,
};
HaarFeature MEMORY_TYPE haar_3_16[2] =
{
0,mvcvRect(7,0,3,5),-1.000000,mvcvRect(8,0,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,5,8,6),-1.000000,mvcvRect(6,7,8,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_16[2] =
{
0.007699,-0.005625,
};
int MEMORY_TYPE feature_left_3_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_3_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_3_16[3] =
{
0.194173,0.627841,0.374604,
};
HaarFeature MEMORY_TYPE haar_3_17[2] =
{
0,mvcvRect(10,3,6,14),-1.000000,mvcvRect(13,3,3,7),2.000000,mvcvRect(10,10,3,7),2.000000,0,mvcvRect(13,5,1,8),-1.000000,mvcvRect(13,9,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_17[2] =
{
-0.000729,0.000618,
};
int MEMORY_TYPE feature_left_3_17[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_3_17[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_3_17[3] =
{
0.384092,0.310649,0.553785,
};
HaarFeature MEMORY_TYPE haar_3_18[2] =
{
0,mvcvRect(4,3,6,14),-1.000000,mvcvRect(4,3,3,7),2.000000,mvcvRect(7,10,3,7),2.000000,0,mvcvRect(6,5,1,8),-1.000000,mvcvRect(6,9,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_3_18[2] =
{
-0.000046,-0.000015,
};
int MEMORY_TYPE feature_left_3_18[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_3_18[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_3_18[3] =
{
0.344445,0.272955,0.642895,
};
HaarClassifier MEMORY_TYPE haar_feature_class_3[19] =
{
{2,haar_3_0,feature_thres_3_0,feature_left_3_0,feature_right_3_0,feature_alpha_3_0},{2,haar_3_1,feature_thres_3_1,feature_left_3_1,feature_right_3_1,feature_alpha_3_1},{2,haar_3_2,feature_thres_3_2,feature_left_3_2,feature_right_3_2,feature_alpha_3_2},{2,haar_3_3,feature_thres_3_3,feature_left_3_3,feature_right_3_3,feature_alpha_3_3},{2,haar_3_4,feature_thres_3_4,feature_left_3_4,feature_right_3_4,feature_alpha_3_4},{2,haar_3_5,feature_thres_3_5,feature_left_3_5,feature_right_3_5,feature_alpha_3_5},{2,haar_3_6,feature_thres_3_6,feature_left_3_6,feature_right_3_6,feature_alpha_3_6},{2,haar_3_7,feature_thres_3_7,feature_left_3_7,feature_right_3_7,feature_alpha_3_7},{2,haar_3_8,feature_thres_3_8,feature_left_3_8,feature_right_3_8,feature_alpha_3_8},{2,haar_3_9,feature_thres_3_9,feature_left_3_9,feature_right_3_9,feature_alpha_3_9},{2,haar_3_10,feature_thres_3_10,feature_left_3_10,feature_right_3_10,feature_alpha_3_10},{2,haar_3_11,feature_thres_3_11,feature_left_3_11,feature_right_3_11,feature_alpha_3_11},{2,haar_3_12,feature_thres_3_12,feature_left_3_12,feature_right_3_12,feature_alpha_3_12},{2,haar_3_13,feature_thres_3_13,feature_left_3_13,feature_right_3_13,feature_alpha_3_13},{2,haar_3_14,feature_thres_3_14,feature_left_3_14,feature_right_3_14,feature_alpha_3_14},{2,haar_3_15,feature_thres_3_15,feature_left_3_15,feature_right_3_15,feature_alpha_3_15},{2,haar_3_16,feature_thres_3_16,feature_left_3_16,feature_right_3_16,feature_alpha_3_16},{2,haar_3_17,feature_thres_3_17,feature_left_3_17,feature_right_3_17,feature_alpha_3_17},{2,haar_3_18,feature_thres_3_18,feature_left_3_18,feature_right_3_18,feature_alpha_3_18},
};
HaarFeature MEMORY_TYPE haar_4_0[2] =
{
0,mvcvRect(8,1,1,6),-1.000000,mvcvRect(8,3,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,0,15,2),-1.000000,mvcvRect(2,1,15,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_0[2] =
{
-0.001347,-0.002477,
};
int MEMORY_TYPE feature_left_4_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_0[3] =
{
0.165709,0.227385,0.698935,
};
HaarFeature MEMORY_TYPE haar_4_1[2] =
{
0,mvcvRect(0,7,20,6),-1.000000,mvcvRect(0,9,20,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,10,6,8),-1.000000,mvcvRect(10,14,6,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_1[2] =
{
0.005263,0.004908,
};
int MEMORY_TYPE feature_left_4_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_1[3] =
{
0.151207,0.556447,0.160544,
};
HaarFeature MEMORY_TYPE haar_4_2[2] =
{
0,mvcvRect(7,1,3,2),-1.000000,mvcvRect(8,1,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,1,2,2),-1.000000,mvcvRect(9,1,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_2[2] =
{
-0.002325,-0.001467,
};
int MEMORY_TYPE feature_left_4_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_2[3] =
{
0.188026,0.312250,0.716540,
};
HaarFeature MEMORY_TYPE haar_4_3[2] =
{
0,mvcvRect(4,3,12,9),-1.000000,mvcvRect(4,6,12,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,5,9,5),-1.000000,mvcvRect(9,5,3,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_3[2] =
{
-0.123117,0.002211,
};
int MEMORY_TYPE feature_left_4_3[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_4_3[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_4_3[3] =
{
0.385958,0.245529,0.569571,
};
HaarFeature MEMORY_TYPE haar_4_4[2] =
{
0,mvcvRect(5,5,9,5),-1.000000,mvcvRect(8,5,3,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,6,6,12),-1.000000,mvcvRect(4,10,6,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_4[2] =
{
0.002066,0.000361,
};
int MEMORY_TYPE feature_left_4_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_4[3] =
{
0.271652,0.229336,0.720863,
};
HaarFeature MEMORY_TYPE haar_4_5[2] =
{
0,mvcvRect(13,0,6,18),-1.000000,mvcvRect(13,0,3,18),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,8,1,12),-1.000000,mvcvRect(10,12,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_5[2] =
{
0.079958,0.002606,
};
int MEMORY_TYPE feature_left_4_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_4_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_4_5[3] =
{
0.783362,0.554523,0.255069,
};
HaarFeature MEMORY_TYPE haar_4_6[2] =
{
0,mvcvRect(3,2,6,10),-1.000000,mvcvRect(3,2,3,5),2.000000,mvcvRect(6,7,3,5),2.000000,0,mvcvRect(1,2,4,6),-1.000000,mvcvRect(3,2,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_6[2] =
{
0.006570,0.001626,
};
int MEMORY_TYPE feature_left_4_6[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_4_6[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_4_6[3] =
{
0.181939,0.352988,0.655282,
};
HaarFeature MEMORY_TYPE haar_4_7[2] =
{
0,mvcvRect(9,18,3,2),-1.000000,mvcvRect(10,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,18,3,2),-1.000000,mvcvRect(11,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_7[2] =
{
0.003620,-0.004439,
};
int MEMORY_TYPE feature_left_4_7[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_7[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_7[3] =
{
0.546231,0.135984,0.541582,
};
HaarFeature MEMORY_TYPE haar_4_8[2] =
{
0,mvcvRect(2,8,2,6),-1.000000,mvcvRect(2,10,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,5,6,6),-1.000000,mvcvRect(7,7,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_8[2] =
{
-0.009054,-0.000461,
};
int MEMORY_TYPE feature_left_4_8[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_8[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_8[3] =
{
0.111512,0.584672,0.259835,
};
HaarFeature MEMORY_TYPE haar_4_9[2] =
{
0,mvcvRect(7,19,6,1),-1.000000,mvcvRect(9,19,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,18,3,2),-1.000000,mvcvRect(11,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_9[2] =
{
-0.005662,0.005117,
};
int MEMORY_TYPE feature_left_4_9[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_9[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_9[3] =
{
0.161057,0.537668,0.173946,
};
HaarFeature MEMORY_TYPE haar_4_10[2] =
{
0,mvcvRect(8,3,3,1),-1.000000,mvcvRect(9,3,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,2,16,2),-1.000000,mvcvRect(2,2,8,1),2.000000,mvcvRect(10,3,8,1),2.000000,
};
float MEMORY_TYPE feature_thres_4_10[2] =
{
-0.002136,-0.005481,
};
int MEMORY_TYPE feature_left_4_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_10[3] =
{
0.190207,0.327201,0.636484,
};
HaarFeature MEMORY_TYPE haar_4_11[2] =
{
0,mvcvRect(8,11,5,3),-1.000000,mvcvRect(8,12,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,13,6,3),-1.000000,mvcvRect(7,14,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_11[2] =
{
-0.008106,0.006005,
};
int MEMORY_TYPE feature_left_4_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_11[3] =
{
0.691485,0.432733,0.696384,
};
HaarFeature MEMORY_TYPE haar_4_12[2] =
{
0,mvcvRect(0,1,6,15),-1.000000,mvcvRect(2,1,2,15),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,12,2,3),-1.000000,mvcvRect(2,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_12[2] =
{
-0.087029,-0.004781,
};
int MEMORY_TYPE feature_left_4_12[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_12[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_12[3] =
{
0.859413,0.097394,0.458703,
};
HaarFeature MEMORY_TYPE haar_4_13[2] =
{
0,mvcvRect(16,13,1,3),-1.000000,mvcvRect(16,14,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,7,6,4),-1.000000,mvcvRect(16,7,3,2),2.000000,mvcvRect(13,9,3,2),2.000000,
};
float MEMORY_TYPE feature_thres_4_13[2] =
{
-0.002217,0.001364,
};
int MEMORY_TYPE feature_left_4_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_13[3] =
{
0.255463,0.331909,0.596410,
};
HaarFeature MEMORY_TYPE haar_4_14[2] =
{
0,mvcvRect(7,13,3,6),-1.000000,mvcvRect(7,16,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,5,1,14),-1.000000,mvcvRect(7,12,1,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_14[2] =
{
-0.009008,-0.015494,
};
int MEMORY_TYPE feature_left_4_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_4_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_4_14[3] =
{
0.266659,0.184819,0.624597,
};
HaarFeature MEMORY_TYPE haar_4_15[2] =
{
0,mvcvRect(15,12,2,3),-1.000000,mvcvRect(15,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,5,3,14),-1.000000,mvcvRect(10,12,3,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_15[2] =
{
-0.004217,0.043250,
};
int MEMORY_TYPE feature_left_4_15[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_4_15[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_4_15[3] =
{
0.537993,0.518303,0.217042,
};
HaarFeature MEMORY_TYPE haar_4_16[2] =
{
0,mvcvRect(6,10,2,6),-1.000000,mvcvRect(6,13,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,5,1,8),-1.000000,mvcvRect(6,9,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_16[2] =
{
0.000288,0.001237,
};
int MEMORY_TYPE feature_left_4_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_4_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_4_16[3] =
{
0.261338,0.278653,0.590899,
};
HaarFeature MEMORY_TYPE haar_4_17[2] =
{
0,mvcvRect(13,11,2,1),-1.000000,mvcvRect(13,11,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,1,6,10),-1.000000,mvcvRect(15,1,3,5),2.000000,mvcvRect(12,6,3,5),2.000000,
};
float MEMORY_TYPE feature_thres_4_17[2] =
{
0.001953,-0.001495,
};
int MEMORY_TYPE feature_left_4_17[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_4_17[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_4_17[3] =
{
0.261287,0.591541,0.345578,
};
HaarFeature MEMORY_TYPE haar_4_18[2] =
{
0,mvcvRect(3,12,2,3),-1.000000,mvcvRect(3,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,18,2,1),-1.000000,mvcvRect(10,18,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_4_18[2] =
{
0.003588,-0.002594,
};
int MEMORY_TYPE feature_left_4_18[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_4_18[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_4_18[3] =
{
0.158705,0.127041,0.597943,
};
HaarClassifier MEMORY_TYPE haar_feature_class_4[19] =
{
{2,haar_4_0,feature_thres_4_0,feature_left_4_0,feature_right_4_0,feature_alpha_4_0},{2,haar_4_1,feature_thres_4_1,feature_left_4_1,feature_right_4_1,feature_alpha_4_1},{2,haar_4_2,feature_thres_4_2,feature_left_4_2,feature_right_4_2,feature_alpha_4_2},{2,haar_4_3,feature_thres_4_3,feature_left_4_3,feature_right_4_3,feature_alpha_4_3},{2,haar_4_4,feature_thres_4_4,feature_left_4_4,feature_right_4_4,feature_alpha_4_4},{2,haar_4_5,feature_thres_4_5,feature_left_4_5,feature_right_4_5,feature_alpha_4_5},{2,haar_4_6,feature_thres_4_6,feature_left_4_6,feature_right_4_6,feature_alpha_4_6},{2,haar_4_7,feature_thres_4_7,feature_left_4_7,feature_right_4_7,feature_alpha_4_7},{2,haar_4_8,feature_thres_4_8,feature_left_4_8,feature_right_4_8,feature_alpha_4_8},{2,haar_4_9,feature_thres_4_9,feature_left_4_9,feature_right_4_9,feature_alpha_4_9},{2,haar_4_10,feature_thres_4_10,feature_left_4_10,feature_right_4_10,feature_alpha_4_10},{2,haar_4_11,feature_thres_4_11,feature_left_4_11,feature_right_4_11,feature_alpha_4_11},{2,haar_4_12,feature_thres_4_12,feature_left_4_12,feature_right_4_12,feature_alpha_4_12},{2,haar_4_13,feature_thres_4_13,feature_left_4_13,feature_right_4_13,feature_alpha_4_13},{2,haar_4_14,feature_thres_4_14,feature_left_4_14,feature_right_4_14,feature_alpha_4_14},{2,haar_4_15,feature_thres_4_15,feature_left_4_15,feature_right_4_15,feature_alpha_4_15},{2,haar_4_16,feature_thres_4_16,feature_left_4_16,feature_right_4_16,feature_alpha_4_16},{2,haar_4_17,feature_thres_4_17,feature_left_4_17,feature_right_4_17,feature_alpha_4_17},{2,haar_4_18,feature_thres_4_18,feature_left_4_18,feature_right_4_18,feature_alpha_4_18},
};
HaarFeature MEMORY_TYPE haar_5_0[2] =
{
0,mvcvRect(1,0,17,9),-1.000000,mvcvRect(1,3,17,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,2,8,8),-1.000000,mvcvRect(1,2,4,4),2.000000,mvcvRect(5,6,4,4),2.000000,
};
float MEMORY_TYPE feature_thres_5_0[2] =
{
0.003581,-0.002855,
};
int MEMORY_TYPE feature_left_5_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_0[3] =
{
0.199510,0.737307,0.292174,
};
HaarFeature MEMORY_TYPE haar_5_1[2] =
{
0,mvcvRect(9,5,6,4),-1.000000,mvcvRect(9,5,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,9,7,10),-1.000000,mvcvRect(10,14,7,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_1[2] =
{
0.001976,0.003258,
};
int MEMORY_TYPE feature_left_5_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_1[3] =
{
0.195642,0.569205,0.183906,
};
HaarFeature MEMORY_TYPE haar_5_2[2] =
{
0,mvcvRect(5,5,6,4),-1.000000,mvcvRect(8,5,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,7,20,6),-1.000000,mvcvRect(0,9,20,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_2[2] =
{
0.000237,0.002594,
};
int MEMORY_TYPE feature_left_5_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_2[3] =
{
0.217167,0.271999,0.715024,
};
HaarFeature MEMORY_TYPE haar_5_3[2] =
{
0,mvcvRect(6,5,9,10),-1.000000,mvcvRect(6,10,9,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,4,4,12),-1.000000,mvcvRect(8,10,4,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_3[2] =
{
-0.025032,0.006309,
};
int MEMORY_TYPE feature_left_5_3[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_3[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_3[3] =
{
0.182518,0.569984,0.350985,
};
HaarFeature MEMORY_TYPE haar_5_4[2] =
{
0,mvcvRect(6,6,8,3),-1.000000,mvcvRect(6,7,8,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,13,10,6),-1.000000,mvcvRect(3,13,5,3),2.000000,mvcvRect(8,16,5,3),2.000000,
};
float MEMORY_TYPE feature_thres_5_4[2] =
{
-0.003249,-0.014886,
};
int MEMORY_TYPE feature_left_5_4[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_5_4[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_5_4[3] =
{
0.402393,0.360410,0.729200,
};
HaarFeature MEMORY_TYPE haar_5_5[2] =
{
0,mvcvRect(15,1,4,11),-1.000000,mvcvRect(15,1,2,11),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,7,10,10),-1.000000,mvcvRect(10,7,5,5),2.000000,mvcvRect(5,12,5,5),2.000000,
};
float MEMORY_TYPE feature_thres_5_5[2] =
{
0.008062,0.027406,
};
int MEMORY_TYPE feature_left_5_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_5_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_5_5[3] =
{
0.649149,0.551899,0.265968,
};
HaarFeature MEMORY_TYPE haar_5_6[2] =
{
0,mvcvRect(1,1,4,11),-1.000000,mvcvRect(3,1,2,11),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,5,8,12),-1.000000,mvcvRect(1,11,8,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_6[2] =
{
0.034369,-0.027293,
};
int MEMORY_TYPE feature_left_5_6[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_5_6[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_5_6[3] =
{
0.671251,0.169138,0.432628,
};
HaarFeature MEMORY_TYPE haar_5_7[2] =
{
0,mvcvRect(13,7,6,4),-1.000000,mvcvRect(16,7,3,2),2.000000,mvcvRect(13,9,3,2),2.000000,0,mvcvRect(11,10,7,4),-1.000000,mvcvRect(11,12,7,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_7[2] =
{
0.000745,0.000703,
};
int MEMORY_TYPE feature_left_5_7[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_7[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_7[3] =
{
0.340510,0.551679,0.331139,
};
HaarFeature MEMORY_TYPE haar_5_8[2] =
{
0,mvcvRect(0,4,20,12),-1.000000,mvcvRect(0,4,10,6),2.000000,mvcvRect(10,10,10,6),2.000000,0,mvcvRect(1,5,6,15),-1.000000,mvcvRect(1,10,6,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_8[2] =
{
-0.122755,0.003256,
};
int MEMORY_TYPE feature_left_5_8[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_8[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_8[3] =
{
0.167532,0.361575,0.642078,
};
HaarFeature MEMORY_TYPE haar_5_9[2] =
{
0,mvcvRect(11,10,3,8),-1.000000,mvcvRect(11,14,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,12,7,6),-1.000000,mvcvRect(11,14,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_9[2] =
{
-0.032090,0.003296,
};
int MEMORY_TYPE feature_left_5_9[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_9[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_9[3] =
{
0.292108,0.561303,0.335786,
};
HaarFeature MEMORY_TYPE haar_5_10[2] =
{
0,mvcvRect(9,11,2,3),-1.000000,mvcvRect(9,12,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,13,4,3),-1.000000,mvcvRect(8,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_10[2] =
{
-0.003227,0.001117,
};
int MEMORY_TYPE feature_left_5_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_10[3] =
{
0.697064,0.354115,0.614401,
};
HaarFeature MEMORY_TYPE haar_5_11[2] =
{
0,mvcvRect(3,14,14,4),-1.000000,mvcvRect(10,14,7,2),2.000000,mvcvRect(3,16,7,2),2.000000,0,mvcvRect(18,7,2,4),-1.000000,mvcvRect(18,9,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_11[2] =
{
-0.017280,0.011741,
};
int MEMORY_TYPE feature_left_5_11[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_5_11[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_5_11[3] =
{
0.553718,0.534196,0.275710,
};
HaarFeature MEMORY_TYPE haar_5_12[2] =
{
0,mvcvRect(3,12,6,6),-1.000000,mvcvRect(3,14,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,4,3,6),-1.000000,mvcvRect(0,6,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_12[2] =
{
0.004641,-0.016913,
};
int MEMORY_TYPE feature_left_5_12[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_5_12[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_5_12[3] =
{
0.248952,0.171193,0.552395,
};
HaarFeature MEMORY_TYPE haar_5_13[2] =
{
0,mvcvRect(9,14,3,3),-1.000000,mvcvRect(9,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,7,10,4),-1.000000,mvcvRect(15,7,5,2),2.000000,mvcvRect(10,9,5,2),2.000000,
};
float MEMORY_TYPE feature_thres_5_13[2] =
{
0.010060,-0.000607,
};
int MEMORY_TYPE feature_left_5_13[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_5_13[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_5_13[3] =
{
0.827345,0.377939,0.547625,
};
HaarFeature MEMORY_TYPE haar_5_14[2] =
{
0,mvcvRect(7,2,6,8),-1.000000,mvcvRect(7,6,6,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,3,6,2),-1.000000,mvcvRect(8,3,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_14[2] =
{
-0.001087,0.008936,
};
int MEMORY_TYPE feature_left_5_14[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_5_14[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_5_14[3] =
{
0.329654,0.606288,0.243422,
};
HaarFeature MEMORY_TYPE haar_5_15[2] =
{
0,mvcvRect(10,6,3,5),-1.000000,mvcvRect(11,6,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,0,6,19),-1.000000,mvcvRect(11,0,2,19),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_15[2] =
{
-0.000264,0.013110,
};
int MEMORY_TYPE feature_left_5_15[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_5_15[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_5_15[3] =
{
0.381409,0.551762,0.372689,
};
HaarFeature MEMORY_TYPE haar_5_16[2] =
{
0,mvcvRect(3,12,1,2),-1.000000,mvcvRect(3,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,14,5,3),-1.000000,mvcvRect(7,15,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_16[2] =
{
-0.002981,-0.004162,
};
int MEMORY_TYPE feature_left_5_16[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_16[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_16[3] =
{
0.122966,0.725227,0.497346,
};
HaarFeature MEMORY_TYPE haar_5_17[2] =
{
0,mvcvRect(2,1,18,4),-1.000000,mvcvRect(11,1,9,2),2.000000,mvcvRect(2,3,9,2),2.000000,0,mvcvRect(10,5,3,8),-1.000000,mvcvRect(11,5,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_17[2] =
{
0.033842,-0.001256,
};
int MEMORY_TYPE feature_left_5_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_17[3] =
{
0.534831,0.585191,0.438417,
};
HaarFeature MEMORY_TYPE haar_5_18[2] =
{
0,mvcvRect(0,1,18,4),-1.000000,mvcvRect(0,1,9,2),2.000000,mvcvRect(9,3,9,2),2.000000,0,mvcvRect(7,5,3,8),-1.000000,mvcvRect(8,5,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_18[2] =
{
-0.019635,-0.000996,
};
int MEMORY_TYPE feature_left_5_18[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_18[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_18[3] =
{
0.229783,0.629594,0.413160,
};
HaarFeature MEMORY_TYPE haar_5_19[2] =
{
0,mvcvRect(9,5,2,6),-1.000000,mvcvRect(9,7,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,8,5,2),-1.000000,mvcvRect(10,9,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_19[2] =
{
-0.023127,0.023526,
};
int MEMORY_TYPE feature_left_5_19[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_19[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_19[3] =
{
0.169546,0.517413,0.059519,
};
HaarFeature MEMORY_TYPE haar_5_20[2] =
{
0,mvcvRect(2,10,15,1),-1.000000,mvcvRect(7,10,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,7,2,6),-1.000000,mvcvRect(2,9,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_20[2] =
{
-0.019357,-0.004179,
};
int MEMORY_TYPE feature_left_5_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_20[3] =
{
0.135725,0.299663,0.579170,
};
HaarFeature MEMORY_TYPE haar_5_21[2] =
{
0,mvcvRect(9,14,3,3),-1.000000,mvcvRect(9,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,4,10),-1.000000,mvcvRect(9,12,4,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_21[2] =
{
0.003149,0.007397,
};
int MEMORY_TYPE feature_left_5_21[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_5_21[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_5_21[3] =
{
0.659259,0.530717,0.379512,
};
HaarFeature MEMORY_TYPE haar_5_22[2] =
{
0,mvcvRect(0,8,8,2),-1.000000,mvcvRect(0,8,4,1),2.000000,mvcvRect(4,9,4,1),2.000000,0,mvcvRect(5,9,10,8),-1.000000,mvcvRect(5,9,5,4),2.000000,mvcvRect(10,13,5,4),2.000000,
};
float MEMORY_TYPE feature_thres_5_22[2] =
{
0.000007,0.047114,
};
int MEMORY_TYPE feature_left_5_22[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_22[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_22[3] =
{
0.312831,0.553789,0.102731,
};
HaarFeature MEMORY_TYPE haar_5_23[2] =
{
0,mvcvRect(9,7,2,4),-1.000000,mvcvRect(9,7,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,6,3,4),-1.000000,mvcvRect(10,6,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_23[2] =
{
0.007288,-0.006189,
};
int MEMORY_TYPE feature_left_5_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_23[3] =
{
0.466086,0.715886,0.472445,
};
HaarFeature MEMORY_TYPE haar_5_24[2] =
{
0,mvcvRect(8,3,2,1),-1.000000,mvcvRect(9,3,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,6,3,4),-1.000000,mvcvRect(9,6,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_24[2] =
{
0.002976,-0.001845,
};
int MEMORY_TYPE feature_left_5_24[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_5_24[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_5_24[3] =
{
0.059346,0.702730,0.471873,
};
HaarFeature MEMORY_TYPE haar_5_25[2] =
{
0,mvcvRect(12,0,4,14),-1.000000,mvcvRect(14,0,2,7),2.000000,mvcvRect(12,7,2,7),2.000000,0,mvcvRect(12,5,6,9),-1.000000,mvcvRect(12,5,3,9),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_25[2] =
{
0.000102,0.002428,
};
int MEMORY_TYPE feature_left_5_25[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_25[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_25[3] =
{
0.589473,0.486236,0.524759,
};
HaarFeature MEMORY_TYPE haar_5_26[2] =
{
0,mvcvRect(0,2,6,16),-1.000000,mvcvRect(3,2,3,16),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,12,4,2),-1.000000,mvcvRect(1,13,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_5_26[2] =
{
-0.064751,0.000394,
};
int MEMORY_TYPE feature_left_5_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_5_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_5_26[3] =
{
0.691747,0.466962,0.238241,
};
HaarClassifier MEMORY_TYPE haar_feature_class_5[27] =
{
{2,haar_5_0,feature_thres_5_0,feature_left_5_0,feature_right_5_0,feature_alpha_5_0},{2,haar_5_1,feature_thres_5_1,feature_left_5_1,feature_right_5_1,feature_alpha_5_1},{2,haar_5_2,feature_thres_5_2,feature_left_5_2,feature_right_5_2,feature_alpha_5_2},{2,haar_5_3,feature_thres_5_3,feature_left_5_3,feature_right_5_3,feature_alpha_5_3},{2,haar_5_4,feature_thres_5_4,feature_left_5_4,feature_right_5_4,feature_alpha_5_4},{2,haar_5_5,feature_thres_5_5,feature_left_5_5,feature_right_5_5,feature_alpha_5_5},{2,haar_5_6,feature_thres_5_6,feature_left_5_6,feature_right_5_6,feature_alpha_5_6},{2,haar_5_7,feature_thres_5_7,feature_left_5_7,feature_right_5_7,feature_alpha_5_7},{2,haar_5_8,feature_thres_5_8,feature_left_5_8,feature_right_5_8,feature_alpha_5_8},{2,haar_5_9,feature_thres_5_9,feature_left_5_9,feature_right_5_9,feature_alpha_5_9},{2,haar_5_10,feature_thres_5_10,feature_left_5_10,feature_right_5_10,feature_alpha_5_10},{2,haar_5_11,feature_thres_5_11,feature_left_5_11,feature_right_5_11,feature_alpha_5_11},{2,haar_5_12,feature_thres_5_12,feature_left_5_12,feature_right_5_12,feature_alpha_5_12},{2,haar_5_13,feature_thres_5_13,feature_left_5_13,feature_right_5_13,feature_alpha_5_13},{2,haar_5_14,feature_thres_5_14,feature_left_5_14,feature_right_5_14,feature_alpha_5_14},{2,haar_5_15,feature_thres_5_15,feature_left_5_15,feature_right_5_15,feature_alpha_5_15},{2,haar_5_16,feature_thres_5_16,feature_left_5_16,feature_right_5_16,feature_alpha_5_16},{2,haar_5_17,feature_thres_5_17,feature_left_5_17,feature_right_5_17,feature_alpha_5_17},{2,haar_5_18,feature_thres_5_18,feature_left_5_18,feature_right_5_18,feature_alpha_5_18},{2,haar_5_19,feature_thres_5_19,feature_left_5_19,feature_right_5_19,feature_alpha_5_19},{2,haar_5_20,feature_thres_5_20,feature_left_5_20,feature_right_5_20,feature_alpha_5_20},{2,haar_5_21,feature_thres_5_21,feature_left_5_21,feature_right_5_21,feature_alpha_5_21},{2,haar_5_22,feature_thres_5_22,feature_left_5_22,feature_right_5_22,feature_alpha_5_22},{2,haar_5_23,feature_thres_5_23,feature_left_5_23,feature_right_5_23,feature_alpha_5_23},{2,haar_5_24,feature_thres_5_24,feature_left_5_24,feature_right_5_24,feature_alpha_5_24},{2,haar_5_25,feature_thres_5_25,feature_left_5_25,feature_right_5_25,feature_alpha_5_25},{2,haar_5_26,feature_thres_5_26,feature_left_5_26,feature_right_5_26,feature_alpha_5_26},
};
HaarFeature MEMORY_TYPE haar_6_0[2] =
{
0,mvcvRect(7,7,6,1),-1.000000,mvcvRect(9,7,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,3,4,9),-1.000000,mvcvRect(8,6,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_0[2] =
{
0.001440,-0.000541,
};
int MEMORY_TYPE feature_left_6_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_0[3] =
{
0.277347,0.742715,0.247974,
};
HaarFeature MEMORY_TYPE haar_6_1[2] =
{
0,mvcvRect(12,10,4,6),-1.000000,mvcvRect(12,13,4,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,1,8,16),-1.000000,mvcvRect(12,1,4,8),2.000000,mvcvRect(8,9,4,8),2.000000,
};
float MEMORY_TYPE feature_thres_6_1[2] =
{
-0.000007,-0.002366,
};
int MEMORY_TYPE feature_left_6_1[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_1[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_1[3] =
{
0.219950,0.588999,0.259572,
};
HaarFeature MEMORY_TYPE haar_6_2[2] =
{
0,mvcvRect(4,6,3,6),-1.000000,mvcvRect(4,9,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,3,6,2),-1.000000,mvcvRect(4,3,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_2[2] =
{
0.001734,0.001587,
};
int MEMORY_TYPE feature_left_6_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_2[3] =
{
0.186013,0.415187,0.710347,
};
HaarFeature MEMORY_TYPE haar_6_3[2] =
{
0,mvcvRect(9,8,3,12),-1.000000,mvcvRect(9,12,3,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,9,7,10),-1.000000,mvcvRect(10,14,7,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_3[2] =
{
0.003729,-0.128838,
};
int MEMORY_TYPE feature_left_6_3[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_3[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_3[3] =
{
0.252797,0.139300,0.525451,
};
HaarFeature MEMORY_TYPE haar_6_4[2] =
{
0,mvcvRect(3,9,7,10),-1.000000,mvcvRect(3,14,7,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,5,1,14),-1.000000,mvcvRect(7,12,1,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_4[2] =
{
0.007941,-0.012662,
};
int MEMORY_TYPE feature_left_6_4[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_4[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_4[3] =
{
0.248773,0.271070,0.661884,
};
HaarFeature MEMORY_TYPE haar_6_5[2] =
{
0,mvcvRect(13,14,1,6),-1.000000,mvcvRect(13,16,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,12,3,6),-1.000000,mvcvRect(14,14,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_5[2] =
{
0.000030,-0.016330,
};
int MEMORY_TYPE feature_left_6_5[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_5[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_5[3] =
{
0.381283,0.232643,0.526301,
};
HaarFeature MEMORY_TYPE haar_6_6[2] =
{
0,mvcvRect(6,14,1,6),-1.000000,mvcvRect(6,16,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,12,3,6),-1.000000,mvcvRect(3,14,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_6[2] =
{
0.000015,-0.020859,
};
int MEMORY_TYPE feature_left_6_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_6[3] =
{
0.429333,0.160040,0.678231,
};
HaarFeature MEMORY_TYPE haar_6_7[2] =
{
0,mvcvRect(8,13,5,3),-1.000000,mvcvRect(8,14,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,14,2,3),-1.000000,mvcvRect(9,15,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_7[2] =
{
0.002819,0.003790,
};
int MEMORY_TYPE feature_left_6_7[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_7[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_7[3] =
{
0.667929,0.458771,0.717624,
};
HaarFeature MEMORY_TYPE haar_6_8[2] =
{
0,mvcvRect(5,1,10,8),-1.000000,mvcvRect(5,1,5,4),2.000000,mvcvRect(10,5,5,4),2.000000,0,mvcvRect(6,4,5,4),-1.000000,mvcvRect(6,6,5,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_8[2] =
{
0.035345,-0.001157,
};
int MEMORY_TYPE feature_left_6_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_8[3] =
{
0.186408,0.553826,0.315045,
};
HaarFeature MEMORY_TYPE haar_6_9[2] =
{
0,mvcvRect(1,10,18,1),-1.000000,mvcvRect(7,10,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,10,4,3),-1.000000,mvcvRect(11,10,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_9[2] =
{
-0.005874,-0.000015,
};
int MEMORY_TYPE feature_left_6_9[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_9[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_9[3] =
{
0.282879,0.587022,0.370482,
};
HaarFeature MEMORY_TYPE haar_6_10[2] =
{
0,mvcvRect(5,11,6,1),-1.000000,mvcvRect(7,11,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,13,2,3),-1.000000,mvcvRect(3,14,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_10[2] =
{
-0.000227,0.003785,
};
int MEMORY_TYPE feature_left_6_10[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_10[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_10[3] =
{
0.421893,0.666700,0.246118,
};
HaarFeature MEMORY_TYPE haar_6_11[2] =
{
0,mvcvRect(12,12,3,4),-1.000000,mvcvRect(12,14,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,10,5,6),-1.000000,mvcvRect(11,12,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_11[2] =
{
-0.000085,-0.044395,
};
int MEMORY_TYPE feature_left_6_11[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_11[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_11[3] =
{
0.355759,0.166555,0.523485,
};
HaarFeature MEMORY_TYPE haar_6_12[2] =
{
0,mvcvRect(0,8,16,2),-1.000000,mvcvRect(0,9,16,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,1,3,4),-1.000000,mvcvRect(2,3,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_12[2] =
{
0.001013,-0.007633,
};
int MEMORY_TYPE feature_left_6_12[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_12[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_12[3] =
{
0.288461,0.296934,0.608011,
};
HaarFeature MEMORY_TYPE haar_6_13[2] =
{
0,mvcvRect(9,7,3,3),-1.000000,mvcvRect(10,7,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,12,6),-1.000000,mvcvRect(9,6,4,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_13[2] =
{
0.004033,0.136767,
};
int MEMORY_TYPE feature_left_6_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_13[3] =
{
0.453639,0.517726,0.144918,
};
HaarFeature MEMORY_TYPE haar_6_14[2] =
{
0,mvcvRect(8,7,3,3),-1.000000,mvcvRect(9,7,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,6,12,6),-1.000000,mvcvRect(7,6,4,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_14[2] =
{
-0.005006,-0.012476,
};
int MEMORY_TYPE feature_left_6_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_14[3] =
{
0.761691,0.215971,0.546019,
};
HaarFeature MEMORY_TYPE haar_6_15[2] =
{
0,mvcvRect(10,5,6,5),-1.000000,mvcvRect(12,5,2,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,7,10,2),-1.000000,mvcvRect(5,7,5,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_15[2] =
{
-0.000940,-0.012192,
};
int MEMORY_TYPE feature_left_6_15[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_15[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_15[3] =
{
0.392630,0.347888,0.554266,
};
HaarFeature MEMORY_TYPE haar_6_16[2] =
{
0,mvcvRect(4,5,6,5),-1.000000,mvcvRect(6,5,2,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,3,2,10),-1.000000,mvcvRect(9,8,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_16[2] =
{
-0.000550,-0.000218,
};
int MEMORY_TYPE feature_left_6_16[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_16[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_16[3] =
{
0.606428,0.569741,0.177971,
};
HaarFeature MEMORY_TYPE haar_6_17[2] =
{
0,mvcvRect(3,1,16,2),-1.000000,mvcvRect(11,1,8,1),2.000000,mvcvRect(3,2,8,1),2.000000,0,mvcvRect(9,9,3,2),-1.000000,mvcvRect(9,10,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_17[2] =
{
0.006912,-0.000976,
};
int MEMORY_TYPE feature_left_6_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_17[3] =
{
0.537937,0.332784,0.546153,
};
HaarFeature MEMORY_TYPE haar_6_18[2] =
{
0,mvcvRect(1,1,16,2),-1.000000,mvcvRect(1,1,8,1),2.000000,mvcvRect(9,2,8,1),2.000000,0,mvcvRect(8,14,1,3),-1.000000,mvcvRect(8,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_18[2] =
{
-0.008787,-0.001676,
};
int MEMORY_TYPE feature_left_6_18[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_18[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_18[3] =
{
0.211616,0.663582,0.436586,
};
HaarFeature MEMORY_TYPE haar_6_19[2] =
{
0,mvcvRect(4,5,12,10),-1.000000,mvcvRect(10,5,6,5),2.000000,mvcvRect(4,10,6,5),2.000000,0,mvcvRect(7,13,6,6),-1.000000,mvcvRect(10,13,3,3),2.000000,mvcvRect(7,16,3,3),2.000000,
};
float MEMORY_TYPE feature_thres_6_19[2] =
{
-0.055695,-0.019844,
};
int MEMORY_TYPE feature_left_6_19[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_19[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_19[3] =
{
0.538742,0.160280,0.533046,
};
HaarFeature MEMORY_TYPE haar_6_20[2] =
{
0,mvcvRect(8,9,3,2),-1.000000,mvcvRect(8,10,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,2,6,4),-1.000000,mvcvRect(9,2,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_20[2] =
{
-0.000748,0.023033,
};
int MEMORY_TYPE feature_left_6_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_20[3] =
{
0.291748,0.560812,0.199798,
};
HaarFeature MEMORY_TYPE haar_6_21[2] =
{
0,mvcvRect(6,6,9,3),-1.000000,mvcvRect(6,7,9,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,7,6,1),-1.000000,mvcvRect(12,7,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_21[2] =
{
-0.003070,-0.001164,
};
int MEMORY_TYPE feature_left_6_21[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_21[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_21[3] =
{
0.393831,0.575744,0.423946,
};
HaarFeature MEMORY_TYPE haar_6_22[2] =
{
0,mvcvRect(0,0,18,6),-1.000000,mvcvRect(6,0,6,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,10,2,6),-1.000000,mvcvRect(6,13,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_22[2] =
{
0.224643,0.001441,
};
int MEMORY_TYPE feature_left_6_22[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_22[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_22[3] =
{
0.767655,0.535387,0.251478,
};
HaarFeature MEMORY_TYPE haar_6_23[2] =
{
0,mvcvRect(11,12,3,6),-1.000000,mvcvRect(11,15,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,4,12,12),-1.000000,mvcvRect(10,4,6,6),2.000000,mvcvRect(4,10,6,6),2.000000,
};
float MEMORY_TYPE feature_thres_6_23[2] =
{
-0.030011,-0.053079,
};
int MEMORY_TYPE feature_left_6_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_23[3] =
{
0.236490,0.238586,0.541466,
};
HaarFeature MEMORY_TYPE haar_6_24[2] =
{
0,mvcvRect(1,2,3,6),-1.000000,mvcvRect(2,2,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,5,3,7),-1.000000,mvcvRect(2,5,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_24[2] =
{
0.002080,-0.004074,
};
int MEMORY_TYPE feature_left_6_24[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_24[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_24[3] =
{
0.651161,0.603041,0.358770,
};
HaarFeature MEMORY_TYPE haar_6_25[2] =
{
0,mvcvRect(4,13,12,4),-1.000000,mvcvRect(10,13,6,2),2.000000,mvcvRect(4,15,6,2),2.000000,0,mvcvRect(3,3,17,12),-1.000000,mvcvRect(3,9,17,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_25[2] =
{
-0.019529,-0.053309,
};
int MEMORY_TYPE feature_left_6_25[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_25[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_25[3] =
{
0.542359,0.236095,0.540176,
};
HaarFeature MEMORY_TYPE haar_6_26[2] =
{
0,mvcvRect(3,3,14,12),-1.000000,mvcvRect(3,3,7,6),2.000000,mvcvRect(10,9,7,6),2.000000,0,mvcvRect(2,11,16,9),-1.000000,mvcvRect(2,14,16,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_26[2] =
{
-0.034850,-0.126585,
};
int MEMORY_TYPE feature_left_6_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_26[3] =
{
0.283699,0.181352,0.542105,
};
HaarFeature MEMORY_TYPE haar_6_27[2] =
{
0,mvcvRect(9,14,3,6),-1.000000,mvcvRect(9,17,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,14,4,6),-1.000000,mvcvRect(10,14,2,3),2.000000,mvcvRect(8,17,2,3),2.000000,
};
float MEMORY_TYPE feature_thres_6_27[2] =
{
0.000007,-0.011844,
};
int MEMORY_TYPE feature_left_6_27[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_27[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_27[3] =
{
0.398037,0.261638,0.523773,
};
HaarFeature MEMORY_TYPE haar_6_28[2] =
{
0,mvcvRect(6,2,6,1),-1.000000,mvcvRect(8,2,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,5,2,5),-1.000000,mvcvRect(10,5,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_28[2] =
{
-0.004847,0.008169,
};
int MEMORY_TYPE feature_left_6_28[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_6_28[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_6_28[3] =
{
0.243811,0.532715,0.819038,
};
HaarFeature MEMORY_TYPE haar_6_29[2] =
{
0,mvcvRect(9,8,3,5),-1.000000,mvcvRect(10,8,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,12,6,1),-1.000000,mvcvRect(9,12,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_29[2] =
{
-0.006472,-0.000015,
};
int MEMORY_TYPE feature_left_6_29[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_29[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_29[3] =
{
0.467969,0.556391,0.436759,
};
HaarFeature MEMORY_TYPE haar_6_30[2] =
{
0,mvcvRect(8,8,3,5),-1.000000,mvcvRect(9,8,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,10,4,3),-1.000000,mvcvRect(8,10,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_6_30[2] =
{
0.003070,-0.000163,
};
int MEMORY_TYPE feature_left_6_30[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_6_30[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_6_30[3] =
{
0.666435,0.559461,0.304271,
};
HaarClassifier MEMORY_TYPE haar_feature_class_6[31] =
{
{2,haar_6_0,feature_thres_6_0,feature_left_6_0,feature_right_6_0,feature_alpha_6_0},{2,haar_6_1,feature_thres_6_1,feature_left_6_1,feature_right_6_1,feature_alpha_6_1},{2,haar_6_2,feature_thres_6_2,feature_left_6_2,feature_right_6_2,feature_alpha_6_2},{2,haar_6_3,feature_thres_6_3,feature_left_6_3,feature_right_6_3,feature_alpha_6_3},{2,haar_6_4,feature_thres_6_4,feature_left_6_4,feature_right_6_4,feature_alpha_6_4},{2,haar_6_5,feature_thres_6_5,feature_left_6_5,feature_right_6_5,feature_alpha_6_5},{2,haar_6_6,feature_thres_6_6,feature_left_6_6,feature_right_6_6,feature_alpha_6_6},{2,haar_6_7,feature_thres_6_7,feature_left_6_7,feature_right_6_7,feature_alpha_6_7},{2,haar_6_8,feature_thres_6_8,feature_left_6_8,feature_right_6_8,feature_alpha_6_8},{2,haar_6_9,feature_thres_6_9,feature_left_6_9,feature_right_6_9,feature_alpha_6_9},{2,haar_6_10,feature_thres_6_10,feature_left_6_10,feature_right_6_10,feature_alpha_6_10},{2,haar_6_11,feature_thres_6_11,feature_left_6_11,feature_right_6_11,feature_alpha_6_11},{2,haar_6_12,feature_thres_6_12,feature_left_6_12,feature_right_6_12,feature_alpha_6_12},{2,haar_6_13,feature_thres_6_13,feature_left_6_13,feature_right_6_13,feature_alpha_6_13},{2,haar_6_14,feature_thres_6_14,feature_left_6_14,feature_right_6_14,feature_alpha_6_14},{2,haar_6_15,feature_thres_6_15,feature_left_6_15,feature_right_6_15,feature_alpha_6_15},{2,haar_6_16,feature_thres_6_16,feature_left_6_16,feature_right_6_16,feature_alpha_6_16},{2,haar_6_17,feature_thres_6_17,feature_left_6_17,feature_right_6_17,feature_alpha_6_17},{2,haar_6_18,feature_thres_6_18,feature_left_6_18,feature_right_6_18,feature_alpha_6_18},{2,haar_6_19,feature_thres_6_19,feature_left_6_19,feature_right_6_19,feature_alpha_6_19},{2,haar_6_20,feature_thres_6_20,feature_left_6_20,feature_right_6_20,feature_alpha_6_20},{2,haar_6_21,feature_thres_6_21,feature_left_6_21,feature_right_6_21,feature_alpha_6_21},{2,haar_6_22,feature_thres_6_22,feature_left_6_22,feature_right_6_22,feature_alpha_6_22},{2,haar_6_23,feature_thres_6_23,feature_left_6_23,feature_right_6_23,feature_alpha_6_23},{2,haar_6_24,feature_thres_6_24,feature_left_6_24,feature_right_6_24,feature_alpha_6_24},{2,haar_6_25,feature_thres_6_25,feature_left_6_25,feature_right_6_25,feature_alpha_6_25},{2,haar_6_26,feature_thres_6_26,feature_left_6_26,feature_right_6_26,feature_alpha_6_26},{2,haar_6_27,feature_thres_6_27,feature_left_6_27,feature_right_6_27,feature_alpha_6_27},{2,haar_6_28,feature_thres_6_28,feature_left_6_28,feature_right_6_28,feature_alpha_6_28},{2,haar_6_29,feature_thres_6_29,feature_left_6_29,feature_right_6_29,feature_alpha_6_29},{2,haar_6_30,feature_thres_6_30,feature_left_6_30,feature_right_6_30,feature_alpha_6_30},
};
HaarFeature MEMORY_TYPE haar_7_0[2] =
{
0,mvcvRect(0,4,20,6),-1.000000,mvcvRect(0,6,20,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,3,8,6),-1.000000,mvcvRect(1,3,4,3),2.000000,mvcvRect(5,6,4,3),2.000000,
};
float MEMORY_TYPE feature_thres_7_0[2] =
{
-0.009828,-0.004169,
};
int MEMORY_TYPE feature_left_7_0[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_0[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_0[3] =
{
0.211602,0.692469,0.304378,
};
HaarFeature MEMORY_TYPE haar_7_1[2] =
{
0,mvcvRect(7,15,6,4),-1.000000,mvcvRect(7,17,6,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,10,14,10),-1.000000,mvcvRect(3,15,14,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_1[2] =
{
0.000353,0.004805,
};
int MEMORY_TYPE feature_left_7_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_1[3] =
{
0.318329,0.545656,0.252227,
};
HaarFeature MEMORY_TYPE haar_7_2[2] =
{
0,mvcvRect(6,4,4,4),-1.000000,mvcvRect(8,4,2,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,4,20,10),-1.000000,mvcvRect(0,9,20,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_2[2] =
{
0.000211,-0.002832,
};
int MEMORY_TYPE feature_left_7_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_2[3] =
{
0.290262,0.313046,0.688494,
};
HaarFeature MEMORY_TYPE haar_7_3[2] =
{
0,mvcvRect(9,4,2,14),-1.000000,mvcvRect(9,11,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,0,16,4),-1.000000,mvcvRect(2,2,16,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_3[2] =
{
-0.000008,-0.000829,
};
int MEMORY_TYPE feature_left_7_3[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_3[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_3[3] =
{
0.296247,0.309963,0.575252,
};
HaarFeature MEMORY_TYPE haar_7_4[2] =
{
0,mvcvRect(4,12,6,8),-1.000000,mvcvRect(4,12,3,4),2.000000,mvcvRect(7,16,3,4),2.000000,0,mvcvRect(0,5,6,7),-1.000000,mvcvRect(3,5,3,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_4[2] =
{
0.001621,0.009134,
};
int MEMORY_TYPE feature_left_7_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_4[3] =
{
0.399320,0.482737,0.753783,
};
HaarFeature MEMORY_TYPE haar_7_5[2] =
{
0,mvcvRect(10,7,10,4),-1.000000,mvcvRect(15,7,5,2),2.000000,mvcvRect(10,9,5,2),2.000000,0,mvcvRect(5,8,12,1),-1.000000,mvcvRect(9,8,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_5[2] =
{
-0.004121,-0.002545,
};
int MEMORY_TYPE feature_left_7_5[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_5[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_5[3] =
{
0.261693,0.310870,0.549124,
};
HaarFeature MEMORY_TYPE haar_7_6[2] =
{
0,mvcvRect(9,9,2,2),-1.000000,mvcvRect(9,10,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,4,2,4),-1.000000,mvcvRect(9,6,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_6[2] =
{
-0.000627,-0.000037,
};
int MEMORY_TYPE feature_left_7_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_6[3] =
{
0.323969,0.651741,0.417891,
};
HaarFeature MEMORY_TYPE haar_7_7[2] =
{
0,mvcvRect(9,6,3,6),-1.000000,mvcvRect(10,6,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,7,6,4),-1.000000,mvcvRect(15,7,3,2),2.000000,mvcvRect(12,9,3,2),2.000000,
};
float MEMORY_TYPE feature_thres_7_7[2] =
{
0.013883,0.001049,
};
int MEMORY_TYPE feature_left_7_7[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_7[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_7[3] =
{
0.677120,0.415951,0.565289,
};
HaarFeature MEMORY_TYPE haar_7_8[2] =
{
0,mvcvRect(8,6,3,6),-1.000000,mvcvRect(9,6,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,6,18,6),-1.000000,mvcvRect(1,6,9,3),2.000000,mvcvRect(10,9,9,3),2.000000,
};
float MEMORY_TYPE feature_thres_7_8[2] =
{
0.018215,-0.011335,
};
int MEMORY_TYPE feature_left_7_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_8[3] =
{
0.768960,0.287332,0.498893,
};
HaarFeature MEMORY_TYPE haar_7_9[2] =
{
0,mvcvRect(9,1,3,3),-1.000000,mvcvRect(10,1,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,8,5,2),-1.000000,mvcvRect(10,9,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_9[2] =
{
-0.004110,0.000426,
};
int MEMORY_TYPE feature_left_7_9[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_9[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_9[3] =
{
0.546301,0.363124,0.551255,
};
HaarFeature MEMORY_TYPE haar_7_10[2] =
{
0,mvcvRect(8,1,3,3),-1.000000,mvcvRect(9,1,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,8,5,2),-1.000000,mvcvRect(5,9,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_10[2] =
{
0.006030,0.000336,
};
int MEMORY_TYPE feature_left_7_10[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_10[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_10[3] =
{
0.114377,0.289108,0.544734,
};
HaarFeature MEMORY_TYPE haar_7_11[2] =
{
0,mvcvRect(8,6,8,8),-1.000000,mvcvRect(12,6,4,4),2.000000,mvcvRect(8,10,4,4),2.000000,0,mvcvRect(5,7,10,2),-1.000000,mvcvRect(5,7,5,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_11[2] =
{
0.000623,-0.025837,
};
int MEMORY_TYPE feature_left_7_11[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_11[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_11[3] =
{
0.302343,0.216701,0.527815,
};
HaarFeature MEMORY_TYPE haar_7_12[2] =
{
0,mvcvRect(4,5,12,10),-1.000000,mvcvRect(4,5,6,5),2.000000,mvcvRect(10,10,6,5),2.000000,0,mvcvRect(5,5,2,3),-1.000000,mvcvRect(5,6,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_12[2] =
{
0.021775,0.001768,
};
int MEMORY_TYPE feature_left_7_12[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_12[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_12[3] =
{
0.325483,0.526305,0.752633,
};
HaarFeature MEMORY_TYPE haar_7_13[2] =
{
0,mvcvRect(7,14,6,3),-1.000000,mvcvRect(7,15,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,14,3,3),-1.000000,mvcvRect(9,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_13[2] =
{
-0.013794,-0.005085,
};
int MEMORY_TYPE feature_left_7_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_13[3] =
{
0.741033,0.683661,0.457907,
};
HaarFeature MEMORY_TYPE haar_7_14[2] =
{
0,mvcvRect(8,14,3,3),-1.000000,mvcvRect(8,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,10,8,9),-1.000000,mvcvRect(1,13,8,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_14[2] =
{
0.006180,0.010030,
};
int MEMORY_TYPE feature_left_7_14[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_14[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_14[3] =
{
0.744994,0.486078,0.236146,
};
HaarFeature MEMORY_TYPE haar_7_15[2] =
{
0,mvcvRect(9,7,2,3),-1.000000,mvcvRect(9,8,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,3,3,3),-1.000000,mvcvRect(13,3,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_15[2] =
{
-0.006420,-0.005696,
};
int MEMORY_TYPE feature_left_7_15[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_15[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_15[3] =
{
0.146733,0.234782,0.532338,
};
HaarFeature MEMORY_TYPE haar_7_16[2] =
{
0,mvcvRect(5,3,3,3),-1.000000,mvcvRect(6,3,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,2,12),-1.000000,mvcvRect(5,10,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_16[2] =
{
-0.007150,0.002445,
};
int MEMORY_TYPE feature_left_7_16[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_16[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_16[3] =
{
0.147706,0.349853,0.580356,
};
HaarFeature MEMORY_TYPE haar_7_17[2] =
{
0,mvcvRect(1,11,18,4),-1.000000,mvcvRect(10,11,9,2),2.000000,mvcvRect(1,13,9,2),2.000000,0,mvcvRect(7,12,6,2),-1.000000,mvcvRect(7,13,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_17[2] =
{
-0.037503,0.000478,
};
int MEMORY_TYPE feature_left_7_17[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_17[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_17[3] =
{
0.525955,0.436288,0.620892,
};
HaarFeature MEMORY_TYPE haar_7_18[2] =
{
0,mvcvRect(6,0,3,6),-1.000000,mvcvRect(7,0,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,11,18,4),-1.000000,mvcvRect(0,11,9,2),2.000000,mvcvRect(9,13,9,2),2.000000,
};
float MEMORY_TYPE feature_thres_7_18[2] =
{
-0.007081,0.032818,
};
int MEMORY_TYPE feature_left_7_18[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_18[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_18[3] =
{
0.203946,0.519836,0.137120,
};
HaarFeature MEMORY_TYPE haar_7_19[2] =
{
0,mvcvRect(7,12,6,2),-1.000000,mvcvRect(7,13,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,12,3,3),-1.000000,mvcvRect(9,13,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_19[2] =
{
0.000652,0.004649,
};
int MEMORY_TYPE feature_left_7_19[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_19[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_19[3] =
{
0.632343,0.472016,0.656709,
};
HaarFeature MEMORY_TYPE haar_7_20[2] =
{
0,mvcvRect(9,12,2,3),-1.000000,mvcvRect(9,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,11,4,3),-1.000000,mvcvRect(8,12,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_20[2] =
{
-0.001983,-0.001601,
};
int MEMORY_TYPE feature_left_7_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_20[3] =
{
0.605306,0.509052,0.311693,
};
HaarFeature MEMORY_TYPE haar_7_21[2] =
{
0,mvcvRect(13,3,4,2),-1.000000,mvcvRect(13,4,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,0,12,2),-1.000000,mvcvRect(4,1,12,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_21[2] =
{
-0.003054,0.000432,
};
int MEMORY_TYPE feature_left_7_21[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_21[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_21[3] =
{
0.342980,0.383840,0.577560,
};
HaarFeature MEMORY_TYPE haar_7_22[2] =
{
0,mvcvRect(6,9,8,8),-1.000000,mvcvRect(6,9,4,4),2.000000,mvcvRect(10,13,4,4),2.000000,0,mvcvRect(1,11,6,2),-1.000000,mvcvRect(1,12,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_22[2] =
{
-0.027452,0.000931,
};
int MEMORY_TYPE feature_left_7_22[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_22[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_22[3] =
{
0.214347,0.595297,0.376016,
};
HaarFeature MEMORY_TYPE haar_7_23[2] =
{
0,mvcvRect(2,5,18,8),-1.000000,mvcvRect(11,5,9,4),2.000000,mvcvRect(2,9,9,4),2.000000,0,mvcvRect(7,1,6,10),-1.000000,mvcvRect(7,6,6,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_23[2] =
{
0.006714,-0.003370,
};
int MEMORY_TYPE feature_left_7_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_23[3] =
{
0.569263,0.578430,0.397428,
};
HaarFeature MEMORY_TYPE haar_7_24[2] =
{
0,mvcvRect(0,3,3,6),-1.000000,mvcvRect(0,5,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,5,4,3),-1.000000,mvcvRect(4,6,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_24[2] =
{
-0.018904,-0.006585,
};
int MEMORY_TYPE feature_left_7_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_24[3] =
{
0.181889,0.684911,0.435158,
};
HaarFeature MEMORY_TYPE haar_7_25[2] =
{
0,mvcvRect(19,3,1,6),-1.000000,mvcvRect(19,5,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,15,8,2),-1.000000,mvcvRect(6,16,8,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_25[2] =
{
0.005881,0.000801,
};
int MEMORY_TYPE feature_left_7_25[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_25[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_25[3] =
{
0.272666,0.423643,0.584468,
};
HaarFeature MEMORY_TYPE haar_7_26[2] =
{
0,mvcvRect(0,3,1,6),-1.000000,mvcvRect(0,5,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,3,3),-1.000000,mvcvRect(5,6,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_26[2] =
{
0.001851,0.006327,
};
int MEMORY_TYPE feature_left_7_26[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_26[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_26[3] =
{
0.337132,0.527022,0.805365,
};
HaarFeature MEMORY_TYPE haar_7_27[2] =
{
0,mvcvRect(8,8,4,3),-1.000000,mvcvRect(8,9,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,6,6,3),-1.000000,mvcvRect(12,6,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_27[2] =
{
-0.003382,-0.001929,
};
int MEMORY_TYPE feature_left_7_27[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_27[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_27[3] =
{
0.286602,0.588895,0.389579,
};
HaarFeature MEMORY_TYPE haar_7_28[2] =
{
0,mvcvRect(8,13,2,6),-1.000000,mvcvRect(8,16,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,11,2,8),-1.000000,mvcvRect(9,15,2,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_28[2] =
{
0.014995,-0.026331,
};
int MEMORY_TYPE feature_left_7_28[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_28[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_28[3] =
{
0.217782,0.177532,0.567147,
};
HaarFeature MEMORY_TYPE haar_7_29[2] =
{
0,mvcvRect(10,6,6,3),-1.000000,mvcvRect(12,6,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,15,15,5),-1.000000,mvcvRect(10,15,5,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_29[2] =
{
-0.004173,0.027268,
};
int MEMORY_TYPE feature_left_7_29[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_29[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_29[3] =
{
0.465296,0.476831,0.569524,
};
HaarFeature MEMORY_TYPE haar_7_30[2] =
{
0,mvcvRect(2,14,2,2),-1.000000,mvcvRect(2,15,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,6,2),-1.000000,mvcvRect(6,7,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_30[2] =
{
0.000989,-0.001053,
};
int MEMORY_TYPE feature_left_7_30[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_30[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_30[3] =
{
0.339740,0.625004,0.428841,
};
HaarFeature MEMORY_TYPE haar_7_31[2] =
{
0,mvcvRect(8,3,6,1),-1.000000,mvcvRect(10,3,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,0,18,12),-1.000000,mvcvRect(7,0,6,12),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_31[2] =
{
0.005229,0.030395,
};
int MEMORY_TYPE feature_left_7_31[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_31[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_31[3] =
{
0.534776,0.411552,0.566075,
};
HaarFeature MEMORY_TYPE haar_7_32[2] =
{
0,mvcvRect(0,14,8,6),-1.000000,mvcvRect(4,14,4,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,15,15,5),-1.000000,mvcvRect(5,15,5,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_32[2] =
{
-0.079114,0.018232,
};
int MEMORY_TYPE feature_left_7_32[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_32[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_32[3] =
{
0.788132,0.360434,0.556951,
};
HaarFeature MEMORY_TYPE haar_7_33[2] =
{
0,mvcvRect(8,3,6,1),-1.000000,mvcvRect(10,3,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,11,3,6),-1.000000,mvcvRect(11,14,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_33[2] =
{
0.005229,0.000439,
};
int MEMORY_TYPE feature_left_7_33[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_33[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_33[3] =
{
0.541664,0.550716,0.388228,
};
HaarFeature MEMORY_TYPE haar_7_34[2] =
{
0,mvcvRect(6,3,6,1),-1.000000,mvcvRect(8,3,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,11,3,6),-1.000000,mvcvRect(6,14,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_34[2] =
{
-0.000865,0.001033,
};
int MEMORY_TYPE feature_left_7_34[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_34[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_34[3] =
{
0.318585,0.557836,0.321925,
};
HaarFeature MEMORY_TYPE haar_7_35[2] =
{
0,mvcvRect(9,6,3,4),-1.000000,mvcvRect(10,6,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,10,4,7),-1.000000,mvcvRect(12,10,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_35[2] =
{
-0.007300,-0.000936,
};
int MEMORY_TYPE feature_left_7_35[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_35[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_35[3] =
{
0.707323,0.555802,0.461384,
};
HaarFeature MEMORY_TYPE haar_7_36[2] =
{
0,mvcvRect(8,6,3,4),-1.000000,mvcvRect(9,6,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,6,4,7),-1.000000,mvcvRect(6,6,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_36[2] =
{
-0.006048,0.006753,
};
int MEMORY_TYPE feature_left_7_36[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_36[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_36[3] =
{
0.686929,0.487032,0.265037,
};
HaarFeature MEMORY_TYPE haar_7_37[2] =
{
0,mvcvRect(10,3,4,12),-1.000000,mvcvRect(10,3,2,12),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,8,3,4),-1.000000,mvcvRect(11,8,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_37[2] =
{
0.053078,-0.001023,
};
int MEMORY_TYPE feature_left_7_37[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_7_37[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_7_37[3] =
{
0.528152,0.608588,0.430487,
};
HaarFeature MEMORY_TYPE haar_7_38[2] =
{
0,mvcvRect(1,0,18,14),-1.000000,mvcvRect(7,0,6,14),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,8,6,11),-1.000000,mvcvRect(5,8,3,11),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_7_38[2] =
{
0.031271,-0.006352,
};
int MEMORY_TYPE feature_left_7_38[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_7_38[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_7_38[3] =
{
0.544583,0.532834,0.236432,
};
HaarClassifier MEMORY_TYPE haar_feature_class_7[39] =
{
{2,haar_7_0,feature_thres_7_0,feature_left_7_0,feature_right_7_0,feature_alpha_7_0},{2,haar_7_1,feature_thres_7_1,feature_left_7_1,feature_right_7_1,feature_alpha_7_1},{2,haar_7_2,feature_thres_7_2,feature_left_7_2,feature_right_7_2,feature_alpha_7_2},{2,haar_7_3,feature_thres_7_3,feature_left_7_3,feature_right_7_3,feature_alpha_7_3},{2,haar_7_4,feature_thres_7_4,feature_left_7_4,feature_right_7_4,feature_alpha_7_4},{2,haar_7_5,feature_thres_7_5,feature_left_7_5,feature_right_7_5,feature_alpha_7_5},{2,haar_7_6,feature_thres_7_6,feature_left_7_6,feature_right_7_6,feature_alpha_7_6},{2,haar_7_7,feature_thres_7_7,feature_left_7_7,feature_right_7_7,feature_alpha_7_7},{2,haar_7_8,feature_thres_7_8,feature_left_7_8,feature_right_7_8,feature_alpha_7_8},{2,haar_7_9,feature_thres_7_9,feature_left_7_9,feature_right_7_9,feature_alpha_7_9},{2,haar_7_10,feature_thres_7_10,feature_left_7_10,feature_right_7_10,feature_alpha_7_10},{2,haar_7_11,feature_thres_7_11,feature_left_7_11,feature_right_7_11,feature_alpha_7_11},{2,haar_7_12,feature_thres_7_12,feature_left_7_12,feature_right_7_12,feature_alpha_7_12},{2,haar_7_13,feature_thres_7_13,feature_left_7_13,feature_right_7_13,feature_alpha_7_13},{2,haar_7_14,feature_thres_7_14,feature_left_7_14,feature_right_7_14,feature_alpha_7_14},{2,haar_7_15,feature_thres_7_15,feature_left_7_15,feature_right_7_15,feature_alpha_7_15},{2,haar_7_16,feature_thres_7_16,feature_left_7_16,feature_right_7_16,feature_alpha_7_16},{2,haar_7_17,feature_thres_7_17,feature_left_7_17,feature_right_7_17,feature_alpha_7_17},{2,haar_7_18,feature_thres_7_18,feature_left_7_18,feature_right_7_18,feature_alpha_7_18},{2,haar_7_19,feature_thres_7_19,feature_left_7_19,feature_right_7_19,feature_alpha_7_19},{2,haar_7_20,feature_thres_7_20,feature_left_7_20,feature_right_7_20,feature_alpha_7_20},{2,haar_7_21,feature_thres_7_21,feature_left_7_21,feature_right_7_21,feature_alpha_7_21},{2,haar_7_22,feature_thres_7_22,feature_left_7_22,feature_right_7_22,feature_alpha_7_22},{2,haar_7_23,feature_thres_7_23,feature_left_7_23,feature_right_7_23,feature_alpha_7_23},{2,haar_7_24,feature_thres_7_24,feature_left_7_24,feature_right_7_24,feature_alpha_7_24},{2,haar_7_25,feature_thres_7_25,feature_left_7_25,feature_right_7_25,feature_alpha_7_25},{2,haar_7_26,feature_thres_7_26,feature_left_7_26,feature_right_7_26,feature_alpha_7_26},{2,haar_7_27,feature_thres_7_27,feature_left_7_27,feature_right_7_27,feature_alpha_7_27},{2,haar_7_28,feature_thres_7_28,feature_left_7_28,feature_right_7_28,feature_alpha_7_28},{2,haar_7_29,feature_thres_7_29,feature_left_7_29,feature_right_7_29,feature_alpha_7_29},{2,haar_7_30,feature_thres_7_30,feature_left_7_30,feature_right_7_30,feature_alpha_7_30},{2,haar_7_31,feature_thres_7_31,feature_left_7_31,feature_right_7_31,feature_alpha_7_31},{2,haar_7_32,feature_thres_7_32,feature_left_7_32,feature_right_7_32,feature_alpha_7_32},{2,haar_7_33,feature_thres_7_33,feature_left_7_33,feature_right_7_33,feature_alpha_7_33},{2,haar_7_34,feature_thres_7_34,feature_left_7_34,feature_right_7_34,feature_alpha_7_34},{2,haar_7_35,feature_thres_7_35,feature_left_7_35,feature_right_7_35,feature_alpha_7_35},{2,haar_7_36,feature_thres_7_36,feature_left_7_36,feature_right_7_36,feature_alpha_7_36},{2,haar_7_37,feature_thres_7_37,feature_left_7_37,feature_right_7_37,feature_alpha_7_37},{2,haar_7_38,feature_thres_7_38,feature_left_7_38,feature_right_7_38,feature_alpha_7_38},
};
HaarFeature MEMORY_TYPE haar_8_0[2] =
{
0,mvcvRect(1,4,15,4),-1.000000,mvcvRect(1,6,15,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,10,8),-1.000000,mvcvRect(5,9,10,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_0[2] =
{
-0.006222,0.002110,
};
int MEMORY_TYPE feature_left_8_0[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_0[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_0[3] =
{
0.262558,0.156499,0.679288,
};
HaarFeature MEMORY_TYPE haar_8_1[2] =
{
0,mvcvRect(14,2,6,8),-1.000000,mvcvRect(14,2,3,8),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,6,6,14),-1.000000,mvcvRect(14,6,3,7),2.000000,mvcvRect(11,13,3,7),2.000000,
};
float MEMORY_TYPE feature_thres_8_1[2] =
{
0.010846,0.000642,
};
int MEMORY_TYPE feature_left_8_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_1[3] =
{
0.348581,0.369826,0.592166,
};
HaarFeature MEMORY_TYPE haar_8_2[2] =
{
0,mvcvRect(9,5,2,12),-1.000000,mvcvRect(9,11,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,7,4,6),-1.000000,mvcvRect(3,9,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_2[2] =
{
0.000733,0.001013,
};
int MEMORY_TYPE feature_left_8_2[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_2[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_2[3] =
{
0.300708,0.362492,0.707243,
};
HaarFeature MEMORY_TYPE haar_8_3[2] =
{
0,mvcvRect(14,3,6,6),-1.000000,mvcvRect(14,3,3,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,2,4,4),-1.000000,mvcvRect(15,4,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_3[2] =
{
0.011094,-0.007913,
};
int MEMORY_TYPE feature_left_8_3[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_3[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_3[3] =
{
0.441670,0.302871,0.541738,
};
HaarFeature MEMORY_TYPE haar_8_4[2] =
{
0,mvcvRect(0,2,6,7),-1.000000,mvcvRect(3,2,3,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,6,6,14),-1.000000,mvcvRect(3,6,3,7),2.000000,mvcvRect(6,13,3,7),2.000000,
};
float MEMORY_TYPE feature_thres_8_4[2] =
{
0.012905,-0.004243,
};
int MEMORY_TYPE feature_left_8_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_4[3] =
{
0.437450,0.440159,0.756519,
};
HaarFeature MEMORY_TYPE haar_8_5[2] =
{
0,mvcvRect(4,6,16,8),-1.000000,mvcvRect(4,10,16,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,12,2,8),-1.000000,mvcvRect(10,16,2,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_5[2] =
{
-0.000213,-0.002231,
};
int MEMORY_TYPE feature_left_8_5[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_5[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_5[3] =
{
0.231079,0.356820,0.575000,
};
HaarFeature MEMORY_TYPE haar_8_6[2] =
{
0,mvcvRect(7,0,6,20),-1.000000,mvcvRect(9,0,2,20),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,7,16,12),-1.000000,mvcvRect(1,7,8,6),2.000000,mvcvRect(9,13,8,6),2.000000,
};
float MEMORY_TYPE feature_thres_8_6[2] =
{
0.002640,0.075101,
};
int MEMORY_TYPE feature_left_8_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_6[3] =
{
0.359369,0.636357,0.232703,
};
HaarFeature MEMORY_TYPE haar_8_7[2] =
{
0,mvcvRect(9,11,3,3),-1.000000,mvcvRect(9,12,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,9,4,5),-1.000000,mvcvRect(11,9,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_7[2] =
{
-0.007701,0.001559,
};
int MEMORY_TYPE feature_left_8_7[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_7[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_7[3] =
{
0.707462,0.570024,0.359045,
};
HaarFeature MEMORY_TYPE haar_8_8[2] =
{
0,mvcvRect(3,3,1,2),-1.000000,mvcvRect(3,4,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,17,5,3),-1.000000,mvcvRect(7,18,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_8[2] =
{
-0.000477,0.000842,
};
int MEMORY_TYPE feature_left_8_8[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_8[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_8[3] =
{
0.280544,0.412542,0.617800,
};
HaarFeature MEMORY_TYPE haar_8_9[2] =
{
0,mvcvRect(8,12,4,8),-1.000000,mvcvRect(10,12,2,4),2.000000,mvcvRect(8,16,2,4),2.000000,0,mvcvRect(7,4,10,12),-1.000000,mvcvRect(12,4,5,6),2.000000,mvcvRect(7,10,5,6),2.000000,
};
float MEMORY_TYPE feature_thres_8_9[2] =
{
-0.012825,-0.000652,
};
int MEMORY_TYPE feature_left_8_9[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_9[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_9[3] =
{
0.540308,0.563364,0.335654,
};
HaarFeature MEMORY_TYPE haar_8_10[2] =
{
0,mvcvRect(8,14,4,3),-1.000000,mvcvRect(8,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,9,4,5),-1.000000,mvcvRect(7,9,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_10[2] =
{
-0.012006,0.001321,
};
int MEMORY_TYPE feature_left_8_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_10[3] =
{
0.710951,0.490385,0.282458,
};
HaarFeature MEMORY_TYPE haar_8_11[2] =
{
0,mvcvRect(9,9,8,2),-1.000000,mvcvRect(9,9,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,15,5,2),-1.000000,mvcvRect(14,16,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_11[2] =
{
-0.020307,0.004018,
};
int MEMORY_TYPE feature_left_8_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_11[3] =
{
0.189137,0.537797,0.311949,
};
HaarFeature MEMORY_TYPE haar_8_12[2] =
{
0,mvcvRect(9,14,2,3),-1.000000,mvcvRect(9,15,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,7,8,4),-1.000000,mvcvRect(1,7,4,2),2.000000,mvcvRect(5,9,4,2),2.000000,
};
float MEMORY_TYPE feature_thres_8_12[2] =
{
0.004532,-0.004438,
};
int MEMORY_TYPE feature_left_8_12[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_12[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_12[3] =
{
0.720676,0.185467,0.498173,
};
HaarFeature MEMORY_TYPE haar_8_13[2] =
{
0,mvcvRect(19,3,1,2),-1.000000,mvcvRect(19,4,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,12,2,3),-1.000000,mvcvRect(9,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_13[2] =
{
0.001569,-0.004952,
};
int MEMORY_TYPE feature_left_8_13[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_13[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_13[3] =
{
0.263827,0.687107,0.471469,
};
HaarFeature MEMORY_TYPE haar_8_14[2] =
{
0,mvcvRect(3,14,14,4),-1.000000,mvcvRect(3,14,7,2),2.000000,mvcvRect(10,16,7,2),2.000000,0,mvcvRect(5,0,10,2),-1.000000,mvcvRect(5,1,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_14[2] =
{
-0.027430,0.001418,
};
int MEMORY_TYPE feature_left_8_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_14[3] =
{
0.154829,0.437684,0.632737,
};
HaarFeature MEMORY_TYPE haar_8_15[2] =
{
0,mvcvRect(11,14,4,6),-1.000000,mvcvRect(11,16,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,14,6,3),-1.000000,mvcvRect(7,15,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_15[2] =
{
-0.013079,-0.003509,
};
int MEMORY_TYPE feature_left_8_15[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_15[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_15[3] =
{
0.316681,0.619974,0.437969,
};
HaarFeature MEMORY_TYPE haar_8_16[2] =
{
0,mvcvRect(7,13,6,6),-1.000000,mvcvRect(7,13,3,3),2.000000,mvcvRect(10,16,3,3),2.000000,0,mvcvRect(0,2,1,6),-1.000000,mvcvRect(0,4,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_16[2] =
{
0.018921,0.002168,
};
int MEMORY_TYPE feature_left_8_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_16[3] =
{
0.147071,0.580946,0.343195,
};
HaarFeature MEMORY_TYPE haar_8_17[2] =
{
0,mvcvRect(6,7,8,2),-1.000000,mvcvRect(6,8,8,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,6,1),-1.000000,mvcvRect(9,7,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_17[2] =
{
0.001640,0.000140,
};
int MEMORY_TYPE feature_left_8_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_17[3] =
{
0.395946,0.324003,0.564665,
};
HaarFeature MEMORY_TYPE haar_8_18[2] =
{
0,mvcvRect(7,1,6,10),-1.000000,mvcvRect(7,6,6,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,2,6,2),-1.000000,mvcvRect(0,3,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_18[2] =
{
-0.003314,-0.002946,
};
int MEMORY_TYPE feature_left_8_18[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_18[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_18[3] =
{
0.427453,0.334167,0.662796,
};
HaarFeature MEMORY_TYPE haar_8_19[2] =
{
0,mvcvRect(11,4,2,4),-1.000000,mvcvRect(11,4,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,10,3,6),-1.000000,mvcvRect(11,13,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_19[2] =
{
0.000136,0.000605,
};
int MEMORY_TYPE feature_left_8_19[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_19[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_19[3] =
{
0.404693,0.548406,0.356994,
};
HaarFeature MEMORY_TYPE haar_8_20[2] =
{
0,mvcvRect(3,9,8,2),-1.000000,mvcvRect(7,9,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,4,6),-1.000000,mvcvRect(2,0,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_20[2] =
{
-0.017514,-0.018735,
};
int MEMORY_TYPE feature_left_8_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_20[3] =
{
0.182415,0.797182,0.506857,
};
HaarFeature MEMORY_TYPE haar_8_21[2] =
{
0,mvcvRect(7,0,6,2),-1.000000,mvcvRect(9,0,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,15,2,3),-1.000000,mvcvRect(9,16,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_21[2] =
{
0.012066,-0.002654,
};
int MEMORY_TYPE feature_left_8_21[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_21[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_21[3] =
{
0.216701,0.658418,0.462824,
};
HaarFeature MEMORY_TYPE haar_8_22[2] =
{
0,mvcvRect(3,12,1,2),-1.000000,mvcvRect(3,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,5,11,3),-1.000000,mvcvRect(4,6,11,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_22[2] =
{
0.001450,0.010954,
};
int MEMORY_TYPE feature_left_8_22[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_22[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_22[3] =
{
0.209025,0.511231,0.778458,
};
HaarFeature MEMORY_TYPE haar_8_23[2] =
{
0,mvcvRect(11,4,2,4),-1.000000,mvcvRect(11,4,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,3,6,3),-1.000000,mvcvRect(10,3,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_23[2] =
{
0.015772,-0.014253,
};
int MEMORY_TYPE feature_left_8_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_23[3] =
{
0.513236,0.174241,0.526715,
};
HaarFeature MEMORY_TYPE haar_8_24[2] =
{
0,mvcvRect(7,4,2,4),-1.000000,mvcvRect(8,4,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,3,6,3),-1.000000,mvcvRect(8,3,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_24[2] =
{
0.000030,0.023486,
};
int MEMORY_TYPE feature_left_8_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_24[3] =
{
0.341845,0.563127,0.200639,
};
HaarFeature MEMORY_TYPE haar_8_25[2] =
{
0,mvcvRect(11,4,4,3),-1.000000,mvcvRect(11,5,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,8,2,8),-1.000000,mvcvRect(11,12,2,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_25[2] =
{
0.005221,-0.025812,
};
int MEMORY_TYPE feature_left_8_25[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_25[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_25[3] =
{
0.624965,0.320323,0.519933,
};
HaarFeature MEMORY_TYPE haar_8_26[2] =
{
0,mvcvRect(8,7,3,5),-1.000000,mvcvRect(9,7,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,2,5),-1.000000,mvcvRect(10,7,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_26[2] =
{
-0.001953,-0.008147,
};
int MEMORY_TYPE feature_left_8_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_26[3] =
{
0.614071,0.659290,0.371112,
};
HaarFeature MEMORY_TYPE haar_8_27[2] =
{
0,mvcvRect(14,11,1,6),-1.000000,mvcvRect(14,13,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,8,4,3),-1.000000,mvcvRect(8,9,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_27[2] =
{
0.003296,-0.001396,
};
int MEMORY_TYPE feature_left_8_27[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_27[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_27[3] =
{
0.295211,0.332080,0.552841,
};
HaarFeature MEMORY_TYPE haar_8_28[2] =
{
0,mvcvRect(0,3,2,2),-1.000000,mvcvRect(0,4,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,14,5,6),-1.000000,mvcvRect(4,16,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_28[2] =
{
-0.004106,-0.010889,
};
int MEMORY_TYPE feature_left_8_28[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_28[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_28[3] =
{
0.171055,0.335943,0.567491,
};
HaarFeature MEMORY_TYPE haar_8_29[2] =
{
0,mvcvRect(11,4,4,3),-1.000000,mvcvRect(11,5,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,4,3,3),-1.000000,mvcvRect(12,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_29[2] =
{
-0.007677,-0.009773,
};
int MEMORY_TYPE feature_left_8_29[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_29[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_29[3] =
{
0.477324,0.808105,0.484583,
};
HaarFeature MEMORY_TYPE haar_8_30[2] =
{
0,mvcvRect(5,4,4,3),-1.000000,mvcvRect(5,5,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,15,4,2),-1.000000,mvcvRect(7,15,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_30[2] =
{
0.006044,-0.000461,
};
int MEMORY_TYPE feature_left_8_30[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_30[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_30[3] =
{
0.678400,0.551464,0.364236,
};
HaarFeature MEMORY_TYPE haar_8_31[2] =
{
0,mvcvRect(15,1,5,9),-1.000000,mvcvRect(15,4,5,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,10,3,3),-1.000000,mvcvRect(9,11,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_31[2] =
{
0.057992,0.000594,
};
int MEMORY_TYPE feature_left_8_31[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_31[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_31[3] =
{
0.125444,0.442488,0.572846,
};
HaarFeature MEMORY_TYPE haar_8_32[2] =
{
0,mvcvRect(1,6,2,6),-1.000000,mvcvRect(1,8,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,4,8,15),-1.000000,mvcvRect(2,9,8,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_32[2] =
{
-0.006235,-0.012785,
};
int MEMORY_TYPE feature_left_8_32[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_32[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_32[3] =
{
0.280504,0.195091,0.565292,
};
HaarFeature MEMORY_TYPE haar_8_33[2] =
{
0,mvcvRect(9,12,3,2),-1.000000,mvcvRect(9,13,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,12,3,3),-1.000000,mvcvRect(9,13,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_33[2] =
{
0.000420,0.000806,
};
int MEMORY_TYPE feature_left_8_33[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_33[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_33[3] =
{
0.616648,0.452658,0.594449,
};
HaarFeature MEMORY_TYPE haar_8_34[2] =
{
0,mvcvRect(7,6,3,5),-1.000000,mvcvRect(8,6,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,3,6,2),-1.000000,mvcvRect(7,3,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_34[2] =
{
-0.001634,-0.004830,
};
int MEMORY_TYPE feature_left_8_34[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_34[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_34[3] =
{
0.408694,0.279353,0.644494,
};
HaarFeature MEMORY_TYPE haar_8_35[2] =
{
0,mvcvRect(6,1,8,10),-1.000000,mvcvRect(10,1,4,5),2.000000,mvcvRect(6,6,4,5),2.000000,0,mvcvRect(0,0,20,10),-1.000000,mvcvRect(10,0,10,5),2.000000,mvcvRect(0,5,10,5),2.000000,
};
float MEMORY_TYPE feature_thres_8_35[2] =
{
-0.006399,0.108192,
};
int MEMORY_TYPE feature_left_8_35[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_35[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_35[3] =
{
0.567166,0.531181,0.261436,
};
HaarFeature MEMORY_TYPE haar_8_36[2] =
{
0,mvcvRect(6,3,3,1),-1.000000,mvcvRect(7,3,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,2,6,8),-1.000000,mvcvRect(2,2,2,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_36[2] =
{
0.000651,0.020611,
};
int MEMORY_TYPE feature_left_8_36[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_36[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_36[3] =
{
0.299677,0.448994,0.688828,
};
HaarFeature MEMORY_TYPE haar_8_37[2] =
{
0,mvcvRect(11,10,3,4),-1.000000,mvcvRect(11,12,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,6,3,8),-1.000000,mvcvRect(12,10,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_37[2] =
{
-0.025129,0.001792,
};
int MEMORY_TYPE feature_left_8_37[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_37[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_37[3] =
{
0.519686,0.346700,0.553359,
};
HaarFeature MEMORY_TYPE haar_8_38[2] =
{
0,mvcvRect(6,10,3,4),-1.000000,mvcvRect(6,12,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,3,8),-1.000000,mvcvRect(5,10,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_38[2] =
{
0.001563,-0.000619,
};
int MEMORY_TYPE feature_left_8_38[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_38[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_38[3] =
{
0.308144,0.269387,0.554449,
};
HaarFeature MEMORY_TYPE haar_8_39[2] =
{
0,mvcvRect(2,6,18,6),-1.000000,mvcvRect(11,6,9,3),2.000000,mvcvRect(2,9,9,3),2.000000,0,mvcvRect(7,14,7,3),-1.000000,mvcvRect(7,15,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_39[2] =
{
0.004811,0.002248,
};
int MEMORY_TYPE feature_left_8_39[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_39[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_39[3] =
{
0.558785,0.467211,0.609083,
};
HaarFeature MEMORY_TYPE haar_8_40[2] =
{
0,mvcvRect(0,0,2,12),-1.000000,mvcvRect(1,0,1,12),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,2,18,16),-1.000000,mvcvRect(1,10,18,8),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_40[2] =
{
-0.030147,0.275487,
};
int MEMORY_TYPE feature_left_8_40[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_40[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_40[3] =
{
0.902759,0.471983,0.219692,
};
HaarFeature MEMORY_TYPE haar_8_41[2] =
{
0,mvcvRect(9,13,5,3),-1.000000,mvcvRect(9,14,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,13,4,3),-1.000000,mvcvRect(8,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_41[2] =
{
0.003689,0.007296,
};
int MEMORY_TYPE feature_left_8_41[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_41[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_41[3] =
{
0.627301,0.483922,0.690906,
};
HaarFeature MEMORY_TYPE haar_8_42[2] =
{
0,mvcvRect(0,6,18,6),-1.000000,mvcvRect(0,6,9,3),2.000000,mvcvRect(9,9,9,3),2.000000,0,mvcvRect(7,13,6,3),-1.000000,mvcvRect(7,14,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_42[2] =
{
-0.056211,-0.002648,
};
int MEMORY_TYPE feature_left_8_42[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_8_42[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_8_42[3] =
{
0.173849,0.630414,0.447430,
};
HaarFeature MEMORY_TYPE haar_8_43[2] =
{
0,mvcvRect(17,4,1,3),-1.000000,mvcvRect(17,5,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,11,1,9),-1.000000,mvcvRect(12,14,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_43[2] =
{
-0.001453,0.002854,
};
int MEMORY_TYPE feature_left_8_43[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_43[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_43[3] =
{
0.530254,0.533840,0.379688,
};
HaarFeature MEMORY_TYPE haar_8_44[2] =
{
0,mvcvRect(2,4,1,3),-1.000000,mvcvRect(2,5,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,2,3),-1.000000,mvcvRect(5,5,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_8_44[2] =
{
0.000582,0.000925,
};
int MEMORY_TYPE feature_left_8_44[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_8_44[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_8_44[3] =
{
0.326984,0.455481,0.635835,
};
HaarClassifier MEMORY_TYPE haar_feature_class_8[45] =
{
{2,haar_8_0,feature_thres_8_0,feature_left_8_0,feature_right_8_0,feature_alpha_8_0},{2,haar_8_1,feature_thres_8_1,feature_left_8_1,feature_right_8_1,feature_alpha_8_1},{2,haar_8_2,feature_thres_8_2,feature_left_8_2,feature_right_8_2,feature_alpha_8_2},{2,haar_8_3,feature_thres_8_3,feature_left_8_3,feature_right_8_3,feature_alpha_8_3},{2,haar_8_4,feature_thres_8_4,feature_left_8_4,feature_right_8_4,feature_alpha_8_4},{2,haar_8_5,feature_thres_8_5,feature_left_8_5,feature_right_8_5,feature_alpha_8_5},{2,haar_8_6,feature_thres_8_6,feature_left_8_6,feature_right_8_6,feature_alpha_8_6},{2,haar_8_7,feature_thres_8_7,feature_left_8_7,feature_right_8_7,feature_alpha_8_7},{2,haar_8_8,feature_thres_8_8,feature_left_8_8,feature_right_8_8,feature_alpha_8_8},{2,haar_8_9,feature_thres_8_9,feature_left_8_9,feature_right_8_9,feature_alpha_8_9},{2,haar_8_10,feature_thres_8_10,feature_left_8_10,feature_right_8_10,feature_alpha_8_10},{2,haar_8_11,feature_thres_8_11,feature_left_8_11,feature_right_8_11,feature_alpha_8_11},{2,haar_8_12,feature_thres_8_12,feature_left_8_12,feature_right_8_12,feature_alpha_8_12},{2,haar_8_13,feature_thres_8_13,feature_left_8_13,feature_right_8_13,feature_alpha_8_13},{2,haar_8_14,feature_thres_8_14,feature_left_8_14,feature_right_8_14,feature_alpha_8_14},{2,haar_8_15,feature_thres_8_15,feature_left_8_15,feature_right_8_15,feature_alpha_8_15},{2,haar_8_16,feature_thres_8_16,feature_left_8_16,feature_right_8_16,feature_alpha_8_16},{2,haar_8_17,feature_thres_8_17,feature_left_8_17,feature_right_8_17,feature_alpha_8_17},{2,haar_8_18,feature_thres_8_18,feature_left_8_18,feature_right_8_18,feature_alpha_8_18},{2,haar_8_19,feature_thres_8_19,feature_left_8_19,feature_right_8_19,feature_alpha_8_19},{2,haar_8_20,feature_thres_8_20,feature_left_8_20,feature_right_8_20,feature_alpha_8_20},{2,haar_8_21,feature_thres_8_21,feature_left_8_21,feature_right_8_21,feature_alpha_8_21},{2,haar_8_22,feature_thres_8_22,feature_left_8_22,feature_right_8_22,feature_alpha_8_22},{2,haar_8_23,feature_thres_8_23,feature_left_8_23,feature_right_8_23,feature_alpha_8_23},{2,haar_8_24,feature_thres_8_24,feature_left_8_24,feature_right_8_24,feature_alpha_8_24},{2,haar_8_25,feature_thres_8_25,feature_left_8_25,feature_right_8_25,feature_alpha_8_25},{2,haar_8_26,feature_thres_8_26,feature_left_8_26,feature_right_8_26,feature_alpha_8_26},{2,haar_8_27,feature_thres_8_27,feature_left_8_27,feature_right_8_27,feature_alpha_8_27},{2,haar_8_28,feature_thres_8_28,feature_left_8_28,feature_right_8_28,feature_alpha_8_28},{2,haar_8_29,feature_thres_8_29,feature_left_8_29,feature_right_8_29,feature_alpha_8_29},{2,haar_8_30,feature_thres_8_30,feature_left_8_30,feature_right_8_30,feature_alpha_8_30},{2,haar_8_31,feature_thres_8_31,feature_left_8_31,feature_right_8_31,feature_alpha_8_31},{2,haar_8_32,feature_thres_8_32,feature_left_8_32,feature_right_8_32,feature_alpha_8_32},{2,haar_8_33,feature_thres_8_33,feature_left_8_33,feature_right_8_33,feature_alpha_8_33},{2,haar_8_34,feature_thres_8_34,feature_left_8_34,feature_right_8_34,feature_alpha_8_34},{2,haar_8_35,feature_thres_8_35,feature_left_8_35,feature_right_8_35,feature_alpha_8_35},{2,haar_8_36,feature_thres_8_36,feature_left_8_36,feature_right_8_36,feature_alpha_8_36},{2,haar_8_37,feature_thres_8_37,feature_left_8_37,feature_right_8_37,feature_alpha_8_37},{2,haar_8_38,feature_thres_8_38,feature_left_8_38,feature_right_8_38,feature_alpha_8_38},{2,haar_8_39,feature_thres_8_39,feature_left_8_39,feature_right_8_39,feature_alpha_8_39},{2,haar_8_40,feature_thres_8_40,feature_left_8_40,feature_right_8_40,feature_alpha_8_40},{2,haar_8_41,feature_thres_8_41,feature_left_8_41,feature_right_8_41,feature_alpha_8_41},{2,haar_8_42,feature_thres_8_42,feature_left_8_42,feature_right_8_42,feature_alpha_8_42},{2,haar_8_43,feature_thres_8_43,feature_left_8_43,feature_right_8_43,feature_alpha_8_43},{2,haar_8_44,feature_thres_8_44,feature_left_8_44,feature_right_8_44,feature_alpha_8_44},
};
HaarFeature MEMORY_TYPE haar_9_0[2] =
{
0,mvcvRect(1,2,18,3),-1.000000,mvcvRect(7,2,6,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,1,20,6),-1.000000,mvcvRect(0,3,20,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_0[2] =
{
0.019806,0.000704,
};
int MEMORY_TYPE feature_left_9_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_0[3] =
{
0.280973,0.311983,0.709031,
};
HaarFeature MEMORY_TYPE haar_9_1[2] =
{
0,mvcvRect(7,5,6,3),-1.000000,mvcvRect(9,5,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,7,6,4),-1.000000,mvcvRect(16,7,3,2),2.000000,mvcvRect(13,9,3,2),2.000000,
};
float MEMORY_TYPE feature_thres_9_1[2] =
{
0.002556,0.001082,
};
int MEMORY_TYPE feature_left_9_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_1[3] =
{
0.298195,0.302056,0.580881,
};
HaarFeature MEMORY_TYPE haar_9_2[2] =
{
0,mvcvRect(3,1,4,10),-1.000000,mvcvRect(3,1,2,5),2.000000,mvcvRect(5,6,2,5),2.000000,0,mvcvRect(0,4,19,10),-1.000000,mvcvRect(0,9,19,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_2[2] =
{
-0.000929,-0.018010,
};
int MEMORY_TYPE feature_left_9_2[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_2[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_2[3] =
{
0.373810,0.216313,0.661925,
};
HaarFeature MEMORY_TYPE haar_9_3[2] =
{
0,mvcvRect(9,8,3,12),-1.000000,mvcvRect(9,12,3,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,18,5,2),-1.000000,mvcvRect(11,19,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_3[2] =
{
0.002350,0.000818,
};
int MEMORY_TYPE feature_left_9_3[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_3[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_3[3] =
{
0.291040,0.557862,0.336663,
};
HaarFeature MEMORY_TYPE haar_9_4[2] =
{
0,mvcvRect(5,16,6,4),-1.000000,mvcvRect(5,16,3,2),2.000000,mvcvRect(8,18,3,2),2.000000,0,mvcvRect(5,18,3,2),-1.000000,mvcvRect(5,19,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_4[2] =
{
0.000621,0.000968,
};
int MEMORY_TYPE feature_left_9_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_4[3] =
{
0.407243,0.685960,0.310546,
};
HaarFeature MEMORY_TYPE haar_9_5[2] =
{
0,mvcvRect(13,11,3,2),-1.000000,mvcvRect(13,12,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,5,8,4),-1.000000,mvcvRect(8,5,4,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_5[2] =
{
0.000480,0.000091,
};
int MEMORY_TYPE feature_left_9_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_5[3] =
{
0.333733,0.337096,0.545121,
};
HaarFeature MEMORY_TYPE haar_9_6[2] =
{
0,mvcvRect(1,2,18,6),-1.000000,mvcvRect(1,2,9,3),2.000000,mvcvRect(10,5,9,3),2.000000,0,mvcvRect(3,5,14,6),-1.000000,mvcvRect(3,7,14,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_6[2] =
{
-0.043915,-0.005650,
};
int MEMORY_TYPE feature_left_9_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_6[3] =
{
0.262567,0.605046,0.323242,
};
HaarFeature MEMORY_TYPE haar_9_7[2] =
{
0,mvcvRect(18,1,2,6),-1.000000,mvcvRect(18,3,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,11,6,1),-1.000000,mvcvRect(11,11,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_7[2] =
{
0.003866,-0.000063,
};
int MEMORY_TYPE feature_left_9_7[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_7[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_7[3] =
{
0.326261,0.581731,0.416439,
};
HaarFeature MEMORY_TYPE haar_9_8[2] =
{
0,mvcvRect(0,2,6,11),-1.000000,mvcvRect(3,2,3,11),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,12,2,3),-1.000000,mvcvRect(4,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_8[2] =
{
0.052534,0.001382,
};
int MEMORY_TYPE feature_left_9_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_8[3] =
{
0.709540,0.529288,0.254139,
};
HaarFeature MEMORY_TYPE haar_9_9[2] =
{
0,mvcvRect(6,12,9,2),-1.000000,mvcvRect(9,12,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,4,6,15),-1.000000,mvcvRect(9,4,3,15),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_9[2] =
{
-0.000893,0.085580,
};
int MEMORY_TYPE feature_left_9_9[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_9[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_9[3] =
{
0.408534,0.526324,0.300320,
};
HaarFeature MEMORY_TYPE haar_9_10[2] =
{
0,mvcvRect(5,11,6,1),-1.000000,mvcvRect(7,11,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,6,15),-1.000000,mvcvRect(8,4,3,15),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_10[2] =
{
-0.000183,-0.009792,
};
int MEMORY_TYPE feature_left_9_10[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_10[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_10[3] =
{
0.402921,0.352132,0.666400,
};
HaarFeature MEMORY_TYPE haar_9_11[2] =
{
0,mvcvRect(14,12,6,7),-1.000000,mvcvRect(14,12,3,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(18,3,2,9),-1.000000,mvcvRect(18,6,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_11[2] =
{
0.014429,-0.045687,
};
int MEMORY_TYPE feature_left_9_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_11[3] =
{
0.459357,0.147476,0.517863,
};
HaarFeature MEMORY_TYPE haar_9_12[2] =
{
0,mvcvRect(8,1,3,1),-1.000000,mvcvRect(9,1,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,12,6,7),-1.000000,mvcvRect(3,12,3,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_12[2] =
{
-0.002576,-0.038302,
};
int MEMORY_TYPE feature_left_9_12[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_12[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_12[3] =
{
0.183728,0.808266,0.516669,
};
HaarFeature MEMORY_TYPE haar_9_13[2] =
{
0,mvcvRect(13,7,6,4),-1.000000,mvcvRect(16,7,3,2),2.000000,mvcvRect(13,9,3,2),2.000000,0,mvcvRect(8,0,10,2),-1.000000,mvcvRect(8,1,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_13[2] =
{
0.002898,-0.002517,
};
int MEMORY_TYPE feature_left_9_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_13[3] =
{
0.479801,0.334630,0.544445,
};
HaarFeature MEMORY_TYPE haar_9_14[2] =
{
0,mvcvRect(1,7,6,4),-1.000000,mvcvRect(1,7,3,2),2.000000,mvcvRect(4,9,3,2),2.000000,0,mvcvRect(1,2,3,3),-1.000000,mvcvRect(1,3,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_14[2] =
{
0.000563,0.003668,
};
int MEMORY_TYPE feature_left_9_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_14[3] =
{
0.358903,0.598313,0.298396,
};
HaarFeature MEMORY_TYPE haar_9_15[2] =
{
0,mvcvRect(9,13,4,3),-1.000000,mvcvRect(9,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,13,7,2),-1.000000,mvcvRect(12,14,7,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_15[2] =
{
0.002132,0.007604,
};
int MEMORY_TYPE feature_left_9_15[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_15[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_15[3] =
{
0.616322,0.521713,0.205416,
};
HaarFeature MEMORY_TYPE haar_9_16[2] =
{
0,mvcvRect(5,12,9,2),-1.000000,mvcvRect(8,12,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,10,4,8),-1.000000,mvcvRect(6,14,4,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_16[2] =
{
-0.000117,0.003166,
};
int MEMORY_TYPE feature_left_9_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_16[3] =
{
0.344667,0.559748,0.267379,
};
HaarFeature MEMORY_TYPE haar_9_17[2] =
{
0,mvcvRect(1,0,18,4),-1.000000,mvcvRect(7,0,6,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,0,5,2),-1.000000,mvcvRect(12,1,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_17[2] =
{
-0.022569,0.000271,
};
int MEMORY_TYPE feature_left_9_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_17[3] =
{
0.690027,0.448664,0.550879,
};
HaarFeature MEMORY_TYPE haar_9_18[2] =
{
0,mvcvRect(7,7,1,12),-1.000000,mvcvRect(7,13,1,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,2,3,4),-1.000000,mvcvRect(7,2,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_18[2] =
{
-0.015434,-0.008486,
};
int MEMORY_TYPE feature_left_9_18[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_18[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_18[3] =
{
0.204832,0.125495,0.506036,
};
HaarFeature MEMORY_TYPE haar_9_19[2] =
{
0,mvcvRect(0,13,20,6),-1.000000,mvcvRect(0,15,20,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,5,12,2),-1.000000,mvcvRect(14,5,6,1),2.000000,mvcvRect(8,6,6,1),2.000000,
};
float MEMORY_TYPE feature_thres_9_19[2] =
{
-0.118075,-0.001230,
};
int MEMORY_TYPE feature_left_9_19[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_19[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_19[3] =
{
0.067633,0.566070,0.429220,
};
HaarFeature MEMORY_TYPE haar_9_20[2] =
{
0,mvcvRect(8,14,2,3),-1.000000,mvcvRect(8,15,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,14,4,3),-1.000000,mvcvRect(8,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_20[2] =
{
-0.007029,0.008933,
};
int MEMORY_TYPE feature_left_9_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_20[3] =
{
0.713640,0.433888,0.706088,
};
HaarFeature MEMORY_TYPE haar_9_21[2] =
{
0,mvcvRect(12,13,7,6),-1.000000,mvcvRect(12,15,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,0,8,12),-1.000000,mvcvRect(10,0,4,6),2.000000,mvcvRect(6,6,4,6),2.000000,
};
float MEMORY_TYPE feature_thres_9_21[2] =
{
-0.047736,-0.044156,
};
int MEMORY_TYPE feature_left_9_21[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_21[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_21[3] =
{
0.526869,0.258058,0.540696,
};
HaarFeature MEMORY_TYPE haar_9_22[2] =
{
0,mvcvRect(0,15,9,4),-1.000000,mvcvRect(0,17,9,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,0,2,5),-1.000000,mvcvRect(10,0,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_22[2] =
{
-0.025983,-0.004789,
};
int MEMORY_TYPE feature_left_9_22[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_22[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_22[3] =
{
0.190505,0.255189,0.533908,
};
HaarFeature MEMORY_TYPE haar_9_23[2] =
{
0,mvcvRect(9,5,2,6),-1.000000,mvcvRect(9,5,1,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(17,2,3,6),-1.000000,mvcvRect(17,4,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_23[2] =
{
0.006742,0.011655,
};
int MEMORY_TYPE feature_left_9_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_23[3] =
{
0.469331,0.526196,0.314543,
};
HaarFeature MEMORY_TYPE haar_9_24[2] =
{
0,mvcvRect(3,11,2,3),-1.000000,mvcvRect(3,12,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,13,3,3),-1.000000,mvcvRect(7,14,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_24[2] =
{
-0.005698,-0.007298,
};
int MEMORY_TYPE feature_left_9_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_24[3] =
{
0.175685,0.777473,0.512429,
};
HaarFeature MEMORY_TYPE haar_9_25[2] =
{
0,mvcvRect(14,12,5,3),-1.000000,mvcvRect(14,13,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,8,14,3),-1.000000,mvcvRect(4,9,14,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_25[2] =
{
0.007909,-0.000159,
};
int MEMORY_TYPE feature_left_9_25[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_25[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_25[3] =
{
0.528456,0.388780,0.550117,
};
HaarFeature MEMORY_TYPE haar_9_26[2] =
{
0,mvcvRect(1,12,5,3),-1.000000,mvcvRect(1,13,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,15,12,2),-1.000000,mvcvRect(1,15,6,1),2.000000,mvcvRect(7,16,6,1),2.000000,
};
float MEMORY_TYPE feature_thres_9_26[2] =
{
-0.006224,0.001331,
};
int MEMORY_TYPE feature_left_9_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_26[3] =
{
0.248983,0.426215,0.593506,
};
HaarFeature MEMORY_TYPE haar_9_27[2] =
{
0,mvcvRect(12,11,4,2),-1.000000,mvcvRect(12,12,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,8,3,5),-1.000000,mvcvRect(10,8,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_27[2] =
{
0.005206,0.014065,
};
int MEMORY_TYPE feature_left_9_27[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_27[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_27[3] =
{
0.254522,0.485199,0.702142,
};
HaarFeature MEMORY_TYPE haar_9_28[2] =
{
0,mvcvRect(9,5,2,6),-1.000000,mvcvRect(10,5,1,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,2,3,6),-1.000000,mvcvRect(0,4,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_28[2] =
{
-0.006738,0.003341,
};
int MEMORY_TYPE feature_left_9_28[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_28[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_28[3] =
{
0.714327,0.517573,0.280864,
};
HaarFeature MEMORY_TYPE haar_9_29[2] =
{
0,mvcvRect(12,11,4,2),-1.000000,mvcvRect(12,12,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,3,5),-1.000000,mvcvRect(10,7,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_29[2] =
{
-0.011881,0.001423,
};
int MEMORY_TYPE feature_left_9_29[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_29[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_29[3] =
{
0.517322,0.450287,0.579570,
};
HaarFeature MEMORY_TYPE haar_9_30[2] =
{
0,mvcvRect(4,11,4,2),-1.000000,mvcvRect(4,12,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,8,3,5),-1.000000,mvcvRect(9,8,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_30[2] =
{
0.002986,-0.002048,
};
int MEMORY_TYPE feature_left_9_30[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_30[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_30[3] =
{
0.191512,0.650243,0.455932,
};
HaarFeature MEMORY_TYPE haar_9_31[2] =
{
0,mvcvRect(9,3,3,1),-1.000000,mvcvRect(10,3,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,5,3,8),-1.000000,mvcvRect(17,5,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_31[2] =
{
0.001712,-0.016981,
};
int MEMORY_TYPE feature_left_9_31[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_31[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_31[3] =
{
0.537625,0.705623,0.491461,
};
HaarFeature MEMORY_TYPE haar_9_32[2] =
{
0,mvcvRect(8,3,3,1),-1.000000,mvcvRect(9,3,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,5,3,8),-1.000000,mvcvRect(2,5,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_32[2] =
{
-0.001129,0.002862,
};
int MEMORY_TYPE feature_left_9_32[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_32[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_32[3] =
{
0.267871,0.441085,0.636832,
};
HaarFeature MEMORY_TYPE haar_9_33[2] =
{
0,mvcvRect(10,1,3,3),-1.000000,mvcvRect(11,1,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(17,5,2,4),-1.000000,mvcvRect(17,5,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_33[2] =
{
-0.003807,0.005909,
};
int MEMORY_TYPE feature_left_9_33[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_33[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_33[3] =
{
0.276356,0.486730,0.672878,
};
HaarFeature MEMORY_TYPE haar_9_34[2] =
{
0,mvcvRect(2,8,14,3),-1.000000,mvcvRect(2,9,14,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,1,3),-1.000000,mvcvRect(9,8,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_34[2] =
{
0.001100,-0.002340,
};
int MEMORY_TYPE feature_left_9_34[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_34[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_34[3] =
{
0.407051,0.260495,0.615486,
};
HaarFeature MEMORY_TYPE haar_9_35[2] =
{
0,mvcvRect(6,1,8,10),-1.000000,mvcvRect(6,6,8,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,0,6,8),-1.000000,mvcvRect(16,0,3,4),2.000000,mvcvRect(13,4,3,4),2.000000,
};
float MEMORY_TYPE feature_thres_9_35[2] =
{
-0.003607,0.040831,
};
int MEMORY_TYPE feature_left_9_35[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_35[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_35[3] =
{
0.573200,0.497338,0.738701,
};
HaarFeature MEMORY_TYPE haar_9_36[2] =
{
0,mvcvRect(1,5,2,4),-1.000000,mvcvRect(2,5,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,2,12,2),-1.000000,mvcvRect(4,3,12,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_36[2] =
{
-0.007108,-0.000938,
};
int MEMORY_TYPE feature_left_9_36[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_36[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_36[3] =
{
0.698475,0.269117,0.474178,
};
HaarFeature MEMORY_TYPE haar_9_37[2] =
{
0,mvcvRect(8,8,4,4),-1.000000,mvcvRect(8,10,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,12,4),-1.000000,mvcvRect(9,6,4,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_37[2] =
{
-0.001674,0.088288,
};
int MEMORY_TYPE feature_left_9_37[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_37[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_37[3] =
{
0.355101,0.524461,0.209665,
};
HaarFeature MEMORY_TYPE haar_9_38[2] =
{
0,mvcvRect(1,2,8,1),-1.000000,mvcvRect(5,2,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,1,6,10),-1.000000,mvcvRect(3,1,2,10),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_38[2] =
{
0.000820,-0.000766,
};
int MEMORY_TYPE feature_left_9_38[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_38[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_38[3] =
{
0.413110,0.462029,0.677541,
};
HaarFeature MEMORY_TYPE haar_9_39[2] =
{
0,mvcvRect(8,6,8,2),-1.000000,mvcvRect(8,6,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,7,6,6),-1.000000,mvcvRect(12,7,2,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_39[2] =
{
0.000658,-0.002130,
};
int MEMORY_TYPE feature_left_9_39[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_39[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_39[3] =
{
0.562828,0.557686,0.457765,
};
HaarFeature MEMORY_TYPE haar_9_40[2] =
{
0,mvcvRect(4,6,8,2),-1.000000,mvcvRect(8,6,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,6,6),-1.000000,mvcvRect(6,7,2,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_40[2] =
{
-0.000373,-0.011172,
};
int MEMORY_TYPE feature_left_9_40[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_40[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_40[3] =
{
0.495926,0.562564,0.204711,
};
HaarFeature MEMORY_TYPE haar_9_41[2] =
{
0,mvcvRect(3,14,16,4),-1.000000,mvcvRect(3,16,16,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,12,4,2),-1.000000,mvcvRect(8,13,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_41[2] =
{
0.043435,0.000967,
};
int MEMORY_TYPE feature_left_9_41[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_41[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_41[3] =
{
0.224215,0.453334,0.619993,
};
HaarFeature MEMORY_TYPE haar_9_42[2] =
{
0,mvcvRect(8,12,3,3),-1.000000,mvcvRect(8,13,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,12,6,1),-1.000000,mvcvRect(8,12,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_42[2] =
{
-0.003145,0.001523,
};
int MEMORY_TYPE feature_left_9_42[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_42[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_42[3] =
{
0.666276,0.500799,0.238499,
};
HaarFeature MEMORY_TYPE haar_9_43[2] =
{
0,mvcvRect(18,10,2,3),-1.000000,mvcvRect(18,11,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,8,4,6),-1.000000,mvcvRect(16,10,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_43[2] =
{
0.002085,0.036098,
};
int MEMORY_TYPE feature_left_9_43[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_43[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_43[3] =
{
0.375350,0.517717,0.163449,
};
HaarFeature MEMORY_TYPE haar_9_44[2] =
{
0,mvcvRect(8,3,2,1),-1.000000,mvcvRect(9,3,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,1,3,9),-1.000000,mvcvRect(8,1,1,9),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_44[2] =
{
0.001618,-0.000621,
};
int MEMORY_TYPE feature_left_9_44[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_44[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_44[3] =
{
0.258738,0.629953,0.465879,
};
HaarFeature MEMORY_TYPE haar_9_45[2] =
{
0,mvcvRect(5,11,11,6),-1.000000,mvcvRect(5,14,11,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,2,3,14),-1.000000,mvcvRect(12,9,3,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_45[2] =
{
0.000719,-0.039340,
};
int MEMORY_TYPE feature_left_9_45[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_9_45[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_9_45[3] =
{
0.335408,0.215413,0.523571,
};
HaarFeature MEMORY_TYPE haar_9_46[2] =
{
0,mvcvRect(8,7,3,3),-1.000000,mvcvRect(9,7,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,5,12,5),-1.000000,mvcvRect(7,5,4,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_9_46[2] =
{
-0.001099,0.002119,
};
int MEMORY_TYPE feature_left_9_46[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_9_46[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_9_46[3] =
{
0.646890,0.289309,0.525482,
};
HaarClassifier MEMORY_TYPE haar_feature_class_9[47] =
{
{2,haar_9_0,feature_thres_9_0,feature_left_9_0,feature_right_9_0,feature_alpha_9_0},{2,haar_9_1,feature_thres_9_1,feature_left_9_1,feature_right_9_1,feature_alpha_9_1},{2,haar_9_2,feature_thres_9_2,feature_left_9_2,feature_right_9_2,feature_alpha_9_2},{2,haar_9_3,feature_thres_9_3,feature_left_9_3,feature_right_9_3,feature_alpha_9_3},{2,haar_9_4,feature_thres_9_4,feature_left_9_4,feature_right_9_4,feature_alpha_9_4},{2,haar_9_5,feature_thres_9_5,feature_left_9_5,feature_right_9_5,feature_alpha_9_5},{2,haar_9_6,feature_thres_9_6,feature_left_9_6,feature_right_9_6,feature_alpha_9_6},{2,haar_9_7,feature_thres_9_7,feature_left_9_7,feature_right_9_7,feature_alpha_9_7},{2,haar_9_8,feature_thres_9_8,feature_left_9_8,feature_right_9_8,feature_alpha_9_8},{2,haar_9_9,feature_thres_9_9,feature_left_9_9,feature_right_9_9,feature_alpha_9_9},{2,haar_9_10,feature_thres_9_10,feature_left_9_10,feature_right_9_10,feature_alpha_9_10},{2,haar_9_11,feature_thres_9_11,feature_left_9_11,feature_right_9_11,feature_alpha_9_11},{2,haar_9_12,feature_thres_9_12,feature_left_9_12,feature_right_9_12,feature_alpha_9_12},{2,haar_9_13,feature_thres_9_13,feature_left_9_13,feature_right_9_13,feature_alpha_9_13},{2,haar_9_14,feature_thres_9_14,feature_left_9_14,feature_right_9_14,feature_alpha_9_14},{2,haar_9_15,feature_thres_9_15,feature_left_9_15,feature_right_9_15,feature_alpha_9_15},{2,haar_9_16,feature_thres_9_16,feature_left_9_16,feature_right_9_16,feature_alpha_9_16},{2,haar_9_17,feature_thres_9_17,feature_left_9_17,feature_right_9_17,feature_alpha_9_17},{2,haar_9_18,feature_thres_9_18,feature_left_9_18,feature_right_9_18,feature_alpha_9_18},{2,haar_9_19,feature_thres_9_19,feature_left_9_19,feature_right_9_19,feature_alpha_9_19},{2,haar_9_20,feature_thres_9_20,feature_left_9_20,feature_right_9_20,feature_alpha_9_20},{2,haar_9_21,feature_thres_9_21,feature_left_9_21,feature_right_9_21,feature_alpha_9_21},{2,haar_9_22,feature_thres_9_22,feature_left_9_22,feature_right_9_22,feature_alpha_9_22},{2,haar_9_23,feature_thres_9_23,feature_left_9_23,feature_right_9_23,feature_alpha_9_23},{2,haar_9_24,feature_thres_9_24,feature_left_9_24,feature_right_9_24,feature_alpha_9_24},{2,haar_9_25,feature_thres_9_25,feature_left_9_25,feature_right_9_25,feature_alpha_9_25},{2,haar_9_26,feature_thres_9_26,feature_left_9_26,feature_right_9_26,feature_alpha_9_26},{2,haar_9_27,feature_thres_9_27,feature_left_9_27,feature_right_9_27,feature_alpha_9_27},{2,haar_9_28,feature_thres_9_28,feature_left_9_28,feature_right_9_28,feature_alpha_9_28},{2,haar_9_29,feature_thres_9_29,feature_left_9_29,feature_right_9_29,feature_alpha_9_29},{2,haar_9_30,feature_thres_9_30,feature_left_9_30,feature_right_9_30,feature_alpha_9_30},{2,haar_9_31,feature_thres_9_31,feature_left_9_31,feature_right_9_31,feature_alpha_9_31},{2,haar_9_32,feature_thres_9_32,feature_left_9_32,feature_right_9_32,feature_alpha_9_32},{2,haar_9_33,feature_thres_9_33,feature_left_9_33,feature_right_9_33,feature_alpha_9_33},{2,haar_9_34,feature_thres_9_34,feature_left_9_34,feature_right_9_34,feature_alpha_9_34},{2,haar_9_35,feature_thres_9_35,feature_left_9_35,feature_right_9_35,feature_alpha_9_35},{2,haar_9_36,feature_thres_9_36,feature_left_9_36,feature_right_9_36,feature_alpha_9_36},{2,haar_9_37,feature_thres_9_37,feature_left_9_37,feature_right_9_37,feature_alpha_9_37},{2,haar_9_38,feature_thres_9_38,feature_left_9_38,feature_right_9_38,feature_alpha_9_38},{2,haar_9_39,feature_thres_9_39,feature_left_9_39,feature_right_9_39,feature_alpha_9_39},{2,haar_9_40,feature_thres_9_40,feature_left_9_40,feature_right_9_40,feature_alpha_9_40},{2,haar_9_41,feature_thres_9_41,feature_left_9_41,feature_right_9_41,feature_alpha_9_41},{2,haar_9_42,feature_thres_9_42,feature_left_9_42,feature_right_9_42,feature_alpha_9_42},{2,haar_9_43,feature_thres_9_43,feature_left_9_43,feature_right_9_43,feature_alpha_9_43},{2,haar_9_44,feature_thres_9_44,feature_left_9_44,feature_right_9_44,feature_alpha_9_44},{2,haar_9_45,feature_thres_9_45,feature_left_9_45,feature_right_9_45,feature_alpha_9_45},{2,haar_9_46,feature_thres_9_46,feature_left_9_46,feature_right_9_46,feature_alpha_9_46},
};
HaarFeature MEMORY_TYPE haar_10_0[2] =
{
0,mvcvRect(1,2,6,3),-1.000000,mvcvRect(4,2,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,6,10),-1.000000,mvcvRect(5,5,3,5),2.000000,mvcvRect(8,10,3,5),2.000000,
};
float MEMORY_TYPE feature_thres_10_0[2] =
{
0.005236,-0.002217,
};
int MEMORY_TYPE feature_left_10_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_0[3] =
{
0.329971,0.704159,0.323547,
};
HaarFeature MEMORY_TYPE haar_10_1[2] =
{
0,mvcvRect(16,18,2,2),-1.000000,mvcvRect(16,18,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,18,2,2),-1.000000,mvcvRect(16,18,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_1[2] =
{
-0.008230,-0.008230,
};
int MEMORY_TYPE feature_left_10_1[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_1[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_1[3] =
{
0.496117,0.712804,0.496117,
};
HaarFeature MEMORY_TYPE haar_10_2[2] =
{
0,mvcvRect(8,4,2,5),-1.000000,mvcvRect(9,4,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,4,1,4),-1.000000,mvcvRect(8,6,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_2[2] =
{
0.000453,-0.000418,
};
int MEMORY_TYPE feature_left_10_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_2[3] =
{
0.320847,0.661392,0.355133,
};
HaarFeature MEMORY_TYPE haar_10_3[2] =
{
0,mvcvRect(7,15,12,4),-1.000000,mvcvRect(13,15,6,2),2.000000,mvcvRect(7,17,6,2),2.000000,0,mvcvRect(11,18,6,2),-1.000000,mvcvRect(11,19,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_3[2] =
{
0.002782,-0.000060,
};
int MEMORY_TYPE feature_left_10_3[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_3[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_3[3] =
{
0.371013,0.574639,0.389488,
};
HaarFeature MEMORY_TYPE haar_10_4[2] =
{
0,mvcvRect(7,7,4,10),-1.000000,mvcvRect(7,12,4,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,10,8),-1.000000,mvcvRect(5,10,10,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_4[2] =
{
0.003506,0.000170,
};
int MEMORY_TYPE feature_left_10_4[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_4[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_4[3] =
{
0.305410,0.288558,0.648775,
};
HaarFeature MEMORY_TYPE haar_10_5[2] =
{
0,mvcvRect(11,1,6,12),-1.000000,mvcvRect(14,1,3,6),2.000000,mvcvRect(11,7,3,6),2.000000,0,mvcvRect(5,8,12,1),-1.000000,mvcvRect(9,8,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_5[2] =
{
-0.002338,-0.002137,
};
int MEMORY_TYPE feature_left_10_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_5[3] =
{
0.317443,0.382092,0.523289,
};
HaarFeature MEMORY_TYPE haar_10_6[2] =
{
0,mvcvRect(4,7,3,6),-1.000000,mvcvRect(4,9,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,11,3,4),-1.000000,mvcvRect(4,13,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_6[2] =
{
0.001025,-0.000045,
};
int MEMORY_TYPE feature_left_10_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_6[3] =
{
0.362280,0.653896,0.400368,
};
HaarFeature MEMORY_TYPE haar_10_7[2] =
{
0,mvcvRect(14,16,2,2),-1.000000,mvcvRect(14,17,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,15,2,2),-1.000000,mvcvRect(15,16,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_7[2] =
{
0.000571,0.000577,
};
int MEMORY_TYPE feature_left_10_7[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_7[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_7[3] =
{
0.389317,0.561453,0.368764,
};
HaarFeature MEMORY_TYPE haar_10_8[2] =
{
0,mvcvRect(7,12,6,2),-1.000000,mvcvRect(7,13,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,13,4,2),-1.000000,mvcvRect(8,14,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_8[2] =
{
0.000797,0.000359,
};
int MEMORY_TYPE feature_left_10_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_8[3] =
{
0.644303,0.338085,0.582465,
};
HaarFeature MEMORY_TYPE haar_10_9[2] =
{
0,mvcvRect(11,1,6,12),-1.000000,mvcvRect(14,1,3,6),2.000000,mvcvRect(11,7,3,6),2.000000,0,mvcvRect(12,2,4,2),-1.000000,mvcvRect(12,3,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_9[2] =
{
0.000440,-0.000891,
};
int MEMORY_TYPE feature_left_10_9[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_9[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_9[3] =
{
0.393877,0.342797,0.551570,
};
HaarFeature MEMORY_TYPE haar_10_10[2] =
{
0,mvcvRect(3,10,12,6),-1.000000,mvcvRect(3,10,6,3),2.000000,mvcvRect(9,13,6,3),2.000000,0,mvcvRect(3,1,6,12),-1.000000,mvcvRect(3,1,3,6),2.000000,mvcvRect(6,7,3,6),2.000000,
};
float MEMORY_TYPE feature_thres_10_10[2] =
{
0.005411,-0.000858,
};
int MEMORY_TYPE feature_left_10_10[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_10[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_10[3] =
{
0.380354,0.643951,0.416835,
};
HaarFeature MEMORY_TYPE haar_10_11[2] =
{
0,mvcvRect(16,6,4,14),-1.000000,mvcvRect(18,6,2,7),2.000000,mvcvRect(16,13,2,7),2.000000,0,mvcvRect(5,1,10,8),-1.000000,mvcvRect(10,1,5,4),2.000000,mvcvRect(5,5,5,4),2.000000,
};
float MEMORY_TYPE feature_thres_10_11[2] =
{
-0.022001,-0.007873,
};
int MEMORY_TYPE feature_left_10_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_11[3] =
{
0.665460,0.418272,0.560472,
};
HaarFeature MEMORY_TYPE haar_10_12[2] =
{
0,mvcvRect(0,6,4,14),-1.000000,mvcvRect(0,6,2,7),2.000000,mvcvRect(2,13,2,7),2.000000,0,mvcvRect(1,15,12,4),-1.000000,mvcvRect(1,15,6,2),2.000000,mvcvRect(7,17,6,2),2.000000,
};
float MEMORY_TYPE feature_thres_10_12[2] =
{
-0.027444,0.001979,
};
int MEMORY_TYPE feature_left_10_12[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_12[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_12[3] =
{
0.658686,0.324491,0.488287,
};
HaarFeature MEMORY_TYPE haar_10_13[2] =
{
0,mvcvRect(10,17,3,3),-1.000000,mvcvRect(11,17,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,2,2,6),-1.000000,mvcvRect(12,2,1,3),2.000000,mvcvRect(11,5,1,3),2.000000,
};
float MEMORY_TYPE feature_thres_10_13[2] =
{
-0.005678,0.000015,
};
int MEMORY_TYPE feature_left_10_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_13[3] =
{
0.222908,0.410729,0.574759,
};
HaarFeature MEMORY_TYPE haar_10_14[2] =
{
0,mvcvRect(7,17,3,3),-1.000000,mvcvRect(8,17,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,15,4,3),-1.000000,mvcvRect(8,16,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_14[2] =
{
-0.005414,0.005368,
};
int MEMORY_TYPE feature_left_10_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_14[3] =
{
0.206580,0.492642,0.713948,
};
HaarFeature MEMORY_TYPE haar_10_15[2] =
{
0,mvcvRect(10,15,4,2),-1.000000,mvcvRect(12,15,2,1),2.000000,mvcvRect(10,16,2,1),2.000000,0,mvcvRect(13,13,4,3),-1.000000,mvcvRect(13,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_15[2] =
{
-0.003143,0.010907,
};
int MEMORY_TYPE feature_left_10_15[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_15[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_15[3] =
{
0.678009,0.521493,0.114400,
};
HaarFeature MEMORY_TYPE haar_10_16[2] =
{
0,mvcvRect(3,13,4,3),-1.000000,mvcvRect(3,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,2,2,6),-1.000000,mvcvRect(7,2,1,3),2.000000,mvcvRect(8,5,1,3),2.000000,
};
float MEMORY_TYPE feature_thres_10_16[2] =
{
0.005844,0.000091,
};
int MEMORY_TYPE feature_left_10_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_16[3] =
{
0.193753,0.381258,0.551419,
};
HaarFeature MEMORY_TYPE haar_10_17[2] =
{
0,mvcvRect(2,1,16,3),-1.000000,mvcvRect(2,2,16,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,15,4,2),-1.000000,mvcvRect(12,15,2,1),2.000000,mvcvRect(10,16,2,1),2.000000,
};
float MEMORY_TYPE feature_thres_10_17[2] =
{
-0.016346,0.001599,
};
int MEMORY_TYPE feature_left_10_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_17[3] =
{
0.247402,0.481778,0.592308,
};
HaarFeature MEMORY_TYPE haar_10_18[2] =
{
0,mvcvRect(6,15,4,2),-1.000000,mvcvRect(6,15,2,1),2.000000,mvcvRect(8,16,2,1),2.000000,0,mvcvRect(3,0,13,3),-1.000000,mvcvRect(3,1,13,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_18[2] =
{
-0.004026,-0.006775,
};
int MEMORY_TYPE feature_left_10_18[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_18[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_18[3] =
{
0.750821,0.287981,0.519970,
};
HaarFeature MEMORY_TYPE haar_10_19[2] =
{
0,mvcvRect(0,9,20,3),-1.000000,mvcvRect(0,10,20,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,7,9,2),-1.000000,mvcvRect(6,8,9,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_19[2] =
{
-0.003247,0.001541,
};
int MEMORY_TYPE feature_left_10_19[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_19[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_19[3] =
{
0.304491,0.406348,0.567656,
};
HaarFeature MEMORY_TYPE haar_10_20[2] =
{
0,mvcvRect(8,14,3,6),-1.000000,mvcvRect(9,14,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,10,2,2),-1.000000,mvcvRect(9,11,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_20[2] =
{
-0.012858,-0.000148,
};
int MEMORY_TYPE feature_left_10_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_20[3] =
{
0.096718,0.453783,0.611538,
};
HaarFeature MEMORY_TYPE haar_10_21[2] =
{
0,mvcvRect(9,7,2,5),-1.000000,mvcvRect(9,7,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,10,3),-1.000000,mvcvRect(5,6,5,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_21[2] =
{
-0.009021,-0.028795,
};
int MEMORY_TYPE feature_left_10_21[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_21[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_21[3] =
{
0.480775,0.340380,0.525553,
};
HaarFeature MEMORY_TYPE haar_10_22[2] =
{
0,mvcvRect(9,7,2,5),-1.000000,mvcvRect(10,7,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,10,3),-1.000000,mvcvRect(10,6,5,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_22[2] =
{
0.009021,0.007412,
};
int MEMORY_TYPE feature_left_10_22[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_22[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_22[3] =
{
0.750584,0.545545,0.322607,
};
HaarFeature MEMORY_TYPE haar_10_23[2] =
{
0,mvcvRect(13,9,2,2),-1.000000,mvcvRect(13,9,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,3,12,11),-1.000000,mvcvRect(8,3,4,11),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_23[2] =
{
-0.003722,0.198659,
};
int MEMORY_TYPE feature_left_10_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_23[3] =
{
0.231185,0.527105,0.146993,
};
HaarFeature MEMORY_TYPE haar_10_24[2] =
{
0,mvcvRect(7,1,2,7),-1.000000,mvcvRect(8,1,1,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,4,3,8),-1.000000,mvcvRect(8,4,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_24[2] =
{
0.000015,-0.003909,
};
int MEMORY_TYPE feature_left_10_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_24[3] =
{
0.367814,0.713193,0.499387,
};
HaarFeature MEMORY_TYPE haar_10_25[2] =
{
0,mvcvRect(13,9,2,2),-1.000000,mvcvRect(13,9,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,6,2,2),-1.000000,mvcvRect(12,6,1,1),2.000000,mvcvRect(11,7,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_10_25[2] =
{
0.002511,0.000239,
};
int MEMORY_TYPE feature_left_10_25[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_25[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_25[3] =
{
0.531205,0.468938,0.571402,
};
HaarFeature MEMORY_TYPE haar_10_26[2] =
{
0,mvcvRect(5,4,2,3),-1.000000,mvcvRect(5,5,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,5,1,3),-1.000000,mvcvRect(6,6,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_26[2] =
{
0.006944,0.001207,
};
int MEMORY_TYPE feature_left_10_26[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_26[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_26[3] =
{
0.694880,0.400450,0.587488,
};
HaarFeature MEMORY_TYPE haar_10_27[2] =
{
0,mvcvRect(13,9,2,2),-1.000000,mvcvRect(13,9,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,14,3,3),-1.000000,mvcvRect(16,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_27[2] =
{
0.002511,0.001751,
};
int MEMORY_TYPE feature_left_10_27[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_27[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_27[3] =
{
0.532957,0.554585,0.344958,
};
HaarFeature MEMORY_TYPE haar_10_28[2] =
{
0,mvcvRect(5,9,2,2),-1.000000,mvcvRect(6,9,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,14,3,3),-1.000000,mvcvRect(1,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_28[2] =
{
-0.004198,0.001309,
};
int MEMORY_TYPE feature_left_10_28[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_28[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_28[3] =
{
0.121718,0.537505,0.341563,
};
HaarFeature MEMORY_TYPE haar_10_29[2] =
{
0,mvcvRect(13,1,1,6),-1.000000,mvcvRect(13,3,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,3,7,2),-1.000000,mvcvRect(13,4,7,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_29[2] =
{
0.000674,-0.010531,
};
int MEMORY_TYPE feature_left_10_29[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_29[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_29[3] =
{
0.419518,0.346075,0.515586,
};
HaarFeature MEMORY_TYPE haar_10_30[2] =
{
0,mvcvRect(0,6,20,14),-1.000000,mvcvRect(0,13,20,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,4,3,6),-1.000000,mvcvRect(0,6,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_30[2] =
{
-0.406723,-0.026315,
};
int MEMORY_TYPE feature_left_10_30[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_30[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_30[3] =
{
0.058066,0.147345,0.555938,
};
HaarFeature MEMORY_TYPE haar_10_31[2] =
{
0,mvcvRect(10,1,9,6),-1.000000,mvcvRect(10,3,9,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,0,12,5),-1.000000,mvcvRect(8,0,6,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_31[2] =
{
0.002256,0.012155,
};
int MEMORY_TYPE feature_left_10_31[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_31[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_31[3] =
{
0.547772,0.420779,0.562188,
};
HaarFeature MEMORY_TYPE haar_10_32[2] =
{
0,mvcvRect(0,0,18,5),-1.000000,mvcvRect(6,0,6,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,1,9,6),-1.000000,mvcvRect(1,3,9,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_32[2] =
{
-0.018437,0.000537,
};
int MEMORY_TYPE feature_left_10_32[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_32[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_32[3] =
{
0.644715,0.276513,0.488860,
};
HaarFeature MEMORY_TYPE haar_10_33[2] =
{
0,mvcvRect(15,15,2,2),-1.000000,mvcvRect(15,16,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,16,3,4),-1.000000,mvcvRect(13,18,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_33[2] =
{
-0.002627,-0.000511,
};
int MEMORY_TYPE feature_left_10_33[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_33[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_33[3] =
{
0.526469,0.578531,0.429110,
};
HaarFeature MEMORY_TYPE haar_10_34[2] =
{
0,mvcvRect(3,15,2,2),-1.000000,mvcvRect(3,16,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,16,3,4),-1.000000,mvcvRect(4,18,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_34[2] =
{
0.000415,-0.000550,
};
int MEMORY_TYPE feature_left_10_34[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_34[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_34[3] =
{
0.345541,0.602692,0.414389,
};
HaarFeature MEMORY_TYPE haar_10_35[2] =
{
0,mvcvRect(11,14,1,3),-1.000000,mvcvRect(11,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,13,5,3),-1.000000,mvcvRect(9,14,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_35[2] =
{
-0.001035,-0.003397,
};
int MEMORY_TYPE feature_left_10_35[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_35[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_35[3] =
{
0.609529,0.610828,0.470772,
};
HaarFeature MEMORY_TYPE haar_10_36[2] =
{
0,mvcvRect(0,0,3,6),-1.000000,mvcvRect(0,2,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,1,6,3),-1.000000,mvcvRect(6,1,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_36[2] =
{
0.003180,-0.000165,
};
int MEMORY_TYPE feature_left_10_36[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_36[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_36[3] =
{
0.324437,0.383076,0.573433,
};
HaarFeature MEMORY_TYPE haar_10_37[2] =
{
0,mvcvRect(9,13,4,3),-1.000000,mvcvRect(9,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,15,5,3),-1.000000,mvcvRect(8,16,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_37[2] =
{
0.008373,-0.002580,
};
int MEMORY_TYPE feature_left_10_37[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_37[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_37[3] =
{
0.661092,0.613931,0.468615,
};
HaarFeature MEMORY_TYPE haar_10_38[2] =
{
0,mvcvRect(8,3,3,2),-1.000000,mvcvRect(9,3,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,8,18,2),-1.000000,mvcvRect(1,9,18,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_38[2] =
{
0.000902,0.000370,
};
int MEMORY_TYPE feature_left_10_38[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_38[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_38[3] =
{
0.352002,0.257875,0.546724,
};
HaarFeature MEMORY_TYPE haar_10_39[2] =
{
0,mvcvRect(11,14,1,3),-1.000000,mvcvRect(11,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,13,6,3),-1.000000,mvcvRect(8,14,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_39[2] =
{
0.000997,-0.003669,
};
int MEMORY_TYPE feature_left_10_39[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_39[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_39[3] =
{
0.482015,0.571015,0.483191,
};
HaarFeature MEMORY_TYPE haar_10_40[2] =
{
0,mvcvRect(8,14,1,3),-1.000000,mvcvRect(8,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,13,12,4),-1.000000,mvcvRect(4,13,6,2),2.000000,mvcvRect(10,15,6,2),2.000000,
};
float MEMORY_TYPE feature_thres_10_40[2] =
{
-0.000895,0.005190,
};
int MEMORY_TYPE feature_left_10_40[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_40[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_40[3] =
{
0.613368,0.492858,0.258131,
};
HaarFeature MEMORY_TYPE haar_10_41[2] =
{
0,mvcvRect(10,7,2,2),-1.000000,mvcvRect(10,7,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,4,2,8),-1.000000,mvcvRect(14,4,1,4),2.000000,mvcvRect(13,8,1,4),2.000000,
};
float MEMORY_TYPE feature_thres_10_41[2] =
{
0.000423,0.008518,
};
int MEMORY_TYPE feature_left_10_41[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_41[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_41[3] =
{
0.447112,0.516102,0.331653,
};
HaarFeature MEMORY_TYPE haar_10_42[2] =
{
0,mvcvRect(0,5,4,6),-1.000000,mvcvRect(0,7,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,2,2),-1.000000,mvcvRect(9,7,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_42[2] =
{
-0.036624,-0.004110,
};
int MEMORY_TYPE feature_left_10_42[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_42[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_42[3] =
{
0.092606,0.852211,0.513791,
};
HaarFeature MEMORY_TYPE haar_10_43[2] =
{
0,mvcvRect(13,0,3,7),-1.000000,mvcvRect(14,0,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,2,2,14),-1.000000,mvcvRect(11,2,1,14),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_43[2] =
{
-0.006602,0.025579,
};
int MEMORY_TYPE feature_left_10_43[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_43[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_43[3] =
{
0.545906,0.521935,0.192719,
};
HaarFeature MEMORY_TYPE haar_10_44[2] =
{
0,mvcvRect(4,0,3,7),-1.000000,mvcvRect(5,0,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,8,12),-1.000000,mvcvRect(5,5,4,6),2.000000,mvcvRect(9,11,4,6),2.000000,
};
float MEMORY_TYPE feature_thres_10_44[2] =
{
0.011447,0.000724,
};
int MEMORY_TYPE feature_left_10_44[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_44[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_44[3] =
{
0.191600,0.523157,0.353534,
};
HaarFeature MEMORY_TYPE haar_10_45[2] =
{
0,mvcvRect(11,4,6,3),-1.000000,mvcvRect(11,5,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,3,4,3),-1.000000,mvcvRect(12,4,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_45[2] =
{
0.009713,-0.011338,
};
int MEMORY_TYPE feature_left_10_45[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_45[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_45[3] =
{
0.646410,0.738304,0.496474,
};
HaarFeature MEMORY_TYPE haar_10_46[2] =
{
0,mvcvRect(5,5,10,12),-1.000000,mvcvRect(5,5,5,6),2.000000,mvcvRect(10,11,5,6),2.000000,0,mvcvRect(3,6,12,3),-1.000000,mvcvRect(9,6,6,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_46[2] =
{
-0.008145,-0.008557,
};
int MEMORY_TYPE feature_left_10_46[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_46[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_46[3] =
{
0.361171,0.342191,0.594351,
};
HaarFeature MEMORY_TYPE haar_10_47[2] =
{
0,mvcvRect(9,6,2,7),-1.000000,mvcvRect(9,6,1,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,5,2,4),-1.000000,mvcvRect(9,5,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_47[2] =
{
0.002299,0.003843,
};
int MEMORY_TYPE feature_left_10_47[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_47[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_47[3] =
{
0.455010,0.471686,0.665619,
};
HaarFeature MEMORY_TYPE haar_10_48[2] =
{
0,mvcvRect(8,7,3,3),-1.000000,mvcvRect(9,7,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,1,6,4),-1.000000,mvcvRect(7,1,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_48[2] =
{
-0.000991,0.025496,
};
int MEMORY_TYPE feature_left_10_48[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_48[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_48[3] =
{
0.459272,0.656340,0.125884,
};
HaarFeature MEMORY_TYPE haar_10_49[2] =
{
0,mvcvRect(13,16,7,3),-1.000000,mvcvRect(13,17,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,4,3,3),-1.000000,mvcvRect(12,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_49[2] =
{
-0.015748,-0.018046,
};
int MEMORY_TYPE feature_left_10_49[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_49[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_49[3] =
{
0.523950,0.801585,0.500796,
};
HaarFeature MEMORY_TYPE haar_10_50[2] =
{
0,mvcvRect(0,16,7,3),-1.000000,mvcvRect(0,17,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,3,3),-1.000000,mvcvRect(5,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_50[2] =
{
0.010323,0.001645,
};
int MEMORY_TYPE feature_left_10_50[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_10_50[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_10_50[3] =
{
0.227482,0.435195,0.586763,
};
HaarFeature MEMORY_TYPE haar_10_51[2] =
{
0,mvcvRect(12,9,8,10),-1.000000,mvcvRect(12,9,4,10),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,10,12,5),-1.000000,mvcvRect(12,10,4,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_51[2] =
{
0.015881,0.010587,
};
int MEMORY_TYPE feature_left_10_51[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_51[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_51[3] =
{
0.446505,0.454446,0.570711,
};
HaarFeature MEMORY_TYPE haar_10_52[2] =
{
0,mvcvRect(0,9,8,10),-1.000000,mvcvRect(4,9,4,10),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,10,12,5),-1.000000,mvcvRect(4,10,4,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_10_52[2] =
{
-0.021532,0.005248,
};
int MEMORY_TYPE feature_left_10_52[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_10_52[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_10_52[3] =
{
0.652764,0.344473,0.532464,
};
HaarClassifier MEMORY_TYPE haar_feature_class_10[53] =
{
{2,haar_10_0,feature_thres_10_0,feature_left_10_0,feature_right_10_0,feature_alpha_10_0},{2,haar_10_1,feature_thres_10_1,feature_left_10_1,feature_right_10_1,feature_alpha_10_1},{2,haar_10_2,feature_thres_10_2,feature_left_10_2,feature_right_10_2,feature_alpha_10_2},{2,haar_10_3,feature_thres_10_3,feature_left_10_3,feature_right_10_3,feature_alpha_10_3},{2,haar_10_4,feature_thres_10_4,feature_left_10_4,feature_right_10_4,feature_alpha_10_4},{2,haar_10_5,feature_thres_10_5,feature_left_10_5,feature_right_10_5,feature_alpha_10_5},{2,haar_10_6,feature_thres_10_6,feature_left_10_6,feature_right_10_6,feature_alpha_10_6},{2,haar_10_7,feature_thres_10_7,feature_left_10_7,feature_right_10_7,feature_alpha_10_7},{2,haar_10_8,feature_thres_10_8,feature_left_10_8,feature_right_10_8,feature_alpha_10_8},{2,haar_10_9,feature_thres_10_9,feature_left_10_9,feature_right_10_9,feature_alpha_10_9},{2,haar_10_10,feature_thres_10_10,feature_left_10_10,feature_right_10_10,feature_alpha_10_10},{2,haar_10_11,feature_thres_10_11,feature_left_10_11,feature_right_10_11,feature_alpha_10_11},{2,haar_10_12,feature_thres_10_12,feature_left_10_12,feature_right_10_12,feature_alpha_10_12},{2,haar_10_13,feature_thres_10_13,feature_left_10_13,feature_right_10_13,feature_alpha_10_13},{2,haar_10_14,feature_thres_10_14,feature_left_10_14,feature_right_10_14,feature_alpha_10_14},{2,haar_10_15,feature_thres_10_15,feature_left_10_15,feature_right_10_15,feature_alpha_10_15},{2,haar_10_16,feature_thres_10_16,feature_left_10_16,feature_right_10_16,feature_alpha_10_16},{2,haar_10_17,feature_thres_10_17,feature_left_10_17,feature_right_10_17,feature_alpha_10_17},{2,haar_10_18,feature_thres_10_18,feature_left_10_18,feature_right_10_18,feature_alpha_10_18},{2,haar_10_19,feature_thres_10_19,feature_left_10_19,feature_right_10_19,feature_alpha_10_19},{2,haar_10_20,feature_thres_10_20,feature_left_10_20,feature_right_10_20,feature_alpha_10_20},{2,haar_10_21,feature_thres_10_21,feature_left_10_21,feature_right_10_21,feature_alpha_10_21},{2,haar_10_22,feature_thres_10_22,feature_left_10_22,feature_right_10_22,feature_alpha_10_22},{2,haar_10_23,feature_thres_10_23,feature_left_10_23,feature_right_10_23,feature_alpha_10_23},{2,haar_10_24,feature_thres_10_24,feature_left_10_24,feature_right_10_24,feature_alpha_10_24},{2,haar_10_25,feature_thres_10_25,feature_left_10_25,feature_right_10_25,feature_alpha_10_25},{2,haar_10_26,feature_thres_10_26,feature_left_10_26,feature_right_10_26,feature_alpha_10_26},{2,haar_10_27,feature_thres_10_27,feature_left_10_27,feature_right_10_27,feature_alpha_10_27},{2,haar_10_28,feature_thres_10_28,feature_left_10_28,feature_right_10_28,feature_alpha_10_28},{2,haar_10_29,feature_thres_10_29,feature_left_10_29,feature_right_10_29,feature_alpha_10_29},{2,haar_10_30,feature_thres_10_30,feature_left_10_30,feature_right_10_30,feature_alpha_10_30},{2,haar_10_31,feature_thres_10_31,feature_left_10_31,feature_right_10_31,feature_alpha_10_31},{2,haar_10_32,feature_thres_10_32,feature_left_10_32,feature_right_10_32,feature_alpha_10_32},{2,haar_10_33,feature_thres_10_33,feature_left_10_33,feature_right_10_33,feature_alpha_10_33},{2,haar_10_34,feature_thres_10_34,feature_left_10_34,feature_right_10_34,feature_alpha_10_34},{2,haar_10_35,feature_thres_10_35,feature_left_10_35,feature_right_10_35,feature_alpha_10_35},{2,haar_10_36,feature_thres_10_36,feature_left_10_36,feature_right_10_36,feature_alpha_10_36},{2,haar_10_37,feature_thres_10_37,feature_left_10_37,feature_right_10_37,feature_alpha_10_37},{2,haar_10_38,feature_thres_10_38,feature_left_10_38,feature_right_10_38,feature_alpha_10_38},{2,haar_10_39,feature_thres_10_39,feature_left_10_39,feature_right_10_39,feature_alpha_10_39},{2,haar_10_40,feature_thres_10_40,feature_left_10_40,feature_right_10_40,feature_alpha_10_40},{2,haar_10_41,feature_thres_10_41,feature_left_10_41,feature_right_10_41,feature_alpha_10_41},{2,haar_10_42,feature_thres_10_42,feature_left_10_42,feature_right_10_42,feature_alpha_10_42},{2,haar_10_43,feature_thres_10_43,feature_left_10_43,feature_right_10_43,feature_alpha_10_43},{2,haar_10_44,feature_thres_10_44,feature_left_10_44,feature_right_10_44,feature_alpha_10_44},{2,haar_10_45,feature_thres_10_45,feature_left_10_45,feature_right_10_45,feature_alpha_10_45},{2,haar_10_46,feature_thres_10_46,feature_left_10_46,feature_right_10_46,feature_alpha_10_46},{2,haar_10_47,feature_thres_10_47,feature_left_10_47,feature_right_10_47,feature_alpha_10_47},{2,haar_10_48,feature_thres_10_48,feature_left_10_48,feature_right_10_48,feature_alpha_10_48},{2,haar_10_49,feature_thres_10_49,feature_left_10_49,feature_right_10_49,feature_alpha_10_49},{2,haar_10_50,feature_thres_10_50,feature_left_10_50,feature_right_10_50,feature_alpha_10_50},{2,haar_10_51,feature_thres_10_51,feature_left_10_51,feature_right_10_51,feature_alpha_10_51},{2,haar_10_52,feature_thres_10_52,feature_left_10_52,feature_right_10_52,feature_alpha_10_52},
};
HaarFeature MEMORY_TYPE haar_11_0[2] =
{
0,mvcvRect(2,3,6,2),-1.000000,mvcvRect(5,3,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,17,9),-1.000000,mvcvRect(0,3,17,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_0[2] =
{
0.001822,0.008131,
};
int MEMORY_TYPE feature_left_11_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_0[3] =
{
0.310879,0.313324,0.664587,
};
HaarFeature MEMORY_TYPE haar_11_1[2] =
{
0,mvcvRect(4,7,12,2),-1.000000,mvcvRect(8,7,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,4,6,4),-1.000000,mvcvRect(12,4,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_1[2] =
{
0.001706,-0.000074,
};
int MEMORY_TYPE feature_left_11_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_1[3] =
{
0.264013,0.564721,0.348537,
};
HaarFeature MEMORY_TYPE haar_11_2[2] =
{
0,mvcvRect(0,10,20,4),-1.000000,mvcvRect(0,12,20,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,3,6,5),-1.000000,mvcvRect(6,3,2,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_2[2] =
{
0.000383,0.003187,
};
int MEMORY_TYPE feature_left_11_2[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_2[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_2[3] =
{
0.314065,0.648920,0.388773,
};
HaarFeature MEMORY_TYPE haar_11_3[2] =
{
0,mvcvRect(1,1,18,4),-1.000000,mvcvRect(7,1,6,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,9,2,3),-1.000000,mvcvRect(13,9,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_3[2] =
{
0.160443,-0.006729,
};
int MEMORY_TYPE feature_left_11_3[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_3[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_3[3] =
{
0.721653,0.165314,0.513983,
};
HaarFeature MEMORY_TYPE haar_11_4[2] =
{
0,mvcvRect(6,15,7,4),-1.000000,mvcvRect(6,17,7,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,17,4,2),-1.000000,mvcvRect(3,18,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_4[2] =
{
0.000007,0.000556,
};
int MEMORY_TYPE feature_left_11_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_4[3] =
{
0.314062,0.599370,0.331740,
};
HaarFeature MEMORY_TYPE haar_11_5[2] =
{
0,mvcvRect(9,4,8,10),-1.000000,mvcvRect(9,9,8,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,17,3,2),-1.000000,mvcvRect(10,17,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_5[2] =
{
-0.010822,-0.004583,
};
int MEMORY_TYPE feature_left_11_5[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_5[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_5[3] =
{
0.265294,0.184957,0.531396,
};
HaarFeature MEMORY_TYPE haar_11_6[2] =
{
0,mvcvRect(8,2,4,8),-1.000000,mvcvRect(8,6,4,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,4,14,12),-1.000000,mvcvRect(3,4,7,6),2.000000,mvcvRect(10,10,7,6),2.000000,
};
float MEMORY_TYPE feature_thres_11_6[2] =
{
-0.003021,0.077865,
};
int MEMORY_TYPE feature_left_11_6[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_6[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_6[3] =
{
0.404010,0.615819,0.178649,
};
HaarFeature MEMORY_TYPE haar_11_7[2] =
{
0,mvcvRect(7,7,6,4),-1.000000,mvcvRect(9,7,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,7,9,4),-1.000000,mvcvRect(6,9,9,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_7[2] =
{
0.026494,0.036912,
};
int MEMORY_TYPE feature_left_11_7[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_7[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_7[3] =
{
0.451109,0.452822,0.597228,
};
HaarFeature MEMORY_TYPE haar_11_8[2] =
{
0,mvcvRect(2,10,3,3),-1.000000,mvcvRect(2,11,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,6,2,9),-1.000000,mvcvRect(4,9,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_8[2] =
{
0.005786,0.000938,
};
int MEMORY_TYPE feature_left_11_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_8[3] =
{
0.253389,0.341041,0.592364,
};
HaarFeature MEMORY_TYPE haar_11_9[2] =
{
0,mvcvRect(9,11,3,3),-1.000000,mvcvRect(9,12,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,1,15,2),-1.000000,mvcvRect(3,2,15,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_9[2] =
{
-0.011003,-0.001174,
};
int MEMORY_TYPE feature_left_11_9[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_9[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_9[3] =
{
0.695804,0.385108,0.540819,
};
HaarFeature MEMORY_TYPE haar_11_10[2] =
{
0,mvcvRect(9,8,2,3),-1.000000,mvcvRect(9,9,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,6,2,5),-1.000000,mvcvRect(10,6,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_10[2] =
{
-0.003660,-0.002482,
};
int MEMORY_TYPE feature_left_11_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_10[3] =
{
0.200931,0.629539,0.439504,
};
HaarFeature MEMORY_TYPE haar_11_11[2] =
{
0,mvcvRect(9,7,2,3),-1.000000,mvcvRect(9,8,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,10,12,10),-1.000000,mvcvRect(4,15,12,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_11[2] =
{
-0.004461,-0.003597,
};
int MEMORY_TYPE feature_left_11_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_11[3] =
{
0.240530,0.545017,0.378236,
};
HaarFeature MEMORY_TYPE haar_11_12[2] =
{
0,mvcvRect(0,10,4,2),-1.000000,mvcvRect(0,11,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,15,9,2),-1.000000,mvcvRect(5,16,9,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_12[2] =
{
-0.003622,0.001206,
};
int MEMORY_TYPE feature_left_11_12[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_12[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_12[3] =
{
0.303390,0.463378,0.633595,
};
HaarFeature MEMORY_TYPE haar_11_13[2] =
{
0,mvcvRect(8,14,6,3),-1.000000,mvcvRect(8,15,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,16,4,3),-1.000000,mvcvRect(8,17,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_13[2] =
{
0.004312,-0.004496,
};
int MEMORY_TYPE feature_left_11_13[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_13[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_13[3] =
{
0.659883,0.662170,0.475525,
};
HaarFeature MEMORY_TYPE haar_11_14[2] =
{
0,mvcvRect(8,9,4,2),-1.000000,mvcvRect(8,10,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,3,14,2),-1.000000,mvcvRect(3,4,14,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_14[2] =
{
-0.001386,-0.000516,
};
int MEMORY_TYPE feature_left_11_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_14[3] =
{
0.280120,0.382949,0.562363,
};
HaarFeature MEMORY_TYPE haar_11_15[2] =
{
0,mvcvRect(11,12,1,2),-1.000000,mvcvRect(11,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,12,12,1),-1.000000,mvcvRect(8,12,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_15[2] =
{
0.000070,-0.000210,
};
int MEMORY_TYPE feature_left_11_15[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_15[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_15[3] =
{
0.453634,0.560814,0.426578,
};
HaarFeature MEMORY_TYPE haar_11_16[2] =
{
0,mvcvRect(0,2,1,2),-1.000000,mvcvRect(0,3,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,4,4,6),-1.000000,mvcvRect(9,4,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_16[2] =
{
0.001364,0.001548,
};
int MEMORY_TYPE feature_left_11_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_16[3] =
{
0.263709,0.417075,0.593299,
};
HaarFeature MEMORY_TYPE haar_11_17[2] =
{
0,mvcvRect(0,2,20,14),-1.000000,mvcvRect(10,2,10,7),2.000000,mvcvRect(0,9,10,7),2.000000,0,mvcvRect(14,6,1,3),-1.000000,mvcvRect(14,7,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_17[2] =
{
0.191796,-0.004478,
};
int MEMORY_TYPE feature_left_11_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_17[3] =
{
0.525676,0.663262,0.489259,
};
HaarFeature MEMORY_TYPE haar_11_18[2] =
{
0,mvcvRect(0,4,20,12),-1.000000,mvcvRect(0,4,10,6),2.000000,mvcvRect(10,10,10,6),2.000000,0,mvcvRect(8,12,1,2),-1.000000,mvcvRect(8,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_18[2] =
{
-0.126492,0.000065,
};
int MEMORY_TYPE feature_left_11_18[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_18[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_18[3] =
{
0.149978,0.423332,0.575604,
};
HaarFeature MEMORY_TYPE haar_11_19[2] =
{
0,mvcvRect(9,18,3,2),-1.000000,mvcvRect(10,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,17,6,2),-1.000000,mvcvRect(11,17,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_19[2] =
{
0.004186,0.000275,
};
int MEMORY_TYPE feature_left_11_19[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_19[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_19[3] =
{
0.528883,0.452402,0.560413,
};
HaarFeature MEMORY_TYPE haar_11_20[2] =
{
0,mvcvRect(5,6,2,3),-1.000000,mvcvRect(5,7,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,3,3),-1.000000,mvcvRect(5,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_20[2] =
{
-0.002291,0.001674,
};
int MEMORY_TYPE feature_left_11_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_20[3] =
{
0.557827,0.332306,0.555879,
};
HaarFeature MEMORY_TYPE haar_11_21[2] =
{
0,mvcvRect(14,15,3,2),-1.000000,mvcvRect(14,16,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,3,3,4),-1.000000,mvcvRect(12,3,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_21[2] =
{
0.001235,-0.008716,
};
int MEMORY_TYPE feature_left_11_21[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_21[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_21[3] =
{
0.365395,0.192453,0.531365,
};
HaarFeature MEMORY_TYPE haar_11_22[2] =
{
0,mvcvRect(3,15,3,2),-1.000000,mvcvRect(3,16,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,12,2,3),-1.000000,mvcvRect(9,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_22[2] =
{
0.004661,-0.008582,
};
int MEMORY_TYPE feature_left_11_22[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_22[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_22[3] =
{
0.202773,0.763606,0.514083,
};
HaarFeature MEMORY_TYPE haar_11_23[2] =
{
0,mvcvRect(9,13,3,7),-1.000000,mvcvRect(10,13,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,12,5,3),-1.000000,mvcvRect(12,13,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_23[2] =
{
0.014352,-0.007795,
};
int MEMORY_TYPE feature_left_11_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_23[3] =
{
0.525298,0.263294,0.532869,
};
HaarFeature MEMORY_TYPE haar_11_24[2] =
{
0,mvcvRect(8,18,3,2),-1.000000,mvcvRect(9,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,12,4),-1.000000,mvcvRect(4,7,6,2),2.000000,mvcvRect(10,9,6,2),2.000000,
};
float MEMORY_TYPE feature_thres_11_24[2] =
{
-0.003416,-0.004264,
};
int MEMORY_TYPE feature_left_11_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_24[3] =
{
0.241609,0.393654,0.547874,
};
HaarFeature MEMORY_TYPE haar_11_25[2] =
{
0,mvcvRect(6,19,14,1),-1.000000,mvcvRect(6,19,7,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,14,3,2),-1.000000,mvcvRect(16,15,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_25[2] =
{
0.008718,-0.003223,
};
int MEMORY_TYPE feature_left_11_25[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_25[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_25[3] =
{
0.478820,0.363161,0.528832,
};
HaarFeature MEMORY_TYPE haar_11_26[2] =
{
0,mvcvRect(1,0,6,10),-1.000000,mvcvRect(1,0,3,5),2.000000,mvcvRect(4,5,3,5),2.000000,0,mvcvRect(1,0,4,10),-1.000000,mvcvRect(1,0,2,5),2.000000,mvcvRect(3,5,2,5),2.000000,
};
float MEMORY_TYPE feature_thres_11_26[2] =
{
-0.042188,0.019876,
};
int MEMORY_TYPE feature_left_11_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_26[3] =
{
0.693114,0.452010,0.685506,
};
HaarFeature MEMORY_TYPE haar_11_27[2] =
{
0,mvcvRect(15,3,5,6),-1.000000,mvcvRect(15,5,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,5,2,15),-1.000000,mvcvRect(9,10,2,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_27[2] =
{
-0.031135,0.005703,
};
int MEMORY_TYPE feature_left_11_27[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_27[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_27[3] =
{
0.530042,0.560689,0.423062,
};
HaarFeature MEMORY_TYPE haar_11_28[2] =
{
0,mvcvRect(0,3,5,6),-1.000000,mvcvRect(0,5,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,0,3,2),-1.000000,mvcvRect(7,0,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_28[2] =
{
0.005273,-0.003123,
};
int MEMORY_TYPE feature_left_11_28[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_28[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_28[3] =
{
0.324723,0.198570,0.534987,
};
HaarFeature MEMORY_TYPE haar_11_29[2] =
{
0,mvcvRect(12,8,8,2),-1.000000,mvcvRect(16,8,4,1),2.000000,mvcvRect(12,9,4,1),2.000000,0,mvcvRect(5,8,12,1),-1.000000,mvcvRect(9,8,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_29[2] =
{
0.000465,0.030356,
};
int MEMORY_TYPE feature_left_11_29[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_29[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_29[3] =
{
0.420751,0.515346,0.311810,
};
HaarFeature MEMORY_TYPE haar_11_30[2] =
{
0,mvcvRect(3,13,3,3),-1.000000,mvcvRect(3,14,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,13,3,2),-1.000000,mvcvRect(5,14,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_30[2] =
{
-0.004299,0.000195,
};
int MEMORY_TYPE feature_left_11_30[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_30[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_30[3] =
{
0.327451,0.595308,0.422552,
};
HaarFeature MEMORY_TYPE haar_11_31[2] =
{
0,mvcvRect(9,15,3,3),-1.000000,mvcvRect(9,16,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,15,7,3),-1.000000,mvcvRect(7,16,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_31[2] =
{
-0.007778,0.016918,
};
int MEMORY_TYPE feature_left_11_31[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_31[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_31[3] =
{
0.721118,0.493659,0.703028,
};
HaarFeature MEMORY_TYPE haar_11_32[2] =
{
0,mvcvRect(3,14,11,6),-1.000000,mvcvRect(3,16,11,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,19,14,1),-1.000000,mvcvRect(7,19,7,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_32[2] =
{
-0.051949,-0.005475,
};
int MEMORY_TYPE feature_left_11_32[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_32[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_32[3] =
{
0.142553,0.605933,0.439400,
};
HaarFeature MEMORY_TYPE haar_11_33[2] =
{
0,mvcvRect(9,17,6,2),-1.000000,mvcvRect(11,17,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,11,6,2),-1.000000,mvcvRect(14,11,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_33[2] =
{
0.000015,0.001024,
};
int MEMORY_TYPE feature_left_11_33[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_33[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_33[3] =
{
0.448885,0.425655,0.579544,
};
HaarFeature MEMORY_TYPE haar_11_34[2] =
{
0,mvcvRect(5,17,6,2),-1.000000,mvcvRect(7,17,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,1,9,10),-1.000000,mvcvRect(3,1,3,10),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_34[2] =
{
-0.000104,0.008785,
};
int MEMORY_TYPE feature_left_11_34[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_34[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_34[3] =
{
0.424604,0.495801,0.675943,
};
HaarFeature MEMORY_TYPE haar_11_35[2] =
{
0,mvcvRect(10,1,3,3),-1.000000,mvcvRect(11,1,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,5,6,4),-1.000000,mvcvRect(9,5,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_35[2] =
{
0.003401,0.000586,
};
int MEMORY_TYPE feature_left_11_35[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_35[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_35[3] =
{
0.542348,0.363654,0.546435,
};
HaarFeature MEMORY_TYPE haar_11_36[2] =
{
0,mvcvRect(7,1,3,3),-1.000000,mvcvRect(8,1,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,4,4,11),-1.000000,mvcvRect(2,4,2,11),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_36[2] =
{
-0.002297,-0.014330,
};
int MEMORY_TYPE feature_left_11_36[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_36[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_36[3] =
{
0.254882,0.658766,0.453280,
};
HaarFeature MEMORY_TYPE haar_11_37[2] =
{
0,mvcvRect(9,5,6,4),-1.000000,mvcvRect(9,5,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,0,8,10),-1.000000,mvcvRect(10,0,4,5),2.000000,mvcvRect(6,5,4,5),2.000000,
};
float MEMORY_TYPE feature_thres_11_37[2] =
{
0.000986,-0.046641,
};
int MEMORY_TYPE feature_left_11_37[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_37[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_37[3] =
{
0.382277,0.307732,0.524413,
};
HaarFeature MEMORY_TYPE haar_11_38[2] =
{
0,mvcvRect(6,6,5,14),-1.000000,mvcvRect(6,13,5,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,5,4,14),-1.000000,mvcvRect(8,12,4,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_38[2] =
{
-0.119073,0.019333,
};
int MEMORY_TYPE feature_left_11_38[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_38[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_38[3] =
{
0.103386,0.555475,0.322132,
};
HaarFeature MEMORY_TYPE haar_11_39[2] =
{
0,mvcvRect(7,7,6,5),-1.000000,mvcvRect(9,7,2,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,3,3,9),-1.000000,mvcvRect(9,6,3,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_39[2] =
{
0.031428,0.000201,
};
int MEMORY_TYPE feature_left_11_39[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_39[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_39[3] =
{
0.468238,0.537307,0.380067,
};
HaarFeature MEMORY_TYPE haar_11_40[2] =
{
0,mvcvRect(8,1,3,3),-1.000000,mvcvRect(9,1,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,6,2,4),-1.000000,mvcvRect(10,6,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_40[2] =
{
-0.006258,0.008286,
};
int MEMORY_TYPE feature_left_11_40[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_40[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_40[3] =
{
0.179921,0.509507,0.754461,
};
HaarFeature MEMORY_TYPE haar_11_41[2] =
{
0,mvcvRect(10,8,6,9),-1.000000,mvcvRect(10,8,3,9),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,4,3,8),-1.000000,mvcvRect(17,4,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_41[2] =
{
0.002053,0.003252,
};
int MEMORY_TYPE feature_left_11_41[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_41[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_41[3] =
{
0.562864,0.480169,0.580210,
};
HaarFeature MEMORY_TYPE haar_11_42[2] =
{
0,mvcvRect(5,9,10,6),-1.000000,mvcvRect(5,9,5,3),2.000000,mvcvRect(10,12,5,3),2.000000,0,mvcvRect(5,5,6,4),-1.000000,mvcvRect(8,5,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_42[2] =
{
-0.031885,0.001838,
};
int MEMORY_TYPE feature_left_11_42[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_42[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_42[3] =
{
0.174275,0.346660,0.510715,
};
HaarFeature MEMORY_TYPE haar_11_43[2] =
{
0,mvcvRect(9,8,4,2),-1.000000,mvcvRect(9,9,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,7,2,2),-1.000000,mvcvRect(11,7,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_43[2] =
{
-0.000485,-0.002541,
};
int MEMORY_TYPE feature_left_11_43[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_43[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_43[3] =
{
0.532609,0.634278,0.499269,
};
HaarFeature MEMORY_TYPE haar_11_44[2] =
{
0,mvcvRect(8,12,4,8),-1.000000,mvcvRect(8,12,2,4),2.000000,mvcvRect(10,16,2,4),2.000000,0,mvcvRect(0,1,4,9),-1.000000,mvcvRect(0,4,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_44[2] =
{
-0.005156,-0.044969,
};
int MEMORY_TYPE feature_left_11_44[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_44[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_44[3] =
{
0.343343,0.186814,0.521546,
};
HaarFeature MEMORY_TYPE haar_11_45[2] =
{
0,mvcvRect(9,10,3,3),-1.000000,mvcvRect(9,11,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,11,4,2),-1.000000,mvcvRect(8,12,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_45[2] =
{
0.005898,0.003276,
};
int MEMORY_TYPE feature_left_11_45[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_45[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_45[3] =
{
0.622931,0.493577,0.721794,
};
HaarFeature MEMORY_TYPE haar_11_46[2] =
{
0,mvcvRect(7,8,4,2),-1.000000,mvcvRect(7,9,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,8,6,1),-1.000000,mvcvRect(9,8,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_46[2] =
{
-0.000102,-0.000163,
};
int MEMORY_TYPE feature_left_11_46[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_46[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_46[3] =
{
0.500798,0.602415,0.232951,
};
HaarFeature MEMORY_TYPE haar_11_47[2] =
{
0,mvcvRect(16,0,4,9),-1.000000,mvcvRect(16,0,2,9),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,0,3,6),-1.000000,mvcvRect(16,3,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_47[2] =
{
0.009054,0.035398,
};
int MEMORY_TYPE feature_left_11_47[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_47[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_47[3] =
{
0.451042,0.514200,0.286029,
};
HaarFeature MEMORY_TYPE haar_11_48[2] =
{
0,mvcvRect(0,0,4,9),-1.000000,mvcvRect(2,0,2,9),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,0,3,6),-1.000000,mvcvRect(1,3,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_48[2] =
{
0.005647,-0.002481,
};
int MEMORY_TYPE feature_left_11_48[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_48[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_48[3] =
{
0.470493,0.417985,0.672665,
};
HaarFeature MEMORY_TYPE haar_11_49[2] =
{
0,mvcvRect(9,7,6,9),-1.000000,mvcvRect(11,7,2,9),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,6,3,6),-1.000000,mvcvRect(11,6,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_49[2] =
{
-0.004109,-0.002071,
};
int MEMORY_TYPE feature_left_11_49[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_49[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_49[3] =
{
0.580980,0.607478,0.452406,
};
HaarFeature MEMORY_TYPE haar_11_50[2] =
{
0,mvcvRect(1,2,18,2),-1.000000,mvcvRect(1,2,9,1),2.000000,mvcvRect(10,3,9,1),2.000000,0,mvcvRect(5,8,6,8),-1.000000,mvcvRect(7,8,2,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_50[2] =
{
-0.002894,0.001347,
};
int MEMORY_TYPE feature_left_11_50[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_50[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_50[3] =
{
0.338352,0.569691,0.397085,
};
HaarFeature MEMORY_TYPE haar_11_51[2] =
{
0,mvcvRect(9,0,6,16),-1.000000,mvcvRect(11,0,2,16),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,1,6,18),-1.000000,mvcvRect(17,1,3,9),2.000000,mvcvRect(14,10,3,9),2.000000,
};
float MEMORY_TYPE feature_thres_11_51[2] =
{
-0.090779,-0.083172,
};
int MEMORY_TYPE feature_left_11_51[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_51[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_51[3] =
{
0.150270,0.757367,0.493644,
};
HaarFeature MEMORY_TYPE haar_11_52[2] =
{
0,mvcvRect(2,9,2,3),-1.000000,mvcvRect(2,10,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,1,6,18),-1.000000,mvcvRect(0,1,3,9),2.000000,mvcvRect(3,10,3,9),2.000000,
};
float MEMORY_TYPE feature_thres_11_52[2] =
{
-0.001411,0.055669,
};
int MEMORY_TYPE feature_left_11_52[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_52[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_52[3] =
{
0.339093,0.502510,0.742208,
};
HaarFeature MEMORY_TYPE haar_11_53[2] =
{
0,mvcvRect(11,8,4,12),-1.000000,mvcvRect(11,8,2,12),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,1,18,18),-1.000000,mvcvRect(2,10,18,9),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_53[2] =
{
0.057702,-0.425033,
};
int MEMORY_TYPE feature_left_11_53[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_53[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_53[3] =
{
0.519737,0.097347,0.518574,
};
HaarFeature MEMORY_TYPE haar_11_54[2] =
{
0,mvcvRect(6,3,3,1),-1.000000,mvcvRect(7,3,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,12,2,2),-1.000000,mvcvRect(4,13,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_54[2] =
{
-0.000444,0.000179,
};
int MEMORY_TYPE feature_left_11_54[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_54[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_54[3] =
{
0.364935,0.561928,0.376030,
};
HaarFeature MEMORY_TYPE haar_11_55[2] =
{
0,mvcvRect(8,13,5,3),-1.000000,mvcvRect(8,14,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,14,4,3),-1.000000,mvcvRect(8,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_55[2] =
{
0.005038,0.015191,
};
int MEMORY_TYPE feature_left_11_55[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_55[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_55[3] =
{
0.632845,0.493608,0.742652,
};
HaarFeature MEMORY_TYPE haar_11_56[2] =
{
0,mvcvRect(3,12,5,3),-1.000000,mvcvRect(3,13,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,3,3,4),-1.000000,mvcvRect(7,3,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_56[2] =
{
-0.012300,0.001517,
};
int MEMORY_TYPE feature_left_11_56[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_56[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_56[3] =
{
0.138935,0.509196,0.348265,
};
HaarFeature MEMORY_TYPE haar_11_57[2] =
{
0,mvcvRect(11,10,2,2),-1.000000,mvcvRect(12,10,1,1),2.000000,mvcvRect(11,11,1,1),2.000000,0,mvcvRect(5,8,12,1),-1.000000,mvcvRect(9,8,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_57[2] =
{
0.000958,-0.018962,
};
int MEMORY_TYPE feature_left_11_57[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_57[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_57[3] =
{
0.603632,0.231917,0.511665,
};
HaarFeature MEMORY_TYPE haar_11_58[2] =
{
0,mvcvRect(8,4,4,8),-1.000000,mvcvRect(10,4,2,8),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,6,8,5),-1.000000,mvcvRect(10,6,4,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_58[2] =
{
-0.022272,-0.025145,
};
int MEMORY_TYPE feature_left_11_58[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_58[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_58[3] =
{
0.655502,0.132607,0.467403,
};
HaarFeature MEMORY_TYPE haar_11_59[2] =
{
0,mvcvRect(10,4,6,4),-1.000000,mvcvRect(12,4,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,7,2,2),-1.000000,mvcvRect(13,7,1,1),2.000000,mvcvRect(12,8,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_11_59[2] =
{
0.019534,-0.001123,
};
int MEMORY_TYPE feature_left_11_59[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_59[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_59[3] =
{
0.518203,0.631824,0.482552,
};
HaarFeature MEMORY_TYPE haar_11_60[2] =
{
0,mvcvRect(3,5,10,8),-1.000000,mvcvRect(3,9,10,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,1,2,12),-1.000000,mvcvRect(7,7,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_60[2] =
{
-0.001486,0.000350,
};
int MEMORY_TYPE feature_left_11_60[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_60[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_60[3] =
{
0.291867,0.562137,0.424921,
};
HaarFeature MEMORY_TYPE haar_11_61[2] =
{
0,mvcvRect(12,7,2,2),-1.000000,mvcvRect(13,7,1,1),2.000000,mvcvRect(12,8,1,1),2.000000,0,mvcvRect(11,13,1,6),-1.000000,mvcvRect(11,16,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_61[2] =
{
-0.001123,0.010410,
};
int MEMORY_TYPE feature_left_11_61[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_61[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_61[3] =
{
0.481375,0.518401,0.205122,
};
HaarFeature MEMORY_TYPE haar_11_62[2] =
{
0,mvcvRect(5,1,6,15),-1.000000,mvcvRect(7,1,2,15),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,7,2,2),-1.000000,mvcvRect(6,7,1,1),2.000000,mvcvRect(7,8,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_11_62[2] =
{
-0.087833,0.001658,
};
int MEMORY_TYPE feature_left_11_62[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_62[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_62[3] =
{
0.117992,0.498781,0.697376,
};
HaarFeature MEMORY_TYPE haar_11_63[2] =
{
0,mvcvRect(17,5,2,2),-1.000000,mvcvRect(17,6,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,3,4,10),-1.000000,mvcvRect(12,3,2,5),2.000000,mvcvRect(10,8,2,5),2.000000,
};
float MEMORY_TYPE feature_thres_11_63[2] =
{
-0.002301,0.033026,
};
int MEMORY_TYPE feature_left_11_63[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_63[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_63[3] =
{
0.533983,0.503329,0.685191,
};
HaarFeature MEMORY_TYPE haar_11_64[2] =
{
0,mvcvRect(1,5,2,2),-1.000000,mvcvRect(1,6,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,10,2,2),-1.000000,mvcvRect(7,10,1,1),2.000000,mvcvRect(8,11,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_11_64[2] =
{
-0.001359,0.000781,
};
int MEMORY_TYPE feature_left_11_64[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_64[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_64[3] =
{
0.300282,0.459308,0.644005,
};
HaarFeature MEMORY_TYPE haar_11_65[2] =
{
0,mvcvRect(3,12,14,4),-1.000000,mvcvRect(10,12,7,2),2.000000,mvcvRect(3,14,7,2),2.000000,0,mvcvRect(9,15,3,2),-1.000000,mvcvRect(9,16,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_65[2] =
{
-0.018026,0.001235,
};
int MEMORY_TYPE feature_left_11_65[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_11_65[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_11_65[3] =
{
0.531129,0.472911,0.572146,
};
HaarFeature MEMORY_TYPE haar_11_66[2] =
{
0,mvcvRect(1,13,3,3),-1.000000,mvcvRect(1,14,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,3,1,2),-1.000000,mvcvRect(0,4,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_11_66[2] =
{
-0.000926,0.000801,
};
int MEMORY_TYPE feature_left_11_66[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_11_66[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_11_66[3] =
{
0.366233,0.536199,0.300863,
};
HaarClassifier MEMORY_TYPE haar_feature_class_11[67] =
{
{2,haar_11_0,feature_thres_11_0,feature_left_11_0,feature_right_11_0,feature_alpha_11_0},{2,haar_11_1,feature_thres_11_1,feature_left_11_1,feature_right_11_1,feature_alpha_11_1},{2,haar_11_2,feature_thres_11_2,feature_left_11_2,feature_right_11_2,feature_alpha_11_2},{2,haar_11_3,feature_thres_11_3,feature_left_11_3,feature_right_11_3,feature_alpha_11_3},{2,haar_11_4,feature_thres_11_4,feature_left_11_4,feature_right_11_4,feature_alpha_11_4},{2,haar_11_5,feature_thres_11_5,feature_left_11_5,feature_right_11_5,feature_alpha_11_5},{2,haar_11_6,feature_thres_11_6,feature_left_11_6,feature_right_11_6,feature_alpha_11_6},{2,haar_11_7,feature_thres_11_7,feature_left_11_7,feature_right_11_7,feature_alpha_11_7},{2,haar_11_8,feature_thres_11_8,feature_left_11_8,feature_right_11_8,feature_alpha_11_8},{2,haar_11_9,feature_thres_11_9,feature_left_11_9,feature_right_11_9,feature_alpha_11_9},{2,haar_11_10,feature_thres_11_10,feature_left_11_10,feature_right_11_10,feature_alpha_11_10},{2,haar_11_11,feature_thres_11_11,feature_left_11_11,feature_right_11_11,feature_alpha_11_11},{2,haar_11_12,feature_thres_11_12,feature_left_11_12,feature_right_11_12,feature_alpha_11_12},{2,haar_11_13,feature_thres_11_13,feature_left_11_13,feature_right_11_13,feature_alpha_11_13},{2,haar_11_14,feature_thres_11_14,feature_left_11_14,feature_right_11_14,feature_alpha_11_14},{2,haar_11_15,feature_thres_11_15,feature_left_11_15,feature_right_11_15,feature_alpha_11_15},{2,haar_11_16,feature_thres_11_16,feature_left_11_16,feature_right_11_16,feature_alpha_11_16},{2,haar_11_17,feature_thres_11_17,feature_left_11_17,feature_right_11_17,feature_alpha_11_17},{2,haar_11_18,feature_thres_11_18,feature_left_11_18,feature_right_11_18,feature_alpha_11_18},{2,haar_11_19,feature_thres_11_19,feature_left_11_19,feature_right_11_19,feature_alpha_11_19},{2,haar_11_20,feature_thres_11_20,feature_left_11_20,feature_right_11_20,feature_alpha_11_20},{2,haar_11_21,feature_thres_11_21,feature_left_11_21,feature_right_11_21,feature_alpha_11_21},{2,haar_11_22,feature_thres_11_22,feature_left_11_22,feature_right_11_22,feature_alpha_11_22},{2,haar_11_23,feature_thres_11_23,feature_left_11_23,feature_right_11_23,feature_alpha_11_23},{2,haar_11_24,feature_thres_11_24,feature_left_11_24,feature_right_11_24,feature_alpha_11_24},{2,haar_11_25,feature_thres_11_25,feature_left_11_25,feature_right_11_25,feature_alpha_11_25},{2,haar_11_26,feature_thres_11_26,feature_left_11_26,feature_right_11_26,feature_alpha_11_26},{2,haar_11_27,feature_thres_11_27,feature_left_11_27,feature_right_11_27,feature_alpha_11_27},{2,haar_11_28,feature_thres_11_28,feature_left_11_28,feature_right_11_28,feature_alpha_11_28},{2,haar_11_29,feature_thres_11_29,feature_left_11_29,feature_right_11_29,feature_alpha_11_29},{2,haar_11_30,feature_thres_11_30,feature_left_11_30,feature_right_11_30,feature_alpha_11_30},{2,haar_11_31,feature_thres_11_31,feature_left_11_31,feature_right_11_31,feature_alpha_11_31},{2,haar_11_32,feature_thres_11_32,feature_left_11_32,feature_right_11_32,feature_alpha_11_32},{2,haar_11_33,feature_thres_11_33,feature_left_11_33,feature_right_11_33,feature_alpha_11_33},{2,haar_11_34,feature_thres_11_34,feature_left_11_34,feature_right_11_34,feature_alpha_11_34},{2,haar_11_35,feature_thres_11_35,feature_left_11_35,feature_right_11_35,feature_alpha_11_35},{2,haar_11_36,feature_thres_11_36,feature_left_11_36,feature_right_11_36,feature_alpha_11_36},{2,haar_11_37,feature_thres_11_37,feature_left_11_37,feature_right_11_37,feature_alpha_11_37},{2,haar_11_38,feature_thres_11_38,feature_left_11_38,feature_right_11_38,feature_alpha_11_38},{2,haar_11_39,feature_thres_11_39,feature_left_11_39,feature_right_11_39,feature_alpha_11_39},{2,haar_11_40,feature_thres_11_40,feature_left_11_40,feature_right_11_40,feature_alpha_11_40},{2,haar_11_41,feature_thres_11_41,feature_left_11_41,feature_right_11_41,feature_alpha_11_41},{2,haar_11_42,feature_thres_11_42,feature_left_11_42,feature_right_11_42,feature_alpha_11_42},{2,haar_11_43,feature_thres_11_43,feature_left_11_43,feature_right_11_43,feature_alpha_11_43},{2,haar_11_44,feature_thres_11_44,feature_left_11_44,feature_right_11_44,feature_alpha_11_44},{2,haar_11_45,feature_thres_11_45,feature_left_11_45,feature_right_11_45,feature_alpha_11_45},{2,haar_11_46,feature_thres_11_46,feature_left_11_46,feature_right_11_46,feature_alpha_11_46},{2,haar_11_47,feature_thres_11_47,feature_left_11_47,feature_right_11_47,feature_alpha_11_47},{2,haar_11_48,feature_thres_11_48,feature_left_11_48,feature_right_11_48,feature_alpha_11_48},{2,haar_11_49,feature_thres_11_49,feature_left_11_49,feature_right_11_49,feature_alpha_11_49},{2,haar_11_50,feature_thres_11_50,feature_left_11_50,feature_right_11_50,feature_alpha_11_50},{2,haar_11_51,feature_thres_11_51,feature_left_11_51,feature_right_11_51,feature_alpha_11_51},{2,haar_11_52,feature_thres_11_52,feature_left_11_52,feature_right_11_52,feature_alpha_11_52},{2,haar_11_53,feature_thres_11_53,feature_left_11_53,feature_right_11_53,feature_alpha_11_53},{2,haar_11_54,feature_thres_11_54,feature_left_11_54,feature_right_11_54,feature_alpha_11_54},{2,haar_11_55,feature_thres_11_55,feature_left_11_55,feature_right_11_55,feature_alpha_11_55},{2,haar_11_56,feature_thres_11_56,feature_left_11_56,feature_right_11_56,feature_alpha_11_56},{2,haar_11_57,feature_thres_11_57,feature_left_11_57,feature_right_11_57,feature_alpha_11_57},{2,haar_11_58,feature_thres_11_58,feature_left_11_58,feature_right_11_58,feature_alpha_11_58},{2,haar_11_59,feature_thres_11_59,feature_left_11_59,feature_right_11_59,feature_alpha_11_59},{2,haar_11_60,feature_thres_11_60,feature_left_11_60,feature_right_11_60,feature_alpha_11_60},{2,haar_11_61,feature_thres_11_61,feature_left_11_61,feature_right_11_61,feature_alpha_11_61},{2,haar_11_62,feature_thres_11_62,feature_left_11_62,feature_right_11_62,feature_alpha_11_62},{2,haar_11_63,feature_thres_11_63,feature_left_11_63,feature_right_11_63,feature_alpha_11_63},{2,haar_11_64,feature_thres_11_64,feature_left_11_64,feature_right_11_64,feature_alpha_11_64},{2,haar_11_65,feature_thres_11_65,feature_left_11_65,feature_right_11_65,feature_alpha_11_65},{2,haar_11_66,feature_thres_11_66,feature_left_11_66,feature_right_11_66,feature_alpha_11_66},
};
HaarFeature MEMORY_TYPE haar_12_0[2] =
{
0,mvcvRect(7,7,6,1),-1.000000,mvcvRect(9,7,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,4,16,6),-1.000000,mvcvRect(0,6,16,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_0[2] =
{
0.002491,-0.050489,
};
int MEMORY_TYPE feature_left_12_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_0[3] =
{
0.342239,0.770346,0.451639,
};
HaarFeature MEMORY_TYPE haar_12_1[2] =
{
0,mvcvRect(9,3,2,14),-1.000000,mvcvRect(9,10,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,0,4,3),-1.000000,mvcvRect(12,0,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_1[2] =
{
-0.000778,0.000236,
};
int MEMORY_TYPE feature_left_12_1[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_1[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_1[3] =
{
0.325634,0.340656,0.589703,
};
HaarFeature MEMORY_TYPE haar_12_2[2] =
{
0,mvcvRect(4,18,12,2),-1.000000,mvcvRect(8,18,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,10,12,4),-1.000000,mvcvRect(8,10,4,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_2[2] =
{
0.004558,0.008124,
};
int MEMORY_TYPE feature_left_12_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_2[3] =
{
0.430658,0.714959,0.434568,
};
HaarFeature MEMORY_TYPE haar_12_3[2] =
{
0,mvcvRect(9,9,2,2),-1.000000,mvcvRect(9,10,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,1,2,8),-1.000000,mvcvRect(15,1,1,4),2.000000,mvcvRect(14,5,1,4),2.000000,
};
float MEMORY_TYPE feature_thres_12_3[2] =
{
-0.000446,-0.000290,
};
int MEMORY_TYPE feature_left_12_3[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_3[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_3[3] =
{
0.329597,0.584562,0.352669,
};
HaarFeature MEMORY_TYPE haar_12_4[2] =
{
0,mvcvRect(3,4,9,1),-1.000000,mvcvRect(6,4,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,3,4,2),-1.000000,mvcvRect(3,4,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_4[2] =
{
0.000007,-0.000385,
};
int MEMORY_TYPE feature_left_12_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_4[3] =
{
0.408195,0.420311,0.663413,
};
HaarFeature MEMORY_TYPE haar_12_5[2] =
{
0,mvcvRect(11,15,2,4),-1.000000,mvcvRect(11,17,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,13,2,6),-1.000000,mvcvRect(14,15,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_5[2] =
{
0.000195,-0.017084,
};
int MEMORY_TYPE feature_left_12_5[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_5[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_5[3] =
{
0.394247,0.229407,0.523896,
};
HaarFeature MEMORY_TYPE haar_12_6[2] =
{
0,mvcvRect(6,6,1,6),-1.000000,mvcvRect(6,9,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,10,8,8),-1.000000,mvcvRect(6,14,8,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_6[2] =
{
0.000835,0.000755,
};
int MEMORY_TYPE feature_left_12_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_6[3] =
{
0.302603,0.603220,0.341246,
};
HaarFeature MEMORY_TYPE haar_12_7[2] =
{
0,mvcvRect(8,13,4,3),-1.000000,mvcvRect(8,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,11,4,8),-1.000000,mvcvRect(10,15,4,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_7[2] =
{
0.008022,-0.038931,
};
int MEMORY_TYPE feature_left_12_7[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_7[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_7[3] =
{
0.730624,0.359933,0.523438,
};
HaarFeature MEMORY_TYPE haar_12_8[2] =
{
0,mvcvRect(5,11,6,1),-1.000000,mvcvRect(7,11,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,6,10),-1.000000,mvcvRect(8,4,3,10),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_8[2] =
{
-0.000070,-0.008535,
};
int MEMORY_TYPE feature_left_12_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_8[3] =
{
0.349376,0.274611,0.562659,
};
HaarFeature MEMORY_TYPE haar_12_9[2] =
{
0,mvcvRect(14,2,6,3),-1.000000,mvcvRect(14,3,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,12,3,2),-1.000000,mvcvRect(9,13,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_9[2] =
{
0.010854,0.000453,
};
int MEMORY_TYPE feature_left_12_9[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_9[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_9[3] =
{
0.528223,0.452205,0.605430,
};
HaarFeature MEMORY_TYPE haar_12_10[2] =
{
0,mvcvRect(8,1,4,6),-1.000000,mvcvRect(8,3,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,5,13,8),-1.000000,mvcvRect(3,9,13,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_10[2] =
{
0.000181,0.000466,
};
int MEMORY_TYPE feature_left_12_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_10[3] =
{
0.330686,0.145500,0.538493,
};
HaarFeature MEMORY_TYPE haar_12_11[2] =
{
0,mvcvRect(12,5,5,3),-1.000000,mvcvRect(12,6,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,14,15,6),-1.000000,mvcvRect(5,16,15,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_11[2] =
{
-0.008485,-0.018934,
};
int MEMORY_TYPE feature_left_12_11[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_11[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_11[3] =
{
0.481416,0.356374,0.540515,
};
HaarFeature MEMORY_TYPE haar_12_12[2] =
{
0,mvcvRect(3,5,5,3),-1.000000,mvcvRect(3,6,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,14,2,6),-1.000000,mvcvRect(9,14,1,3),2.000000,mvcvRect(10,17,1,3),2.000000,
};
float MEMORY_TYPE feature_thres_12_12[2] =
{
0.004981,0.003429,
};
int MEMORY_TYPE feature_left_12_12[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_12[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_12[3] =
{
0.695774,0.505089,0.231699,
};
HaarFeature MEMORY_TYPE haar_12_13[2] =
{
0,mvcvRect(9,12,3,2),-1.000000,mvcvRect(9,13,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,13,3,2),-1.000000,mvcvRect(9,14,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_13[2] =
{
0.000442,0.000238,
};
int MEMORY_TYPE feature_left_12_13[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_13[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_13[3] =
{
0.601858,0.475508,0.558524,
};
HaarFeature MEMORY_TYPE haar_12_14[2] =
{
0,mvcvRect(0,2,6,3),-1.000000,mvcvRect(0,3,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,1,9,11),-1.000000,mvcvRect(3,1,3,11),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_14[2] =
{
-0.006426,0.009964,
};
int MEMORY_TYPE feature_left_12_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_14[3] =
{
0.228247,0.404059,0.565017,
};
HaarFeature MEMORY_TYPE haar_12_15[2] =
{
0,mvcvRect(8,13,4,6),-1.000000,mvcvRect(10,13,2,3),2.000000,mvcvRect(8,16,2,3),2.000000,0,mvcvRect(7,13,6,3),-1.000000,mvcvRect(7,14,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_15[2] =
{
0.013654,-0.009989,
};
int MEMORY_TYPE feature_left_12_15[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_15[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_15[3] =
{
0.526774,0.679405,0.479703,
};
HaarFeature MEMORY_TYPE haar_12_16[2] =
{
0,mvcvRect(3,12,14,4),-1.000000,mvcvRect(3,12,7,2),2.000000,mvcvRect(10,14,7,2),2.000000,0,mvcvRect(7,14,1,4),-1.000000,mvcvRect(7,16,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_16[2] =
{
0.036559,0.000049,
};
int MEMORY_TYPE feature_left_12_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_16[3] =
{
0.088426,0.402079,0.545733,
};
HaarFeature MEMORY_TYPE haar_12_17[2] =
{
0,mvcvRect(8,13,4,6),-1.000000,mvcvRect(10,13,2,3),2.000000,mvcvRect(8,16,2,3),2.000000,0,mvcvRect(10,14,1,3),-1.000000,mvcvRect(10,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_17[2] =
{
0.013654,0.001880,
};
int MEMORY_TYPE feature_left_12_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_17[3] =
{
0.526761,0.480605,0.639436,
};
HaarFeature MEMORY_TYPE haar_12_18[2] =
{
0,mvcvRect(8,13,4,6),-1.000000,mvcvRect(8,13,2,3),2.000000,mvcvRect(10,16,2,3),2.000000,0,mvcvRect(9,14,1,3),-1.000000,mvcvRect(9,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_18[2] =
{
-0.013654,0.001278,
};
int MEMORY_TYPE feature_left_12_18[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_18[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_18[3] =
{
0.172481,0.447982,0.631001,
};
HaarFeature MEMORY_TYPE haar_12_19[2] =
{
0,mvcvRect(10,15,2,3),-1.000000,mvcvRect(10,16,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,16,1,2),-1.000000,mvcvRect(11,17,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_19[2] =
{
0.000988,0.000015,
};
int MEMORY_TYPE feature_left_12_19[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_19[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_19[3] =
{
0.594817,0.485417,0.530936,
};
HaarFeature MEMORY_TYPE haar_12_20[2] =
{
0,mvcvRect(9,0,2,2),-1.000000,mvcvRect(9,1,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,1,5,8),-1.000000,mvcvRect(0,5,5,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_20[2] =
{
-0.000228,-0.014754,
};
int MEMORY_TYPE feature_left_12_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_20[3] =
{
0.318363,0.308498,0.535203,
};
HaarFeature MEMORY_TYPE haar_12_21[2] =
{
0,mvcvRect(10,14,2,3),-1.000000,mvcvRect(10,15,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,13,2,3),-1.000000,mvcvRect(10,14,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_21[2] =
{
-0.003415,0.007581,
};
int MEMORY_TYPE feature_left_12_21[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_21[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_21[3] =
{
0.611533,0.495165,0.706133,
};
HaarFeature MEMORY_TYPE haar_12_22[2] =
{
0,mvcvRect(0,3,16,6),-1.000000,mvcvRect(0,6,16,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,1,2,2),-1.000000,mvcvRect(5,1,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_22[2] =
{
-0.005773,0.000074,
};
int MEMORY_TYPE feature_left_12_22[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_22[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_22[3] =
{
0.375422,0.411552,0.588944,
};
HaarFeature MEMORY_TYPE haar_12_23[2] =
{
0,mvcvRect(9,7,2,3),-1.000000,mvcvRect(9,8,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,8,2,12),-1.000000,mvcvRect(10,12,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_23[2] =
{
-0.008228,0.005338,
};
int MEMORY_TYPE feature_left_12_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_23[3] =
{
0.095611,0.530051,0.396190,
};
HaarFeature MEMORY_TYPE haar_12_24[2] =
{
0,mvcvRect(9,7,2,2),-1.000000,mvcvRect(10,7,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,0,6,8),-1.000000,mvcvRect(7,0,2,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_24[2] =
{
-0.002705,0.007734,
};
int MEMORY_TYPE feature_left_12_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_24[3] =
{
0.648187,0.511044,0.312152,
};
HaarFeature MEMORY_TYPE haar_12_25[2] =
{
0,mvcvRect(9,7,3,6),-1.000000,mvcvRect(10,7,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,12,10,8),-1.000000,mvcvRect(8,16,10,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_25[2] =
{
0.010887,0.011039,
};
int MEMORY_TYPE feature_left_12_25[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_25[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_25[3] =
{
0.480143,0.542971,0.416236,
};
HaarFeature MEMORY_TYPE haar_12_26[2] =
{
0,mvcvRect(8,7,3,6),-1.000000,mvcvRect(9,7,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,12,2),-1.000000,mvcvRect(10,7,6,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_26[2] =
{
-0.010054,0.007707,
};
int MEMORY_TYPE feature_left_12_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_26[3] =
{
0.732934,0.535687,0.345555,
};
HaarFeature MEMORY_TYPE haar_12_27[2] =
{
0,mvcvRect(8,6,8,3),-1.000000,mvcvRect(8,6,4,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,15,3,3),-1.000000,mvcvRect(16,16,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_27[2] =
{
-0.000583,-0.002574,
};
int MEMORY_TYPE feature_left_12_27[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_27[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_27[3] =
{
0.365502,0.377676,0.539177,
};
HaarFeature MEMORY_TYPE haar_12_28[2] =
{
0,mvcvRect(4,6,12,3),-1.000000,mvcvRect(10,6,6,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,8,3,5),-1.000000,mvcvRect(8,8,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_28[2] =
{
-0.007017,-0.001773,
};
int MEMORY_TYPE feature_left_12_28[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_28[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_28[3] =
{
0.403930,0.695044,0.498112,
};
HaarFeature MEMORY_TYPE haar_12_29[2] =
{
0,mvcvRect(0,10,20,2),-1.000000,mvcvRect(10,10,10,1),2.000000,mvcvRect(0,11,10,1),2.000000,0,mvcvRect(11,16,9,4),-1.000000,mvcvRect(14,16,3,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_29[2] =
{
-0.016318,-0.011663,
};
int MEMORY_TYPE feature_left_12_29[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_29[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_29[3] =
{
0.529673,0.584264,0.478950,
};
HaarFeature MEMORY_TYPE haar_12_30[2] =
{
0,mvcvRect(0,5,3,4),-1.000000,mvcvRect(1,5,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,15,4,2),-1.000000,mvcvRect(8,15,2,1),2.000000,mvcvRect(10,16,2,1),2.000000,
};
float MEMORY_TYPE feature_thres_12_30[2] =
{
0.002588,-0.003733,
};
int MEMORY_TYPE feature_left_12_30[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_30[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_30[3] =
{
0.609218,0.672174,0.406689,
};
HaarFeature MEMORY_TYPE haar_12_31[2] =
{
0,mvcvRect(1,8,19,3),-1.000000,mvcvRect(1,9,19,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,16,3,3),-1.000000,mvcvRect(15,17,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_31[2] =
{
-0.001436,0.001834,
};
int MEMORY_TYPE feature_left_12_31[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_31[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_31[3] =
{
0.358509,0.537116,0.403351,
};
HaarFeature MEMORY_TYPE haar_12_32[2] =
{
0,mvcvRect(0,4,20,10),-1.000000,mvcvRect(0,4,10,5),2.000000,mvcvRect(10,9,10,5),2.000000,0,mvcvRect(2,14,7,6),-1.000000,mvcvRect(2,16,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_32[2] =
{
0.122803,0.050229,
};
int MEMORY_TYPE feature_left_12_32[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_32[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_32[3] =
{
0.154757,0.543384,0.084293,
};
HaarFeature MEMORY_TYPE haar_12_33[2] =
{
0,mvcvRect(8,6,6,6),-1.000000,mvcvRect(10,6,2,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,4,4,6),-1.000000,mvcvRect(16,6,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_33[2] =
{
-0.021437,-0.031010,
};
int MEMORY_TYPE feature_left_12_33[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_33[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_33[3] =
{
0.486005,0.183301,0.520755,
};
HaarFeature MEMORY_TYPE haar_12_34[2] =
{
0,mvcvRect(7,13,6,3),-1.000000,mvcvRect(7,14,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,13,4,3),-1.000000,mvcvRect(7,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_34[2] =
{
-0.012974,0.001582,
};
int MEMORY_TYPE feature_left_12_34[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_34[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_34[3] =
{
0.704824,0.417059,0.586516,
};
HaarFeature MEMORY_TYPE haar_12_35[2] =
{
0,mvcvRect(13,13,6,2),-1.000000,mvcvRect(13,14,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,12,2,3),-1.000000,mvcvRect(14,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_35[2] =
{
-0.009781,0.001174,
};
int MEMORY_TYPE feature_left_12_35[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_35[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_35[3] =
{
0.530792,0.552245,0.350717,
};
HaarFeature MEMORY_TYPE haar_12_36[2] =
{
0,mvcvRect(1,13,6,2),-1.000000,mvcvRect(1,14,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,12,2,3),-1.000000,mvcvRect(4,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_36[2] =
{
0.001465,0.002353,
};
int MEMORY_TYPE feature_left_12_36[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_36[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_36[3] =
{
0.304265,0.533932,0.280624,
};
HaarFeature MEMORY_TYPE haar_12_37[2] =
{
0,mvcvRect(17,4,3,5),-1.000000,mvcvRect(18,4,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,14,8),-1.000000,mvcvRect(12,5,7,4),2.000000,mvcvRect(5,9,7,4),2.000000,
};
float MEMORY_TYPE feature_thres_12_37[2] =
{
-0.006181,0.000657,
};
int MEMORY_TYPE feature_left_12_37[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_37[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_37[3] =
{
0.641013,0.562087,0.439032,
};
HaarFeature MEMORY_TYPE haar_12_38[2] =
{
0,mvcvRect(6,8,6,5),-1.000000,mvcvRect(8,8,2,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,4,4,6),-1.000000,mvcvRect(0,6,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_38[2] =
{
0.026228,-0.017958,
};
int MEMORY_TYPE feature_left_12_38[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_38[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_38[3] =
{
0.644556,0.200271,0.462467,
};
HaarFeature MEMORY_TYPE haar_12_39[2] =
{
0,mvcvRect(9,1,3,6),-1.000000,mvcvRect(10,1,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,4,6,3),-1.000000,mvcvRect(10,5,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_39[2] =
{
-0.007647,-0.002748,
};
int MEMORY_TYPE feature_left_12_39[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_39[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_39[3] =
{
0.526320,0.587398,0.483660,
};
HaarFeature MEMORY_TYPE haar_12_40[2] =
{
0,mvcvRect(8,1,3,6),-1.000000,mvcvRect(9,1,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,4,6,3),-1.000000,mvcvRect(4,5,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_40[2] =
{
0.013852,0.002637,
};
int MEMORY_TYPE feature_left_12_40[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_40[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_40[3] =
{
0.156613,0.427018,0.580666,
};
HaarFeature MEMORY_TYPE haar_12_41[2] =
{
0,mvcvRect(12,4,3,3),-1.000000,mvcvRect(12,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,11,4,2),-1.000000,mvcvRect(12,12,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_41[2] =
{
-0.003151,-0.000015,
};
int MEMORY_TYPE feature_left_12_41[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_41[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_41[3] =
{
0.621587,0.557664,0.412200,
};
HaarFeature MEMORY_TYPE haar_12_42[2] =
{
0,mvcvRect(0,2,20,6),-1.000000,mvcvRect(0,2,10,3),2.000000,mvcvRect(10,5,10,3),2.000000,0,mvcvRect(5,4,3,3),-1.000000,mvcvRect(5,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_42[2] =
{
-0.073677,-0.003091,
};
int MEMORY_TYPE feature_left_12_42[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_42[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_42[3] =
{
0.153671,0.634427,0.450741,
};
HaarFeature MEMORY_TYPE haar_12_43[2] =
{
0,mvcvRect(2,10,16,4),-1.000000,mvcvRect(10,10,8,2),2.000000,mvcvRect(2,12,8,2),2.000000,0,mvcvRect(3,10,16,6),-1.000000,mvcvRect(11,10,8,3),2.000000,mvcvRect(3,13,8,3),2.000000,
};
float MEMORY_TYPE feature_thres_12_43[2] =
{
0.007924,0.008578,
};
int MEMORY_TYPE feature_left_12_43[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_43[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_43[3] =
{
0.545798,0.540166,0.389080,
};
HaarFeature MEMORY_TYPE haar_12_44[2] =
{
0,mvcvRect(1,10,16,6),-1.000000,mvcvRect(1,10,8,3),2.000000,mvcvRect(9,13,8,3),2.000000,0,mvcvRect(4,7,2,4),-1.000000,mvcvRect(5,7,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_44[2] =
{
0.005540,-0.000119,
};
int MEMORY_TYPE feature_left_12_44[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_44[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_44[3] =
{
0.355561,0.583675,0.427432,
};
HaarFeature MEMORY_TYPE haar_12_45[2] =
{
0,mvcvRect(11,16,9,4),-1.000000,mvcvRect(14,16,3,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,16,14,4),-1.000000,mvcvRect(10,16,7,2),2.000000,mvcvRect(3,18,7,2),2.000000,
};
float MEMORY_TYPE feature_thres_12_45[2] =
{
-0.018408,-0.002349,
};
int MEMORY_TYPE feature_left_12_45[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_45[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_45[3] =
{
0.586044,0.449896,0.549820,
};
HaarFeature MEMORY_TYPE haar_12_46[2] =
{
0,mvcvRect(0,16,9,4),-1.000000,mvcvRect(3,16,3,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,14,6,6),-1.000000,mvcvRect(1,14,3,3),2.000000,mvcvRect(4,17,3,3),2.000000,
};
float MEMORY_TYPE feature_thres_12_46[2] =
{
-0.007616,-0.003319,
};
int MEMORY_TYPE feature_left_12_46[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_46[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_46[3] =
{
0.410099,0.670138,0.435300,
};
HaarFeature MEMORY_TYPE haar_12_47[2] =
{
0,mvcvRect(9,0,2,1),-1.000000,mvcvRect(9,0,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,7,8,10),-1.000000,mvcvRect(10,7,4,5),2.000000,mvcvRect(6,12,4,5),2.000000,
};
float MEMORY_TYPE feature_thres_12_47[2] =
{
-0.000946,0.008786,
};
int MEMORY_TYPE feature_left_12_47[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_47[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_47[3] =
{
0.539118,0.550405,0.399094,
};
HaarFeature MEMORY_TYPE haar_12_48[2] =
{
0,mvcvRect(2,15,1,2),-1.000000,mvcvRect(2,16,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,14,7,6),-1.000000,mvcvRect(0,16,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_48[2] =
{
0.000164,-0.002351,
};
int MEMORY_TYPE feature_left_12_48[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_48[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_48[3] =
{
0.359293,0.403417,0.580608,
};
HaarFeature MEMORY_TYPE haar_12_49[2] =
{
0,mvcvRect(7,8,6,2),-1.000000,mvcvRect(7,9,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,2,2,15),-1.000000,mvcvRect(9,7,2,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_49[2] =
{
0.000075,0.027018,
};
int MEMORY_TYPE feature_left_12_49[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_49[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_49[3] =
{
0.541238,0.494492,0.558944,
};
HaarFeature MEMORY_TYPE haar_12_50[2] =
{
0,mvcvRect(5,6,2,2),-1.000000,mvcvRect(5,7,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,6,8,3),-1.000000,mvcvRect(6,7,8,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_50[2] =
{
0.000846,-0.001169,
};
int MEMORY_TYPE feature_left_12_50[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_50[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_50[3] =
{
0.580922,0.474696,0.284590,
};
HaarFeature MEMORY_TYPE haar_12_51[2] =
{
0,mvcvRect(12,13,5,6),-1.000000,mvcvRect(12,15,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,20,18),-1.000000,mvcvRect(0,9,20,9),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_51[2] =
{
0.022898,0.708793,
};
int MEMORY_TYPE feature_left_12_51[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_51[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_51[3] =
{
0.241441,0.519576,0.103009,
};
HaarFeature MEMORY_TYPE haar_12_52[2] =
{
0,mvcvRect(5,1,6,6),-1.000000,mvcvRect(7,1,2,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,1,4,9),-1.000000,mvcvRect(7,1,2,9),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_52[2] =
{
0.037484,0.001283,
};
int MEMORY_TYPE feature_left_12_52[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_52[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_52[3] =
{
0.181464,0.424607,0.570797,
};
HaarFeature MEMORY_TYPE haar_12_53[2] =
{
0,mvcvRect(1,19,18,1),-1.000000,mvcvRect(7,19,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,16,5,2),-1.000000,mvcvRect(14,17,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_53[2] =
{
-0.005172,0.002755,
};
int MEMORY_TYPE feature_left_12_53[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_53[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_53[3] =
{
0.614332,0.520567,0.422044,
};
HaarFeature MEMORY_TYPE haar_12_54[2] =
{
0,mvcvRect(0,5,15,10),-1.000000,mvcvRect(0,10,15,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,15,4,2),-1.000000,mvcvRect(7,15,2,1),2.000000,mvcvRect(9,16,2,1),2.000000,
};
float MEMORY_TYPE feature_thres_12_54[2] =
{
-0.003607,-0.000253,
};
int MEMORY_TYPE feature_left_12_54[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_54[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_54[3] =
{
0.318259,0.571047,0.422609,
};
HaarFeature MEMORY_TYPE haar_12_55[2] =
{
0,mvcvRect(14,11,2,2),-1.000000,mvcvRect(14,12,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,8,3,3),-1.000000,mvcvRect(9,9,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_55[2] =
{
-0.007051,-0.005432,
};
int MEMORY_TYPE feature_left_12_55[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_55[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_55[3] =
{
0.516283,0.266629,0.521468,
};
HaarFeature MEMORY_TYPE haar_12_56[2] =
{
0,mvcvRect(4,11,2,2),-1.000000,mvcvRect(4,12,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,8,3,3),-1.000000,mvcvRect(8,9,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_56[2] =
{
-0.000015,-0.001856,
};
int MEMORY_TYPE feature_left_12_56[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_56[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_56[3] =
{
0.398176,0.332276,0.570583,
};
HaarFeature MEMORY_TYPE haar_12_57[2] =
{
0,mvcvRect(9,10,2,3),-1.000000,mvcvRect(9,11,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,8,4,3),-1.000000,mvcvRect(8,9,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_57[2] =
{
0.004761,0.001568,
};
int MEMORY_TYPE feature_left_12_57[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_57[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_57[3] =
{
0.663656,0.550557,0.442066,
};
HaarFeature MEMORY_TYPE haar_12_58[2] =
{
0,mvcvRect(1,9,4,10),-1.000000,mvcvRect(1,9,2,5),2.000000,mvcvRect(3,14,2,5),2.000000,0,mvcvRect(0,12,6,8),-1.000000,mvcvRect(2,12,2,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_58[2] =
{
0.005424,-0.006469,
};
int MEMORY_TYPE feature_left_12_58[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_12_58[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_12_58[3] =
{
0.595994,0.536959,0.374434,
};
HaarFeature MEMORY_TYPE haar_12_59[2] =
{
0,mvcvRect(9,1,4,2),-1.000000,mvcvRect(11,1,2,1),2.000000,mvcvRect(9,2,2,1),2.000000,0,mvcvRect(12,13,7,6),-1.000000,mvcvRect(12,15,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_59[2] =
{
-0.000780,0.045086,
};
int MEMORY_TYPE feature_left_12_59[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_59[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_59[3] =
{
0.410360,0.517751,0.187810,
};
HaarFeature MEMORY_TYPE haar_12_60[2] =
{
0,mvcvRect(7,0,2,3),-1.000000,mvcvRect(7,1,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,14,6,3),-1.000000,mvcvRect(9,14,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_60[2] =
{
-0.005141,-0.021236,
};
int MEMORY_TYPE feature_left_12_60[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_60[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_60[3] =
{
0.235289,0.170875,0.542497,
};
HaarFeature MEMORY_TYPE haar_12_61[2] =
{
0,mvcvRect(9,6,6,4),-1.000000,mvcvRect(11,6,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,10,8,3),-1.000000,mvcvRect(8,10,4,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_61[2] =
{
-0.002376,0.054123,
};
int MEMORY_TYPE feature_left_12_61[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_61[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_61[3] =
{
0.583653,0.511743,0.186593,
};
HaarFeature MEMORY_TYPE haar_12_62[2] =
{
0,mvcvRect(6,10,4,3),-1.000000,mvcvRect(8,10,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,8,3,5),-1.000000,mvcvRect(7,8,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_12_62[2] =
{
-0.000535,-0.000585,
};
int MEMORY_TYPE feature_left_12_62[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_12_62[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_12_62[3] =
{
0.510869,0.477549,0.243985,
};
HaarClassifier MEMORY_TYPE haar_feature_class_12[63] =
{
{2,haar_12_0,feature_thres_12_0,feature_left_12_0,feature_right_12_0,feature_alpha_12_0},{2,haar_12_1,feature_thres_12_1,feature_left_12_1,feature_right_12_1,feature_alpha_12_1},{2,haar_12_2,feature_thres_12_2,feature_left_12_2,feature_right_12_2,feature_alpha_12_2},{2,haar_12_3,feature_thres_12_3,feature_left_12_3,feature_right_12_3,feature_alpha_12_3},{2,haar_12_4,feature_thres_12_4,feature_left_12_4,feature_right_12_4,feature_alpha_12_4},{2,haar_12_5,feature_thres_12_5,feature_left_12_5,feature_right_12_5,feature_alpha_12_5},{2,haar_12_6,feature_thres_12_6,feature_left_12_6,feature_right_12_6,feature_alpha_12_6},{2,haar_12_7,feature_thres_12_7,feature_left_12_7,feature_right_12_7,feature_alpha_12_7},{2,haar_12_8,feature_thres_12_8,feature_left_12_8,feature_right_12_8,feature_alpha_12_8},{2,haar_12_9,feature_thres_12_9,feature_left_12_9,feature_right_12_9,feature_alpha_12_9},{2,haar_12_10,feature_thres_12_10,feature_left_12_10,feature_right_12_10,feature_alpha_12_10},{2,haar_12_11,feature_thres_12_11,feature_left_12_11,feature_right_12_11,feature_alpha_12_11},{2,haar_12_12,feature_thres_12_12,feature_left_12_12,feature_right_12_12,feature_alpha_12_12},{2,haar_12_13,feature_thres_12_13,feature_left_12_13,feature_right_12_13,feature_alpha_12_13},{2,haar_12_14,feature_thres_12_14,feature_left_12_14,feature_right_12_14,feature_alpha_12_14},{2,haar_12_15,feature_thres_12_15,feature_left_12_15,feature_right_12_15,feature_alpha_12_15},{2,haar_12_16,feature_thres_12_16,feature_left_12_16,feature_right_12_16,feature_alpha_12_16},{2,haar_12_17,feature_thres_12_17,feature_left_12_17,feature_right_12_17,feature_alpha_12_17},{2,haar_12_18,feature_thres_12_18,feature_left_12_18,feature_right_12_18,feature_alpha_12_18},{2,haar_12_19,feature_thres_12_19,feature_left_12_19,feature_right_12_19,feature_alpha_12_19},{2,haar_12_20,feature_thres_12_20,feature_left_12_20,feature_right_12_20,feature_alpha_12_20},{2,haar_12_21,feature_thres_12_21,feature_left_12_21,feature_right_12_21,feature_alpha_12_21},{2,haar_12_22,feature_thres_12_22,feature_left_12_22,feature_right_12_22,feature_alpha_12_22},{2,haar_12_23,feature_thres_12_23,feature_left_12_23,feature_right_12_23,feature_alpha_12_23},{2,haar_12_24,feature_thres_12_24,feature_left_12_24,feature_right_12_24,feature_alpha_12_24},{2,haar_12_25,feature_thres_12_25,feature_left_12_25,feature_right_12_25,feature_alpha_12_25},{2,haar_12_26,feature_thres_12_26,feature_left_12_26,feature_right_12_26,feature_alpha_12_26},{2,haar_12_27,feature_thres_12_27,feature_left_12_27,feature_right_12_27,feature_alpha_12_27},{2,haar_12_28,feature_thres_12_28,feature_left_12_28,feature_right_12_28,feature_alpha_12_28},{2,haar_12_29,feature_thres_12_29,feature_left_12_29,feature_right_12_29,feature_alpha_12_29},{2,haar_12_30,feature_thres_12_30,feature_left_12_30,feature_right_12_30,feature_alpha_12_30},{2,haar_12_31,feature_thres_12_31,feature_left_12_31,feature_right_12_31,feature_alpha_12_31},{2,haar_12_32,feature_thres_12_32,feature_left_12_32,feature_right_12_32,feature_alpha_12_32},{2,haar_12_33,feature_thres_12_33,feature_left_12_33,feature_right_12_33,feature_alpha_12_33},{2,haar_12_34,feature_thres_12_34,feature_left_12_34,feature_right_12_34,feature_alpha_12_34},{2,haar_12_35,feature_thres_12_35,feature_left_12_35,feature_right_12_35,feature_alpha_12_35},{2,haar_12_36,feature_thres_12_36,feature_left_12_36,feature_right_12_36,feature_alpha_12_36},{2,haar_12_37,feature_thres_12_37,feature_left_12_37,feature_right_12_37,feature_alpha_12_37},{2,haar_12_38,feature_thres_12_38,feature_left_12_38,feature_right_12_38,feature_alpha_12_38},{2,haar_12_39,feature_thres_12_39,feature_left_12_39,feature_right_12_39,feature_alpha_12_39},{2,haar_12_40,feature_thres_12_40,feature_left_12_40,feature_right_12_40,feature_alpha_12_40},{2,haar_12_41,feature_thres_12_41,feature_left_12_41,feature_right_12_41,feature_alpha_12_41},{2,haar_12_42,feature_thres_12_42,feature_left_12_42,feature_right_12_42,feature_alpha_12_42},{2,haar_12_43,feature_thres_12_43,feature_left_12_43,feature_right_12_43,feature_alpha_12_43},{2,haar_12_44,feature_thres_12_44,feature_left_12_44,feature_right_12_44,feature_alpha_12_44},{2,haar_12_45,feature_thres_12_45,feature_left_12_45,feature_right_12_45,feature_alpha_12_45},{2,haar_12_46,feature_thres_12_46,feature_left_12_46,feature_right_12_46,feature_alpha_12_46},{2,haar_12_47,feature_thres_12_47,feature_left_12_47,feature_right_12_47,feature_alpha_12_47},{2,haar_12_48,feature_thres_12_48,feature_left_12_48,feature_right_12_48,feature_alpha_12_48},{2,haar_12_49,feature_thres_12_49,feature_left_12_49,feature_right_12_49,feature_alpha_12_49},{2,haar_12_50,feature_thres_12_50,feature_left_12_50,feature_right_12_50,feature_alpha_12_50},{2,haar_12_51,feature_thres_12_51,feature_left_12_51,feature_right_12_51,feature_alpha_12_51},{2,haar_12_52,feature_thres_12_52,feature_left_12_52,feature_right_12_52,feature_alpha_12_52},{2,haar_12_53,feature_thres_12_53,feature_left_12_53,feature_right_12_53,feature_alpha_12_53},{2,haar_12_54,feature_thres_12_54,feature_left_12_54,feature_right_12_54,feature_alpha_12_54},{2,haar_12_55,feature_thres_12_55,feature_left_12_55,feature_right_12_55,feature_alpha_12_55},{2,haar_12_56,feature_thres_12_56,feature_left_12_56,feature_right_12_56,feature_alpha_12_56},{2,haar_12_57,feature_thres_12_57,feature_left_12_57,feature_right_12_57,feature_alpha_12_57},{2,haar_12_58,feature_thres_12_58,feature_left_12_58,feature_right_12_58,feature_alpha_12_58},{2,haar_12_59,feature_thres_12_59,feature_left_12_59,feature_right_12_59,feature_alpha_12_59},{2,haar_12_60,feature_thres_12_60,feature_left_12_60,feature_right_12_60,feature_alpha_12_60},{2,haar_12_61,feature_thres_12_61,feature_left_12_61,feature_right_12_61,feature_alpha_12_61},{2,haar_12_62,feature_thres_12_62,feature_left_12_62,feature_right_12_62,feature_alpha_12_62},
};
HaarFeature MEMORY_TYPE haar_13_0[2] =
{
0,mvcvRect(0,4,8,1),-1.000000,mvcvRect(4,4,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,2,2,6),-1.000000,mvcvRect(8,2,1,3),2.000000,mvcvRect(9,5,1,3),2.000000,
};
float MEMORY_TYPE feature_thres_13_0[2] =
{
0.003003,0.000692,
};
int MEMORY_TYPE feature_left_13_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_0[3] =
{
0.334965,0.451837,0.728935,
};
HaarFeature MEMORY_TYPE haar_13_1[2] =
{
0,mvcvRect(0,7,20,6),-1.000000,mvcvRect(0,9,20,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,10,3,6),-1.000000,mvcvRect(12,13,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_1[2] =
{
0.011213,-0.000761,
};
int MEMORY_TYPE feature_left_13_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_1[3] =
{
0.295080,0.566905,0.283085,
};
HaarFeature MEMORY_TYPE haar_13_2[2] =
{
0,mvcvRect(8,15,1,4),-1.000000,mvcvRect(8,17,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,16,2,4),-1.000000,mvcvRect(5,18,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_2[2] =
{
0.000120,-0.000197,
};
int MEMORY_TYPE feature_left_13_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_2[3] =
{
0.409058,0.695149,0.463787,
};
HaarFeature MEMORY_TYPE haar_13_3[2] =
{
0,mvcvRect(6,2,8,12),-1.000000,mvcvRect(6,6,8,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,12,2),-1.000000,mvcvRect(8,7,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_3[2] =
{
-0.005518,0.001215,
};
int MEMORY_TYPE feature_left_13_3[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_3[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_3[3] =
{
0.316768,0.331671,0.539640,
};
HaarFeature MEMORY_TYPE haar_13_4[2] =
{
0,mvcvRect(7,0,6,1),-1.000000,mvcvRect(9,0,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,11,3,3),-1.000000,mvcvRect(8,12,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_4[2] =
{
-0.004250,-0.009492,
};
int MEMORY_TYPE feature_left_13_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_4[3] =
{
0.260057,0.748429,0.507319,
};
HaarFeature MEMORY_TYPE haar_13_5[2] =
{
0,mvcvRect(12,11,3,6),-1.000000,mvcvRect(12,14,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,2,6,10),-1.000000,mvcvRect(14,2,3,5),2.000000,mvcvRect(11,7,3,5),2.000000,
};
float MEMORY_TYPE feature_thres_13_5[2] =
{
0.000654,-0.000497,
};
int MEMORY_TYPE feature_left_13_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_5[3] =
{
0.395201,0.588027,0.355212,
};
HaarFeature MEMORY_TYPE haar_13_6[2] =
{
0,mvcvRect(5,7,10,12),-1.000000,mvcvRect(5,7,5,6),2.000000,mvcvRect(10,13,5,6),2.000000,0,mvcvRect(4,4,2,10),-1.000000,mvcvRect(4,9,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_6[2] =
{
-0.043079,-0.000520,
};
int MEMORY_TYPE feature_left_13_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_6[3] =
{
0.243488,0.319556,0.558545,
};
HaarFeature MEMORY_TYPE haar_13_7[2] =
{
0,mvcvRect(9,7,2,3),-1.000000,mvcvRect(9,7,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,9,6,2),-1.000000,mvcvRect(11,9,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_7[2] =
{
-0.004545,-0.007961,
};
int MEMORY_TYPE feature_left_13_7[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_7[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_7[3] =
{
0.484529,0.380118,0.535851,
};
HaarFeature MEMORY_TYPE haar_13_8[2] =
{
0,mvcvRect(4,7,2,2),-1.000000,mvcvRect(5,7,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,2,4,6),-1.000000,mvcvRect(0,4,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_8[2] =
{
-0.000319,-0.019224,
};
int MEMORY_TYPE feature_left_13_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_8[3] =
{
0.435633,0.261307,0.615550,
};
HaarFeature MEMORY_TYPE haar_13_9[2] =
{
0,mvcvRect(10,7,3,4),-1.000000,mvcvRect(11,7,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,3,5),-1.000000,mvcvRect(10,7,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_9[2] =
{
-0.001308,0.019825,
};
int MEMORY_TYPE feature_left_13_9[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_9[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_9[3] =
{
0.594206,0.494543,0.738486,
};
HaarFeature MEMORY_TYPE haar_13_10[2] =
{
0,mvcvRect(9,1,1,3),-1.000000,mvcvRect(9,2,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,6,16,6),-1.000000,mvcvRect(0,6,8,3),2.000000,mvcvRect(8,9,8,3),2.000000,
};
float MEMORY_TYPE feature_thres_13_10[2] =
{
-0.002201,-0.007860,
};
int MEMORY_TYPE feature_left_13_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_10[3] =
{
0.221448,0.360098,0.529855,
};
HaarFeature MEMORY_TYPE haar_13_11[2] =
{
0,mvcvRect(10,15,3,3),-1.000000,mvcvRect(10,16,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,14,4,3),-1.000000,mvcvRect(9,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_11[2] =
{
0.001414,-0.011233,
};
int MEMORY_TYPE feature_left_13_11[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_11[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_11[3] =
{
0.577657,0.693446,0.482721,
};
HaarFeature MEMORY_TYPE haar_13_12[2] =
{
0,mvcvRect(3,2,6,10),-1.000000,mvcvRect(3,2,3,5),2.000000,mvcvRect(6,7,3,5),2.000000,0,mvcvRect(3,0,14,2),-1.000000,mvcvRect(3,1,14,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_12[2] =
{
0.002975,0.000533,
};
int MEMORY_TYPE feature_left_13_12[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_12[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_12[3] =
{
0.321668,0.396250,0.568036,
};
HaarFeature MEMORY_TYPE haar_13_13[2] =
{
0,mvcvRect(9,14,3,3),-1.000000,mvcvRect(9,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,15,3,3),-1.000000,mvcvRect(10,16,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_13[2] =
{
0.010105,-0.011654,
};
int MEMORY_TYPE feature_left_13_13[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_13[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_13[3] =
{
0.756742,0.652356,0.502705,
};
HaarFeature MEMORY_TYPE haar_13_14[2] =
{
0,mvcvRect(9,13,2,6),-1.000000,mvcvRect(9,16,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,13,6,3),-1.000000,mvcvRect(7,14,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_14[2] =
{
-0.007061,0.002234,
};
int MEMORY_TYPE feature_left_13_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_14[3] =
{
0.253877,0.438728,0.617763,
};
HaarFeature MEMORY_TYPE haar_13_15[2] =
{
0,mvcvRect(12,11,3,6),-1.000000,mvcvRect(12,14,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,12,5,2),-1.000000,mvcvRect(8,13,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_15[2] =
{
-0.029802,0.001161,
};
int MEMORY_TYPE feature_left_13_15[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_15[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_15[3] =
{
0.520114,0.464791,0.618425,
};
HaarFeature MEMORY_TYPE haar_13_16[2] =
{
0,mvcvRect(5,11,3,6),-1.000000,mvcvRect(5,14,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,12,3,2),-1.000000,mvcvRect(8,13,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_16[2] =
{
0.000948,0.000413,
};
int MEMORY_TYPE feature_left_13_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_16[3] =
{
0.304099,0.451881,0.624578,
};
HaarFeature MEMORY_TYPE haar_13_17[2] =
{
0,mvcvRect(11,13,7,6),-1.000000,mvcvRect(11,15,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,14,6,3),-1.000000,mvcvRect(7,15,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_17[2] =
{
-0.031204,0.002765,
};
int MEMORY_TYPE feature_left_13_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_17[3] =
{
0.278894,0.469850,0.650245,
};
HaarFeature MEMORY_TYPE haar_13_18[2] =
{
0,mvcvRect(3,13,14,4),-1.000000,mvcvRect(3,13,7,2),2.000000,mvcvRect(10,15,7,2),2.000000,0,mvcvRect(8,14,4,6),-1.000000,mvcvRect(8,14,2,3),2.000000,mvcvRect(10,17,2,3),2.000000,
};
float MEMORY_TYPE feature_thres_13_18[2] =
{
0.025645,-0.007533,
};
int MEMORY_TYPE feature_left_13_18[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_18[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_18[3] =
{
0.180517,0.320807,0.552202,
};
HaarFeature MEMORY_TYPE haar_13_19[2] =
{
0,mvcvRect(8,15,4,3),-1.000000,mvcvRect(8,16,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,16,6,2),-1.000000,mvcvRect(9,16,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_19[2] =
{
0.003205,-0.000243,
};
int MEMORY_TYPE feature_left_13_19[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_19[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_19[3] =
{
0.643693,0.567671,0.450910,
};
HaarFeature MEMORY_TYPE haar_13_20[2] =
{
0,mvcvRect(7,7,6,2),-1.000000,mvcvRect(7,8,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,9,13,3),-1.000000,mvcvRect(3,10,13,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_20[2] =
{
-0.000620,-0.000801,
};
int MEMORY_TYPE feature_left_13_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_20[3] =
{
0.312215,0.296519,0.523049,
};
HaarFeature MEMORY_TYPE haar_13_21[2] =
{
0,mvcvRect(9,8,3,4),-1.000000,mvcvRect(9,10,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,10,4,3),-1.000000,mvcvRect(8,11,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_21[2] =
{
-0.000918,0.001224,
};
int MEMORY_TYPE feature_left_13_21[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_21[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_21[3] =
{
0.546471,0.461850,0.567955,
};
HaarFeature MEMORY_TYPE haar_13_22[2] =
{
0,mvcvRect(7,7,3,4),-1.000000,mvcvRect(8,7,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,3,5),-1.000000,mvcvRect(9,7,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_22[2] =
{
-0.000687,-0.001825,
};
int MEMORY_TYPE feature_left_13_22[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_22[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_22[3] =
{
0.543088,0.543362,0.338522,
};
HaarFeature MEMORY_TYPE haar_13_23[2] =
{
0,mvcvRect(12,3,3,4),-1.000000,mvcvRect(13,3,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,2,3),-1.000000,mvcvRect(9,7,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_23[2] =
{
-0.007457,0.005378,
};
int MEMORY_TYPE feature_left_13_23[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_23[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_23[3] =
{
0.526559,0.485722,0.681512,
};
HaarFeature MEMORY_TYPE haar_13_24[2] =
{
0,mvcvRect(5,3,3,4),-1.000000,mvcvRect(6,3,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,7,12,1),-1.000000,mvcvRect(7,7,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_24[2] =
{
0.003760,0.000878,
};
int MEMORY_TYPE feature_left_13_24[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_24[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_24[3] =
{
0.283216,0.396683,0.551248,
};
HaarFeature MEMORY_TYPE haar_13_25[2] =
{
0,mvcvRect(12,5,3,3),-1.000000,mvcvRect(12,6,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,2,6,2),-1.000000,mvcvRect(11,3,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_25[2] =
{
0.005508,-0.000759,
};
int MEMORY_TYPE feature_left_13_25[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_25[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_25[3] =
{
0.678462,0.390650,0.545720,
};
HaarFeature MEMORY_TYPE haar_13_26[2] =
{
0,mvcvRect(3,2,14,2),-1.000000,mvcvRect(3,2,7,1),2.000000,mvcvRect(10,3,7,1),2.000000,0,mvcvRect(6,1,7,14),-1.000000,mvcvRect(6,8,7,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_26[2] =
{
0.001635,-0.000128,
};
int MEMORY_TYPE feature_left_13_26[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_26[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_26[3] =
{
0.364020,0.582972,0.419498,
};
HaarFeature MEMORY_TYPE haar_13_27[2] =
{
0,mvcvRect(8,0,12,5),-1.000000,mvcvRect(8,0,6,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,9,18,1),-1.000000,mvcvRect(7,9,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_27[2] =
{
0.022068,-0.019204,
};
int MEMORY_TYPE feature_left_13_27[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_27[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_27[3] =
{
0.460670,0.326148,0.523608,
};
HaarFeature MEMORY_TYPE haar_13_28[2] =
{
0,mvcvRect(0,0,10,5),-1.000000,mvcvRect(5,0,5,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,5,8,15),-1.000000,mvcvRect(2,10,8,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_28[2] =
{
-0.012998,-0.003133,
};
int MEMORY_TYPE feature_left_13_28[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_28[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_28[3] =
{
0.702211,0.287047,0.507648,
};
HaarFeature MEMORY_TYPE haar_13_29[2] =
{
0,mvcvRect(12,5,3,3),-1.000000,mvcvRect(12,6,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,4,2,3),-1.000000,mvcvRect(13,5,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_29[2] =
{
-0.005294,0.002186,
};
int MEMORY_TYPE feature_left_13_29[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_29[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_29[3] =
{
0.470952,0.470829,0.616984,
};
HaarFeature MEMORY_TYPE haar_13_30[2] =
{
0,mvcvRect(2,15,4,3),-1.000000,mvcvRect(2,16,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,10,3),-1.000000,mvcvRect(10,6,5,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_30[2] =
{
-0.004575,-0.045152,
};
int MEMORY_TYPE feature_left_13_30[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_30[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_30[3] =
{
0.311425,0.185144,0.550481,
};
HaarFeature MEMORY_TYPE haar_13_31[2] =
{
0,mvcvRect(11,6,2,2),-1.000000,mvcvRect(12,6,1,1),2.000000,mvcvRect(11,7,1,1),2.000000,0,mvcvRect(12,4,4,3),-1.000000,mvcvRect(12,5,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_31[2] =
{
-0.002778,-0.002575,
};
int MEMORY_TYPE feature_left_13_31[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_31[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_31[3] =
{
0.493735,0.615295,0.473550,
};
HaarFeature MEMORY_TYPE haar_13_32[2] =
{
0,mvcvRect(7,6,2,2),-1.000000,mvcvRect(7,6,1,1),2.000000,mvcvRect(8,7,1,1),2.000000,0,mvcvRect(4,4,4,3),-1.000000,mvcvRect(4,5,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_32[2] =
{
0.001161,0.002335,
};
int MEMORY_TYPE feature_left_13_32[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_32[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_32[3] =
{
0.651057,0.408834,0.568415,
};
HaarFeature MEMORY_TYPE haar_13_33[2] =
{
0,mvcvRect(11,4,3,3),-1.000000,mvcvRect(12,4,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,3,2,1),-1.000000,mvcvRect(9,3,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_33[2] =
{
0.003850,0.002453,
};
int MEMORY_TYPE feature_left_13_33[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_33[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_33[3] =
{
0.302583,0.523250,0.201762,
};
HaarFeature MEMORY_TYPE haar_13_34[2] =
{
0,mvcvRect(4,5,5,3),-1.000000,mvcvRect(4,6,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,6,4,3),-1.000000,mvcvRect(4,7,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_34[2] =
{
0.003673,0.002194,
};
int MEMORY_TYPE feature_left_13_34[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_34[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_34[3] =
{
0.642843,0.432887,0.642051,
};
HaarFeature MEMORY_TYPE haar_13_35[2] =
{
0,mvcvRect(11,4,3,3),-1.000000,mvcvRect(12,4,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,8,4,3),-1.000000,mvcvRect(8,9,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_35[2] =
{
-0.006467,-0.005719,
};
int MEMORY_TYPE feature_left_13_35[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_35[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_35[3] =
{
0.525407,0.249098,0.528762,
};
HaarFeature MEMORY_TYPE haar_13_36[2] =
{
0,mvcvRect(6,4,3,3),-1.000000,mvcvRect(7,4,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,14,1,3),-1.000000,mvcvRect(4,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_36[2] =
{
0.000999,-0.000783,
};
int MEMORY_TYPE feature_left_13_36[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_36[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_36[3] =
{
0.332980,0.359834,0.549834,
};
HaarFeature MEMORY_TYPE haar_13_37[2] =
{
0,mvcvRect(9,7,2,3),-1.000000,mvcvRect(9,7,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(17,0,3,2),-1.000000,mvcvRect(17,1,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_37[2] =
{
0.004323,0.004084,
};
int MEMORY_TYPE feature_left_13_37[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_37[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_37[3] =
{
0.481871,0.526633,0.310579,
};
HaarFeature MEMORY_TYPE haar_13_38[2] =
{
0,mvcvRect(8,10,2,9),-1.000000,mvcvRect(8,13,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,8,18,2),-1.000000,mvcvRect(0,9,18,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_38[2] =
{
0.000305,0.001264,
};
int MEMORY_TYPE feature_left_13_38[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_38[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_38[3] =
{
0.399529,0.322844,0.581922,
};
HaarFeature MEMORY_TYPE haar_13_39[2] =
{
0,mvcvRect(9,15,2,3),-1.000000,mvcvRect(9,16,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,4,3),-1.000000,mvcvRect(8,8,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_39[2] =
{
-0.010153,-0.002686,
};
int MEMORY_TYPE feature_left_13_39[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_39[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_39[3] =
{
0.802607,0.387562,0.546657,
};
HaarFeature MEMORY_TYPE haar_13_40[2] =
{
0,mvcvRect(1,14,6,6),-1.000000,mvcvRect(1,14,3,3),2.000000,mvcvRect(4,17,3,3),2.000000,0,mvcvRect(0,18,6,2),-1.000000,mvcvRect(0,19,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_40[2] =
{
-0.009052,-0.006320,
};
int MEMORY_TYPE feature_left_13_40[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_40[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_40[3] =
{
0.437206,0.112655,0.639542,
};
HaarFeature MEMORY_TYPE haar_13_41[2] =
{
0,mvcvRect(12,9,4,3),-1.000000,mvcvRect(12,9,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,8,3,8),-1.000000,mvcvRect(10,8,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_41[2] =
{
0.002612,0.014339,
};
int MEMORY_TYPE feature_left_13_41[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_41[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_41[3] =
{
0.542399,0.497927,0.604224,
};
HaarFeature MEMORY_TYPE haar_13_42[2] =
{
0,mvcvRect(4,9,4,3),-1.000000,mvcvRect(6,9,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,18,6,1),-1.000000,mvcvRect(6,18,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_42[2] =
{
0.002845,0.000015,
};
int MEMORY_TYPE feature_left_13_42[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_42[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_42[3] =
{
0.349109,0.419507,0.577597,
};
HaarFeature MEMORY_TYPE haar_13_43[2] =
{
0,mvcvRect(9,7,3,2),-1.000000,mvcvRect(10,7,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,7,8,12),-1.000000,mvcvRect(10,7,4,6),2.000000,mvcvRect(6,13,4,6),2.000000,
};
float MEMORY_TYPE feature_thres_13_43[2] =
{
0.008181,0.006632,
};
int MEMORY_TYPE feature_left_13_43[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_43[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_43[3] =
{
0.488599,0.544447,0.442100,
};
HaarFeature MEMORY_TYPE haar_13_44[2] =
{
0,mvcvRect(8,7,3,2),-1.000000,mvcvRect(9,7,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,3,6),-1.000000,mvcvRect(9,7,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_44[2] =
{
-0.002248,0.012375,
};
int MEMORY_TYPE feature_left_13_44[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_44[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_44[3] =
{
0.669979,0.447861,0.656489,
};
HaarFeature MEMORY_TYPE haar_13_45[2] =
{
0,mvcvRect(3,16,14,4),-1.000000,mvcvRect(10,16,7,2),2.000000,mvcvRect(3,18,7,2),2.000000,0,mvcvRect(1,14,18,4),-1.000000,mvcvRect(10,14,9,2),2.000000,mvcvRect(1,16,9,2),2.000000,
};
float MEMORY_TYPE feature_thres_13_45[2] =
{
-0.006652,-0.008575,
};
int MEMORY_TYPE feature_left_13_45[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_45[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_45[3] =
{
0.551188,0.401745,0.540554,
};
HaarFeature MEMORY_TYPE haar_13_46[2] =
{
0,mvcvRect(8,7,3,3),-1.000000,mvcvRect(8,8,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,4,20,12),-1.000000,mvcvRect(0,4,10,6),2.000000,mvcvRect(10,10,10,6),2.000000,
};
float MEMORY_TYPE feature_thres_13_46[2] =
{
0.006508,0.028675,
};
int MEMORY_TYPE feature_left_13_46[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_46[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_46[3] =
{
0.229439,0.517790,0.356776,
};
HaarFeature MEMORY_TYPE haar_13_47[2] =
{
0,mvcvRect(5,5,10,12),-1.000000,mvcvRect(10,5,5,6),2.000000,mvcvRect(5,11,5,6),2.000000,0,mvcvRect(10,2,4,7),-1.000000,mvcvRect(10,2,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_47[2] =
{
0.007067,0.001237,
};
int MEMORY_TYPE feature_left_13_47[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_47[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_47[3] =
{
0.556470,0.362770,0.557241,
};
HaarFeature MEMORY_TYPE haar_13_48[2] =
{
0,mvcvRect(8,11,4,3),-1.000000,mvcvRect(8,12,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,12,3,3),-1.000000,mvcvRect(8,13,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_48[2] =
{
0.007482,0.004711,
};
int MEMORY_TYPE feature_left_13_48[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_48[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_48[3] =
{
0.678491,0.412125,0.607224,
};
HaarFeature MEMORY_TYPE haar_13_49[2] =
{
0,mvcvRect(13,13,5,6),-1.000000,mvcvRect(13,15,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,0,6,6),-1.000000,mvcvRect(9,0,2,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_49[2] =
{
-0.006941,0.033302,
};
int MEMORY_TYPE feature_left_13_49[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_49[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_49[3] =
{
0.545977,0.527671,0.237492,
};
HaarFeature MEMORY_TYPE haar_13_50[2] =
{
0,mvcvRect(2,13,5,6),-1.000000,mvcvRect(2,15,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,4,2,12),-1.000000,mvcvRect(0,4,1,6),2.000000,mvcvRect(1,10,1,6),2.000000,
};
float MEMORY_TYPE feature_thres_13_50[2] =
{
0.036105,0.019675,
};
int MEMORY_TYPE feature_left_13_50[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_50[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_50[3] =
{
0.072493,0.462635,0.820896,
};
HaarFeature MEMORY_TYPE haar_13_51[2] =
{
0,mvcvRect(9,19,3,1),-1.000000,mvcvRect(10,19,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(18,0,2,6),-1.000000,mvcvRect(18,2,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_51[2] =
{
0.003477,0.001399,
};
int MEMORY_TYPE feature_left_13_51[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_51[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_51[3] =
{
0.520873,0.548441,0.423003,
};
HaarFeature MEMORY_TYPE haar_13_52[2] =
{
0,mvcvRect(0,3,1,6),-1.000000,mvcvRect(0,5,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,3,6),-1.000000,mvcvRect(0,2,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_52[2] =
{
0.004097,0.002697,
};
int MEMORY_TYPE feature_left_13_52[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_52[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_52[3] =
{
0.278055,0.540383,0.379099,
};
HaarFeature MEMORY_TYPE haar_13_53[2] =
{
0,mvcvRect(17,2,3,7),-1.000000,mvcvRect(18,2,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,3,4,7),-1.000000,mvcvRect(10,3,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_53[2] =
{
-0.005659,0.000395,
};
int MEMORY_TYPE feature_left_13_53[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_53[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_53[3] =
{
0.479834,0.376695,0.542923,
};
HaarFeature MEMORY_TYPE haar_13_54[2] =
{
0,mvcvRect(0,2,3,7),-1.000000,mvcvRect(1,2,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,2,4,8),-1.000000,mvcvRect(8,2,2,8),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_54[2] =
{
0.002175,0.001461,
};
int MEMORY_TYPE feature_left_13_54[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_54[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_54[3] =
{
0.620716,0.335795,0.514263,
};
HaarFeature MEMORY_TYPE haar_13_55[2] =
{
0,mvcvRect(13,0,1,4),-1.000000,mvcvRect(13,2,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,1,12,5),-1.000000,mvcvRect(9,1,4,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_55[2] =
{
-0.000530,0.148693,
};
int MEMORY_TYPE feature_left_13_55[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_55[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_55[3] =
{
0.534464,0.515961,0.256182,
};
HaarFeature MEMORY_TYPE haar_13_56[2] =
{
0,mvcvRect(6,0,1,4),-1.000000,mvcvRect(6,2,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,1,12,5),-1.000000,mvcvRect(7,1,4,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_56[2] =
{
-0.000059,-0.001628,
};
int MEMORY_TYPE feature_left_13_56[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_56[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_56[3] =
{
0.512309,0.601765,0.310937,
};
HaarFeature MEMORY_TYPE haar_13_57[2] =
{
0,mvcvRect(9,12,3,8),-1.000000,mvcvRect(10,12,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,13,6,1),-1.000000,mvcvRect(9,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_57[2] =
{
-0.012882,0.000950,
};
int MEMORY_TYPE feature_left_13_57[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_57[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_57[3] =
{
0.271229,0.544244,0.402889,
};
HaarFeature MEMORY_TYPE haar_13_58[2] =
{
0,mvcvRect(7,14,6,3),-1.000000,mvcvRect(7,15,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,16,7,3),-1.000000,mvcvRect(5,17,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_58[2] =
{
-0.012316,0.009029,
};
int MEMORY_TYPE feature_left_13_58[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_58[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_58[3] =
{
0.473607,0.745143,0.348799,
};
HaarFeature MEMORY_TYPE haar_13_59[2] =
{
0,mvcvRect(0,12,20,6),-1.000000,mvcvRect(0,14,20,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,18,14,2),-1.000000,mvcvRect(4,19,14,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_59[2] =
{
-0.086876,-0.000015,
};
int MEMORY_TYPE feature_left_13_59[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_59[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_59[3] =
{
0.229033,0.551789,0.439315,
};
HaarFeature MEMORY_TYPE haar_13_60[2] =
{
0,mvcvRect(8,12,3,8),-1.000000,mvcvRect(9,12,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,13,3,3),-1.000000,mvcvRect(7,14,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_60[2] =
{
-0.017458,-0.002522,
};
int MEMORY_TYPE feature_left_13_60[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_60[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_60[3] =
{
0.090168,0.623354,0.478946,
};
HaarFeature MEMORY_TYPE haar_13_61[2] =
{
0,mvcvRect(5,5,12,10),-1.000000,mvcvRect(11,5,6,5),2.000000,mvcvRect(5,10,6,5),2.000000,0,mvcvRect(8,1,5,10),-1.000000,mvcvRect(8,6,5,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_61[2] =
{
0.001066,-0.004254,
};
int MEMORY_TYPE feature_left_13_61[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_61[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_61[3] =
{
0.548970,0.557981,0.437588,
};
HaarFeature MEMORY_TYPE haar_13_62[2] =
{
0,mvcvRect(5,4,9,12),-1.000000,mvcvRect(5,10,9,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,13,6,6),-1.000000,mvcvRect(7,15,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_62[2] =
{
-0.009035,-0.001523,
};
int MEMORY_TYPE feature_left_13_62[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_62[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_62[3] =
{
0.357916,0.561366,0.393904,
};
HaarFeature MEMORY_TYPE haar_13_63[2] =
{
0,mvcvRect(8,4,5,16),-1.000000,mvcvRect(8,12,5,8),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,12,4,6),-1.000000,mvcvRect(8,15,4,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_63[2] =
{
0.002844,-0.003282,
};
int MEMORY_TYPE feature_left_13_63[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_63[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_63[3] =
{
0.390155,0.452862,0.544134,
};
HaarFeature MEMORY_TYPE haar_13_64[2] =
{
0,mvcvRect(7,13,2,2),-1.000000,mvcvRect(7,13,1,1),2.000000,mvcvRect(8,14,1,1),2.000000,0,mvcvRect(7,12,2,2),-1.000000,mvcvRect(7,12,1,1),2.000000,mvcvRect(8,13,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_13_64[2] =
{
0.000032,0.000030,
};
int MEMORY_TYPE feature_left_13_64[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_64[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_64[3] =
{
0.580311,0.333685,0.550486,
};
HaarFeature MEMORY_TYPE haar_13_65[2] =
{
0,mvcvRect(18,0,2,14),-1.000000,mvcvRect(18,0,1,14),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,11,7,2),-1.000000,mvcvRect(12,12,7,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_65[2] =
{
-0.005615,-0.017389,
};
int MEMORY_TYPE feature_left_13_65[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_65[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_65[3] =
{
0.612479,0.087272,0.520459,
};
HaarFeature MEMORY_TYPE haar_13_66[2] =
{
0,mvcvRect(1,18,1,2),-1.000000,mvcvRect(1,19,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,18,1,2),-1.000000,mvcvRect(2,19,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_66[2] =
{
-0.000044,0.000104,
};
int MEMORY_TYPE feature_left_13_66[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_66[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_66[3] =
{
0.393533,0.591885,0.411961,
};
HaarFeature MEMORY_TYPE haar_13_67[2] =
{
0,mvcvRect(9,7,2,1),-1.000000,mvcvRect(9,7,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,6,2,3),-1.000000,mvcvRect(9,6,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_67[2] =
{
0.001594,0.002544,
};
int MEMORY_TYPE feature_left_13_67[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_67[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_67[3] =
{
0.483962,0.478736,0.636066,
};
HaarFeature MEMORY_TYPE haar_13_68[2] =
{
0,mvcvRect(3,1,2,2),-1.000000,mvcvRect(4,1,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,0,3,2),-1.000000,mvcvRect(3,1,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_68[2] =
{
0.000015,-0.000099,
};
int MEMORY_TYPE feature_left_13_68[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_13_68[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_13_68[3] =
{
0.423112,0.427459,0.609405,
};
HaarFeature MEMORY_TYPE haar_13_69[2] =
{
0,mvcvRect(12,10,3,4),-1.000000,mvcvRect(12,12,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,7,8,2),-1.000000,mvcvRect(7,8,8,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_69[2] =
{
0.000554,0.001919,
};
int MEMORY_TYPE feature_left_13_69[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_69[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_69[3] =
{
0.427199,0.449711,0.554912,
};
HaarFeature MEMORY_TYPE haar_13_70[2] =
{
0,mvcvRect(8,8,3,4),-1.000000,mvcvRect(8,10,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,12,6,3),-1.000000,mvcvRect(7,13,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_13_70[2] =
{
-0.000508,0.001724,
};
int MEMORY_TYPE feature_left_13_70[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_13_70[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_13_70[3] =
{
0.547720,0.288292,0.561513,
};
HaarClassifier MEMORY_TYPE haar_feature_class_13[71] =
{
{2,haar_13_0,feature_thres_13_0,feature_left_13_0,feature_right_13_0,feature_alpha_13_0},{2,haar_13_1,feature_thres_13_1,feature_left_13_1,feature_right_13_1,feature_alpha_13_1},{2,haar_13_2,feature_thres_13_2,feature_left_13_2,feature_right_13_2,feature_alpha_13_2},{2,haar_13_3,feature_thres_13_3,feature_left_13_3,feature_right_13_3,feature_alpha_13_3},{2,haar_13_4,feature_thres_13_4,feature_left_13_4,feature_right_13_4,feature_alpha_13_4},{2,haar_13_5,feature_thres_13_5,feature_left_13_5,feature_right_13_5,feature_alpha_13_5},{2,haar_13_6,feature_thres_13_6,feature_left_13_6,feature_right_13_6,feature_alpha_13_6},{2,haar_13_7,feature_thres_13_7,feature_left_13_7,feature_right_13_7,feature_alpha_13_7},{2,haar_13_8,feature_thres_13_8,feature_left_13_8,feature_right_13_8,feature_alpha_13_8},{2,haar_13_9,feature_thres_13_9,feature_left_13_9,feature_right_13_9,feature_alpha_13_9},{2,haar_13_10,feature_thres_13_10,feature_left_13_10,feature_right_13_10,feature_alpha_13_10},{2,haar_13_11,feature_thres_13_11,feature_left_13_11,feature_right_13_11,feature_alpha_13_11},{2,haar_13_12,feature_thres_13_12,feature_left_13_12,feature_right_13_12,feature_alpha_13_12},{2,haar_13_13,feature_thres_13_13,feature_left_13_13,feature_right_13_13,feature_alpha_13_13},{2,haar_13_14,feature_thres_13_14,feature_left_13_14,feature_right_13_14,feature_alpha_13_14},{2,haar_13_15,feature_thres_13_15,feature_left_13_15,feature_right_13_15,feature_alpha_13_15},{2,haar_13_16,feature_thres_13_16,feature_left_13_16,feature_right_13_16,feature_alpha_13_16},{2,haar_13_17,feature_thres_13_17,feature_left_13_17,feature_right_13_17,feature_alpha_13_17},{2,haar_13_18,feature_thres_13_18,feature_left_13_18,feature_right_13_18,feature_alpha_13_18},{2,haar_13_19,feature_thres_13_19,feature_left_13_19,feature_right_13_19,feature_alpha_13_19},{2,haar_13_20,feature_thres_13_20,feature_left_13_20,feature_right_13_20,feature_alpha_13_20},{2,haar_13_21,feature_thres_13_21,feature_left_13_21,feature_right_13_21,feature_alpha_13_21},{2,haar_13_22,feature_thres_13_22,feature_left_13_22,feature_right_13_22,feature_alpha_13_22},{2,haar_13_23,feature_thres_13_23,feature_left_13_23,feature_right_13_23,feature_alpha_13_23},{2,haar_13_24,feature_thres_13_24,feature_left_13_24,feature_right_13_24,feature_alpha_13_24},{2,haar_13_25,feature_thres_13_25,feature_left_13_25,feature_right_13_25,feature_alpha_13_25},{2,haar_13_26,feature_thres_13_26,feature_left_13_26,feature_right_13_26,feature_alpha_13_26},{2,haar_13_27,feature_thres_13_27,feature_left_13_27,feature_right_13_27,feature_alpha_13_27},{2,haar_13_28,feature_thres_13_28,feature_left_13_28,feature_right_13_28,feature_alpha_13_28},{2,haar_13_29,feature_thres_13_29,feature_left_13_29,feature_right_13_29,feature_alpha_13_29},{2,haar_13_30,feature_thres_13_30,feature_left_13_30,feature_right_13_30,feature_alpha_13_30},{2,haar_13_31,feature_thres_13_31,feature_left_13_31,feature_right_13_31,feature_alpha_13_31},{2,haar_13_32,feature_thres_13_32,feature_left_13_32,feature_right_13_32,feature_alpha_13_32},{2,haar_13_33,feature_thres_13_33,feature_left_13_33,feature_right_13_33,feature_alpha_13_33},{2,haar_13_34,feature_thres_13_34,feature_left_13_34,feature_right_13_34,feature_alpha_13_34},{2,haar_13_35,feature_thres_13_35,feature_left_13_35,feature_right_13_35,feature_alpha_13_35},{2,haar_13_36,feature_thres_13_36,feature_left_13_36,feature_right_13_36,feature_alpha_13_36},{2,haar_13_37,feature_thres_13_37,feature_left_13_37,feature_right_13_37,feature_alpha_13_37},{2,haar_13_38,feature_thres_13_38,feature_left_13_38,feature_right_13_38,feature_alpha_13_38},{2,haar_13_39,feature_thres_13_39,feature_left_13_39,feature_right_13_39,feature_alpha_13_39},{2,haar_13_40,feature_thres_13_40,feature_left_13_40,feature_right_13_40,feature_alpha_13_40},{2,haar_13_41,feature_thres_13_41,feature_left_13_41,feature_right_13_41,feature_alpha_13_41},{2,haar_13_42,feature_thres_13_42,feature_left_13_42,feature_right_13_42,feature_alpha_13_42},{2,haar_13_43,feature_thres_13_43,feature_left_13_43,feature_right_13_43,feature_alpha_13_43},{2,haar_13_44,feature_thres_13_44,feature_left_13_44,feature_right_13_44,feature_alpha_13_44},{2,haar_13_45,feature_thres_13_45,feature_left_13_45,feature_right_13_45,feature_alpha_13_45},{2,haar_13_46,feature_thres_13_46,feature_left_13_46,feature_right_13_46,feature_alpha_13_46},{2,haar_13_47,feature_thres_13_47,feature_left_13_47,feature_right_13_47,feature_alpha_13_47},{2,haar_13_48,feature_thres_13_48,feature_left_13_48,feature_right_13_48,feature_alpha_13_48},{2,haar_13_49,feature_thres_13_49,feature_left_13_49,feature_right_13_49,feature_alpha_13_49},{2,haar_13_50,feature_thres_13_50,feature_left_13_50,feature_right_13_50,feature_alpha_13_50},{2,haar_13_51,feature_thres_13_51,feature_left_13_51,feature_right_13_51,feature_alpha_13_51},{2,haar_13_52,feature_thres_13_52,feature_left_13_52,feature_right_13_52,feature_alpha_13_52},{2,haar_13_53,feature_thres_13_53,feature_left_13_53,feature_right_13_53,feature_alpha_13_53},{2,haar_13_54,feature_thres_13_54,feature_left_13_54,feature_right_13_54,feature_alpha_13_54},{2,haar_13_55,feature_thres_13_55,feature_left_13_55,feature_right_13_55,feature_alpha_13_55},{2,haar_13_56,feature_thres_13_56,feature_left_13_56,feature_right_13_56,feature_alpha_13_56},{2,haar_13_57,feature_thres_13_57,feature_left_13_57,feature_right_13_57,feature_alpha_13_57},{2,haar_13_58,feature_thres_13_58,feature_left_13_58,feature_right_13_58,feature_alpha_13_58},{2,haar_13_59,feature_thres_13_59,feature_left_13_59,feature_right_13_59,feature_alpha_13_59},{2,haar_13_60,feature_thres_13_60,feature_left_13_60,feature_right_13_60,feature_alpha_13_60},{2,haar_13_61,feature_thres_13_61,feature_left_13_61,feature_right_13_61,feature_alpha_13_61},{2,haar_13_62,feature_thres_13_62,feature_left_13_62,feature_right_13_62,feature_alpha_13_62},{2,haar_13_63,feature_thres_13_63,feature_left_13_63,feature_right_13_63,feature_alpha_13_63},{2,haar_13_64,feature_thres_13_64,feature_left_13_64,feature_right_13_64,feature_alpha_13_64},{2,haar_13_65,feature_thres_13_65,feature_left_13_65,feature_right_13_65,feature_alpha_13_65},{2,haar_13_66,feature_thres_13_66,feature_left_13_66,feature_right_13_66,feature_alpha_13_66},{2,haar_13_67,feature_thres_13_67,feature_left_13_67,feature_right_13_67,feature_alpha_13_67},{2,haar_13_68,feature_thres_13_68,feature_left_13_68,feature_right_13_68,feature_alpha_13_68},{2,haar_13_69,feature_thres_13_69,feature_left_13_69,feature_right_13_69,feature_alpha_13_69},{2,haar_13_70,feature_thres_13_70,feature_left_13_70,feature_right_13_70,feature_alpha_13_70},
};
HaarFeature MEMORY_TYPE haar_14_0[2] =
{
0,mvcvRect(0,2,10,3),-1.000000,mvcvRect(5,2,5,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,1,20,6),-1.000000,mvcvRect(0,3,20,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_0[2] =
{
0.013092,0.000414,
};
int MEMORY_TYPE feature_left_14_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_0[3] =
{
0.333887,0.309935,0.667749,
};
HaarFeature MEMORY_TYPE haar_14_1[2] =
{
0,mvcvRect(7,6,6,3),-1.000000,mvcvRect(9,6,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,7,14,4),-1.000000,mvcvRect(3,9,14,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_1[2] =
{
0.021836,0.048324,
};
int MEMORY_TYPE feature_left_14_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_1[3] =
{
0.436905,0.430172,0.615389,
};
HaarFeature MEMORY_TYPE haar_14_2[2] =
{
0,mvcvRect(5,7,3,6),-1.000000,mvcvRect(5,9,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,8,3,12),-1.000000,mvcvRect(8,12,3,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_2[2] =
{
0.001609,0.001347,
};
int MEMORY_TYPE feature_left_14_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_2[3] =
{
0.338733,0.624871,0.359413,
};
HaarFeature MEMORY_TYPE haar_14_3[2] =
{
0,mvcvRect(9,17,6,2),-1.000000,mvcvRect(12,17,3,1),2.000000,mvcvRect(9,18,3,1),2.000000,0,mvcvRect(10,17,4,3),-1.000000,mvcvRect(10,18,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_3[2] =
{
0.000177,0.000367,
};
int MEMORY_TYPE feature_left_14_3[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_3[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_3[3] =
{
0.386842,0.440935,0.547647,
};
HaarFeature MEMORY_TYPE haar_14_4[2] =
{
0,mvcvRect(4,2,4,2),-1.000000,mvcvRect(4,3,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,3,6,14),-1.000000,mvcvRect(9,3,2,14),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_4[2] =
{
-0.001235,0.001171,
};
int MEMORY_TYPE feature_left_14_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_4[3] =
{
0.326017,0.411135,0.608816,
};
HaarFeature MEMORY_TYPE haar_14_5[2] =
{
0,mvcvRect(15,13,1,6),-1.000000,mvcvRect(15,16,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,14,2,6),-1.000000,mvcvRect(13,16,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_5[2] =
{
-0.000030,0.000271,
};
int MEMORY_TYPE feature_left_14_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_5[3] =
{
0.426942,0.430647,0.581051,
};
HaarFeature MEMORY_TYPE haar_14_6[2] =
{
0,mvcvRect(4,11,5,6),-1.000000,mvcvRect(4,14,5,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,17,4,2),-1.000000,mvcvRect(6,17,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_6[2] =
{
-0.000080,0.000332,
};
int MEMORY_TYPE feature_left_14_6[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_6[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_6[3] =
{
0.366914,0.461066,0.629059,
};
HaarFeature MEMORY_TYPE haar_14_7[2] =
{
0,mvcvRect(0,6,20,2),-1.000000,mvcvRect(0,6,10,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,5,10,12),-1.000000,mvcvRect(11,5,5,6),2.000000,mvcvRect(6,11,5,6),2.000000,
};
float MEMORY_TYPE feature_thres_14_7[2] =
{
-0.052306,0.026880,
};
int MEMORY_TYPE feature_left_14_7[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_7[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_7[3] =
{
0.532869,0.521326,0.323122,
};
HaarFeature MEMORY_TYPE haar_14_8[2] =
{
0,mvcvRect(4,0,2,12),-1.000000,mvcvRect(4,0,1,6),2.000000,mvcvRect(5,6,1,6),2.000000,0,mvcvRect(4,1,6,2),-1.000000,mvcvRect(6,1,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_8[2] =
{
-0.000242,-0.001642,
};
int MEMORY_TYPE feature_left_14_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_8[3] =
{
0.356857,0.344066,0.562560,
};
HaarFeature MEMORY_TYPE haar_14_9[2] =
{
0,mvcvRect(13,7,2,1),-1.000000,mvcvRect(13,7,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,15,6),-1.000000,mvcvRect(5,7,15,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_9[2] =
{
-0.000268,-0.002265,
};
int MEMORY_TYPE feature_left_14_9[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_9[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_9[3] =
{
0.456117,0.532135,0.367415,
};
HaarFeature MEMORY_TYPE haar_14_10[2] =
{
0,mvcvRect(1,10,18,2),-1.000000,mvcvRect(1,10,9,1),2.000000,mvcvRect(10,11,9,1),2.000000,0,mvcvRect(1,6,15,7),-1.000000,mvcvRect(6,6,5,7),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_10[2] =
{
0.015627,0.162113,
};
int MEMORY_TYPE feature_left_14_10[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_10[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_10[3] =
{
0.202935,0.556303,0.261885,
};
HaarFeature MEMORY_TYPE haar_14_11[2] =
{
0,mvcvRect(8,14,4,3),-1.000000,mvcvRect(8,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,14,3,3),-1.000000,mvcvRect(9,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_11[2] =
{
-0.003739,-0.002088,
};
int MEMORY_TYPE feature_left_14_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_11[3] =
{
0.606219,0.595076,0.454512,
};
HaarFeature MEMORY_TYPE haar_14_12[2] =
{
0,mvcvRect(8,14,4,3),-1.000000,mvcvRect(8,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,13,3,2),-1.000000,mvcvRect(8,14,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_12[2] =
{
0.002333,0.000065,
};
int MEMORY_TYPE feature_left_14_12[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_12[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_12[3] =
{
0.643552,0.352073,0.517978,
};
HaarFeature MEMORY_TYPE haar_14_13[2] =
{
0,mvcvRect(15,14,5,3),-1.000000,mvcvRect(15,15,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,14,20,1),-1.000000,mvcvRect(0,14,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_13[2] =
{
0.007463,-0.022033,
};
int MEMORY_TYPE feature_left_14_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_13[3] =
{
0.532669,0.349198,0.542924,
};
HaarFeature MEMORY_TYPE haar_14_14[2] =
{
0,mvcvRect(0,14,6,3),-1.000000,mvcvRect(0,15,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,3,4,2),-1.000000,mvcvRect(5,4,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_14[2] =
{
-0.008308,-0.000433,
};
int MEMORY_TYPE feature_left_14_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_14[3] =
{
0.208402,0.396527,0.542545,
};
HaarFeature MEMORY_TYPE haar_14_15[2] =
{
0,mvcvRect(0,6,20,1),-1.000000,mvcvRect(0,6,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,3,10,14),-1.000000,mvcvRect(11,3,5,7),2.000000,mvcvRect(6,10,5,7),2.000000,
};
float MEMORY_TYPE feature_thres_14_15[2] =
{
-0.032209,-0.000904,
};
int MEMORY_TYPE feature_left_14_15[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_15[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_15[3] =
{
0.530641,0.545039,0.425670,
};
HaarFeature MEMORY_TYPE haar_14_16[2] =
{
0,mvcvRect(8,12,4,2),-1.000000,mvcvRect(8,13,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,3,8,6),-1.000000,mvcvRect(6,3,4,3),2.000000,mvcvRect(10,6,4,3),2.000000,
};
float MEMORY_TYPE feature_thres_14_16[2] =
{
0.002273,0.005982,
};
int MEMORY_TYPE feature_left_14_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_16[3] =
{
0.596861,0.475814,0.315094,
};
HaarFeature MEMORY_TYPE haar_14_17[2] =
{
0,mvcvRect(13,7,2,1),-1.000000,mvcvRect(13,7,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,3,10,14),-1.000000,mvcvRect(11,3,5,7),2.000000,mvcvRect(6,10,5,7),2.000000,
};
float MEMORY_TYPE feature_thres_14_17[2] =
{
-0.000589,-0.000882,
};
int MEMORY_TYPE feature_left_14_17[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_17[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_17[3] =
{
0.484775,0.542632,0.433834,
};
HaarFeature MEMORY_TYPE haar_14_18[2] =
{
0,mvcvRect(5,7,2,1),-1.000000,mvcvRect(6,7,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,3,10,14),-1.000000,mvcvRect(4,3,5,7),2.000000,mvcvRect(9,10,5,7),2.000000,
};
float MEMORY_TYPE feature_thres_14_18[2] =
{
-0.000074,0.000391,
};
int MEMORY_TYPE feature_left_14_18[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_18[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_18[3] =
{
0.428751,0.634519,0.410185,
};
HaarFeature MEMORY_TYPE haar_14_19[2] =
{
0,mvcvRect(9,7,2,2),-1.000000,mvcvRect(9,7,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,3,20,1),-1.000000,mvcvRect(0,3,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_19[2] =
{
-0.003694,-0.011208,
};
int MEMORY_TYPE feature_left_14_19[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_19[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_19[3] =
{
0.484910,0.414634,0.547126,
};
HaarFeature MEMORY_TYPE haar_14_20[2] =
{
0,mvcvRect(2,1,10,3),-1.000000,mvcvRect(2,2,10,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,2,2),-1.000000,mvcvRect(10,7,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_20[2] =
{
-0.010337,0.003688,
};
int MEMORY_TYPE feature_left_14_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_20[3] =
{
0.287718,0.510190,0.721695,
};
HaarFeature MEMORY_TYPE haar_14_21[2] =
{
0,mvcvRect(9,17,3,2),-1.000000,mvcvRect(10,17,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,3,6),-1.000000,mvcvRect(10,7,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_21[2] =
{
-0.003898,-0.005999,
};
int MEMORY_TYPE feature_left_14_21[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_21[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_21[3] =
{
0.527618,0.661846,0.484163,
};
HaarFeature MEMORY_TYPE haar_14_22[2] =
{
0,mvcvRect(8,17,3,2),-1.000000,mvcvRect(9,17,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,3,6),-1.000000,mvcvRect(9,7,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_22[2] =
{
0.004504,0.017800,
};
int MEMORY_TYPE feature_left_14_22[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_22[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_22[3] =
{
0.187416,0.461693,0.708897,
};
HaarFeature MEMORY_TYPE haar_14_23[2] =
{
0,mvcvRect(16,3,4,6),-1.000000,mvcvRect(16,5,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,6,2,12),-1.000000,mvcvRect(16,6,1,6),2.000000,mvcvRect(15,12,1,6),2.000000,
};
float MEMORY_TYPE feature_thres_14_23[2] =
{
-0.018463,0.000015,
};
int MEMORY_TYPE feature_left_14_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_23[3] =
{
0.300198,0.456181,0.561079,
};
HaarFeature MEMORY_TYPE haar_14_24[2] =
{
0,mvcvRect(1,4,18,10),-1.000000,mvcvRect(1,4,9,5),2.000000,mvcvRect(10,9,9,5),2.000000,0,mvcvRect(9,4,2,4),-1.000000,mvcvRect(9,6,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_24[2] =
{
-0.086021,-0.000061,
};
int MEMORY_TYPE feature_left_14_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_24[3] =
{
0.234170,0.567229,0.419996,
};
HaarFeature MEMORY_TYPE haar_14_25[2] =
{
0,mvcvRect(12,5,3,2),-1.000000,mvcvRect(12,6,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,12,10,4),-1.000000,mvcvRect(5,14,10,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_25[2] =
{
0.001267,0.001370,
};
int MEMORY_TYPE feature_left_14_25[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_25[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_25[3] =
{
0.620748,0.539496,0.382386,
};
HaarFeature MEMORY_TYPE haar_14_26[2] =
{
0,mvcvRect(5,5,3,2),-1.000000,mvcvRect(5,6,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,6,12,6),-1.000000,mvcvRect(8,6,4,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_26[2] =
{
0.003316,-0.001453,
};
int MEMORY_TYPE feature_left_14_26[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_26[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_26[3] =
{
0.706168,0.306551,0.482737,
};
HaarFeature MEMORY_TYPE haar_14_27[2] =
{
0,mvcvRect(14,4,6,6),-1.000000,mvcvRect(14,6,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,0,4,6),-1.000000,mvcvRect(18,0,2,3),2.000000,mvcvRect(16,3,2,3),2.000000,
};
float MEMORY_TYPE feature_thres_14_27[2] =
{
-0.071492,0.001986,
};
int MEMORY_TYPE feature_left_14_27[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_27[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_27[3] =
{
0.519312,0.464244,0.580769,
};
HaarFeature MEMORY_TYPE haar_14_28[2] =
{
0,mvcvRect(0,4,6,6),-1.000000,mvcvRect(0,6,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,4,6),-1.000000,mvcvRect(0,0,2,3),2.000000,mvcvRect(2,3,2,3),2.000000,
};
float MEMORY_TYPE feature_thres_14_28[2] =
{
0.006252,0.002701,
};
int MEMORY_TYPE feature_left_14_28[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_28[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_28[3] =
{
0.294981,0.458589,0.602235,
};
HaarFeature MEMORY_TYPE haar_14_29[2] =
{
0,mvcvRect(12,0,8,5),-1.000000,mvcvRect(12,0,4,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,0,4,17),-1.000000,mvcvRect(16,0,2,17),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_29[2] =
{
0.011130,0.015093,
};
int MEMORY_TYPE feature_left_14_29[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_29[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_29[3] =
{
0.435784,0.456154,0.611906,
};
HaarFeature MEMORY_TYPE haar_14_30[2] =
{
0,mvcvRect(1,0,18,20),-1.000000,mvcvRect(7,0,6,20),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,0,2,5),-1.000000,mvcvRect(7,0,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_30[2] =
{
-0.027943,0.000044,
};
int MEMORY_TYPE feature_left_14_30[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_30[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_30[3] =
{
0.653714,0.347472,0.533697,
};
HaarFeature MEMORY_TYPE haar_14_31[2] =
{
0,mvcvRect(0,6,20,1),-1.000000,mvcvRect(0,6,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,6,4),-1.000000,mvcvRect(10,7,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_31[2] =
{
-0.012233,-0.000686,
};
int MEMORY_TYPE feature_left_14_31[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_31[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_31[3] =
{
0.373168,0.571723,0.479338,
};
HaarFeature MEMORY_TYPE haar_14_32[2] =
{
0,mvcvRect(1,1,16,4),-1.000000,mvcvRect(1,1,8,2),2.000000,mvcvRect(9,3,8,2),2.000000,0,mvcvRect(7,2,4,2),-1.000000,mvcvRect(7,2,2,1),2.000000,mvcvRect(9,3,2,1),2.000000,
};
float MEMORY_TYPE feature_thres_14_32[2] =
{
-0.003899,0.000491,
};
int MEMORY_TYPE feature_left_14_32[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_32[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_32[3] =
{
0.405644,0.617405,0.447175,
};
HaarFeature MEMORY_TYPE haar_14_33[2] =
{
0,mvcvRect(7,4,9,3),-1.000000,mvcvRect(7,5,9,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,4,5,12),-1.000000,mvcvRect(10,10,5,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_33[2] =
{
0.008212,-0.045564,
};
int MEMORY_TYPE feature_left_14_33[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_33[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_33[3] =
{
0.617970,0.228549,0.524957,
};
HaarFeature MEMORY_TYPE haar_14_34[2] =
{
0,mvcvRect(3,12,2,3),-1.000000,mvcvRect(3,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,8,3,5),-1.000000,mvcvRect(9,8,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_34[2] =
{
-0.005363,-0.012275,
};
int MEMORY_TYPE feature_left_14_34[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_34[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_34[3] =
{
0.178495,0.726195,0.455040,
};
HaarFeature MEMORY_TYPE haar_14_35[2] =
{
0,mvcvRect(13,9,2,3),-1.000000,mvcvRect(13,9,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,11,2,2),-1.000000,mvcvRect(15,12,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_35[2] =
{
0.005419,0.000818,
};
int MEMORY_TYPE feature_left_14_35[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_35[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_35[3] =
{
0.525299,0.544522,0.327222,
};
HaarFeature MEMORY_TYPE haar_14_36[2] =
{
0,mvcvRect(5,6,2,3),-1.000000,mvcvRect(5,7,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,11,6,2),-1.000000,mvcvRect(2,12,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_36[2] =
{
0.004136,0.000396,
};
int MEMORY_TYPE feature_left_14_36[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_36[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_36[3] =
{
0.701383,0.496594,0.329560,
};
HaarFeature MEMORY_TYPE haar_14_37[2] =
{
0,mvcvRect(15,11,4,3),-1.000000,mvcvRect(15,12,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,0,4,17),-1.000000,mvcvRect(16,0,2,17),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_37[2] =
{
0.004689,-0.018255,
};
int MEMORY_TYPE feature_left_14_37[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_37[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_37[3] =
{
0.536264,0.649611,0.475714,
};
HaarFeature MEMORY_TYPE haar_14_38[2] =
{
0,mvcvRect(1,11,4,3),-1.000000,mvcvRect(1,12,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,11,1,3),-1.000000,mvcvRect(9,12,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_38[2] =
{
-0.006274,0.002432,
};
int MEMORY_TYPE feature_left_14_38[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_38[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_38[3] =
{
0.234374,0.462012,0.689842,
};
HaarFeature MEMORY_TYPE haar_14_39[2] =
{
0,mvcvRect(10,9,6,7),-1.000000,mvcvRect(10,9,3,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,15,4,2),-1.000000,mvcvRect(8,16,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_39[2] =
{
-0.049618,0.001170,
};
int MEMORY_TYPE feature_left_14_39[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_39[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_39[3] =
{
0.210072,0.462153,0.579714,
};
HaarFeature MEMORY_TYPE haar_14_40[2] =
{
0,mvcvRect(4,9,6,7),-1.000000,mvcvRect(7,9,3,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,14,2,3),-1.000000,mvcvRect(9,15,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_40[2] =
{
-0.045237,0.004756,
};
int MEMORY_TYPE feature_left_14_40[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_40[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_40[3] =
{
0.211826,0.488461,0.687250,
};
HaarFeature MEMORY_TYPE haar_14_41[2] =
{
0,mvcvRect(0,2,20,2),-1.000000,mvcvRect(10,2,10,1),2.000000,mvcvRect(0,3,10,1),2.000000,0,mvcvRect(6,7,8,2),-1.000000,mvcvRect(6,8,8,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_41[2] =
{
-0.014836,0.000774,
};
int MEMORY_TYPE feature_left_14_41[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_41[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_41[3] =
{
0.527511,0.417232,0.549114,
};
HaarFeature MEMORY_TYPE haar_14_42[2] =
{
0,mvcvRect(0,2,20,2),-1.000000,mvcvRect(0,2,10,1),2.000000,mvcvRect(10,3,10,1),2.000000,0,mvcvRect(3,1,2,10),-1.000000,mvcvRect(3,1,1,5),2.000000,mvcvRect(4,6,1,5),2.000000,
};
float MEMORY_TYPE feature_thres_14_42[2] =
{
0.014836,-0.000809,
};
int MEMORY_TYPE feature_left_14_42[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_42[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_42[3] =
{
0.212488,0.549522,0.420780,
};
HaarFeature MEMORY_TYPE haar_14_43[2] =
{
0,mvcvRect(13,4,1,10),-1.000000,mvcvRect(13,9,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,8,4,3),-1.000000,mvcvRect(9,9,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_43[2] =
{
0.000775,-0.006762,
};
int MEMORY_TYPE feature_left_14_43[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_43[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_43[3] =
{
0.332194,0.221296,0.523265,
};
HaarFeature MEMORY_TYPE haar_14_44[2] =
{
0,mvcvRect(2,11,16,4),-1.000000,mvcvRect(2,11,8,2),2.000000,mvcvRect(10,13,8,2),2.000000,0,mvcvRect(5,1,3,5),-1.000000,mvcvRect(6,1,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_44[2] =
{
-0.040136,-0.003365,
};
int MEMORY_TYPE feature_left_14_44[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_44[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_44[3] =
{
0.110180,0.381010,0.561729,
};
HaarFeature MEMORY_TYPE haar_14_45[2] =
{
0,mvcvRect(9,10,2,3),-1.000000,mvcvRect(9,11,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,11,2,2),-1.000000,mvcvRect(9,12,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_45[2] =
{
0.000747,-0.004273,
};
int MEMORY_TYPE feature_left_14_45[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_45[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_45[3] =
{
0.579506,0.639227,0.471144,
};
HaarFeature MEMORY_TYPE haar_14_46[2] =
{
0,mvcvRect(0,10,20,2),-1.000000,mvcvRect(0,11,20,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,7,6,4),-1.000000,mvcvRect(1,7,3,2),2.000000,mvcvRect(4,9,3,2),2.000000,
};
float MEMORY_TYPE feature_thres_14_46[2] =
{
0.003620,0.000473,
};
int MEMORY_TYPE feature_left_14_46[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_46[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_46[3] =
{
0.340988,0.365930,0.538817,
};
HaarFeature MEMORY_TYPE haar_14_47[2] =
{
0,mvcvRect(12,0,8,8),-1.000000,mvcvRect(16,0,4,4),2.000000,mvcvRect(12,4,4,4),2.000000,0,mvcvRect(14,1,6,4),-1.000000,mvcvRect(16,1,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_47[2] =
{
0.033095,-0.011544,
};
int MEMORY_TYPE feature_left_14_47[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_47[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_47[3] =
{
0.717039,0.638682,0.468130,
};
HaarFeature MEMORY_TYPE haar_14_48[2] =
{
0,mvcvRect(6,3,2,14),-1.000000,mvcvRect(6,10,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,1,7,12),-1.000000,mvcvRect(6,7,7,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_48[2] =
{
-0.007423,-0.004225,
};
int MEMORY_TYPE feature_left_14_48[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_48[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_48[3] =
{
0.326370,0.576782,0.434642,
};
HaarFeature MEMORY_TYPE haar_14_49[2] =
{
0,mvcvRect(5,0,15,5),-1.000000,mvcvRect(10,0,5,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,0,4,10),-1.000000,mvcvRect(15,0,2,10),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_49[2] =
{
0.018133,0.007090,
};
int MEMORY_TYPE feature_left_14_49[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_49[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_49[3] =
{
0.469783,0.443739,0.606167,
};
HaarFeature MEMORY_TYPE haar_14_50[2] =
{
0,mvcvRect(1,0,18,3),-1.000000,mvcvRect(7,0,6,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,17,2),-1.000000,mvcvRect(0,1,17,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_50[2] =
{
-0.013273,0.000146,
};
int MEMORY_TYPE feature_left_14_50[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_50[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_50[3] =
{
0.655851,0.337635,0.509166,
};
HaarFeature MEMORY_TYPE haar_14_51[2] =
{
0,mvcvRect(10,0,3,3),-1.000000,mvcvRect(11,0,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,0,3,12),-1.000000,mvcvRect(11,0,1,12),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_51[2] =
{
-0.003579,-0.000470,
};
int MEMORY_TYPE feature_left_14_51[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_51[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_51[3] =
{
0.294788,0.555698,0.466546,
};
HaarFeature MEMORY_TYPE haar_14_52[2] =
{
0,mvcvRect(1,3,4,16),-1.000000,mvcvRect(1,3,2,8),2.000000,mvcvRect(3,11,2,8),2.000000,0,mvcvRect(7,0,3,3),-1.000000,mvcvRect(8,0,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_52[2] =
{
-0.048179,-0.000926,
};
int MEMORY_TYPE feature_left_14_52[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_52[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_52[3] =
{
0.733836,0.354387,0.528515,
};
HaarFeature MEMORY_TYPE haar_14_53[2] =
{
0,mvcvRect(9,13,2,6),-1.000000,mvcvRect(9,16,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,0,6,13),-1.000000,mvcvRect(11,0,2,13),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_53[2] =
{
-0.014781,-0.100275,
};
int MEMORY_TYPE feature_left_14_53[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_53[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_53[3] =
{
0.194444,0.099049,0.513985,
};
HaarFeature MEMORY_TYPE haar_14_54[2] =
{
0,mvcvRect(7,7,3,2),-1.000000,mvcvRect(8,7,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,2,1,12),-1.000000,mvcvRect(8,6,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_54[2] =
{
-0.000938,-0.002886,
};
int MEMORY_TYPE feature_left_14_54[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_54[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_54[3] =
{
0.582711,0.344143,0.514884,
};
HaarFeature MEMORY_TYPE haar_14_55[2] =
{
0,mvcvRect(4,10,12,6),-1.000000,mvcvRect(10,10,6,3),2.000000,mvcvRect(4,13,6,3),2.000000,0,mvcvRect(13,5,2,3),-1.000000,mvcvRect(13,6,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_55[2] =
{
-0.043683,0.002612,
};
int MEMORY_TYPE feature_left_14_55[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_55[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_55[3] =
{
0.520800,0.483550,0.632222,
};
HaarFeature MEMORY_TYPE haar_14_56[2] =
{
0,mvcvRect(4,10,12,6),-1.000000,mvcvRect(4,10,6,3),2.000000,mvcvRect(10,13,6,3),2.000000,0,mvcvRect(5,5,2,3),-1.000000,mvcvRect(5,6,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_56[2] =
{
0.043683,0.001718,
};
int MEMORY_TYPE feature_left_14_56[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_56[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_56[3] =
{
0.136454,0.453732,0.606675,
};
HaarFeature MEMORY_TYPE haar_14_57[2] =
{
0,mvcvRect(8,6,6,7),-1.000000,mvcvRect(10,6,2,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,6,2,4),-1.000000,mvcvRect(9,6,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_57[2] =
{
-0.033965,-0.001099,
};
int MEMORY_TYPE feature_left_14_57[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_57[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_57[3] =
{
0.496837,0.583168,0.468824,
};
HaarFeature MEMORY_TYPE haar_14_58[2] =
{
0,mvcvRect(6,6,6,7),-1.000000,mvcvRect(8,6,2,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,6,2,4),-1.000000,mvcvRect(10,6,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_58[2] =
{
0.054301,0.001099,
};
int MEMORY_TYPE feature_left_14_58[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_58[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_58[3] =
{
0.756829,0.433015,0.576847,
};
HaarFeature MEMORY_TYPE haar_14_59[2] =
{
0,mvcvRect(12,9,2,3),-1.000000,mvcvRect(12,9,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,6,20,1),-1.000000,mvcvRect(0,6,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_59[2] =
{
-0.000015,0.031416,
};
int MEMORY_TYPE feature_left_14_59[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_59[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_59[3] =
{
0.444328,0.527447,0.303786,
};
HaarFeature MEMORY_TYPE haar_14_60[2] =
{
0,mvcvRect(5,7,10,2),-1.000000,mvcvRect(10,7,5,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,16,4,3),-1.000000,mvcvRect(1,17,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_60[2] =
{
0.010832,0.000865,
};
int MEMORY_TYPE feature_left_14_60[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_60[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_60[3] =
{
0.358172,0.593758,0.429463,
};
HaarFeature MEMORY_TYPE haar_14_61[2] =
{
0,mvcvRect(12,4,3,3),-1.000000,mvcvRect(12,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,3,5,3),-1.000000,mvcvRect(10,4,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_61[2] =
{
0.002274,0.003934,
};
int MEMORY_TYPE feature_left_14_61[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_61[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_61[3] =
{
0.595458,0.479222,0.585613,
};
HaarFeature MEMORY_TYPE haar_14_62[2] =
{
0,mvcvRect(3,9,14,8),-1.000000,mvcvRect(3,9,7,4),2.000000,mvcvRect(10,13,7,4),2.000000,0,mvcvRect(6,8,8,10),-1.000000,mvcvRect(6,8,4,5),2.000000,mvcvRect(10,13,4,5),2.000000,
};
float MEMORY_TYPE feature_thres_14_62[2] =
{
0.008145,-0.005276,
};
int MEMORY_TYPE feature_left_14_62[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_62[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_62[3] =
{
0.357348,0.402602,0.576474,
};
HaarFeature MEMORY_TYPE haar_14_63[2] =
{
0,mvcvRect(12,4,3,3),-1.000000,mvcvRect(12,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,3,5,3),-1.000000,mvcvRect(10,4,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_63[2] =
{
-0.008379,0.001562,
};
int MEMORY_TYPE feature_left_14_63[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_63[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_63[3] =
{
0.498133,0.473659,0.558361,
};
HaarFeature MEMORY_TYPE haar_14_64[2] =
{
0,mvcvRect(5,4,3,3),-1.000000,mvcvRect(5,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,3,5,3),-1.000000,mvcvRect(5,4,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_64[2] =
{
0.003232,0.006680,
};
int MEMORY_TYPE feature_left_14_64[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_64[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_64[3] =
{
0.616744,0.413142,0.628070,
};
HaarFeature MEMORY_TYPE haar_14_65[2] =
{
0,mvcvRect(13,16,2,3),-1.000000,mvcvRect(13,17,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,5,20,6),-1.000000,mvcvRect(0,7,20,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_65[2] =
{
-0.003340,-0.209335,
};
int MEMORY_TYPE feature_left_14_65[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_14_65[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_14_65[3] =
{
0.344636,0.103866,0.520449,
};
HaarFeature MEMORY_TYPE haar_14_66[2] =
{
0,mvcvRect(3,14,3,3),-1.000000,mvcvRect(3,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,15,5,3),-1.000000,mvcvRect(7,16,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_66[2] =
{
0.006381,-0.006014,
};
int MEMORY_TYPE feature_left_14_66[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_66[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_66[3] =
{
0.216740,0.673840,0.489665,
};
HaarFeature MEMORY_TYPE haar_14_67[2] =
{
0,mvcvRect(12,9,2,3),-1.000000,mvcvRect(12,9,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,13,2,6),-1.000000,mvcvRect(15,13,1,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_67[2] =
{
-0.008176,0.000640,
};
int MEMORY_TYPE feature_left_14_67[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_67[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_67[3] =
{
0.517792,0.481965,0.546444,
};
HaarFeature MEMORY_TYPE haar_14_68[2] =
{
0,mvcvRect(6,9,2,3),-1.000000,mvcvRect(7,9,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,13,2,6),-1.000000,mvcvRect(4,13,1,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_68[2] =
{
0.001013,0.000498,
};
int MEMORY_TYPE feature_left_14_68[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_68[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_68[3] =
{
0.342360,0.448846,0.591267,
};
HaarFeature MEMORY_TYPE haar_14_69[2] =
{
0,mvcvRect(11,4,2,4),-1.000000,mvcvRect(11,4,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,4,2,5),-1.000000,mvcvRect(13,4,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_69[2] =
{
0.000136,0.013572,
};
int MEMORY_TYPE feature_left_14_69[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_69[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_69[3] =
{
0.556886,0.516107,0.171300,
};
HaarFeature MEMORY_TYPE haar_14_70[2] =
{
0,mvcvRect(7,4,2,4),-1.000000,mvcvRect(8,4,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,2,5),-1.000000,mvcvRect(6,4,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_70[2] =
{
0.000030,-0.003263,
};
int MEMORY_TYPE feature_left_14_70[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_70[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_70[3] =
{
0.491620,0.640466,0.285908,
};
HaarFeature MEMORY_TYPE haar_14_71[2] =
{
0,mvcvRect(19,6,1,2),-1.000000,mvcvRect(19,7,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,7,8,13),-1.000000,mvcvRect(12,7,4,13),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_71[2] =
{
-0.000192,0.021994,
};
int MEMORY_TYPE feature_left_14_71[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_71[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_71[3] =
{
0.545928,0.471571,0.569008,
};
HaarFeature MEMORY_TYPE haar_14_72[2] =
{
0,mvcvRect(0,6,1,2),-1.000000,mvcvRect(0,7,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,15,4,3),-1.000000,mvcvRect(6,16,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_72[2] =
{
0.000789,0.000509,
};
int MEMORY_TYPE feature_left_14_72[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_72[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_72[3] =
{
0.327983,0.430201,0.569605,
};
HaarFeature MEMORY_TYPE haar_14_73[2] =
{
0,mvcvRect(11,8,2,2),-1.000000,mvcvRect(11,9,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,7,2,4),-1.000000,mvcvRect(11,7,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_73[2] =
{
0.000117,0.008060,
};
int MEMORY_TYPE feature_left_14_73[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_73[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_73[3] =
{
0.538724,0.502142,0.596532,
};
HaarFeature MEMORY_TYPE haar_14_74[2] =
{
0,mvcvRect(4,13,2,3),-1.000000,mvcvRect(4,14,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,17,18,3),-1.000000,mvcvRect(6,17,6,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_14_74[2] =
{
0.000959,-0.019526,
};
int MEMORY_TYPE feature_left_14_74[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_14_74[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_14_74[3] =
{
0.347349,0.647555,0.464378,
};
HaarClassifier MEMORY_TYPE haar_feature_class_14[75] =
{
{2,haar_14_0,feature_thres_14_0,feature_left_14_0,feature_right_14_0,feature_alpha_14_0},{2,haar_14_1,feature_thres_14_1,feature_left_14_1,feature_right_14_1,feature_alpha_14_1},{2,haar_14_2,feature_thres_14_2,feature_left_14_2,feature_right_14_2,feature_alpha_14_2},{2,haar_14_3,feature_thres_14_3,feature_left_14_3,feature_right_14_3,feature_alpha_14_3},{2,haar_14_4,feature_thres_14_4,feature_left_14_4,feature_right_14_4,feature_alpha_14_4},{2,haar_14_5,feature_thres_14_5,feature_left_14_5,feature_right_14_5,feature_alpha_14_5},{2,haar_14_6,feature_thres_14_6,feature_left_14_6,feature_right_14_6,feature_alpha_14_6},{2,haar_14_7,feature_thres_14_7,feature_left_14_7,feature_right_14_7,feature_alpha_14_7},{2,haar_14_8,feature_thres_14_8,feature_left_14_8,feature_right_14_8,feature_alpha_14_8},{2,haar_14_9,feature_thres_14_9,feature_left_14_9,feature_right_14_9,feature_alpha_14_9},{2,haar_14_10,feature_thres_14_10,feature_left_14_10,feature_right_14_10,feature_alpha_14_10},{2,haar_14_11,feature_thres_14_11,feature_left_14_11,feature_right_14_11,feature_alpha_14_11},{2,haar_14_12,feature_thres_14_12,feature_left_14_12,feature_right_14_12,feature_alpha_14_12},{2,haar_14_13,feature_thres_14_13,feature_left_14_13,feature_right_14_13,feature_alpha_14_13},{2,haar_14_14,feature_thres_14_14,feature_left_14_14,feature_right_14_14,feature_alpha_14_14},{2,haar_14_15,feature_thres_14_15,feature_left_14_15,feature_right_14_15,feature_alpha_14_15},{2,haar_14_16,feature_thres_14_16,feature_left_14_16,feature_right_14_16,feature_alpha_14_16},{2,haar_14_17,feature_thres_14_17,feature_left_14_17,feature_right_14_17,feature_alpha_14_17},{2,haar_14_18,feature_thres_14_18,feature_left_14_18,feature_right_14_18,feature_alpha_14_18},{2,haar_14_19,feature_thres_14_19,feature_left_14_19,feature_right_14_19,feature_alpha_14_19},{2,haar_14_20,feature_thres_14_20,feature_left_14_20,feature_right_14_20,feature_alpha_14_20},{2,haar_14_21,feature_thres_14_21,feature_left_14_21,feature_right_14_21,feature_alpha_14_21},{2,haar_14_22,feature_thres_14_22,feature_left_14_22,feature_right_14_22,feature_alpha_14_22},{2,haar_14_23,feature_thres_14_23,feature_left_14_23,feature_right_14_23,feature_alpha_14_23},{2,haar_14_24,feature_thres_14_24,feature_left_14_24,feature_right_14_24,feature_alpha_14_24},{2,haar_14_25,feature_thres_14_25,feature_left_14_25,feature_right_14_25,feature_alpha_14_25},{2,haar_14_26,feature_thres_14_26,feature_left_14_26,feature_right_14_26,feature_alpha_14_26},{2,haar_14_27,feature_thres_14_27,feature_left_14_27,feature_right_14_27,feature_alpha_14_27},{2,haar_14_28,feature_thres_14_28,feature_left_14_28,feature_right_14_28,feature_alpha_14_28},{2,haar_14_29,feature_thres_14_29,feature_left_14_29,feature_right_14_29,feature_alpha_14_29},{2,haar_14_30,feature_thres_14_30,feature_left_14_30,feature_right_14_30,feature_alpha_14_30},{2,haar_14_31,feature_thres_14_31,feature_left_14_31,feature_right_14_31,feature_alpha_14_31},{2,haar_14_32,feature_thres_14_32,feature_left_14_32,feature_right_14_32,feature_alpha_14_32},{2,haar_14_33,feature_thres_14_33,feature_left_14_33,feature_right_14_33,feature_alpha_14_33},{2,haar_14_34,feature_thres_14_34,feature_left_14_34,feature_right_14_34,feature_alpha_14_34},{2,haar_14_35,feature_thres_14_35,feature_left_14_35,feature_right_14_35,feature_alpha_14_35},{2,haar_14_36,feature_thres_14_36,feature_left_14_36,feature_right_14_36,feature_alpha_14_36},{2,haar_14_37,feature_thres_14_37,feature_left_14_37,feature_right_14_37,feature_alpha_14_37},{2,haar_14_38,feature_thres_14_38,feature_left_14_38,feature_right_14_38,feature_alpha_14_38},{2,haar_14_39,feature_thres_14_39,feature_left_14_39,feature_right_14_39,feature_alpha_14_39},{2,haar_14_40,feature_thres_14_40,feature_left_14_40,feature_right_14_40,feature_alpha_14_40},{2,haar_14_41,feature_thres_14_41,feature_left_14_41,feature_right_14_41,feature_alpha_14_41},{2,haar_14_42,feature_thres_14_42,feature_left_14_42,feature_right_14_42,feature_alpha_14_42},{2,haar_14_43,feature_thres_14_43,feature_left_14_43,feature_right_14_43,feature_alpha_14_43},{2,haar_14_44,feature_thres_14_44,feature_left_14_44,feature_right_14_44,feature_alpha_14_44},{2,haar_14_45,feature_thres_14_45,feature_left_14_45,feature_right_14_45,feature_alpha_14_45},{2,haar_14_46,feature_thres_14_46,feature_left_14_46,feature_right_14_46,feature_alpha_14_46},{2,haar_14_47,feature_thres_14_47,feature_left_14_47,feature_right_14_47,feature_alpha_14_47},{2,haar_14_48,feature_thres_14_48,feature_left_14_48,feature_right_14_48,feature_alpha_14_48},{2,haar_14_49,feature_thres_14_49,feature_left_14_49,feature_right_14_49,feature_alpha_14_49},{2,haar_14_50,feature_thres_14_50,feature_left_14_50,feature_right_14_50,feature_alpha_14_50},{2,haar_14_51,feature_thres_14_51,feature_left_14_51,feature_right_14_51,feature_alpha_14_51},{2,haar_14_52,feature_thres_14_52,feature_left_14_52,feature_right_14_52,feature_alpha_14_52},{2,haar_14_53,feature_thres_14_53,feature_left_14_53,feature_right_14_53,feature_alpha_14_53},{2,haar_14_54,feature_thres_14_54,feature_left_14_54,feature_right_14_54,feature_alpha_14_54},{2,haar_14_55,feature_thres_14_55,feature_left_14_55,feature_right_14_55,feature_alpha_14_55},{2,haar_14_56,feature_thres_14_56,feature_left_14_56,feature_right_14_56,feature_alpha_14_56},{2,haar_14_57,feature_thres_14_57,feature_left_14_57,feature_right_14_57,feature_alpha_14_57},{2,haar_14_58,feature_thres_14_58,feature_left_14_58,feature_right_14_58,feature_alpha_14_58},{2,haar_14_59,feature_thres_14_59,feature_left_14_59,feature_right_14_59,feature_alpha_14_59},{2,haar_14_60,feature_thres_14_60,feature_left_14_60,feature_right_14_60,feature_alpha_14_60},{2,haar_14_61,feature_thres_14_61,feature_left_14_61,feature_right_14_61,feature_alpha_14_61},{2,haar_14_62,feature_thres_14_62,feature_left_14_62,feature_right_14_62,feature_alpha_14_62},{2,haar_14_63,feature_thres_14_63,feature_left_14_63,feature_right_14_63,feature_alpha_14_63},{2,haar_14_64,feature_thres_14_64,feature_left_14_64,feature_right_14_64,feature_alpha_14_64},{2,haar_14_65,feature_thres_14_65,feature_left_14_65,feature_right_14_65,feature_alpha_14_65},{2,haar_14_66,feature_thres_14_66,feature_left_14_66,feature_right_14_66,feature_alpha_14_66},{2,haar_14_67,feature_thres_14_67,feature_left_14_67,feature_right_14_67,feature_alpha_14_67},{2,haar_14_68,feature_thres_14_68,feature_left_14_68,feature_right_14_68,feature_alpha_14_68},{2,haar_14_69,feature_thres_14_69,feature_left_14_69,feature_right_14_69,feature_alpha_14_69},{2,haar_14_70,feature_thres_14_70,feature_left_14_70,feature_right_14_70,feature_alpha_14_70},{2,haar_14_71,feature_thres_14_71,feature_left_14_71,feature_right_14_71,feature_alpha_14_71},{2,haar_14_72,feature_thres_14_72,feature_left_14_72,feature_right_14_72,feature_alpha_14_72},{2,haar_14_73,feature_thres_14_73,feature_left_14_73,feature_right_14_73,feature_alpha_14_73},{2,haar_14_74,feature_thres_14_74,feature_left_14_74,feature_right_14_74,feature_alpha_14_74},
};
HaarFeature MEMORY_TYPE haar_15_0[2] =
{
0,mvcvRect(1,0,18,5),-1.000000,mvcvRect(7,0,6,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,7,3,4),-1.000000,mvcvRect(5,9,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_0[2] =
{
0.041242,0.015627,
};
int MEMORY_TYPE feature_left_15_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_0[3] =
{
0.339332,0.510410,0.777282,
};
HaarFeature MEMORY_TYPE haar_15_1[2] =
{
0,mvcvRect(10,6,2,2),-1.000000,mvcvRect(10,6,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,4,14,4),-1.000000,mvcvRect(13,4,7,2),2.000000,mvcvRect(6,6,7,2),2.000000,
};
float MEMORY_TYPE feature_thres_15_1[2] =
{
0.000299,-0.001004,
};
int MEMORY_TYPE feature_left_15_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_1[3] =
{
0.366467,0.540565,0.392621,
};
HaarFeature MEMORY_TYPE haar_15_2[2] =
{
0,mvcvRect(5,16,6,4),-1.000000,mvcvRect(5,16,3,2),2.000000,mvcvRect(8,18,3,2),2.000000,0,mvcvRect(7,15,2,4),-1.000000,mvcvRect(7,17,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_2[2] =
{
0.000681,0.000131,
};
int MEMORY_TYPE feature_left_15_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_2[3] =
{
0.425152,0.413514,0.692575,
};
HaarFeature MEMORY_TYPE haar_15_3[2] =
{
0,mvcvRect(8,5,5,14),-1.000000,mvcvRect(8,12,5,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,9,2,2),-1.000000,mvcvRect(9,10,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_3[2] =
{
0.003170,-0.002059,
};
int MEMORY_TYPE feature_left_15_3[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_3[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_3[3] =
{
0.345587,0.223419,0.528612,
};
HaarFeature MEMORY_TYPE haar_15_4[2] =
{
0,mvcvRect(7,5,3,7),-1.000000,mvcvRect(8,5,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,3,9),-1.000000,mvcvRect(0,3,3,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_4[2] =
{
-0.000464,0.003509,
};
int MEMORY_TYPE feature_left_15_4[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_4[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_4[3] =
{
0.420652,0.650298,0.411760,
};
HaarFeature MEMORY_TYPE haar_15_5[2] =
{
0,mvcvRect(8,6,8,8),-1.000000,mvcvRect(12,6,4,4),2.000000,mvcvRect(8,10,4,4),2.000000,0,mvcvRect(4,8,13,2),-1.000000,mvcvRect(4,9,13,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_5[2] =
{
-0.002398,0.001090,
};
int MEMORY_TYPE feature_left_15_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_5[3] =
{
0.367330,0.290624,0.544511,
};
HaarFeature MEMORY_TYPE haar_15_6[2] =
{
0,mvcvRect(4,3,6,1),-1.000000,mvcvRect(6,3,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,1,2,6),-1.000000,mvcvRect(9,3,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_6[2] =
{
-0.000165,-0.000416,
};
int MEMORY_TYPE feature_left_15_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_6[3] =
{
0.423352,0.388636,0.626917,
};
HaarFeature MEMORY_TYPE haar_15_7[2] =
{
0,mvcvRect(10,5,6,4),-1.000000,mvcvRect(12,5,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,5,2,12),-1.000000,mvcvRect(9,9,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_7[2] =
{
-0.000237,0.024740,
};
int MEMORY_TYPE feature_left_15_7[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_7[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_7[3] =
{
0.552445,0.496010,0.537349,
};
HaarFeature MEMORY_TYPE haar_15_8[2] =
{
0,mvcvRect(8,14,4,3),-1.000000,mvcvRect(8,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,12,4,3),-1.000000,mvcvRect(8,13,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_8[2] =
{
-0.015343,0.011540,
};
int MEMORY_TYPE feature_left_15_8[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_8[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_8[3] =
{
0.684941,0.403724,0.678694,
};
HaarFeature MEMORY_TYPE haar_15_9[2] =
{
0,mvcvRect(10,3,6,7),-1.000000,mvcvRect(12,3,2,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,10,16,6),-1.000000,mvcvRect(3,12,16,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_9[2] =
{
0.006423,0.012978,
};
int MEMORY_TYPE feature_left_15_9[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_9[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_9[3] =
{
0.381468,0.552706,0.374496,
};
HaarFeature MEMORY_TYPE haar_15_10[2] =
{
0,mvcvRect(5,5,3,10),-1.000000,mvcvRect(5,10,3,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,10,3,6),-1.000000,mvcvRect(6,13,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_10[2] =
{
0.001106,0.001374,
};
int MEMORY_TYPE feature_left_15_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_10[3] =
{
0.352093,0.564190,0.307503,
};
HaarFeature MEMORY_TYPE haar_15_11[2] =
{
0,mvcvRect(17,2,2,12),-1.000000,mvcvRect(17,2,1,12),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,6,2,14),-1.000000,mvcvRect(16,13,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_11[2] =
{
0.016234,-0.000815,
};
int MEMORY_TYPE feature_left_15_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_11[3] =
{
0.488883,0.545632,0.474355,
};
HaarFeature MEMORY_TYPE haar_15_12[2] =
{
0,mvcvRect(3,11,12,9),-1.000000,mvcvRect(3,14,12,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,2,4,12),-1.000000,mvcvRect(2,2,2,12),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_12[2] =
{
-0.090782,0.011665,
};
int MEMORY_TYPE feature_left_15_12[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_12[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_12[3] =
{
0.292525,0.468845,0.623035,
};
HaarFeature MEMORY_TYPE haar_15_13[2] =
{
0,mvcvRect(18,0,2,18),-1.000000,mvcvRect(18,0,1,18),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,12,3,2),-1.000000,mvcvRect(16,13,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_13[2] =
{
-0.023286,0.002156,
};
int MEMORY_TYPE feature_left_15_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_13[3] =
{
0.689584,0.535580,0.342347,
};
HaarFeature MEMORY_TYPE haar_15_14[2] =
{
0,mvcvRect(0,2,2,15),-1.000000,mvcvRect(1,2,1,15),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,10,2,4),-1.000000,mvcvRect(1,12,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_14[2] =
{
-0.004317,0.001561,
};
int MEMORY_TYPE feature_left_15_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_14[3] =
{
0.593708,0.470866,0.273700,
};
HaarFeature MEMORY_TYPE haar_15_15[2] =
{
0,mvcvRect(11,1,2,18),-1.000000,mvcvRect(11,1,1,18),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,2,14,2),-1.000000,mvcvRect(10,2,7,1),2.000000,mvcvRect(3,3,7,1),2.000000,
};
float MEMORY_TYPE feature_thres_15_15[2] =
{
0.014077,0.007102,
};
int MEMORY_TYPE feature_left_15_15[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_15[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_15[3] =
{
0.528716,0.533619,0.322481,
};
HaarFeature MEMORY_TYPE haar_15_16[2] =
{
0,mvcvRect(7,1,2,18),-1.000000,mvcvRect(8,1,1,18),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,1,8,12),-1.000000,mvcvRect(6,7,8,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_16[2] =
{
-0.004822,-0.005385,
};
int MEMORY_TYPE feature_left_15_16[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_16[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_16[3] =
{
0.298391,0.562400,0.429591,
};
HaarFeature MEMORY_TYPE haar_15_17[2] =
{
0,mvcvRect(8,14,4,3),-1.000000,mvcvRect(8,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,14,6,3),-1.000000,mvcvRect(7,15,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_17[2] =
{
0.007348,-0.003571,
};
int MEMORY_TYPE feature_left_15_17[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_17[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_17[3] =
{
0.681396,0.585797,0.460343,
};
HaarFeature MEMORY_TYPE haar_15_18[2] =
{
0,mvcvRect(0,13,5,2),-1.000000,mvcvRect(0,14,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,0,2,6),-1.000000,mvcvRect(9,0,1,3),2.000000,mvcvRect(10,3,1,3),2.000000,
};
float MEMORY_TYPE feature_thres_15_18[2] =
{
0.002334,0.004743,
};
int MEMORY_TYPE feature_left_15_18[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_18[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_18[3] =
{
0.274485,0.504753,0.236274,
};
HaarFeature MEMORY_TYPE haar_15_19[2] =
{
0,mvcvRect(9,0,2,6),-1.000000,mvcvRect(10,0,1,3),2.000000,mvcvRect(9,3,1,3),2.000000,0,mvcvRect(9,7,3,6),-1.000000,mvcvRect(10,7,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_19[2] =
{
0.006506,0.012589,
};
int MEMORY_TYPE feature_left_15_19[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_19[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_19[3] =
{
0.524225,0.482369,0.675254,
};
HaarFeature MEMORY_TYPE haar_15_20[2] =
{
0,mvcvRect(9,0,2,6),-1.000000,mvcvRect(9,0,1,3),2.000000,mvcvRect(10,3,1,3),2.000000,0,mvcvRect(8,7,3,6),-1.000000,mvcvRect(9,7,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_20[2] =
{
-0.006336,-0.005764,
};
int MEMORY_TYPE feature_left_15_20[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_20[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_20[3] =
{
0.173463,0.635438,0.458748,
};
HaarFeature MEMORY_TYPE haar_15_21[2] =
{
0,mvcvRect(9,6,2,6),-1.000000,mvcvRect(9,6,1,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,4,4,3),-1.000000,mvcvRect(9,4,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_21[2] =
{
0.001360,0.028404,
};
int MEMORY_TYPE feature_left_15_21[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_21[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_21[3] =
{
0.458038,0.517638,0.120439,
};
HaarFeature MEMORY_TYPE haar_15_22[2] =
{
0,mvcvRect(0,4,4,3),-1.000000,mvcvRect(0,5,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,4,2),-1.000000,mvcvRect(8,8,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_22[2] =
{
-0.009296,-0.001180,
};
int MEMORY_TYPE feature_left_15_22[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_22[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_22[3] =
{
0.233796,0.390281,0.565293,
};
HaarFeature MEMORY_TYPE haar_15_23[2] =
{
0,mvcvRect(10,6,6,3),-1.000000,mvcvRect(12,6,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,6,3,12),-1.000000,mvcvRect(9,10,3,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_23[2] =
{
-0.002095,0.004168,
};
int MEMORY_TYPE feature_left_15_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_23[3] =
{
0.551203,0.545598,0.479895,
};
HaarFeature MEMORY_TYPE haar_15_24[2] =
{
0,mvcvRect(5,4,2,3),-1.000000,mvcvRect(5,5,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,1,3),-1.000000,mvcvRect(5,7,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_24[2] =
{
0.005446,-0.001277,
};
int MEMORY_TYPE feature_left_15_24[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_24[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_24[3] =
{
0.612709,0.531713,0.385093,
};
HaarFeature MEMORY_TYPE haar_15_25[2] =
{
0,mvcvRect(9,17,3,2),-1.000000,mvcvRect(10,17,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,7,20,2),-1.000000,mvcvRect(0,8,20,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_25[2] =
{
0.000594,0.042310,
};
int MEMORY_TYPE feature_left_15_25[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_25[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_25[3] =
{
0.544644,0.523464,0.221304,
};
HaarFeature MEMORY_TYPE haar_15_26[2] =
{
0,mvcvRect(4,3,6,7),-1.000000,mvcvRect(6,3,2,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,10,6,10),-1.000000,mvcvRect(5,10,3,5),2.000000,mvcvRect(8,15,3,5),2.000000,
};
float MEMORY_TYPE feature_thres_15_26[2] =
{
0.005619,0.007240,
};
int MEMORY_TYPE feature_left_15_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_26[3] =
{
0.491620,0.147148,0.485289,
};
HaarFeature MEMORY_TYPE haar_15_27[2] =
{
0,mvcvRect(9,17,3,2),-1.000000,mvcvRect(10,17,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,10,2,2),-1.000000,mvcvRect(9,11,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_27[2] =
{
-0.004561,0.000046,
};
int MEMORY_TYPE feature_left_15_27[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_27[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_27[3] =
{
0.277377,0.462646,0.576808,
};
HaarFeature MEMORY_TYPE haar_15_28[2] =
{
0,mvcvRect(8,17,3,2),-1.000000,mvcvRect(9,17,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,1,3),-1.000000,mvcvRect(5,7,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_28[2] =
{
-0.006190,0.000812,
};
int MEMORY_TYPE feature_left_15_28[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_28[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_28[3] =
{
0.164429,0.477859,0.626186,
};
HaarFeature MEMORY_TYPE haar_15_29[2] =
{
0,mvcvRect(0,1,20,2),-1.000000,mvcvRect(10,1,10,1),2.000000,mvcvRect(0,2,10,1),2.000000,0,mvcvRect(14,2,6,9),-1.000000,mvcvRect(14,5,6,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_29[2] =
{
0.013780,0.001129,
};
int MEMORY_TYPE feature_left_15_29[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_29[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_29[3] =
{
0.525731,0.549805,0.398311,
};
HaarFeature MEMORY_TYPE haar_15_30[2] =
{
0,mvcvRect(5,3,3,2),-1.000000,mvcvRect(5,4,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,4,2),-1.000000,mvcvRect(7,4,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_30[2] =
{
-0.000106,0.000167,
};
int MEMORY_TYPE feature_left_15_30[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_30[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_30[3] =
{
0.403352,0.414934,0.579534,
};
HaarFeature MEMORY_TYPE haar_15_31[2] =
{
0,mvcvRect(14,2,6,9),-1.000000,mvcvRect(14,5,6,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,12,20,6),-1.000000,mvcvRect(0,14,20,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_31[2] =
{
0.001129,-0.120193,
};
int MEMORY_TYPE feature_left_15_31[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_31[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_31[3] =
{
0.393411,0.073400,0.520259,
};
HaarFeature MEMORY_TYPE haar_15_32[2] =
{
0,mvcvRect(2,2,16,4),-1.000000,mvcvRect(2,2,8,2),2.000000,mvcvRect(10,4,8,2),2.000000,0,mvcvRect(7,12,5,3),-1.000000,mvcvRect(7,13,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_32[2] =
{
-0.015231,0.003576,
};
int MEMORY_TYPE feature_left_15_32[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_32[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_32[3] =
{
0.374951,0.507815,0.660607,
};
HaarFeature MEMORY_TYPE haar_15_33[2] =
{
0,mvcvRect(14,9,6,10),-1.000000,mvcvRect(14,9,3,10),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,6,3,2),-1.000000,mvcvRect(16,7,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_33[2] =
{
0.013479,-0.002116,
};
int MEMORY_TYPE feature_left_15_33[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_33[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_33[3] =
{
0.454771,0.331101,0.538426,
};
HaarFeature MEMORY_TYPE haar_15_34[2] =
{
0,mvcvRect(0,9,6,10),-1.000000,mvcvRect(3,9,3,10),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,16,5,2),-1.000000,mvcvRect(0,17,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_34[2] =
{
-0.017878,0.001093,
};
int MEMORY_TYPE feature_left_15_34[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_34[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_34[3] =
{
0.651325,0.526477,0.345699,
};
HaarFeature MEMORY_TYPE haar_15_35[2] =
{
0,mvcvRect(9,12,2,3),-1.000000,mvcvRect(9,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,2,12),-1.000000,mvcvRect(9,11,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_35[2] =
{
-0.003055,0.003637,
};
int MEMORY_TYPE feature_left_15_35[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_35[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_35[3] =
{
0.626861,0.539921,0.434540,
};
HaarFeature MEMORY_TYPE haar_15_36[2] =
{
0,mvcvRect(3,2,6,2),-1.000000,mvcvRect(5,2,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,1,1,2),-1.000000,mvcvRect(4,2,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_36[2] =
{
0.000098,-0.000327,
};
int MEMORY_TYPE feature_left_15_36[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_36[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_36[3] =
{
0.383561,0.333767,0.553917,
};
HaarFeature MEMORY_TYPE haar_15_37[2] =
{
0,mvcvRect(11,15,1,2),-1.000000,mvcvRect(11,16,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,1,16,2),-1.000000,mvcvRect(11,1,8,1),2.000000,mvcvRect(3,2,8,1),2.000000,
};
float MEMORY_TYPE feature_thres_15_37[2] =
{
0.000434,0.014006,
};
int MEMORY_TYPE feature_left_15_37[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_37[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_37[3] =
{
0.578827,0.527508,0.270113,
};
HaarFeature MEMORY_TYPE haar_15_38[2] =
{
0,mvcvRect(3,6,2,2),-1.000000,mvcvRect(3,6,1,1),2.000000,mvcvRect(4,7,1,1),2.000000,0,mvcvRect(5,11,10,6),-1.000000,mvcvRect(5,11,5,3),2.000000,mvcvRect(10,14,5,3),2.000000,
};
float MEMORY_TYPE feature_thres_15_38[2] =
{
-0.000927,0.003950,
};
int MEMORY_TYPE feature_left_15_38[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_38[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_38[3] =
{
0.585228,0.472834,0.331392,
};
HaarFeature MEMORY_TYPE haar_15_39[2] =
{
0,mvcvRect(10,11,4,6),-1.000000,mvcvRect(10,14,4,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,9,6,11),-1.000000,mvcvRect(16,9,2,11),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_39[2] =
{
-0.000581,-0.012018,
};
int MEMORY_TYPE feature_left_15_39[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_39[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_39[3] =
{
0.425881,0.560979,0.489519,
};
HaarFeature MEMORY_TYPE haar_15_40[2] =
{
0,mvcvRect(0,9,6,11),-1.000000,mvcvRect(2,9,2,11),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,11,16,6),-1.000000,mvcvRect(2,11,8,3),2.000000,mvcvRect(10,14,8,3),2.000000,
};
float MEMORY_TYPE feature_thres_15_40[2] =
{
-0.145215,-0.006605,
};
int MEMORY_TYPE feature_left_15_40[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_40[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_40[3] =
{
0.043894,0.422917,0.561629,
};
HaarFeature MEMORY_TYPE haar_15_41[2] =
{
0,mvcvRect(12,0,8,10),-1.000000,mvcvRect(16,0,4,5),2.000000,mvcvRect(12,5,4,5),2.000000,0,mvcvRect(14,2,6,4),-1.000000,mvcvRect(16,2,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_41[2] =
{
-0.034910,0.003748,
};
int MEMORY_TYPE feature_left_15_41[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_41[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_41[3] =
{
0.478813,0.480028,0.580139,
};
HaarFeature MEMORY_TYPE haar_15_42[2] =
{
0,mvcvRect(0,0,8,10),-1.000000,mvcvRect(0,0,4,5),2.000000,mvcvRect(4,5,4,5),2.000000,0,mvcvRect(0,2,6,4),-1.000000,mvcvRect(2,2,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_42[2] =
{
0.033038,0.003687,
};
int MEMORY_TYPE feature_left_15_42[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_42[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_42[3] =
{
0.707818,0.444962,0.595773,
};
HaarFeature MEMORY_TYPE haar_15_43[2] =
{
0,mvcvRect(4,9,15,2),-1.000000,mvcvRect(9,9,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,3,4,8),-1.000000,mvcvRect(14,3,2,4),2.000000,mvcvRect(12,7,2,4),2.000000,
};
float MEMORY_TYPE feature_thres_15_43[2] =
{
-0.004531,0.004106,
};
int MEMORY_TYPE feature_left_15_43[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_43[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_43[3] =
{
0.417705,0.537295,0.373693,
};
HaarFeature MEMORY_TYPE haar_15_44[2] =
{
0,mvcvRect(9,2,2,9),-1.000000,mvcvRect(10,2,1,9),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,2,20,1),-1.000000,mvcvRect(10,2,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_44[2] =
{
-0.008760,-0.023003,
};
int MEMORY_TYPE feature_left_15_44[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_44[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_44[3] =
{
0.665881,0.264792,0.510182,
};
HaarFeature MEMORY_TYPE haar_15_45[2] =
{
0,mvcvRect(16,1,4,5),-1.000000,mvcvRect(16,1,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,0,4,6),-1.000000,mvcvRect(16,3,4,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_45[2] =
{
0.005366,0.038972,
};
int MEMORY_TYPE feature_left_15_45[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_45[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_45[3] =
{
0.454863,0.515706,0.343644,
};
HaarFeature MEMORY_TYPE haar_15_46[2] =
{
0,mvcvRect(4,3,6,4),-1.000000,mvcvRect(6,3,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,18,5),-1.000000,mvcvRect(6,0,6,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_46[2] =
{
-0.027767,-0.009889,
};
int MEMORY_TYPE feature_left_15_46[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_46[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_46[3] =
{
0.235439,0.688774,0.511105,
};
HaarFeature MEMORY_TYPE haar_15_47[2] =
{
0,mvcvRect(6,2,12,14),-1.000000,mvcvRect(12,2,6,7),2.000000,mvcvRect(6,9,6,7),2.000000,0,mvcvRect(11,8,3,5),-1.000000,mvcvRect(12,8,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_47[2] =
{
-0.003207,-0.000675,
};
int MEMORY_TYPE feature_left_15_47[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_47[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_47[3] =
{
0.543887,0.545115,0.483135,
};
HaarFeature MEMORY_TYPE haar_15_48[2] =
{
0,mvcvRect(5,12,2,2),-1.000000,mvcvRect(5,13,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,10,4,3),-1.000000,mvcvRect(7,10,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_48[2] =
{
-0.005195,-0.000262,
};
int MEMORY_TYPE feature_left_15_48[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_48[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_48[3] =
{
0.211342,0.527368,0.399259,
};
HaarFeature MEMORY_TYPE haar_15_49[2] =
{
0,mvcvRect(4,9,15,2),-1.000000,mvcvRect(9,9,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,7,6,2),-1.000000,mvcvRect(12,7,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_49[2] =
{
0.002242,-0.001214,
};
int MEMORY_TYPE feature_left_15_49[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_49[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_49[3] =
{
0.468826,0.550424,0.438487,
};
HaarFeature MEMORY_TYPE haar_15_50[2] =
{
0,mvcvRect(1,9,15,2),-1.000000,mvcvRect(6,9,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,0,2,10),-1.000000,mvcvRect(5,0,1,5),2.000000,mvcvRect(6,5,1,5),2.000000,
};
float MEMORY_TYPE feature_thres_15_50[2] =
{
-0.002947,-0.000393,
};
int MEMORY_TYPE feature_left_15_50[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_50[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_50[3] =
{
0.389285,0.600172,0.456166,
};
HaarFeature MEMORY_TYPE haar_15_51[2] =
{
0,mvcvRect(0,0,20,14),-1.000000,mvcvRect(0,7,20,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,7,8,4),-1.000000,mvcvRect(12,7,4,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_51[2] =
{
0.625507,0.009774,
};
int MEMORY_TYPE feature_left_15_51[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_51[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_51[3] =
{
0.068126,0.481303,0.562066,
};
HaarFeature MEMORY_TYPE haar_15_52[2] =
{
0,mvcvRect(0,7,8,4),-1.000000,mvcvRect(4,7,4,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,1,3,3),-1.000000,mvcvRect(9,1,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_52[2] =
{
0.094378,-0.001956,
};
int MEMORY_TYPE feature_left_15_52[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_52[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_52[3] =
{
0.066632,0.358823,0.529541,
};
HaarFeature MEMORY_TYPE haar_15_53[2] =
{
0,mvcvRect(9,7,3,4),-1.000000,mvcvRect(10,7,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,9,3,1),-1.000000,mvcvRect(10,9,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_53[2] =
{
0.009065,0.000421,
};
int MEMORY_TYPE feature_left_15_53[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_53[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_53[3] =
{
0.482269,0.467033,0.568311,
};
HaarFeature MEMORY_TYPE haar_15_54[2] =
{
0,mvcvRect(8,9,3,2),-1.000000,mvcvRect(8,10,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,4,2,8),-1.000000,mvcvRect(8,4,1,4),2.000000,mvcvRect(9,8,1,4),2.000000,
};
float MEMORY_TYPE feature_thres_15_54[2] =
{
-0.000442,-0.004731,
};
int MEMORY_TYPE feature_left_15_54[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_54[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_54[3] =
{
0.536080,0.613725,0.318809,
};
HaarFeature MEMORY_TYPE haar_15_55[2] =
{
0,mvcvRect(5,8,12,3),-1.000000,mvcvRect(5,9,12,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,14,1,3),-1.000000,mvcvRect(11,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_55[2] =
{
0.001540,0.002432,
};
int MEMORY_TYPE feature_left_15_55[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_55[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_55[3] =
{
0.448772,0.489417,0.671665,
};
HaarFeature MEMORY_TYPE haar_15_56[2] =
{
0,mvcvRect(6,10,3,6),-1.000000,mvcvRect(6,12,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,17,8,3),-1.000000,mvcvRect(4,18,8,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_56[2] =
{
-0.015582,0.001082,
};
int MEMORY_TYPE feature_left_15_56[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_56[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_56[3] =
{
0.333674,0.471822,0.596063,
};
HaarFeature MEMORY_TYPE haar_15_57[2] =
{
0,mvcvRect(17,6,2,3),-1.000000,mvcvRect(17,7,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,12,2,2),-1.000000,mvcvRect(10,12,1,1),2.000000,mvcvRect(9,13,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_15_57[2] =
{
-0.002220,-0.000930,
};
int MEMORY_TYPE feature_left_15_57[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_57[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_57[3] =
{
0.358855,0.621871,0.481730,
};
HaarFeature MEMORY_TYPE haar_15_58[2] =
{
0,mvcvRect(9,13,2,4),-1.000000,mvcvRect(9,13,1,2),2.000000,mvcvRect(10,15,1,2),2.000000,0,mvcvRect(9,11,2,3),-1.000000,mvcvRect(9,12,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_58[2] =
{
-0.004742,-0.006295,
};
int MEMORY_TYPE feature_left_15_58[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_58[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_58[3] =
{
0.255003,0.672808,0.505106,
};
HaarFeature MEMORY_TYPE haar_15_59[2] =
{
0,mvcvRect(5,5,12,10),-1.000000,mvcvRect(11,5,6,5),2.000000,mvcvRect(5,10,6,5),2.000000,0,mvcvRect(6,3,12,12),-1.000000,mvcvRect(12,3,6,6),2.000000,mvcvRect(6,9,6,6),2.000000,
};
float MEMORY_TYPE feature_thres_15_59[2] =
{
0.003522,-0.002429,
};
int MEMORY_TYPE feature_left_15_59[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_59[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_59[3] =
{
0.540191,0.541946,0.434714,
};
HaarFeature MEMORY_TYPE haar_15_60[2] =
{
0,mvcvRect(5,7,2,2),-1.000000,mvcvRect(5,7,1,1),2.000000,mvcvRect(6,8,1,1),2.000000,0,mvcvRect(4,3,3,2),-1.000000,mvcvRect(5,3,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_60[2] =
{
-0.002526,-0.001482,
};
int MEMORY_TYPE feature_left_15_60[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_60[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_60[3] =
{
0.697062,0.326342,0.491787,
};
HaarFeature MEMORY_TYPE haar_15_61[2] =
{
0,mvcvRect(6,2,12,14),-1.000000,mvcvRect(12,2,6,7),2.000000,mvcvRect(6,9,6,7),2.000000,0,mvcvRect(5,2,12,3),-1.000000,mvcvRect(9,2,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_61[2] =
{
-0.224745,0.002834,
};
int MEMORY_TYPE feature_left_15_61[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_61[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_61[3] =
{
0.007294,0.457923,0.537988,
};
HaarFeature MEMORY_TYPE haar_15_62[2] =
{
0,mvcvRect(1,1,18,17),-1.000000,mvcvRect(7,1,6,17),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,9,10,1),-1.000000,mvcvRect(5,9,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_62[2] =
{
-0.020822,0.000149,
};
int MEMORY_TYPE feature_left_15_62[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_62[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_62[3] =
{
0.602409,0.333614,0.496282,
};
HaarFeature MEMORY_TYPE haar_15_63[2] =
{
0,mvcvRect(16,8,4,3),-1.000000,mvcvRect(16,9,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,13,6,6),-1.000000,mvcvRect(7,16,6,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_63[2] =
{
-0.003352,-0.037280,
};
int MEMORY_TYPE feature_left_15_63[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_63[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_63[3] =
{
0.355875,0.169856,0.520899,
};
HaarFeature MEMORY_TYPE haar_15_64[2] =
{
0,mvcvRect(6,14,1,6),-1.000000,mvcvRect(6,16,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,17,4,2),-1.000000,mvcvRect(6,18,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_64[2] =
{
0.000139,-0.000319,
};
int MEMORY_TYPE feature_left_15_64[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_64[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_64[3] =
{
0.559069,0.584873,0.379584,
};
HaarFeature MEMORY_TYPE haar_15_65[2] =
{
0,mvcvRect(10,18,6,2),-1.000000,mvcvRect(13,18,3,1),2.000000,mvcvRect(10,19,3,1),2.000000,0,mvcvRect(16,8,1,3),-1.000000,mvcvRect(16,9,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_65[2] =
{
0.000540,0.003896,
};
int MEMORY_TYPE feature_left_15_65[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_65[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_65[3] =
{
0.567029,0.518269,0.332771,
};
HaarFeature MEMORY_TYPE haar_15_66[2] =
{
0,mvcvRect(8,13,4,3),-1.000000,mvcvRect(8,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,15,1,2),-1.000000,mvcvRect(9,16,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_66[2] =
{
0.001608,-0.000575,
};
int MEMORY_TYPE feature_left_15_66[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_66[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_66[3] =
{
0.541049,0.602264,0.364464,
};
HaarFeature MEMORY_TYPE haar_15_67[2] =
{
0,mvcvRect(13,0,3,12),-1.000000,mvcvRect(14,0,1,12),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,11,1,3),-1.000000,mvcvRect(15,12,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_67[2] =
{
0.013435,0.002137,
};
int MEMORY_TYPE feature_left_15_67[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_67[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_67[3] =
{
0.344128,0.529243,0.274708,
};
HaarFeature MEMORY_TYPE haar_15_68[2] =
{
0,mvcvRect(8,15,3,3),-1.000000,mvcvRect(8,16,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,0,3,12),-1.000000,mvcvRect(5,0,1,12),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_68[2] =
{
0.014158,0.005388,
};
int MEMORY_TYPE feature_left_15_68[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_68[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_68[3] =
{
0.802787,0.522232,0.358673,
};
HaarFeature MEMORY_TYPE haar_15_69[2] =
{
0,mvcvRect(9,7,3,3),-1.000000,mvcvRect(10,7,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,9,3,1),-1.000000,mvcvRect(10,9,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_69[2] =
{
0.008801,0.000389,
};
int MEMORY_TYPE feature_left_15_69[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_69[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_69[3] =
{
0.490039,0.468106,0.572195,
};
HaarFeature MEMORY_TYPE haar_15_70[2] =
{
0,mvcvRect(2,2,12,14),-1.000000,mvcvRect(2,2,6,7),2.000000,mvcvRect(8,9,6,7),2.000000,0,mvcvRect(4,2,12,3),-1.000000,mvcvRect(8,2,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_70[2] =
{
-0.002214,-0.008464,
};
int MEMORY_TYPE feature_left_15_70[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_15_70[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_15_70[3] =
{
0.538881,0.667554,0.344844,
};
HaarFeature MEMORY_TYPE haar_15_71[2] =
{
0,mvcvRect(18,18,2,2),-1.000000,mvcvRect(18,18,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(17,2,3,8),-1.000000,mvcvRect(18,2,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_71[2] =
{
0.015044,0.007635,
};
int MEMORY_TYPE feature_left_15_71[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_71[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_71[3] =
{
0.923961,0.488490,0.630605,
};
HaarFeature MEMORY_TYPE haar_15_72[2] =
{
0,mvcvRect(0,18,2,2),-1.000000,mvcvRect(1,18,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,11,2,6),-1.000000,mvcvRect(6,14,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_72[2] =
{
0.000339,0.000212,
};
int MEMORY_TYPE feature_left_15_72[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_72[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_72[3] =
{
0.399743,0.566398,0.397298,
};
HaarFeature MEMORY_TYPE haar_15_73[2] =
{
0,mvcvRect(13,10,5,6),-1.000000,mvcvRect(13,12,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,8,15,3),-1.000000,mvcvRect(5,9,15,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_73[2] =
{
-0.027515,0.051603,
};
int MEMORY_TYPE feature_left_15_73[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_73[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_73[3] =
{
0.520106,0.514073,0.124513,
};
HaarFeature MEMORY_TYPE haar_15_74[2] =
{
0,mvcvRect(2,10,5,6),-1.000000,mvcvRect(2,12,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,8,15,3),-1.000000,mvcvRect(0,9,15,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_74[2] =
{
0.003751,-0.002146,
};
int MEMORY_TYPE feature_left_15_74[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_74[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_74[3] =
{
0.380210,0.330945,0.547454,
};
HaarFeature MEMORY_TYPE haar_15_75[2] =
{
0,mvcvRect(16,2,3,1),-1.000000,mvcvRect(17,2,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(17,4,3,2),-1.000000,mvcvRect(18,4,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_15_75[2] =
{
-0.000582,-0.000936,
};
int MEMORY_TYPE feature_left_15_75[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_75[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_75[3] =
{
0.489260,0.593740,0.466467,
};
HaarFeature MEMORY_TYPE haar_15_76[2] =
{
0,mvcvRect(0,8,8,12),-1.000000,mvcvRect(0,8,4,6),2.000000,mvcvRect(4,14,4,6),2.000000,0,mvcvRect(1,7,8,6),-1.000000,mvcvRect(1,7,4,3),2.000000,mvcvRect(5,10,4,3),2.000000,
};
float MEMORY_TYPE feature_thres_15_76[2] =
{
0.041667,-0.006776,
};
int MEMORY_TYPE feature_left_15_76[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_76[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_76[3] =
{
0.702135,0.322275,0.506840,
};
HaarFeature MEMORY_TYPE haar_15_77[2] =
{
0,mvcvRect(14,1,6,2),-1.000000,mvcvRect(16,1,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,0,4,4),-1.000000,mvcvRect(17,0,2,2),2.000000,mvcvRect(15,2,2,2),2.000000,
};
float MEMORY_TYPE feature_thres_15_77[2] =
{
-0.002917,0.000328,
};
int MEMORY_TYPE feature_left_15_77[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_15_77[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_15_77[3] =
{
0.471770,0.450938,0.565116,
};
HaarClassifier MEMORY_TYPE haar_feature_class_15[78] =
{
{2,haar_15_0,feature_thres_15_0,feature_left_15_0,feature_right_15_0,feature_alpha_15_0},{2,haar_15_1,feature_thres_15_1,feature_left_15_1,feature_right_15_1,feature_alpha_15_1},{2,haar_15_2,feature_thres_15_2,feature_left_15_2,feature_right_15_2,feature_alpha_15_2},{2,haar_15_3,feature_thres_15_3,feature_left_15_3,feature_right_15_3,feature_alpha_15_3},{2,haar_15_4,feature_thres_15_4,feature_left_15_4,feature_right_15_4,feature_alpha_15_4},{2,haar_15_5,feature_thres_15_5,feature_left_15_5,feature_right_15_5,feature_alpha_15_5},{2,haar_15_6,feature_thres_15_6,feature_left_15_6,feature_right_15_6,feature_alpha_15_6},{2,haar_15_7,feature_thres_15_7,feature_left_15_7,feature_right_15_7,feature_alpha_15_7},{2,haar_15_8,feature_thres_15_8,feature_left_15_8,feature_right_15_8,feature_alpha_15_8},{2,haar_15_9,feature_thres_15_9,feature_left_15_9,feature_right_15_9,feature_alpha_15_9},{2,haar_15_10,feature_thres_15_10,feature_left_15_10,feature_right_15_10,feature_alpha_15_10},{2,haar_15_11,feature_thres_15_11,feature_left_15_11,feature_right_15_11,feature_alpha_15_11},{2,haar_15_12,feature_thres_15_12,feature_left_15_12,feature_right_15_12,feature_alpha_15_12},{2,haar_15_13,feature_thres_15_13,feature_left_15_13,feature_right_15_13,feature_alpha_15_13},{2,haar_15_14,feature_thres_15_14,feature_left_15_14,feature_right_15_14,feature_alpha_15_14},{2,haar_15_15,feature_thres_15_15,feature_left_15_15,feature_right_15_15,feature_alpha_15_15},{2,haar_15_16,feature_thres_15_16,feature_left_15_16,feature_right_15_16,feature_alpha_15_16},{2,haar_15_17,feature_thres_15_17,feature_left_15_17,feature_right_15_17,feature_alpha_15_17},{2,haar_15_18,feature_thres_15_18,feature_left_15_18,feature_right_15_18,feature_alpha_15_18},{2,haar_15_19,feature_thres_15_19,feature_left_15_19,feature_right_15_19,feature_alpha_15_19},{2,haar_15_20,feature_thres_15_20,feature_left_15_20,feature_right_15_20,feature_alpha_15_20},{2,haar_15_21,feature_thres_15_21,feature_left_15_21,feature_right_15_21,feature_alpha_15_21},{2,haar_15_22,feature_thres_15_22,feature_left_15_22,feature_right_15_22,feature_alpha_15_22},{2,haar_15_23,feature_thres_15_23,feature_left_15_23,feature_right_15_23,feature_alpha_15_23},{2,haar_15_24,feature_thres_15_24,feature_left_15_24,feature_right_15_24,feature_alpha_15_24},{2,haar_15_25,feature_thres_15_25,feature_left_15_25,feature_right_15_25,feature_alpha_15_25},{2,haar_15_26,feature_thres_15_26,feature_left_15_26,feature_right_15_26,feature_alpha_15_26},{2,haar_15_27,feature_thres_15_27,feature_left_15_27,feature_right_15_27,feature_alpha_15_27},{2,haar_15_28,feature_thres_15_28,feature_left_15_28,feature_right_15_28,feature_alpha_15_28},{2,haar_15_29,feature_thres_15_29,feature_left_15_29,feature_right_15_29,feature_alpha_15_29},{2,haar_15_30,feature_thres_15_30,feature_left_15_30,feature_right_15_30,feature_alpha_15_30},{2,haar_15_31,feature_thres_15_31,feature_left_15_31,feature_right_15_31,feature_alpha_15_31},{2,haar_15_32,feature_thres_15_32,feature_left_15_32,feature_right_15_32,feature_alpha_15_32},{2,haar_15_33,feature_thres_15_33,feature_left_15_33,feature_right_15_33,feature_alpha_15_33},{2,haar_15_34,feature_thres_15_34,feature_left_15_34,feature_right_15_34,feature_alpha_15_34},{2,haar_15_35,feature_thres_15_35,feature_left_15_35,feature_right_15_35,feature_alpha_15_35},{2,haar_15_36,feature_thres_15_36,feature_left_15_36,feature_right_15_36,feature_alpha_15_36},{2,haar_15_37,feature_thres_15_37,feature_left_15_37,feature_right_15_37,feature_alpha_15_37},{2,haar_15_38,feature_thres_15_38,feature_left_15_38,feature_right_15_38,feature_alpha_15_38},{2,haar_15_39,feature_thres_15_39,feature_left_15_39,feature_right_15_39,feature_alpha_15_39},{2,haar_15_40,feature_thres_15_40,feature_left_15_40,feature_right_15_40,feature_alpha_15_40},{2,haar_15_41,feature_thres_15_41,feature_left_15_41,feature_right_15_41,feature_alpha_15_41},{2,haar_15_42,feature_thres_15_42,feature_left_15_42,feature_right_15_42,feature_alpha_15_42},{2,haar_15_43,feature_thres_15_43,feature_left_15_43,feature_right_15_43,feature_alpha_15_43},{2,haar_15_44,feature_thres_15_44,feature_left_15_44,feature_right_15_44,feature_alpha_15_44},{2,haar_15_45,feature_thres_15_45,feature_left_15_45,feature_right_15_45,feature_alpha_15_45},{2,haar_15_46,feature_thres_15_46,feature_left_15_46,feature_right_15_46,feature_alpha_15_46},{2,haar_15_47,feature_thres_15_47,feature_left_15_47,feature_right_15_47,feature_alpha_15_47},{2,haar_15_48,feature_thres_15_48,feature_left_15_48,feature_right_15_48,feature_alpha_15_48},{2,haar_15_49,feature_thres_15_49,feature_left_15_49,feature_right_15_49,feature_alpha_15_49},{2,haar_15_50,feature_thres_15_50,feature_left_15_50,feature_right_15_50,feature_alpha_15_50},{2,haar_15_51,feature_thres_15_51,feature_left_15_51,feature_right_15_51,feature_alpha_15_51},{2,haar_15_52,feature_thres_15_52,feature_left_15_52,feature_right_15_52,feature_alpha_15_52},{2,haar_15_53,feature_thres_15_53,feature_left_15_53,feature_right_15_53,feature_alpha_15_53},{2,haar_15_54,feature_thres_15_54,feature_left_15_54,feature_right_15_54,feature_alpha_15_54},{2,haar_15_55,feature_thres_15_55,feature_left_15_55,feature_right_15_55,feature_alpha_15_55},{2,haar_15_56,feature_thres_15_56,feature_left_15_56,feature_right_15_56,feature_alpha_15_56},{2,haar_15_57,feature_thres_15_57,feature_left_15_57,feature_right_15_57,feature_alpha_15_57},{2,haar_15_58,feature_thres_15_58,feature_left_15_58,feature_right_15_58,feature_alpha_15_58},{2,haar_15_59,feature_thres_15_59,feature_left_15_59,feature_right_15_59,feature_alpha_15_59},{2,haar_15_60,feature_thres_15_60,feature_left_15_60,feature_right_15_60,feature_alpha_15_60},{2,haar_15_61,feature_thres_15_61,feature_left_15_61,feature_right_15_61,feature_alpha_15_61},{2,haar_15_62,feature_thres_15_62,feature_left_15_62,feature_right_15_62,feature_alpha_15_62},{2,haar_15_63,feature_thres_15_63,feature_left_15_63,feature_right_15_63,feature_alpha_15_63},{2,haar_15_64,feature_thres_15_64,feature_left_15_64,feature_right_15_64,feature_alpha_15_64},{2,haar_15_65,feature_thres_15_65,feature_left_15_65,feature_right_15_65,feature_alpha_15_65},{2,haar_15_66,feature_thres_15_66,feature_left_15_66,feature_right_15_66,feature_alpha_15_66},{2,haar_15_67,feature_thres_15_67,feature_left_15_67,feature_right_15_67,feature_alpha_15_67},{2,haar_15_68,feature_thres_15_68,feature_left_15_68,feature_right_15_68,feature_alpha_15_68},{2,haar_15_69,feature_thres_15_69,feature_left_15_69,feature_right_15_69,feature_alpha_15_69},{2,haar_15_70,feature_thres_15_70,feature_left_15_70,feature_right_15_70,feature_alpha_15_70},{2,haar_15_71,feature_thres_15_71,feature_left_15_71,feature_right_15_71,feature_alpha_15_71},{2,haar_15_72,feature_thres_15_72,feature_left_15_72,feature_right_15_72,feature_alpha_15_72},{2,haar_15_73,feature_thres_15_73,feature_left_15_73,feature_right_15_73,feature_alpha_15_73},{2,haar_15_74,feature_thres_15_74,feature_left_15_74,feature_right_15_74,feature_alpha_15_74},{2,haar_15_75,feature_thres_15_75,feature_left_15_75,feature_right_15_75,feature_alpha_15_75},{2,haar_15_76,feature_thres_15_76,feature_left_15_76,feature_right_15_76,feature_alpha_15_76},{2,haar_15_77,feature_thres_15_77,feature_left_15_77,feature_right_15_77,feature_alpha_15_77},
};
HaarFeature MEMORY_TYPE haar_16_0[2] =
{
0,mvcvRect(1,1,4,11),-1.000000,mvcvRect(3,1,2,11),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,1,8),-1.000000,mvcvRect(5,9,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_0[2] =
{
0.011730,0.001171,
};
int MEMORY_TYPE feature_left_16_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_0[3] =
{
0.380522,0.314002,0.685815,
};
HaarFeature MEMORY_TYPE haar_16_1[2] =
{
0,mvcvRect(7,7,6,1),-1.000000,mvcvRect(9,7,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,12,2),-1.000000,mvcvRect(8,7,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_1[2] =
{
0.009356,0.001657,
};
int MEMORY_TYPE feature_left_16_1[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_1[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_1[3] =
{
0.683467,0.299247,0.547568,
};
HaarFeature MEMORY_TYPE haar_16_2[2] =
{
0,mvcvRect(8,4,4,4),-1.000000,mvcvRect(8,6,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,4,9,1),-1.000000,mvcvRect(5,4,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_2[2] =
{
-0.001339,0.000176,
};
int MEMORY_TYPE feature_left_16_2[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_2[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_2[3] =
{
0.294141,0.389698,0.587297,
};
HaarFeature MEMORY_TYPE haar_16_3[2] =
{
0,mvcvRect(9,12,2,8),-1.000000,mvcvRect(9,16,2,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,8,14,12),-1.000000,mvcvRect(3,14,14,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_3[2] =
{
-0.002947,0.008322,
};
int MEMORY_TYPE feature_left_16_3[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_3[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_3[3] =
{
0.357657,0.523240,0.323109,
};
HaarFeature MEMORY_TYPE haar_16_4[2] =
{
0,mvcvRect(6,13,7,3),-1.000000,mvcvRect(6,14,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,9,6,3),-1.000000,mvcvRect(7,9,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_4[2] =
{
0.007437,-0.000213,
};
int MEMORY_TYPE feature_left_16_4[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_4[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_4[3] =
{
0.671567,0.547054,0.386340,
};
HaarFeature MEMORY_TYPE haar_16_5[2] =
{
0,mvcvRect(12,1,6,3),-1.000000,mvcvRect(12,2,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,12,6,2),-1.000000,mvcvRect(8,13,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_5[2] =
{
-0.007802,0.000566,
};
int MEMORY_TYPE feature_left_16_5[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_5[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_5[3] =
{
0.277146,0.468914,0.585196,
};
HaarFeature MEMORY_TYPE haar_16_6[2] =
{
0,mvcvRect(0,2,18,2),-1.000000,mvcvRect(0,2,9,1),2.000000,mvcvRect(9,3,9,1),2.000000,0,mvcvRect(6,10,3,6),-1.000000,mvcvRect(6,13,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_6[2] =
{
-0.009235,-0.000015,
};
int MEMORY_TYPE feature_left_16_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_6[3] =
{
0.270440,0.562255,0.357932,
};
HaarFeature MEMORY_TYPE haar_16_7[2] =
{
0,mvcvRect(14,0,6,6),-1.000000,mvcvRect(14,0,3,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,0,5,8),-1.000000,mvcvRect(15,4,5,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_7[2] =
{
0.009701,-0.003532,
};
int MEMORY_TYPE feature_left_16_7[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_7[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_7[3] =
{
0.417387,0.419501,0.554947,
};
HaarFeature MEMORY_TYPE haar_16_8[2] =
{
0,mvcvRect(7,16,6,4),-1.000000,mvcvRect(9,16,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,11,14,4),-1.000000,mvcvRect(2,11,7,2),2.000000,mvcvRect(9,13,7,2),2.000000,
};
float MEMORY_TYPE feature_thres_16_8[2] =
{
0.021616,0.003457,
};
int MEMORY_TYPE feature_left_16_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_8[3] =
{
0.285739,0.602453,0.437751,
};
HaarFeature MEMORY_TYPE haar_16_9[2] =
{
0,mvcvRect(14,10,6,10),-1.000000,mvcvRect(14,10,3,10),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,8,10,12),-1.000000,mvcvRect(14,8,5,6),2.000000,mvcvRect(9,14,5,6),2.000000,
};
float MEMORY_TYPE feature_thres_16_9[2] =
{
0.022914,0.003433,
};
int MEMORY_TYPE feature_left_16_9[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_9[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_9[3] =
{
0.468935,0.466460,0.576256,
};
HaarFeature MEMORY_TYPE haar_16_10[2] =
{
0,mvcvRect(0,10,6,10),-1.000000,mvcvRect(3,10,3,10),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,8,10,12),-1.000000,mvcvRect(1,8,5,6),2.000000,mvcvRect(6,14,5,6),2.000000,
};
float MEMORY_TYPE feature_thres_16_10[2] =
{
-0.008651,0.001451,
};
int MEMORY_TYPE feature_left_16_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_10[3] =
{
0.638174,0.371149,0.553075,
};
HaarFeature MEMORY_TYPE haar_16_11[2] =
{
0,mvcvRect(9,3,6,1),-1.000000,mvcvRect(11,3,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,4,6,3),-1.000000,mvcvRect(9,4,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_11[2] =
{
0.007819,0.000208,
};
int MEMORY_TYPE feature_left_16_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_11[3] =
{
0.526436,0.373051,0.544573,
};
HaarFeature MEMORY_TYPE haar_16_12[2] =
{
0,mvcvRect(5,3,6,1),-1.000000,mvcvRect(7,3,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,5,6,3),-1.000000,mvcvRect(6,5,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_12[2] =
{
-0.003996,-0.000015,
};
int MEMORY_TYPE feature_left_16_12[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_12[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_12[3] =
{
0.243817,0.532467,0.368299,
};
HaarFeature MEMORY_TYPE haar_16_13[2] =
{
0,mvcvRect(9,16,3,3),-1.000000,mvcvRect(9,17,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,14,6,3),-1.000000,mvcvRect(8,15,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_13[2] =
{
-0.004243,0.009137,
};
int MEMORY_TYPE feature_left_16_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_13[3] =
{
0.648147,0.489616,0.655884,
};
HaarFeature MEMORY_TYPE haar_16_14[2] =
{
0,mvcvRect(6,0,8,12),-1.000000,mvcvRect(6,0,4,6),2.000000,mvcvRect(10,6,4,6),2.000000,0,mvcvRect(4,12,2,3),-1.000000,mvcvRect(4,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_14[2] =
{
0.008825,0.000941,
};
int MEMORY_TYPE feature_left_16_14[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_14[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_14[3] =
{
0.361387,0.550290,0.363252,
};
HaarFeature MEMORY_TYPE haar_16_15[2] =
{
0,mvcvRect(12,16,6,3),-1.000000,mvcvRect(12,17,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,12,7,2),-1.000000,mvcvRect(7,13,7,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_15[2] =
{
-0.012503,0.008676,
};
int MEMORY_TYPE feature_left_16_15[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_15[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_15[3] =
{
0.226113,0.498789,0.684720,
};
HaarFeature MEMORY_TYPE haar_16_16[2] =
{
0,mvcvRect(2,16,6,3),-1.000000,mvcvRect(2,17,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,7,16,6),-1.000000,mvcvRect(0,10,16,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_16[2] =
{
-0.010417,0.002743,
};
int MEMORY_TYPE feature_left_16_16[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_16[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_16[3] =
{
0.244630,0.351153,0.539983,
};
HaarFeature MEMORY_TYPE haar_16_17[2] =
{
0,mvcvRect(9,7,3,3),-1.000000,mvcvRect(10,7,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,3,5),-1.000000,mvcvRect(10,7,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_17[2] =
{
-0.004239,0.018326,
};
int MEMORY_TYPE feature_left_16_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_17[3] =
{
0.682367,0.489158,0.713562,
};
HaarFeature MEMORY_TYPE haar_16_18[2] =
{
0,mvcvRect(0,5,20,10),-1.000000,mvcvRect(0,5,10,5),2.000000,mvcvRect(10,10,10,5),2.000000,0,mvcvRect(3,1,4,2),-1.000000,mvcvRect(5,1,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_18[2] =
{
-0.024335,0.000465,
};
int MEMORY_TYPE feature_left_16_18[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_18[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_18[3] =
{
0.352252,0.404987,0.551583,
};
HaarFeature MEMORY_TYPE haar_16_19[2] =
{
0,mvcvRect(7,6,8,10),-1.000000,mvcvRect(11,6,4,5),2.000000,mvcvRect(7,11,4,5),2.000000,0,mvcvRect(17,6,3,2),-1.000000,mvcvRect(17,7,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_19[2] =
{
0.003426,-0.002583,
};
int MEMORY_TYPE feature_left_16_19[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_19[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_19[3] =
{
0.412677,0.289943,0.538643,
};
HaarFeature MEMORY_TYPE haar_16_20[2] =
{
0,mvcvRect(5,6,8,10),-1.000000,mvcvRect(5,6,4,5),2.000000,mvcvRect(9,11,4,5),2.000000,0,mvcvRect(5,12,10,6),-1.000000,mvcvRect(5,14,10,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_20[2] =
{
0.001055,-0.000913,
};
int MEMORY_TYPE feature_left_16_20[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_20[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_20[3] =
{
0.377134,0.582739,0.426756,
};
HaarFeature MEMORY_TYPE haar_16_21[2] =
{
0,mvcvRect(9,7,3,3),-1.000000,mvcvRect(10,7,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,3,2,6),-1.000000,mvcvRect(11,3,1,3),2.000000,mvcvRect(10,6,1,3),2.000000,
};
float MEMORY_TYPE feature_thres_16_21[2] =
{
0.002659,0.004860,
};
int MEMORY_TYPE feature_left_16_21[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_21[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_21[3] =
{
0.468812,0.485392,0.616364,
};
HaarFeature MEMORY_TYPE haar_16_22[2] =
{
0,mvcvRect(0,4,3,3),-1.000000,mvcvRect(0,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,16,8,4),-1.000000,mvcvRect(3,16,4,2),2.000000,mvcvRect(7,18,4,2),2.000000,
};
float MEMORY_TYPE feature_thres_16_22[2] =
{
0.008064,-0.007590,
};
int MEMORY_TYPE feature_left_16_22[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_22[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_22[3] =
{
0.174920,0.682619,0.489407,
};
HaarFeature MEMORY_TYPE haar_16_23[2] =
{
0,mvcvRect(8,13,5,2),-1.000000,mvcvRect(8,14,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,4,12),-1.000000,mvcvRect(8,11,4,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_23[2] =
{
0.000364,0.062595,
};
int MEMORY_TYPE feature_left_16_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_23[3] =
{
0.461460,0.518302,0.268670,
};
HaarFeature MEMORY_TYPE haar_16_24[2] =
{
0,mvcvRect(5,9,2,2),-1.000000,mvcvRect(6,9,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,15,2,3),-1.000000,mvcvRect(9,16,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_24[2] =
{
-0.004975,-0.002088,
};
int MEMORY_TYPE feature_left_16_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_24[3] =
{
0.175847,0.636938,0.493004,
};
HaarFeature MEMORY_TYPE haar_16_25[2] =
{
0,mvcvRect(13,9,2,3),-1.000000,mvcvRect(13,9,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,0,6,17),-1.000000,mvcvRect(16,0,2,17),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_25[2] =
{
0.000956,-0.031721,
};
int MEMORY_TYPE feature_left_16_25[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_25[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_25[3] =
{
0.413940,0.604556,0.481636,
};
HaarFeature MEMORY_TYPE haar_16_26[2] =
{
0,mvcvRect(5,10,2,2),-1.000000,mvcvRect(6,10,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,9,9,1),-1.000000,mvcvRect(5,9,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_26[2] =
{
0.001290,0.009841,
};
int MEMORY_TYPE feature_left_16_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_26[3] =
{
0.545081,0.292400,0.669961,
};
HaarFeature MEMORY_TYPE haar_16_27[2] =
{
0,mvcvRect(9,11,2,3),-1.000000,mvcvRect(9,12,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,11,6,3),-1.000000,mvcvRect(7,12,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_27[2] =
{
0.001224,-0.008423,
};
int MEMORY_TYPE feature_left_16_27[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_27[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_27[3] =
{
0.628284,0.598657,0.485258,
};
HaarFeature MEMORY_TYPE haar_16_28[2] =
{
0,mvcvRect(0,6,3,2),-1.000000,mvcvRect(0,7,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,0,6,1),-1.000000,mvcvRect(9,0,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_28[2] =
{
-0.000727,0.004684,
};
int MEMORY_TYPE feature_left_16_28[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_28[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_28[3] =
{
0.334005,0.516892,0.267948,
};
HaarFeature MEMORY_TYPE haar_16_29[2] =
{
0,mvcvRect(9,16,3,3),-1.000000,mvcvRect(9,17,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,13,17,6),-1.000000,mvcvRect(2,16,17,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_29[2] =
{
-0.001038,0.009134,
};
int MEMORY_TYPE feature_left_16_29[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_29[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_29[3] =
{
0.592579,0.543773,0.434680,
};
HaarFeature MEMORY_TYPE haar_16_30[2] =
{
0,mvcvRect(1,3,3,7),-1.000000,mvcvRect(2,3,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,1,6,4),-1.000000,mvcvRect(3,1,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_30[2] =
{
0.001497,0.001576,
};
int MEMORY_TYPE feature_left_16_30[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_30[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_30[3] =
{
0.412950,0.452287,0.655629,
};
HaarFeature MEMORY_TYPE haar_16_31[2] =
{
0,mvcvRect(14,1,6,5),-1.000000,mvcvRect(14,1,3,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,2,3,2),-1.000000,mvcvRect(13,3,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_31[2] =
{
0.008750,-0.000851,
};
int MEMORY_TYPE feature_left_16_31[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_31[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_31[3] =
{
0.453203,0.378598,0.541698,
};
HaarFeature MEMORY_TYPE haar_16_32[2] =
{
0,mvcvRect(0,1,6,5),-1.000000,mvcvRect(3,1,3,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,3,2,6),-1.000000,mvcvRect(2,5,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_32[2] =
{
-0.017326,-0.008327,
};
int MEMORY_TYPE feature_left_16_32[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_32[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_32[3] =
{
0.688425,0.309133,0.524365,
};
HaarFeature MEMORY_TYPE haar_16_33[2] =
{
0,mvcvRect(9,10,3,2),-1.000000,mvcvRect(9,11,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,13,4,3),-1.000000,mvcvRect(8,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_33[2] =
{
0.000015,0.001804,
};
int MEMORY_TYPE feature_left_16_33[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_33[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_33[3] =
{
0.476579,0.472539,0.571656,
};
HaarFeature MEMORY_TYPE haar_16_34[2] =
{
0,mvcvRect(6,3,3,1),-1.000000,mvcvRect(7,3,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,2,3,12),-1.000000,mvcvRect(8,6,3,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_34[2] =
{
0.003069,-0.000052,
};
int MEMORY_TYPE feature_left_16_34[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_34[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_34[3] =
{
0.214336,0.565321,0.438511,
};
HaarFeature MEMORY_TYPE haar_16_35[2] =
{
0,mvcvRect(11,12,1,2),-1.000000,mvcvRect(11,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,12,2,2),-1.000000,mvcvRect(12,12,1,1),2.000000,mvcvRect(11,13,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_16_35[2] =
{
0.000101,0.000136,
};
int MEMORY_TYPE feature_left_16_35[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_35[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_35[3] =
{
0.592478,0.457345,0.576938,
};
HaarFeature MEMORY_TYPE haar_16_36[2] =
{
0,mvcvRect(5,5,2,2),-1.000000,mvcvRect(5,6,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,1,3),-1.000000,mvcvRect(5,5,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_36[2] =
{
0.000921,0.000303,
};
int MEMORY_TYPE feature_left_16_36[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_36[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_36[3] =
{
0.599261,0.361008,0.504933,
};
HaarFeature MEMORY_TYPE haar_16_37[2] =
{
0,mvcvRect(3,11,16,4),-1.000000,mvcvRect(11,11,8,2),2.000000,mvcvRect(3,13,8,2),2.000000,0,mvcvRect(0,10,20,3),-1.000000,mvcvRect(0,11,20,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_37[2] =
{
0.039582,0.047520,
};
int MEMORY_TYPE feature_left_16_37[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_37[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_37[3] =
{
0.153849,0.521614,0.142839,
};
HaarFeature MEMORY_TYPE haar_16_38[2] =
{
0,mvcvRect(1,11,16,4),-1.000000,mvcvRect(1,11,8,2),2.000000,mvcvRect(9,13,8,2),2.000000,0,mvcvRect(4,2,4,2),-1.000000,mvcvRect(4,3,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_38[2] =
{
0.018872,-0.000399,
};
int MEMORY_TYPE feature_left_16_38[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_38[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_38[3] =
{
0.282551,0.403502,0.543779,
};
HaarFeature MEMORY_TYPE haar_16_39[2] =
{
0,mvcvRect(12,6,2,2),-1.000000,mvcvRect(13,6,1,1),2.000000,mvcvRect(12,7,1,1),2.000000,0,mvcvRect(12,11,6,6),-1.000000,mvcvRect(12,13,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_39[2] =
{
0.000466,0.006709,
};
int MEMORY_TYPE feature_left_16_39[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_39[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_39[3] =
{
0.466900,0.533135,0.413657,
};
HaarFeature MEMORY_TYPE haar_16_40[2] =
{
0,mvcvRect(6,6,2,2),-1.000000,mvcvRect(6,6,1,1),2.000000,mvcvRect(7,7,1,1),2.000000,0,mvcvRect(6,4,4,16),-1.000000,mvcvRect(8,4,2,16),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_40[2] =
{
-0.001893,-0.013057,
};
int MEMORY_TYPE feature_left_16_40[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_40[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_40[3] =
{
0.715516,0.311790,0.520844,
};
HaarFeature MEMORY_TYPE haar_16_41[2] =
{
0,mvcvRect(11,18,3,2),-1.000000,mvcvRect(11,19,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,17,6,2),-1.000000,mvcvRect(12,17,3,1),2.000000,mvcvRect(9,18,3,1),2.000000,
};
float MEMORY_TYPE feature_thres_16_41[2] =
{
-0.000195,0.000015,
};
int MEMORY_TYPE feature_left_16_41[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_41[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_41[3] =
{
0.463766,0.456165,0.544523,
};
HaarFeature MEMORY_TYPE haar_16_42[2] =
{
0,mvcvRect(2,13,5,2),-1.000000,mvcvRect(2,14,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,15,2,2),-1.000000,mvcvRect(3,16,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_42[2] =
{
-0.000007,0.000302,
};
int MEMORY_TYPE feature_left_16_42[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_42[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_42[3] =
{
0.419311,0.596624,0.410050,
};
HaarFeature MEMORY_TYPE haar_16_43[2] =
{
0,mvcvRect(9,7,3,3),-1.000000,mvcvRect(10,7,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,6,2,6),-1.000000,mvcvRect(9,6,1,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_43[2] =
{
0.004420,-0.007398,
};
int MEMORY_TYPE feature_left_16_43[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_43[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_43[3] =
{
0.484506,0.620685,0.493121,
};
HaarFeature MEMORY_TYPE haar_16_44[2] =
{
0,mvcvRect(1,14,7,6),-1.000000,mvcvRect(1,16,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,1,2,11),-1.000000,mvcvRect(9,1,1,11),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_44[2] =
{
-0.007803,-0.010731,
};
int MEMORY_TYPE feature_left_16_44[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_44[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_44[3] =
{
0.528246,0.910483,0.345592,
};
HaarFeature MEMORY_TYPE haar_16_45[2] =
{
0,mvcvRect(9,7,2,4),-1.000000,mvcvRect(9,7,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,10,2,1),-1.000000,mvcvRect(11,10,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_45[2] =
{
0.001425,-0.000083,
};
int MEMORY_TYPE feature_left_16_45[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_45[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_45[3] =
{
0.470855,0.565162,0.473102,
};
HaarFeature MEMORY_TYPE haar_16_46[2] =
{
0,mvcvRect(0,3,3,9),-1.000000,mvcvRect(1,3,1,9),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,3,3,6),-1.000000,mvcvRect(0,5,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_46[2] =
{
0.004480,0.003079,
};
int MEMORY_TYPE feature_left_16_46[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_46[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_46[3] =
{
0.617589,0.513953,0.342309,
};
HaarFeature MEMORY_TYPE haar_16_47[2] =
{
0,mvcvRect(11,15,2,2),-1.000000,mvcvRect(12,15,1,1),2.000000,mvcvRect(11,16,1,1),2.000000,0,mvcvRect(11,14,2,2),-1.000000,mvcvRect(12,14,1,1),2.000000,mvcvRect(11,15,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_16_47[2] =
{
-0.001131,-0.001041,
};
int MEMORY_TYPE feature_left_16_47[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_47[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_47[3] =
{
0.491828,0.594209,0.492304,
};
HaarFeature MEMORY_TYPE haar_16_48[2] =
{
0,mvcvRect(7,15,2,2),-1.000000,mvcvRect(7,15,1,1),2.000000,mvcvRect(8,16,1,1),2.000000,0,mvcvRect(7,14,2,2),-1.000000,mvcvRect(7,14,1,1),2.000000,mvcvRect(8,15,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_16_48[2] =
{
0.001165,0.000901,
};
int MEMORY_TYPE feature_left_16_48[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_48[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_48[3] =
{
0.640527,0.450440,0.619208,
};
HaarFeature MEMORY_TYPE haar_16_49[2] =
{
0,mvcvRect(8,13,4,6),-1.000000,mvcvRect(10,13,2,3),2.000000,mvcvRect(8,16,2,3),2.000000,0,mvcvRect(2,14,16,4),-1.000000,mvcvRect(10,14,8,2),2.000000,mvcvRect(2,16,8,2),2.000000,
};
float MEMORY_TYPE feature_thres_16_49[2] =
{
0.006878,-0.035284,
};
int MEMORY_TYPE feature_left_16_49[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_49[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_49[3] =
{
0.537481,0.224710,0.521717,
};
HaarFeature MEMORY_TYPE haar_16_50[2] =
{
0,mvcvRect(9,8,2,2),-1.000000,mvcvRect(9,9,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,7,5,3),-1.000000,mvcvRect(7,8,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_50[2] =
{
-0.001332,-0.002318,
};
int MEMORY_TYPE feature_left_16_50[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_50[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_50[3] =
{
0.255470,0.379252,0.524323,
};
HaarFeature MEMORY_TYPE haar_16_51[2] =
{
0,mvcvRect(7,5,6,2),-1.000000,mvcvRect(9,5,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,1,6,18),-1.000000,mvcvRect(11,1,2,18),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_51[2] =
{
0.000213,0.013468,
};
int MEMORY_TYPE feature_left_16_51[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_51[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_51[3] =
{
0.386034,0.538069,0.417836,
};
HaarFeature MEMORY_TYPE haar_16_52[2] =
{
0,mvcvRect(8,6,3,4),-1.000000,mvcvRect(9,6,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,5,2,4),-1.000000,mvcvRect(8,5,1,2),2.000000,mvcvRect(9,7,1,2),2.000000,
};
float MEMORY_TYPE feature_thres_16_52[2] =
{
-0.001283,0.000516,
};
int MEMORY_TYPE feature_left_16_52[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_52[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_52[3] =
{
0.613362,0.402854,0.553685,
};
HaarFeature MEMORY_TYPE haar_16_53[2] =
{
0,mvcvRect(9,13,2,6),-1.000000,mvcvRect(10,13,1,3),2.000000,mvcvRect(9,16,1,3),2.000000,0,mvcvRect(11,0,3,18),-1.000000,mvcvRect(12,0,1,18),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_53[2] =
{
0.003925,-0.033781,
};
int MEMORY_TYPE feature_left_16_53[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_53[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_53[3] =
{
0.527992,0.233468,0.517591,
};
HaarFeature MEMORY_TYPE haar_16_54[2] =
{
0,mvcvRect(6,0,3,18),-1.000000,mvcvRect(7,0,1,18),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,15,4,2),-1.000000,mvcvRect(7,15,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_54[2] =
{
-0.037854,-0.000408,
};
int MEMORY_TYPE feature_left_16_54[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_54[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_54[3] =
{
0.107485,0.534593,0.419894,
};
HaarFeature MEMORY_TYPE haar_16_55[2] =
{
0,mvcvRect(1,9,18,1),-1.000000,mvcvRect(7,9,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,20,3),-1.000000,mvcvRect(0,1,20,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_55[2] =
{
-0.003119,-0.015715,
};
int MEMORY_TYPE feature_left_16_55[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_55[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_55[3] =
{
0.385583,0.333519,0.526320,
};
HaarFeature MEMORY_TYPE haar_16_56[2] =
{
0,mvcvRect(9,6,2,4),-1.000000,mvcvRect(10,6,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,10,6,2),-1.000000,mvcvRect(8,10,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_56[2] =
{
-0.000785,-0.000288,
};
int MEMORY_TYPE feature_left_16_56[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_56[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_56[3] =
{
0.586040,0.543778,0.371610,
};
HaarFeature MEMORY_TYPE haar_16_57[2] =
{
0,mvcvRect(0,7,20,1),-1.000000,mvcvRect(0,7,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,3,5,4),-1.000000,mvcvRect(11,5,5,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_57[2] =
{
0.028017,-0.001902,
};
int MEMORY_TYPE feature_left_16_57[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_57[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_57[3] =
{
0.333075,0.536660,0.469379,
};
HaarFeature MEMORY_TYPE haar_16_58[2] =
{
0,mvcvRect(5,7,10,1),-1.000000,mvcvRect(10,7,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,10,3,3),-1.000000,mvcvRect(8,11,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_58[2] =
{
0.020648,0.004300,
};
int MEMORY_TYPE feature_left_16_58[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_58[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_58[3] =
{
0.100696,0.481604,0.621568,
};
HaarFeature MEMORY_TYPE haar_16_59[2] =
{
0,mvcvRect(2,0,16,8),-1.000000,mvcvRect(10,0,8,4),2.000000,mvcvRect(2,4,8,4),2.000000,0,mvcvRect(11,0,9,10),-1.000000,mvcvRect(11,5,9,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_59[2] =
{
0.013459,-0.010320,
};
int MEMORY_TYPE feature_left_16_59[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_59[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_59[3] =
{
0.546195,0.457845,0.541931,
};
HaarFeature MEMORY_TYPE haar_16_60[2] =
{
0,mvcvRect(0,2,8,18),-1.000000,mvcvRect(4,2,4,18),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,2,6),-1.000000,mvcvRect(0,2,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_60[2] =
{
0.319907,0.000922,
};
int MEMORY_TYPE feature_left_16_60[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_60[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_60[3] =
{
0.200805,0.519328,0.391219,
};
HaarFeature MEMORY_TYPE haar_16_61[2] =
{
0,mvcvRect(6,0,9,2),-1.000000,mvcvRect(6,1,9,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,1,12,2),-1.000000,mvcvRect(4,2,12,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_61[2] =
{
0.000419,0.000359,
};
int MEMORY_TYPE feature_left_16_61[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_61[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_61[3] =
{
0.429974,0.434450,0.553197,
};
HaarFeature MEMORY_TYPE haar_16_62[2] =
{
0,mvcvRect(2,1,16,14),-1.000000,mvcvRect(2,8,16,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,1,8,12),-1.000000,mvcvRect(5,7,8,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_62[2] =
{
-0.209924,-0.004933,
};
int MEMORY_TYPE feature_left_16_62[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_62[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_62[3] =
{
0.107572,0.576280,0.457464,
};
HaarFeature MEMORY_TYPE haar_16_63[2] =
{
0,mvcvRect(9,11,2,2),-1.000000,mvcvRect(9,12,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,10,5,6),-1.000000,mvcvRect(9,12,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_63[2] =
{
0.002341,0.004712,
};
int MEMORY_TYPE feature_left_16_63[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_63[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_63[3] =
{
0.747681,0.526177,0.450555,
};
HaarFeature MEMORY_TYPE haar_16_64[2] =
{
0,mvcvRect(3,0,13,8),-1.000000,mvcvRect(3,4,13,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,7,5,8),-1.000000,mvcvRect(6,11,5,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_64[2] =
{
0.028713,-0.002616,
};
int MEMORY_TYPE feature_left_16_64[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_64[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_64[3] =
{
0.440710,0.424427,0.689298,
};
HaarFeature MEMORY_TYPE haar_16_65[2] =
{
0,mvcvRect(9,5,2,3),-1.000000,mvcvRect(9,6,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,8,8,3),-1.000000,mvcvRect(6,9,8,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_65[2] =
{
-0.013559,-0.000303,
};
int MEMORY_TYPE feature_left_16_65[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_65[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_65[3] =
{
0.125227,0.407779,0.544282,
};
HaarFeature MEMORY_TYPE haar_16_66[2] =
{
0,mvcvRect(2,2,7,6),-1.000000,mvcvRect(2,5,7,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,1,14,4),-1.000000,mvcvRect(2,1,7,2),2.000000,mvcvRect(9,3,7,2),2.000000,
};
float MEMORY_TYPE feature_thres_16_66[2] =
{
-0.000556,0.002403,
};
int MEMORY_TYPE feature_left_16_66[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_66[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_66[3] =
{
0.537800,0.316658,0.528574,
};
HaarFeature MEMORY_TYPE haar_16_67[2] =
{
0,mvcvRect(11,14,1,3),-1.000000,mvcvRect(11,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,15,8,2),-1.000000,mvcvRect(6,16,8,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_67[2] =
{
-0.003409,0.000800,
};
int MEMORY_TYPE feature_left_16_67[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_67[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_67[3] =
{
0.490521,0.452274,0.558061,
};
HaarFeature MEMORY_TYPE haar_16_68[2] =
{
0,mvcvRect(8,14,1,3),-1.000000,mvcvRect(8,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,11,2,8),-1.000000,mvcvRect(8,15,2,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_68[2] =
{
0.002190,0.003375,
};
int MEMORY_TYPE feature_left_16_68[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_68[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_68[3] =
{
0.661268,0.510777,0.338693,
};
HaarFeature MEMORY_TYPE haar_16_69[2] =
{
0,mvcvRect(6,15,8,2),-1.000000,mvcvRect(6,16,8,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,16,8,3),-1.000000,mvcvRect(7,17,8,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_69[2] =
{
0.000800,0.017346,
};
int MEMORY_TYPE feature_left_16_69[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_69[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_69[3] =
{
0.570756,0.501602,0.630646,
};
HaarFeature MEMORY_TYPE haar_16_70[2] =
{
0,mvcvRect(0,16,2,2),-1.000000,mvcvRect(0,17,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,16,8,4),-1.000000,mvcvRect(1,16,4,2),2.000000,mvcvRect(5,18,4,2),2.000000,
};
float MEMORY_TYPE feature_thres_16_70[2] =
{
-0.001957,-0.011229,
};
int MEMORY_TYPE feature_left_16_70[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_70[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_70[3] =
{
0.301781,0.629385,0.452049,
};
HaarFeature MEMORY_TYPE haar_16_71[2] =
{
0,mvcvRect(2,9,16,3),-1.000000,mvcvRect(2,10,16,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,11,2,4),-1.000000,mvcvRect(13,11,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_71[2] =
{
-0.002661,-0.011615,
};
int MEMORY_TYPE feature_left_16_71[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_71[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_71[3] =
{
0.334401,0.282538,0.515097,
};
HaarFeature MEMORY_TYPE haar_16_72[2] =
{
0,mvcvRect(0,13,16,6),-1.000000,mvcvRect(0,15,16,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,11,2,4),-1.000000,mvcvRect(6,11,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_72[2] =
{
-0.095249,0.007370,
};
int MEMORY_TYPE feature_left_16_72[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_72[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_72[3] =
{
0.139827,0.529400,0.233173,
};
HaarFeature MEMORY_TYPE haar_16_73[2] =
{
0,mvcvRect(18,2,2,18),-1.000000,mvcvRect(19,2,1,9),2.000000,mvcvRect(18,11,1,9),2.000000,0,mvcvRect(19,7,1,9),-1.000000,mvcvRect(19,10,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_73[2] =
{
-0.014954,0.000570,
};
int MEMORY_TYPE feature_left_16_73[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_73[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_73[3] =
{
0.494047,0.546657,0.462677,
};
HaarFeature MEMORY_TYPE haar_16_74[2] =
{
0,mvcvRect(0,2,2,18),-1.000000,mvcvRect(0,2,1,9),2.000000,mvcvRect(1,11,1,9),2.000000,0,mvcvRect(0,7,1,9),-1.000000,mvcvRect(0,10,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_74[2] =
{
0.005852,0.000212,
};
int MEMORY_TYPE feature_left_16_74[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_74[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_74[3] =
{
0.627004,0.550814,0.406187,
};
HaarFeature MEMORY_TYPE haar_16_75[2] =
{
0,mvcvRect(14,12,2,2),-1.000000,mvcvRect(14,13,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,14,2,3),-1.000000,mvcvRect(11,15,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_75[2] =
{
-0.000007,-0.000797,
};
int MEMORY_TYPE feature_left_16_75[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_75[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_75[3] =
{
0.409657,0.561556,0.466689,
};
HaarFeature MEMORY_TYPE haar_16_76[2] =
{
0,mvcvRect(7,8,6,2),-1.000000,mvcvRect(7,9,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,12,4,6),-1.000000,mvcvRect(7,12,2,3),2.000000,mvcvRect(9,15,2,3),2.000000,
};
float MEMORY_TYPE feature_thres_16_76[2] =
{
0.019459,-0.011161,
};
int MEMORY_TYPE feature_left_16_76[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_76[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_76[3] =
{
0.231148,0.308701,0.551466,
};
HaarFeature MEMORY_TYPE haar_16_77[2] =
{
0,mvcvRect(8,13,5,3),-1.000000,mvcvRect(8,14,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,14,2,2),-1.000000,mvcvRect(13,14,1,1),2.000000,mvcvRect(12,15,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_16_77[2] =
{
0.014056,-0.000330,
};
int MEMORY_TYPE feature_left_16_77[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_77[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_77[3] =
{
0.700506,0.579749,0.469165,
};
HaarFeature MEMORY_TYPE haar_16_78[2] =
{
0,mvcvRect(7,13,6,3),-1.000000,mvcvRect(7,14,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,13,5,2),-1.000000,mvcvRect(7,14,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_78[2] =
{
-0.005464,0.000059,
};
int MEMORY_TYPE feature_left_16_78[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_78[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_78[3] =
{
0.592860,0.374140,0.517017,
};
HaarFeature MEMORY_TYPE haar_16_79[2] =
{
0,mvcvRect(2,10,16,4),-1.000000,mvcvRect(10,10,8,2),2.000000,mvcvRect(2,12,8,2),2.000000,0,mvcvRect(7,0,6,6),-1.000000,mvcvRect(9,0,2,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_79[2] =
{
0.006634,0.045263,
};
int MEMORY_TYPE feature_left_16_79[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_79[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_79[3] =
{
0.541499,0.518033,0.152968,
};
HaarFeature MEMORY_TYPE haar_16_80[2] =
{
0,mvcvRect(7,1,6,3),-1.000000,mvcvRect(7,2,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,12,6,2),-1.000000,mvcvRect(0,13,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_80[2] =
{
-0.008065,0.000474,
};
int MEMORY_TYPE feature_left_16_80[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_80[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_80[3] =
{
0.251547,0.512200,0.372595,
};
HaarFeature MEMORY_TYPE haar_16_81[2] =
{
0,mvcvRect(6,3,11,2),-1.000000,mvcvRect(6,4,11,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,0,8,6),-1.000000,mvcvRect(16,0,4,3),2.000000,mvcvRect(12,3,4,3),2.000000,
};
float MEMORY_TYPE feature_thres_16_81[2] =
{
0.000015,0.024321,
};
int MEMORY_TYPE feature_left_16_81[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_81[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_81[3] =
{
0.553244,0.496077,0.598332,
};
HaarFeature MEMORY_TYPE haar_16_82[2] =
{
0,mvcvRect(8,12,1,2),-1.000000,mvcvRect(8,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,8,1,12),-1.000000,mvcvRect(8,12,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_82[2] =
{
0.000070,0.002629,
};
int MEMORY_TYPE feature_left_16_82[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_82[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_82[3] =
{
0.416395,0.588014,0.339966,
};
HaarFeature MEMORY_TYPE haar_16_83[2] =
{
0,mvcvRect(11,11,2,2),-1.000000,mvcvRect(12,11,1,1),2.000000,mvcvRect(11,12,1,1),2.000000,0,mvcvRect(12,7,3,13),-1.000000,mvcvRect(13,7,1,13),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_83[2] =
{
0.003819,-0.025989,
};
int MEMORY_TYPE feature_left_16_83[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_83[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_83[3] =
{
0.784662,0.328811,0.515509,
};
HaarFeature MEMORY_TYPE haar_16_84[2] =
{
0,mvcvRect(7,11,2,2),-1.000000,mvcvRect(7,11,1,1),2.000000,mvcvRect(8,12,1,1),2.000000,0,mvcvRect(3,13,1,3),-1.000000,mvcvRect(3,14,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_84[2] =
{
0.001206,-0.001556,
};
int MEMORY_TYPE feature_left_16_84[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_84[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_84[3] =
{
0.459606,0.312699,0.718340,
};
HaarFeature MEMORY_TYPE haar_16_85[2] =
{
0,mvcvRect(10,18,3,2),-1.000000,mvcvRect(11,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,11,2,1),-1.000000,mvcvRect(11,11,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_85[2] =
{
-0.002269,0.000233,
};
int MEMORY_TYPE feature_left_16_85[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_85[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_85[3] =
{
0.527401,0.487867,0.561515,
};
HaarFeature MEMORY_TYPE haar_16_86[2] =
{
0,mvcvRect(1,10,5,9),-1.000000,mvcvRect(1,13,5,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,8,6,4),-1.000000,mvcvRect(6,8,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_86[2] =
{
-0.005600,-0.010496,
};
int MEMORY_TYPE feature_left_16_86[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_86[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_86[3] =
{
0.516081,0.570161,0.320485,
};
HaarFeature MEMORY_TYPE haar_16_87[2] =
{
0,mvcvRect(13,12,1,4),-1.000000,mvcvRect(13,14,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,3,4,14),-1.000000,mvcvRect(13,3,2,7),2.000000,mvcvRect(11,10,2,7),2.000000,
};
float MEMORY_TYPE feature_thres_16_87[2] =
{
-0.000015,-0.000643,
};
int MEMORY_TYPE feature_left_16_87[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_87[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_87[3] =
{
0.553884,0.534943,0.447215,
};
HaarFeature MEMORY_TYPE haar_16_88[2] =
{
0,mvcvRect(6,12,1,4),-1.000000,mvcvRect(6,14,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,3,4,14),-1.000000,mvcvRect(5,3,2,7),2.000000,mvcvRect(7,10,2,7),2.000000,
};
float MEMORY_TYPE feature_thres_16_88[2] =
{
-0.000189,-0.009041,
};
int MEMORY_TYPE feature_left_16_88[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_16_88[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_16_88[3] =
{
0.501284,0.256294,0.450338,
};
HaarFeature MEMORY_TYPE haar_16_89[2] =
{
0,mvcvRect(10,18,3,2),-1.000000,mvcvRect(11,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,12,3,3),-1.000000,mvcvRect(9,13,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_89[2] =
{
0.007953,-0.002791,
};
int MEMORY_TYPE feature_left_16_89[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_89[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_89[3] =
{
0.263050,0.575651,0.485486,
};
HaarFeature MEMORY_TYPE haar_16_90[2] =
{
0,mvcvRect(2,2,12,6),-1.000000,mvcvRect(2,2,6,3),2.000000,mvcvRect(8,5,6,3),2.000000,0,mvcvRect(6,6,6,2),-1.000000,mvcvRect(9,6,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_16_90[2] =
{
0.003286,0.000771,
};
int MEMORY_TYPE feature_left_16_90[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_16_90[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_16_90[3] =
{
0.408475,0.407336,0.592024,
};
HaarClassifier MEMORY_TYPE haar_feature_class_16[91] =
{
{2,haar_16_0,feature_thres_16_0,feature_left_16_0,feature_right_16_0,feature_alpha_16_0},{2,haar_16_1,feature_thres_16_1,feature_left_16_1,feature_right_16_1,feature_alpha_16_1},{2,haar_16_2,feature_thres_16_2,feature_left_16_2,feature_right_16_2,feature_alpha_16_2},{2,haar_16_3,feature_thres_16_3,feature_left_16_3,feature_right_16_3,feature_alpha_16_3},{2,haar_16_4,feature_thres_16_4,feature_left_16_4,feature_right_16_4,feature_alpha_16_4},{2,haar_16_5,feature_thres_16_5,feature_left_16_5,feature_right_16_5,feature_alpha_16_5},{2,haar_16_6,feature_thres_16_6,feature_left_16_6,feature_right_16_6,feature_alpha_16_6},{2,haar_16_7,feature_thres_16_7,feature_left_16_7,feature_right_16_7,feature_alpha_16_7},{2,haar_16_8,feature_thres_16_8,feature_left_16_8,feature_right_16_8,feature_alpha_16_8},{2,haar_16_9,feature_thres_16_9,feature_left_16_9,feature_right_16_9,feature_alpha_16_9},{2,haar_16_10,feature_thres_16_10,feature_left_16_10,feature_right_16_10,feature_alpha_16_10},{2,haar_16_11,feature_thres_16_11,feature_left_16_11,feature_right_16_11,feature_alpha_16_11},{2,haar_16_12,feature_thres_16_12,feature_left_16_12,feature_right_16_12,feature_alpha_16_12},{2,haar_16_13,feature_thres_16_13,feature_left_16_13,feature_right_16_13,feature_alpha_16_13},{2,haar_16_14,feature_thres_16_14,feature_left_16_14,feature_right_16_14,feature_alpha_16_14},{2,haar_16_15,feature_thres_16_15,feature_left_16_15,feature_right_16_15,feature_alpha_16_15},{2,haar_16_16,feature_thres_16_16,feature_left_16_16,feature_right_16_16,feature_alpha_16_16},{2,haar_16_17,feature_thres_16_17,feature_left_16_17,feature_right_16_17,feature_alpha_16_17},{2,haar_16_18,feature_thres_16_18,feature_left_16_18,feature_right_16_18,feature_alpha_16_18},{2,haar_16_19,feature_thres_16_19,feature_left_16_19,feature_right_16_19,feature_alpha_16_19},{2,haar_16_20,feature_thres_16_20,feature_left_16_20,feature_right_16_20,feature_alpha_16_20},{2,haar_16_21,feature_thres_16_21,feature_left_16_21,feature_right_16_21,feature_alpha_16_21},{2,haar_16_22,feature_thres_16_22,feature_left_16_22,feature_right_16_22,feature_alpha_16_22},{2,haar_16_23,feature_thres_16_23,feature_left_16_23,feature_right_16_23,feature_alpha_16_23},{2,haar_16_24,feature_thres_16_24,feature_left_16_24,feature_right_16_24,feature_alpha_16_24},{2,haar_16_25,feature_thres_16_25,feature_left_16_25,feature_right_16_25,feature_alpha_16_25},{2,haar_16_26,feature_thres_16_26,feature_left_16_26,feature_right_16_26,feature_alpha_16_26},{2,haar_16_27,feature_thres_16_27,feature_left_16_27,feature_right_16_27,feature_alpha_16_27},{2,haar_16_28,feature_thres_16_28,feature_left_16_28,feature_right_16_28,feature_alpha_16_28},{2,haar_16_29,feature_thres_16_29,feature_left_16_29,feature_right_16_29,feature_alpha_16_29},{2,haar_16_30,feature_thres_16_30,feature_left_16_30,feature_right_16_30,feature_alpha_16_30},{2,haar_16_31,feature_thres_16_31,feature_left_16_31,feature_right_16_31,feature_alpha_16_31},{2,haar_16_32,feature_thres_16_32,feature_left_16_32,feature_right_16_32,feature_alpha_16_32},{2,haar_16_33,feature_thres_16_33,feature_left_16_33,feature_right_16_33,feature_alpha_16_33},{2,haar_16_34,feature_thres_16_34,feature_left_16_34,feature_right_16_34,feature_alpha_16_34},{2,haar_16_35,feature_thres_16_35,feature_left_16_35,feature_right_16_35,feature_alpha_16_35},{2,haar_16_36,feature_thres_16_36,feature_left_16_36,feature_right_16_36,feature_alpha_16_36},{2,haar_16_37,feature_thres_16_37,feature_left_16_37,feature_right_16_37,feature_alpha_16_37},{2,haar_16_38,feature_thres_16_38,feature_left_16_38,feature_right_16_38,feature_alpha_16_38},{2,haar_16_39,feature_thres_16_39,feature_left_16_39,feature_right_16_39,feature_alpha_16_39},{2,haar_16_40,feature_thres_16_40,feature_left_16_40,feature_right_16_40,feature_alpha_16_40},{2,haar_16_41,feature_thres_16_41,feature_left_16_41,feature_right_16_41,feature_alpha_16_41},{2,haar_16_42,feature_thres_16_42,feature_left_16_42,feature_right_16_42,feature_alpha_16_42},{2,haar_16_43,feature_thres_16_43,feature_left_16_43,feature_right_16_43,feature_alpha_16_43},{2,haar_16_44,feature_thres_16_44,feature_left_16_44,feature_right_16_44,feature_alpha_16_44},{2,haar_16_45,feature_thres_16_45,feature_left_16_45,feature_right_16_45,feature_alpha_16_45},{2,haar_16_46,feature_thres_16_46,feature_left_16_46,feature_right_16_46,feature_alpha_16_46},{2,haar_16_47,feature_thres_16_47,feature_left_16_47,feature_right_16_47,feature_alpha_16_47},{2,haar_16_48,feature_thres_16_48,feature_left_16_48,feature_right_16_48,feature_alpha_16_48},{2,haar_16_49,feature_thres_16_49,feature_left_16_49,feature_right_16_49,feature_alpha_16_49},{2,haar_16_50,feature_thres_16_50,feature_left_16_50,feature_right_16_50,feature_alpha_16_50},{2,haar_16_51,feature_thres_16_51,feature_left_16_51,feature_right_16_51,feature_alpha_16_51},{2,haar_16_52,feature_thres_16_52,feature_left_16_52,feature_right_16_52,feature_alpha_16_52},{2,haar_16_53,feature_thres_16_53,feature_left_16_53,feature_right_16_53,feature_alpha_16_53},{2,haar_16_54,feature_thres_16_54,feature_left_16_54,feature_right_16_54,feature_alpha_16_54},{2,haar_16_55,feature_thres_16_55,feature_left_16_55,feature_right_16_55,feature_alpha_16_55},{2,haar_16_56,feature_thres_16_56,feature_left_16_56,feature_right_16_56,feature_alpha_16_56},{2,haar_16_57,feature_thres_16_57,feature_left_16_57,feature_right_16_57,feature_alpha_16_57},{2,haar_16_58,feature_thres_16_58,feature_left_16_58,feature_right_16_58,feature_alpha_16_58},{2,haar_16_59,feature_thres_16_59,feature_left_16_59,feature_right_16_59,feature_alpha_16_59},{2,haar_16_60,feature_thres_16_60,feature_left_16_60,feature_right_16_60,feature_alpha_16_60},{2,haar_16_61,feature_thres_16_61,feature_left_16_61,feature_right_16_61,feature_alpha_16_61},{2,haar_16_62,feature_thres_16_62,feature_left_16_62,feature_right_16_62,feature_alpha_16_62},{2,haar_16_63,feature_thres_16_63,feature_left_16_63,feature_right_16_63,feature_alpha_16_63},{2,haar_16_64,feature_thres_16_64,feature_left_16_64,feature_right_16_64,feature_alpha_16_64},{2,haar_16_65,feature_thres_16_65,feature_left_16_65,feature_right_16_65,feature_alpha_16_65},{2,haar_16_66,feature_thres_16_66,feature_left_16_66,feature_right_16_66,feature_alpha_16_66},{2,haar_16_67,feature_thres_16_67,feature_left_16_67,feature_right_16_67,feature_alpha_16_67},{2,haar_16_68,feature_thres_16_68,feature_left_16_68,feature_right_16_68,feature_alpha_16_68},{2,haar_16_69,feature_thres_16_69,feature_left_16_69,feature_right_16_69,feature_alpha_16_69},{2,haar_16_70,feature_thres_16_70,feature_left_16_70,feature_right_16_70,feature_alpha_16_70},{2,haar_16_71,feature_thres_16_71,feature_left_16_71,feature_right_16_71,feature_alpha_16_71},{2,haar_16_72,feature_thres_16_72,feature_left_16_72,feature_right_16_72,feature_alpha_16_72},{2,haar_16_73,feature_thres_16_73,feature_left_16_73,feature_right_16_73,feature_alpha_16_73},{2,haar_16_74,feature_thres_16_74,feature_left_16_74,feature_right_16_74,feature_alpha_16_74},{2,haar_16_75,feature_thres_16_75,feature_left_16_75,feature_right_16_75,feature_alpha_16_75},{2,haar_16_76,feature_thres_16_76,feature_left_16_76,feature_right_16_76,feature_alpha_16_76},{2,haar_16_77,feature_thres_16_77,feature_left_16_77,feature_right_16_77,feature_alpha_16_77},{2,haar_16_78,feature_thres_16_78,feature_left_16_78,feature_right_16_78,feature_alpha_16_78},{2,haar_16_79,feature_thres_16_79,feature_left_16_79,feature_right_16_79,feature_alpha_16_79},{2,haar_16_80,feature_thres_16_80,feature_left_16_80,feature_right_16_80,feature_alpha_16_80},{2,haar_16_81,feature_thres_16_81,feature_left_16_81,feature_right_16_81,feature_alpha_16_81},{2,haar_16_82,feature_thres_16_82,feature_left_16_82,feature_right_16_82,feature_alpha_16_82},{2,haar_16_83,feature_thres_16_83,feature_left_16_83,feature_right_16_83,feature_alpha_16_83},{2,haar_16_84,feature_thres_16_84,feature_left_16_84,feature_right_16_84,feature_alpha_16_84},{2,haar_16_85,feature_thres_16_85,feature_left_16_85,feature_right_16_85,feature_alpha_16_85},{2,haar_16_86,feature_thres_16_86,feature_left_16_86,feature_right_16_86,feature_alpha_16_86},{2,haar_16_87,feature_thres_16_87,feature_left_16_87,feature_right_16_87,feature_alpha_16_87},{2,haar_16_88,feature_thres_16_88,feature_left_16_88,feature_right_16_88,feature_alpha_16_88},{2,haar_16_89,feature_thres_16_89,feature_left_16_89,feature_right_16_89,feature_alpha_16_89},{2,haar_16_90,feature_thres_16_90,feature_left_16_90,feature_right_16_90,feature_alpha_16_90},
};
HaarFeature MEMORY_TYPE haar_17_0[2] =
{
0,mvcvRect(1,0,18,12),-1.000000,mvcvRect(7,0,6,12),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,7,6,4),-1.000000,mvcvRect(5,7,3,2),2.000000,mvcvRect(8,9,3,2),2.000000,
};
float MEMORY_TYPE feature_thres_17_0[2] =
{
0.063022,-0.002837,
};
int MEMORY_TYPE feature_left_17_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_0[3] =
{
0.341938,0.682956,0.440452,
};
HaarFeature MEMORY_TYPE haar_17_1[2] =
{
0,mvcvRect(5,7,10,4),-1.000000,mvcvRect(5,9,10,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,7,6,4),-1.000000,mvcvRect(9,7,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_1[2] =
{
0.046462,0.029153,
};
int MEMORY_TYPE feature_left_17_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_1[3] =
{
0.439175,0.460106,0.635794,
};
HaarFeature MEMORY_TYPE haar_17_2[2] =
{
0,mvcvRect(9,5,2,2),-1.000000,mvcvRect(9,6,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,9,2,2),-1.000000,mvcvRect(9,10,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_2[2] =
{
-0.000014,-0.001276,
};
int MEMORY_TYPE feature_left_17_2[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_2[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_2[3] =
{
0.373001,0.309382,0.590137,
};
HaarFeature MEMORY_TYPE haar_17_3[2] =
{
0,mvcvRect(6,17,8,3),-1.000000,mvcvRect(6,18,8,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,17,6,2),-1.000000,mvcvRect(12,17,3,1),2.000000,mvcvRect(9,18,3,1),2.000000,
};
float MEMORY_TYPE feature_thres_17_3[2] =
{
0.001360,0.000180,
};
int MEMORY_TYPE feature_left_17_3[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_3[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_3[3] =
{
0.433757,0.421750,0.584685,
};
HaarFeature MEMORY_TYPE haar_17_4[2] =
{
0,mvcvRect(4,12,2,2),-1.000000,mvcvRect(4,13,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,12,9,2),-1.000000,mvcvRect(3,13,9,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_4[2] =
{
-0.000014,0.000060,
};
int MEMORY_TYPE feature_left_17_4[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_4[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_4[3] =
{
0.408469,0.508729,0.727718,
};
HaarFeature MEMORY_TYPE haar_17_5[2] =
{
0,mvcvRect(8,3,6,1),-1.000000,mvcvRect(10,3,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,3,4,6),-1.000000,mvcvRect(11,3,2,3),2.000000,mvcvRect(9,6,2,3),2.000000,
};
float MEMORY_TYPE feature_thres_17_5[2] =
{
0.006432,0.000467,
};
int MEMORY_TYPE feature_left_17_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_5[3] =
{
0.296790,0.411046,0.558122,
};
HaarFeature MEMORY_TYPE haar_17_6[2] =
{
0,mvcvRect(0,3,6,5),-1.000000,mvcvRect(3,3,3,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,0,2,18),-1.000000,mvcvRect(2,6,2,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_6[2] =
{
0.005744,0.003202,
};
int MEMORY_TYPE feature_left_17_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_6[3] =
{
0.428731,0.426620,0.644405,
};
HaarFeature MEMORY_TYPE haar_17_7[2] =
{
0,mvcvRect(14,2,4,9),-1.000000,mvcvRect(14,5,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,18,3,2),-1.000000,mvcvRect(11,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_7[2] =
{
-0.000576,-0.003790,
};
int MEMORY_TYPE feature_left_17_7[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_7[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_7[3] =
{
0.408482,0.318192,0.523069,
};
HaarFeature MEMORY_TYPE haar_17_8[2] =
{
0,mvcvRect(2,2,4,9),-1.000000,mvcvRect(2,5,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,18,3,2),-1.000000,mvcvRect(8,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_8[2] =
{
0.004891,0.004646,
};
int MEMORY_TYPE feature_left_17_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_8[3] =
{
0.354836,0.561060,0.269385,
};
HaarFeature MEMORY_TYPE haar_17_9[2] =
{
0,mvcvRect(10,14,3,3),-1.000000,mvcvRect(10,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,12,2,6),-1.000000,mvcvRect(10,15,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_9[2] =
{
-0.006880,-0.018147,
};
int MEMORY_TYPE feature_left_17_9[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_9[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_9[3] =
{
0.623541,0.286198,0.522685,
};
HaarFeature MEMORY_TYPE haar_17_10[2] =
{
0,mvcvRect(7,5,3,6),-1.000000,mvcvRect(7,7,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,3,6,2),-1.000000,mvcvRect(3,4,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_10[2] =
{
0.000114,-0.000543,
};
int MEMORY_TYPE feature_left_17_10[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_10[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_10[3] =
{
0.325783,0.388297,0.534117,
};
HaarFeature MEMORY_TYPE haar_17_11[2] =
{
0,mvcvRect(8,4,7,3),-1.000000,mvcvRect(8,5,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,6,2,3),-1.000000,mvcvRect(13,7,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_11[2] =
{
-0.002760,-0.001973,
};
int MEMORY_TYPE feature_left_17_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_11[3] =
{
0.635397,0.588076,0.459309,
};
HaarFeature MEMORY_TYPE haar_17_12[2] =
{
0,mvcvRect(8,8,2,12),-1.000000,mvcvRect(8,12,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,8,14),-1.000000,mvcvRect(5,4,4,7),2.000000,mvcvRect(9,11,4,7),2.000000,
};
float MEMORY_TYPE feature_thres_17_12[2] =
{
0.002457,0.000194,
};
int MEMORY_TYPE feature_left_17_12[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_12[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_12[3] =
{
0.313401,0.527713,0.360411,
};
HaarFeature MEMORY_TYPE haar_17_13[2] =
{
0,mvcvRect(0,1,20,8),-1.000000,mvcvRect(10,1,10,4),2.000000,mvcvRect(0,5,10,4),2.000000,0,mvcvRect(4,0,12,2),-1.000000,mvcvRect(4,1,12,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_13[2] =
{
0.078643,0.006528,
};
int MEMORY_TYPE feature_left_17_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_13[3] =
{
0.529034,0.465448,0.604491,
};
HaarFeature MEMORY_TYPE haar_17_14[2] =
{
0,mvcvRect(0,1,20,8),-1.000000,mvcvRect(0,1,10,4),2.000000,mvcvRect(10,5,10,4),2.000000,0,mvcvRect(4,0,12,2),-1.000000,mvcvRect(4,1,12,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_14[2] =
{
-0.078717,0.005730,
};
int MEMORY_TYPE feature_left_17_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_14[3] =
{
0.254113,0.436692,0.582289,
};
HaarFeature MEMORY_TYPE haar_17_15[2] =
{
0,mvcvRect(9,5,6,3),-1.000000,mvcvRect(9,5,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,13,10,6),-1.000000,mvcvRect(8,15,10,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_15[2] =
{
0.000624,-0.085267,
};
int MEMORY_TYPE feature_left_17_15[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_15[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_15[3] =
{
0.547269,0.146161,0.518181,
};
HaarFeature MEMORY_TYPE haar_17_16[2] =
{
0,mvcvRect(5,5,6,3),-1.000000,mvcvRect(8,5,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,3,6,1),-1.000000,mvcvRect(8,3,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_16[2] =
{
0.040981,0.007714,
};
int MEMORY_TYPE feature_left_17_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_16[3] =
{
0.127014,0.483268,0.222358,
};
HaarFeature MEMORY_TYPE haar_17_17[2] =
{
0,mvcvRect(11,18,9,2),-1.000000,mvcvRect(14,18,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,11,6,7),-1.000000,mvcvRect(13,11,3,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_17[2] =
{
-0.006866,0.014560,
};
int MEMORY_TYPE feature_left_17_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_17[3] =
{
0.591893,0.476151,0.572722,
};
HaarFeature MEMORY_TYPE haar_17_18[2] =
{
0,mvcvRect(4,6,12,10),-1.000000,mvcvRect(4,6,6,5),2.000000,mvcvRect(10,11,6,5),2.000000,0,mvcvRect(8,17,3,3),-1.000000,mvcvRect(9,17,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_18[2] =
{
-0.010064,0.003627,
};
int MEMORY_TYPE feature_left_17_18[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_18[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_18[3] =
{
0.363673,0.527173,0.274053,
};
HaarFeature MEMORY_TYPE haar_17_19[2] =
{
0,mvcvRect(11,18,9,2),-1.000000,mvcvRect(14,18,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,11,6,8),-1.000000,mvcvRect(13,11,3,8),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_19[2] =
{
-0.002342,-0.024686,
};
int MEMORY_TYPE feature_left_17_19[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_19[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_19[3] =
{
0.549778,0.605990,0.496031,
};
HaarFeature MEMORY_TYPE haar_17_20[2] =
{
0,mvcvRect(4,16,2,2),-1.000000,mvcvRect(4,17,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,15,4,4),-1.000000,mvcvRect(7,17,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_20[2] =
{
0.000195,0.000317,
};
int MEMORY_TYPE feature_left_17_20[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_20[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_20[3] =
{
0.376947,0.406236,0.566822,
};
HaarFeature MEMORY_TYPE haar_17_21[2] =
{
0,mvcvRect(12,4,3,3),-1.000000,mvcvRect(12,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,6,2,3),-1.000000,mvcvRect(13,7,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_21[2] =
{
0.002079,0.001798,
};
int MEMORY_TYPE feature_left_17_21[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_21[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_21[3] =
{
0.461866,0.486751,0.651845,
};
HaarFeature MEMORY_TYPE haar_17_22[2] =
{
0,mvcvRect(5,11,6,1),-1.000000,mvcvRect(7,11,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,10,3,1),-1.000000,mvcvRect(8,10,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_22[2] =
{
-0.000223,0.000326,
};
int MEMORY_TYPE feature_left_17_22[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_22[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_22[3] =
{
0.567760,0.371073,0.567661,
};
HaarFeature MEMORY_TYPE haar_17_23[2] =
{
0,mvcvRect(0,12,20,4),-1.000000,mvcvRect(0,14,20,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,2,3,2),-1.000000,mvcvRect(10,3,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_23[2] =
{
-0.066793,-0.001487,
};
int MEMORY_TYPE feature_left_17_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_23[3] =
{
0.251152,0.388675,0.526225,
};
HaarFeature MEMORY_TYPE haar_17_24[2] =
{
0,mvcvRect(5,4,3,3),-1.000000,mvcvRect(5,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,4,3),-1.000000,mvcvRect(5,6,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_24[2] =
{
-0.005045,-0.004830,
};
int MEMORY_TYPE feature_left_17_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_24[3] =
{
0.655747,0.593411,0.428592,
};
HaarFeature MEMORY_TYPE haar_17_25[2] =
{
0,mvcvRect(8,8,4,3),-1.000000,mvcvRect(8,9,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,4,2,12),-1.000000,mvcvRect(10,8,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_25[2] =
{
-0.001072,0.008790,
};
int MEMORY_TYPE feature_left_17_25[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_25[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_25[3] =
{
0.542606,0.535130,0.483428,
};
HaarFeature MEMORY_TYPE haar_17_26[2] =
{
0,mvcvRect(0,3,4,3),-1.000000,mvcvRect(0,4,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,3,2,3),-1.000000,mvcvRect(1,4,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_26[2] =
{
-0.007175,0.001125,
};
int MEMORY_TYPE feature_left_17_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_26[3] =
{
0.206717,0.511225,0.346871,
};
HaarFeature MEMORY_TYPE haar_17_27[2] =
{
0,mvcvRect(16,1,4,11),-1.000000,mvcvRect(16,1,2,11),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(18,2,2,16),-1.000000,mvcvRect(19,2,1,8),2.000000,mvcvRect(18,10,1,8),2.000000,
};
float MEMORY_TYPE feature_thres_17_27[2] =
{
0.010635,-0.011763,
};
int MEMORY_TYPE feature_left_17_27[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_27[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_27[3] =
{
0.447901,0.625390,0.496899,
};
HaarFeature MEMORY_TYPE haar_17_28[2] =
{
0,mvcvRect(1,8,6,12),-1.000000,mvcvRect(3,8,2,12),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,2,6,2),-1.000000,mvcvRect(7,2,3,1),2.000000,mvcvRect(10,3,3,1),2.000000,
};
float MEMORY_TYPE feature_thres_17_28[2] =
{
0.092324,0.001899,
};
int MEMORY_TYPE feature_left_17_28[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_28[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_28[3] =
{
0.203130,0.561872,0.404657,
};
HaarFeature MEMORY_TYPE haar_17_29[2] =
{
0,mvcvRect(12,4,8,2),-1.000000,mvcvRect(16,4,4,1),2.000000,mvcvRect(12,5,4,1),2.000000,0,mvcvRect(10,6,6,2),-1.000000,mvcvRect(12,6,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_29[2] =
{
-0.010510,-0.000745,
};
int MEMORY_TYPE feature_left_17_29[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_29[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_29[3] =
{
0.494326,0.561343,0.384533,
};
HaarFeature MEMORY_TYPE haar_17_30[2] =
{
0,mvcvRect(0,4,8,2),-1.000000,mvcvRect(0,4,4,1),2.000000,mvcvRect(4,5,4,1),2.000000,0,mvcvRect(1,3,3,5),-1.000000,mvcvRect(2,3,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_30[2] =
{
0.008004,0.005811,
};
int MEMORY_TYPE feature_left_17_30[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_30[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_30[3] =
{
0.775984,0.462473,0.628628,
};
HaarFeature MEMORY_TYPE haar_17_31[2] =
{
0,mvcvRect(16,3,4,6),-1.000000,mvcvRect(16,5,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,6,4,3),-1.000000,mvcvRect(8,7,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_31[2] =
{
-0.027919,0.002174,
};
int MEMORY_TYPE feature_left_17_31[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_31[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_31[3] =
{
0.240931,0.534550,0.350796,
};
HaarFeature MEMORY_TYPE haar_17_32[2] =
{
0,mvcvRect(8,14,1,3),-1.000000,mvcvRect(8,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,11,1,2),-1.000000,mvcvRect(4,12,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_32[2] =
{
-0.004064,0.000600,
};
int MEMORY_TYPE feature_left_17_32[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_32[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_32[3] =
{
0.664710,0.499851,0.302217,
};
HaarFeature MEMORY_TYPE haar_17_33[2] =
{
0,mvcvRect(8,14,6,3),-1.000000,mvcvRect(8,15,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,15,7,3),-1.000000,mvcvRect(7,16,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_33[2] =
{
0.001921,-0.013861,
};
int MEMORY_TYPE feature_left_17_33[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_33[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_33[3] =
{
0.591915,0.635177,0.499331,
};
HaarFeature MEMORY_TYPE haar_17_34[2] =
{
0,mvcvRect(9,12,2,8),-1.000000,mvcvRect(9,16,2,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,6,6,2),-1.000000,mvcvRect(6,6,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_34[2] =
{
0.023007,-0.001386,
};
int MEMORY_TYPE feature_left_17_34[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_34[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_34[3] =
{
0.190234,0.525337,0.398586,
};
HaarFeature MEMORY_TYPE haar_17_35[2] =
{
0,mvcvRect(12,7,4,2),-1.000000,mvcvRect(12,8,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,3,13,10),-1.000000,mvcvRect(5,8,13,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_35[2] =
{
0.001264,-0.014675,
};
int MEMORY_TYPE feature_left_17_35[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_35[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_35[3] =
{
0.466610,0.382316,0.532663,
};
HaarFeature MEMORY_TYPE haar_17_36[2] =
{
0,mvcvRect(4,7,4,2),-1.000000,mvcvRect(4,8,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,8,16,2),-1.000000,mvcvRect(0,8,8,1),2.000000,mvcvRect(8,9,8,1),2.000000,
};
float MEMORY_TYPE feature_thres_17_36[2] =
{
-0.002954,-0.001719,
};
int MEMORY_TYPE feature_left_17_36[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_36[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_36[3] =
{
0.706366,0.381346,0.524674,
};
HaarFeature MEMORY_TYPE haar_17_37[2] =
{
0,mvcvRect(11,8,2,5),-1.000000,mvcvRect(11,8,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,0,6,13),-1.000000,mvcvRect(10,0,3,13),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_37[2] =
{
-0.000425,-0.000852,
};
int MEMORY_TYPE feature_left_17_37[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_37[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_37[3] =
{
0.479164,0.449122,0.537090,
};
HaarFeature MEMORY_TYPE haar_17_38[2] =
{
0,mvcvRect(1,6,4,2),-1.000000,mvcvRect(1,7,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,3,2,1),-1.000000,mvcvRect(5,3,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_38[2] =
{
0.008903,0.000015,
};
int MEMORY_TYPE feature_left_17_38[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_38[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_38[3] =
{
0.207647,0.444764,0.566716,
};
HaarFeature MEMORY_TYPE haar_17_39[2] =
{
0,mvcvRect(11,8,2,5),-1.000000,mvcvRect(11,8,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,10,4,8),-1.000000,mvcvRect(12,10,2,8),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_39[2] =
{
-0.000471,0.000431,
};
int MEMORY_TYPE feature_left_17_39[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_39[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_39[3] =
{
0.546507,0.549326,0.458071,
};
HaarFeature MEMORY_TYPE haar_17_40[2] =
{
0,mvcvRect(7,8,2,5),-1.000000,mvcvRect(8,8,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,10,4,8),-1.000000,mvcvRect(6,10,2,8),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_40[2] =
{
-0.000639,-0.000074,
};
int MEMORY_TYPE feature_left_17_40[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_40[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_40[3] =
{
0.550157,0.508579,0.330570,
};
HaarFeature MEMORY_TYPE haar_17_41[2] =
{
0,mvcvRect(6,7,9,12),-1.000000,mvcvRect(9,7,3,12),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,13,2,3),-1.000000,mvcvRect(11,13,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_41[2] =
{
-0.008899,-0.010253,
};
int MEMORY_TYPE feature_left_17_41[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_41[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_41[3] =
{
0.427647,0.112322,0.515272,
};
HaarFeature MEMORY_TYPE haar_17_42[2] =
{
0,mvcvRect(7,10,6,10),-1.000000,mvcvRect(10,10,3,10),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,11,4,8),-1.000000,mvcvRect(8,11,2,4),2.000000,mvcvRect(10,15,2,4),2.000000,
};
float MEMORY_TYPE feature_thres_17_42[2] =
{
-0.059637,0.021707,
};
int MEMORY_TYPE feature_left_17_42[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_42[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_42[3] =
{
0.738677,0.499629,0.133941,
};
HaarFeature MEMORY_TYPE haar_17_43[2] =
{
0,mvcvRect(16,1,4,11),-1.000000,mvcvRect(16,1,2,11),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(18,2,2,4),-1.000000,mvcvRect(18,2,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_43[2] =
{
0.009911,-0.010998,
};
int MEMORY_TYPE feature_left_17_43[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_43[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_43[3] =
{
0.467901,0.692866,0.501207,
};
HaarFeature MEMORY_TYPE haar_17_44[2] =
{
0,mvcvRect(5,6,6,2),-1.000000,mvcvRect(5,6,3,1),2.000000,mvcvRect(8,7,3,1),2.000000,0,mvcvRect(5,4,1,3),-1.000000,mvcvRect(5,5,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_44[2] =
{
0.000746,0.000295,
};
int MEMORY_TYPE feature_left_17_44[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_44[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_44[3] =
{
0.583358,0.382639,0.556635,
};
HaarFeature MEMORY_TYPE haar_17_45[2] =
{
0,mvcvRect(11,1,4,14),-1.000000,mvcvRect(11,1,2,14),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,2,12,3),-1.000000,mvcvRect(8,2,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_45[2] =
{
0.050054,-0.007233,
};
int MEMORY_TYPE feature_left_17_45[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_45[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_45[3] =
{
0.300272,0.590804,0.500087,
};
HaarFeature MEMORY_TYPE haar_17_46[2] =
{
0,mvcvRect(5,1,4,14),-1.000000,mvcvRect(7,1,2,14),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,3,6,2),-1.000000,mvcvRect(9,3,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_46[2] =
{
-0.002686,-0.001020,
};
int MEMORY_TYPE feature_left_17_46[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_46[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_46[3] =
{
0.397503,0.369769,0.575619,
};
HaarFeature MEMORY_TYPE haar_17_47[2] =
{
0,mvcvRect(2,0,18,4),-1.000000,mvcvRect(8,0,6,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,5,2,10),-1.000000,mvcvRect(9,10,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_47[2] =
{
-0.020205,0.002134,
};
int MEMORY_TYPE feature_left_17_47[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_47[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_47[3] =
{
0.637527,0.536327,0.443317,
};
HaarFeature MEMORY_TYPE haar_17_48[2] =
{
0,mvcvRect(8,6,3,4),-1.000000,mvcvRect(9,6,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,9,11),-1.000000,mvcvRect(8,5,3,11),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_48[2] =
{
-0.001835,-0.005949,
};
int MEMORY_TYPE feature_left_17_48[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_48[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_48[3] =
{
0.582900,0.268067,0.464289,
};
HaarFeature MEMORY_TYPE haar_17_49[2] =
{
0,mvcvRect(10,6,3,5),-1.000000,mvcvRect(11,6,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,9,6,5),-1.000000,mvcvRect(8,9,3,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_49[2] =
{
-0.000230,0.005058,
};
int MEMORY_TYPE feature_left_17_49[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_49[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_49[3] =
{
0.547532,0.532083,0.464649,
};
HaarFeature MEMORY_TYPE haar_17_50[2] =
{
0,mvcvRect(7,6,3,5),-1.000000,mvcvRect(8,6,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,10,6,3),-1.000000,mvcvRect(9,10,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_50[2] =
{
-0.000520,-0.000686,
};
int MEMORY_TYPE feature_left_17_50[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_50[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_50[3] =
{
0.523274,0.493509,0.310312,
};
HaarFeature MEMORY_TYPE haar_17_51[2] =
{
0,mvcvRect(10,0,3,7),-1.000000,mvcvRect(11,0,1,7),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,3,20,12),-1.000000,mvcvRect(0,9,20,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_51[2] =
{
-0.007494,-0.015683,
};
int MEMORY_TYPE feature_left_17_51[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_51[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_51[3] =
{
0.288305,0.364031,0.536875,
};
HaarFeature MEMORY_TYPE haar_17_52[2] =
{
0,mvcvRect(9,7,2,2),-1.000000,mvcvRect(10,7,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,9,4,1),-1.000000,mvcvRect(7,9,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_52[2] =
{
-0.003265,0.000385,
};
int MEMORY_TYPE feature_left_17_52[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_52[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_52[3] =
{
0.646863,0.525966,0.383143,
};
HaarFeature MEMORY_TYPE haar_17_53[2] =
{
0,mvcvRect(13,13,3,2),-1.000000,mvcvRect(13,14,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,9,4,6),-1.000000,mvcvRect(16,9,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_53[2] =
{
0.004449,0.023118,
};
int MEMORY_TYPE feature_left_17_53[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_53[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_53[3] =
{
0.208682,0.497853,0.596126,
};
HaarFeature MEMORY_TYPE haar_17_54[2] =
{
0,mvcvRect(7,15,6,3),-1.000000,mvcvRect(7,16,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,16,7,3),-1.000000,mvcvRect(6,17,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_54[2] =
{
0.002084,0.001151,
};
int MEMORY_TYPE feature_left_17_54[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_54[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_54[3] =
{
0.574642,0.358685,0.536347,
};
HaarFeature MEMORY_TYPE haar_17_55[2] =
{
0,mvcvRect(11,14,9,6),-1.000000,mvcvRect(11,16,9,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(19,14,1,3),-1.000000,mvcvRect(19,15,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_55[2] =
{
0.036105,0.000363,
};
int MEMORY_TYPE feature_left_17_55[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_55[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_55[3] =
{
0.283314,0.547772,0.411053,
};
HaarFeature MEMORY_TYPE haar_17_56[2] =
{
0,mvcvRect(0,9,6,6),-1.000000,mvcvRect(3,9,3,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,19,9,1),-1.000000,mvcvRect(3,19,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_56[2] =
{
-0.003464,-0.002880,
};
int MEMORY_TYPE feature_left_17_56[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_56[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_56[3] =
{
0.599039,0.572525,0.414951,
};
HaarFeature MEMORY_TYPE haar_17_57[2] =
{
0,mvcvRect(11,14,9,6),-1.000000,mvcvRect(11,16,9,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,12,6,6),-1.000000,mvcvRect(12,14,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_57[2] =
{
-0.008112,0.004593,
};
int MEMORY_TYPE feature_left_17_57[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_57[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_57[3] =
{
0.539635,0.537970,0.389130,
};
HaarFeature MEMORY_TYPE haar_17_58[2] =
{
0,mvcvRect(1,14,8,6),-1.000000,mvcvRect(1,16,8,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,1,3,2),-1.000000,mvcvRect(9,1,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_58[2] =
{
0.007001,0.000802,
};
int MEMORY_TYPE feature_left_17_58[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_58[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_58[3] =
{
0.371467,0.552957,0.375580,
};
HaarFeature MEMORY_TYPE haar_17_59[2] =
{
0,mvcvRect(18,2,2,4),-1.000000,mvcvRect(18,2,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,0,6,3),-1.000000,mvcvRect(16,0,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_59[2] =
{
-0.008665,-0.002732,
};
int MEMORY_TYPE feature_left_17_59[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_59[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_59[3] =
{
0.502577,0.585032,0.461757,
};
HaarFeature MEMORY_TYPE haar_17_60[2] =
{
0,mvcvRect(0,2,2,4),-1.000000,mvcvRect(1,2,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,6,3),-1.000000,mvcvRect(2,0,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_60[2] =
{
0.001330,-0.004265,
};
int MEMORY_TYPE feature_left_17_60[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_60[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_60[3] =
{
0.593770,0.564537,0.393762,
};
HaarFeature MEMORY_TYPE haar_17_61[2] =
{
0,mvcvRect(9,0,3,2),-1.000000,mvcvRect(10,0,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,1,2,2),-1.000000,mvcvRect(12,1,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_61[2] =
{
0.006325,-0.003075,
};
int MEMORY_TYPE feature_left_17_61[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_61[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_61[3] =
{
0.518211,0.300742,0.519640,
};
HaarFeature MEMORY_TYPE haar_17_62[2] =
{
0,mvcvRect(8,0,3,2),-1.000000,mvcvRect(9,0,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,1,2,2),-1.000000,mvcvRect(7,1,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_62[2] =
{
-0.000736,0.000030,
};
int MEMORY_TYPE feature_left_17_62[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_62[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_62[3] =
{
0.369758,0.432759,0.571581,
};
HaarFeature MEMORY_TYPE haar_17_63[2] =
{
0,mvcvRect(10,8,2,3),-1.000000,mvcvRect(10,9,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,15,6,2),-1.000000,mvcvRect(13,16,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_63[2] =
{
-0.003872,0.000629,
};
int MEMORY_TYPE feature_left_17_63[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_63[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_63[3] =
{
0.347371,0.543826,0.445391,
};
HaarFeature MEMORY_TYPE haar_17_64[2] =
{
0,mvcvRect(8,12,2,2),-1.000000,mvcvRect(8,12,1,1),2.000000,mvcvRect(9,13,1,1),2.000000,0,mvcvRect(8,15,3,5),-1.000000,mvcvRect(9,15,1,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_64[2] =
{
0.001341,-0.008368,
};
int MEMORY_TYPE feature_left_17_64[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_64[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_64[3] =
{
0.651171,0.144330,0.488820,
};
HaarFeature MEMORY_TYPE haar_17_65[2] =
{
0,mvcvRect(8,6,4,12),-1.000000,mvcvRect(8,12,4,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,6,7,8),-1.000000,mvcvRect(7,10,7,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_65[2] =
{
0.000933,-0.001075,
};
int MEMORY_TYPE feature_left_17_65[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_65[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_65[3] =
{
0.395111,0.391027,0.534950,
};
HaarFeature MEMORY_TYPE haar_17_66[2] =
{
0,mvcvRect(0,11,8,2),-1.000000,mvcvRect(0,12,8,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,11,2,2),-1.000000,mvcvRect(8,11,1,1),2.000000,mvcvRect(9,12,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_17_66[2] =
{
-0.018610,0.001365,
};
int MEMORY_TYPE feature_left_17_66[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_66[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_66[3] =
{
0.127574,0.503829,0.695130,
};
HaarFeature MEMORY_TYPE haar_17_67[2] =
{
0,mvcvRect(7,7,12,1),-1.000000,mvcvRect(11,7,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,8,3,2),-1.000000,mvcvRect(11,8,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_67[2] =
{
0.007374,0.008416,
};
int MEMORY_TYPE feature_left_17_67[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_67[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_67[3] =
{
0.525344,0.501124,0.731133,
};
HaarFeature MEMORY_TYPE haar_17_68[2] =
{
0,mvcvRect(1,7,12,1),-1.000000,mvcvRect(5,7,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,5,8,2),-1.000000,mvcvRect(6,5,4,1),2.000000,mvcvRect(10,6,4,1),2.000000,
};
float MEMORY_TYPE feature_thres_17_68[2] =
{
0.005141,0.004585,
};
int MEMORY_TYPE feature_left_17_68[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_68[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_68[3] =
{
0.495354,0.253556,0.646244,
};
HaarFeature MEMORY_TYPE haar_17_69[2] =
{
0,mvcvRect(9,10,3,10),-1.000000,mvcvRect(10,10,1,10),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,0,2,4),-1.000000,mvcvRect(16,0,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_69[2] =
{
0.028565,0.000440,
};
int MEMORY_TYPE feature_left_17_69[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_69[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_69[3] =
{
0.233072,0.470224,0.554455,
};
HaarFeature MEMORY_TYPE haar_17_70[2] =
{
0,mvcvRect(8,10,3,10),-1.000000,mvcvRect(9,10,1,10),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,10,2,3),-1.000000,mvcvRect(9,11,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_70[2] =
{
0.031459,0.005601,
};
int MEMORY_TYPE feature_left_17_70[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_70[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_70[3] =
{
0.033690,0.478712,0.633835,
};
HaarFeature MEMORY_TYPE haar_17_71[2] =
{
0,mvcvRect(8,9,4,2),-1.000000,mvcvRect(10,9,2,1),2.000000,mvcvRect(8,10,2,1),2.000000,0,mvcvRect(12,14,7,6),-1.000000,mvcvRect(12,16,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_71[2] =
{
0.000718,-0.005530,
};
int MEMORY_TYPE feature_left_17_71[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_71[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_71[3] =
{
0.543149,0.410583,0.540399,
};
HaarFeature MEMORY_TYPE haar_17_72[2] =
{
0,mvcvRect(6,1,3,1),-1.000000,mvcvRect(7,1,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,0,2,4),-1.000000,mvcvRect(3,0,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_72[2] =
{
0.001413,0.000255,
};
int MEMORY_TYPE feature_left_17_72[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_72[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_72[3] =
{
0.310554,0.425447,0.544715,
};
HaarFeature MEMORY_TYPE haar_17_73[2] =
{
0,mvcvRect(11,11,2,2),-1.000000,mvcvRect(12,11,1,1),2.000000,mvcvRect(11,12,1,1),2.000000,0,mvcvRect(12,12,6,6),-1.000000,mvcvRect(12,14,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_73[2] =
{
0.000320,0.005041,
};
int MEMORY_TYPE feature_left_17_73[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_73[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_73[3] =
{
0.611836,0.529004,0.422479,
};
HaarFeature MEMORY_TYPE haar_17_74[2] =
{
0,mvcvRect(1,0,6,10),-1.000000,mvcvRect(1,0,3,5),2.000000,mvcvRect(4,5,3,5),2.000000,0,mvcvRect(3,0,2,9),-1.000000,mvcvRect(3,3,2,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_74[2] =
{
0.007762,0.002937,
};
int MEMORY_TYPE feature_left_17_74[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_74[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_74[3] =
{
0.431535,0.662926,0.302896,
};
HaarFeature MEMORY_TYPE haar_17_75[2] =
{
0,mvcvRect(14,13,3,2),-1.000000,mvcvRect(14,14,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,2,3,2),-1.000000,mvcvRect(15,3,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_75[2] =
{
-0.001650,-0.005883,
};
int MEMORY_TYPE feature_left_17_75[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_75[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_75[3] =
{
0.549185,0.318855,0.518429,
};
HaarFeature MEMORY_TYPE haar_17_76[2] =
{
0,mvcvRect(2,13,5,2),-1.000000,mvcvRect(2,14,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,4,12,10),-1.000000,mvcvRect(3,4,6,5),2.000000,mvcvRect(9,9,6,5),2.000000,
};
float MEMORY_TYPE feature_thres_17_76[2] =
{
0.000875,-0.015309,
};
int MEMORY_TYPE feature_left_17_76[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_76[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_76[3] =
{
0.332883,0.392361,0.523514,
};
HaarFeature MEMORY_TYPE haar_17_77[2] =
{
0,mvcvRect(5,1,14,6),-1.000000,mvcvRect(5,3,14,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,3,3,2),-1.000000,mvcvRect(15,4,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_77[2] =
{
0.032292,-0.000438,
};
int MEMORY_TYPE feature_left_17_77[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_77[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_77[3] =
{
0.597765,0.454169,0.536943,
};
HaarFeature MEMORY_TYPE haar_17_78[2] =
{
0,mvcvRect(7,11,2,2),-1.000000,mvcvRect(7,11,1,1),2.000000,mvcvRect(8,12,1,1),2.000000,0,mvcvRect(2,14,6,6),-1.000000,mvcvRect(2,16,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_78[2] =
{
0.001543,-0.002473,
};
int MEMORY_TYPE feature_left_17_78[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_78[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_78[3] =
{
0.631814,0.349063,0.475902,
};
HaarFeature MEMORY_TYPE haar_17_79[2] =
{
0,mvcvRect(6,13,8,3),-1.000000,mvcvRect(6,14,8,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,19,18,1),-1.000000,mvcvRect(7,19,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_79[2] =
{
0.002099,-0.005754,
};
int MEMORY_TYPE feature_left_17_79[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_79[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_79[3] =
{
0.588720,0.596133,0.484198,
};
HaarFeature MEMORY_TYPE haar_17_80[2] =
{
0,mvcvRect(8,12,1,6),-1.000000,mvcvRect(8,15,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,14,15),-1.000000,mvcvRect(0,5,14,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_80[2] =
{
-0.010233,0.225545,
};
int MEMORY_TYPE feature_left_17_80[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_80[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_80[3] =
{
0.170540,0.477938,0.097880,
};
HaarFeature MEMORY_TYPE haar_17_81[2] =
{
0,mvcvRect(3,0,16,8),-1.000000,mvcvRect(3,4,16,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,1,8,12),-1.000000,mvcvRect(6,7,8,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_81[2] =
{
0.029667,-0.002852,
};
int MEMORY_TYPE feature_left_17_81[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_81[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_81[3] =
{
0.582222,0.545963,0.461007,
};
HaarFeature MEMORY_TYPE haar_17_82[2] =
{
0,mvcvRect(5,3,3,3),-1.000000,mvcvRect(6,3,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,1,3,4),-1.000000,mvcvRect(6,1,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_82[2] =
{
0.000975,0.000014,
};
int MEMORY_TYPE feature_left_17_82[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_82[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_82[3] =
{
0.367032,0.430239,0.569171,
};
HaarFeature MEMORY_TYPE haar_17_83[2] =
{
0,mvcvRect(15,14,4,6),-1.000000,mvcvRect(17,14,2,3),2.000000,mvcvRect(15,17,2,3),2.000000,0,mvcvRect(12,11,6,8),-1.000000,mvcvRect(15,11,3,4),2.000000,mvcvRect(12,15,3,4),2.000000,
};
float MEMORY_TYPE feature_thres_17_83[2] =
{
-0.017579,-0.052382,
};
int MEMORY_TYPE feature_left_17_83[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_83[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_83[3] =
{
0.691732,0.711004,0.506015,
};
HaarFeature MEMORY_TYPE haar_17_84[2] =
{
0,mvcvRect(8,7,2,4),-1.000000,mvcvRect(9,7,1,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,11,3,1),-1.000000,mvcvRect(7,11,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_84[2] =
{
-0.011242,-0.003673,
};
int MEMORY_TYPE feature_left_17_84[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_84[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_84[3] =
{
0.876919,0.651919,0.454607,
};
HaarFeature MEMORY_TYPE haar_17_85[2] =
{
0,mvcvRect(12,3,2,14),-1.000000,mvcvRect(12,3,1,14),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,11,6,2),-1.000000,mvcvRect(15,11,3,1),2.000000,mvcvRect(12,12,3,1),2.000000,
};
float MEMORY_TYPE feature_thres_17_85[2] =
{
0.003508,0.006168,
};
int MEMORY_TYPE feature_left_17_85[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_85[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_85[3] =
{
0.532987,0.522046,0.295352,
};
HaarFeature MEMORY_TYPE haar_17_86[2] =
{
0,mvcvRect(0,2,5,2),-1.000000,mvcvRect(0,3,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,0,15,1),-1.000000,mvcvRect(5,0,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_86[2] =
{
-0.000970,-0.010957,
};
int MEMORY_TYPE feature_left_17_86[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_86[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_86[3] =
{
0.504863,0.583736,0.302009,
};
HaarFeature MEMORY_TYPE haar_17_87[2] =
{
0,mvcvRect(12,11,6,2),-1.000000,mvcvRect(15,11,3,1),2.000000,mvcvRect(12,12,3,1),2.000000,0,mvcvRect(10,5,2,2),-1.000000,mvcvRect(10,5,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_87[2] =
{
-0.008327,0.000030,
};
int MEMORY_TYPE feature_left_17_87[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_87[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_87[3] =
{
0.315806,0.438639,0.544321,
};
HaarFeature MEMORY_TYPE haar_17_88[2] =
{
0,mvcvRect(9,7,2,2),-1.000000,mvcvRect(10,7,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,0,2,10),-1.000000,mvcvRect(9,0,1,5),2.000000,mvcvRect(10,5,1,5),2.000000,
};
float MEMORY_TYPE feature_thres_17_88[2] =
{
0.000282,-0.000814,
};
int MEMORY_TYPE feature_left_17_88[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_88[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_88[3] =
{
0.562540,0.528120,0.340141,
};
HaarFeature MEMORY_TYPE haar_17_89[2] =
{
0,mvcvRect(18,14,2,2),-1.000000,mvcvRect(18,15,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,11,4,9),-1.000000,mvcvRect(13,14,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_89[2] =
{
0.001801,-0.006994,
};
int MEMORY_TYPE feature_left_17_89[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_89[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_89[3] =
{
0.347166,0.448170,0.538577,
};
HaarFeature MEMORY_TYPE haar_17_90[2] =
{
0,mvcvRect(8,13,2,2),-1.000000,mvcvRect(8,13,1,1),2.000000,mvcvRect(9,14,1,1),2.000000,0,mvcvRect(7,8,4,3),-1.000000,mvcvRect(7,9,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_90[2] =
{
0.000046,-0.000732,
};
int MEMORY_TYPE feature_left_17_90[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_90[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_90[3] =
{
0.449251,0.416731,0.602110,
};
HaarFeature MEMORY_TYPE haar_17_91[2] =
{
0,mvcvRect(8,9,4,2),-1.000000,mvcvRect(8,10,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,12,4,2),-1.000000,mvcvRect(13,13,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_91[2] =
{
-0.000300,-0.000029,
};
int MEMORY_TYPE feature_left_17_91[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_91[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_91[3] =
{
0.414843,0.559209,0.407321,
};
HaarFeature MEMORY_TYPE haar_17_92[2] =
{
0,mvcvRect(6,14,2,2),-1.000000,mvcvRect(6,14,1,1),2.000000,mvcvRect(7,15,1,1),2.000000,0,mvcvRect(0,14,2,2),-1.000000,mvcvRect(0,15,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_92[2] =
{
-0.000597,0.000148,
};
int MEMORY_TYPE feature_left_17_92[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_92[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_92[3] =
{
0.608891,0.529831,0.376195,
};
HaarFeature MEMORY_TYPE haar_17_93[2] =
{
0,mvcvRect(7,13,6,3),-1.000000,mvcvRect(7,14,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,9,10,6),-1.000000,mvcvRect(7,11,10,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_93[2] =
{
-0.002944,0.137412,
};
int MEMORY_TYPE feature_left_17_93[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_17_93[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_17_93[3] =
{
0.471608,0.510134,0.046747,
};
HaarFeature MEMORY_TYPE haar_17_94[2] =
{
0,mvcvRect(2,9,12,4),-1.000000,mvcvRect(6,9,4,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,9,6,11),-1.000000,mvcvRect(10,9,3,11),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_94[2] =
{
-0.088414,0.070610,
};
int MEMORY_TYPE feature_left_17_94[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_94[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_94[3] =
{
0.118187,0.511906,0.777844,
};
HaarFeature MEMORY_TYPE haar_17_95[2] =
{
0,mvcvRect(9,7,2,3),-1.000000,mvcvRect(9,8,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,14,4,3),-1.000000,mvcvRect(9,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_95[2] =
{
-0.007719,0.015115,
};
int MEMORY_TYPE feature_left_17_95[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_95[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_95[3] =
{
0.187413,0.498003,0.700582,
};
HaarFeature MEMORY_TYPE haar_17_96[2] =
{
0,mvcvRect(2,3,3,17),-1.000000,mvcvRect(3,3,1,17),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,11,6,3),-1.000000,mvcvRect(0,12,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_17_96[2] =
{
0.001067,0.000705,
};
int MEMORY_TYPE feature_left_17_96[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_17_96[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_17_96[3] =
{
0.448224,0.626575,0.440266,
};
HaarClassifier MEMORY_TYPE haar_feature_class_17[97] =
{
{2,haar_17_0,feature_thres_17_0,feature_left_17_0,feature_right_17_0,feature_alpha_17_0},{2,haar_17_1,feature_thres_17_1,feature_left_17_1,feature_right_17_1,feature_alpha_17_1},{2,haar_17_2,feature_thres_17_2,feature_left_17_2,feature_right_17_2,feature_alpha_17_2},{2,haar_17_3,feature_thres_17_3,feature_left_17_3,feature_right_17_3,feature_alpha_17_3},{2,haar_17_4,feature_thres_17_4,feature_left_17_4,feature_right_17_4,feature_alpha_17_4},{2,haar_17_5,feature_thres_17_5,feature_left_17_5,feature_right_17_5,feature_alpha_17_5},{2,haar_17_6,feature_thres_17_6,feature_left_17_6,feature_right_17_6,feature_alpha_17_6},{2,haar_17_7,feature_thres_17_7,feature_left_17_7,feature_right_17_7,feature_alpha_17_7},{2,haar_17_8,feature_thres_17_8,feature_left_17_8,feature_right_17_8,feature_alpha_17_8},{2,haar_17_9,feature_thres_17_9,feature_left_17_9,feature_right_17_9,feature_alpha_17_9},{2,haar_17_10,feature_thres_17_10,feature_left_17_10,feature_right_17_10,feature_alpha_17_10},{2,haar_17_11,feature_thres_17_11,feature_left_17_11,feature_right_17_11,feature_alpha_17_11},{2,haar_17_12,feature_thres_17_12,feature_left_17_12,feature_right_17_12,feature_alpha_17_12},{2,haar_17_13,feature_thres_17_13,feature_left_17_13,feature_right_17_13,feature_alpha_17_13},{2,haar_17_14,feature_thres_17_14,feature_left_17_14,feature_right_17_14,feature_alpha_17_14},{2,haar_17_15,feature_thres_17_15,feature_left_17_15,feature_right_17_15,feature_alpha_17_15},{2,haar_17_16,feature_thres_17_16,feature_left_17_16,feature_right_17_16,feature_alpha_17_16},{2,haar_17_17,feature_thres_17_17,feature_left_17_17,feature_right_17_17,feature_alpha_17_17},{2,haar_17_18,feature_thres_17_18,feature_left_17_18,feature_right_17_18,feature_alpha_17_18},{2,haar_17_19,feature_thres_17_19,feature_left_17_19,feature_right_17_19,feature_alpha_17_19},{2,haar_17_20,feature_thres_17_20,feature_left_17_20,feature_right_17_20,feature_alpha_17_20},{2,haar_17_21,feature_thres_17_21,feature_left_17_21,feature_right_17_21,feature_alpha_17_21},{2,haar_17_22,feature_thres_17_22,feature_left_17_22,feature_right_17_22,feature_alpha_17_22},{2,haar_17_23,feature_thres_17_23,feature_left_17_23,feature_right_17_23,feature_alpha_17_23},{2,haar_17_24,feature_thres_17_24,feature_left_17_24,feature_right_17_24,feature_alpha_17_24},{2,haar_17_25,feature_thres_17_25,feature_left_17_25,feature_right_17_25,feature_alpha_17_25},{2,haar_17_26,feature_thres_17_26,feature_left_17_26,feature_right_17_26,feature_alpha_17_26},{2,haar_17_27,feature_thres_17_27,feature_left_17_27,feature_right_17_27,feature_alpha_17_27},{2,haar_17_28,feature_thres_17_28,feature_left_17_28,feature_right_17_28,feature_alpha_17_28},{2,haar_17_29,feature_thres_17_29,feature_left_17_29,feature_right_17_29,feature_alpha_17_29},{2,haar_17_30,feature_thres_17_30,feature_left_17_30,feature_right_17_30,feature_alpha_17_30},{2,haar_17_31,feature_thres_17_31,feature_left_17_31,feature_right_17_31,feature_alpha_17_31},{2,haar_17_32,feature_thres_17_32,feature_left_17_32,feature_right_17_32,feature_alpha_17_32},{2,haar_17_33,feature_thres_17_33,feature_left_17_33,feature_right_17_33,feature_alpha_17_33},{2,haar_17_34,feature_thres_17_34,feature_left_17_34,feature_right_17_34,feature_alpha_17_34},{2,haar_17_35,feature_thres_17_35,feature_left_17_35,feature_right_17_35,feature_alpha_17_35},{2,haar_17_36,feature_thres_17_36,feature_left_17_36,feature_right_17_36,feature_alpha_17_36},{2,haar_17_37,feature_thres_17_37,feature_left_17_37,feature_right_17_37,feature_alpha_17_37},{2,haar_17_38,feature_thres_17_38,feature_left_17_38,feature_right_17_38,feature_alpha_17_38},{2,haar_17_39,feature_thres_17_39,feature_left_17_39,feature_right_17_39,feature_alpha_17_39},{2,haar_17_40,feature_thres_17_40,feature_left_17_40,feature_right_17_40,feature_alpha_17_40},{2,haar_17_41,feature_thres_17_41,feature_left_17_41,feature_right_17_41,feature_alpha_17_41},{2,haar_17_42,feature_thres_17_42,feature_left_17_42,feature_right_17_42,feature_alpha_17_42},{2,haar_17_43,feature_thres_17_43,feature_left_17_43,feature_right_17_43,feature_alpha_17_43},{2,haar_17_44,feature_thres_17_44,feature_left_17_44,feature_right_17_44,feature_alpha_17_44},{2,haar_17_45,feature_thres_17_45,feature_left_17_45,feature_right_17_45,feature_alpha_17_45},{2,haar_17_46,feature_thres_17_46,feature_left_17_46,feature_right_17_46,feature_alpha_17_46},{2,haar_17_47,feature_thres_17_47,feature_left_17_47,feature_right_17_47,feature_alpha_17_47},{2,haar_17_48,feature_thres_17_48,feature_left_17_48,feature_right_17_48,feature_alpha_17_48},{2,haar_17_49,feature_thres_17_49,feature_left_17_49,feature_right_17_49,feature_alpha_17_49},{2,haar_17_50,feature_thres_17_50,feature_left_17_50,feature_right_17_50,feature_alpha_17_50},{2,haar_17_51,feature_thres_17_51,feature_left_17_51,feature_right_17_51,feature_alpha_17_51},{2,haar_17_52,feature_thres_17_52,feature_left_17_52,feature_right_17_52,feature_alpha_17_52},{2,haar_17_53,feature_thres_17_53,feature_left_17_53,feature_right_17_53,feature_alpha_17_53},{2,haar_17_54,feature_thres_17_54,feature_left_17_54,feature_right_17_54,feature_alpha_17_54},{2,haar_17_55,feature_thres_17_55,feature_left_17_55,feature_right_17_55,feature_alpha_17_55},{2,haar_17_56,feature_thres_17_56,feature_left_17_56,feature_right_17_56,feature_alpha_17_56},{2,haar_17_57,feature_thres_17_57,feature_left_17_57,feature_right_17_57,feature_alpha_17_57},{2,haar_17_58,feature_thres_17_58,feature_left_17_58,feature_right_17_58,feature_alpha_17_58},{2,haar_17_59,feature_thres_17_59,feature_left_17_59,feature_right_17_59,feature_alpha_17_59},{2,haar_17_60,feature_thres_17_60,feature_left_17_60,feature_right_17_60,feature_alpha_17_60},{2,haar_17_61,feature_thres_17_61,feature_left_17_61,feature_right_17_61,feature_alpha_17_61},{2,haar_17_62,feature_thres_17_62,feature_left_17_62,feature_right_17_62,feature_alpha_17_62},{2,haar_17_63,feature_thres_17_63,feature_left_17_63,feature_right_17_63,feature_alpha_17_63},{2,haar_17_64,feature_thres_17_64,feature_left_17_64,feature_right_17_64,feature_alpha_17_64},{2,haar_17_65,feature_thres_17_65,feature_left_17_65,feature_right_17_65,feature_alpha_17_65},{2,haar_17_66,feature_thres_17_66,feature_left_17_66,feature_right_17_66,feature_alpha_17_66},{2,haar_17_67,feature_thres_17_67,feature_left_17_67,feature_right_17_67,feature_alpha_17_67},{2,haar_17_68,feature_thres_17_68,feature_left_17_68,feature_right_17_68,feature_alpha_17_68},{2,haar_17_69,feature_thres_17_69,feature_left_17_69,feature_right_17_69,feature_alpha_17_69},{2,haar_17_70,feature_thres_17_70,feature_left_17_70,feature_right_17_70,feature_alpha_17_70},{2,haar_17_71,feature_thres_17_71,feature_left_17_71,feature_right_17_71,feature_alpha_17_71},{2,haar_17_72,feature_thres_17_72,feature_left_17_72,feature_right_17_72,feature_alpha_17_72},{2,haar_17_73,feature_thres_17_73,feature_left_17_73,feature_right_17_73,feature_alpha_17_73},{2,haar_17_74,feature_thres_17_74,feature_left_17_74,feature_right_17_74,feature_alpha_17_74},{2,haar_17_75,feature_thres_17_75,feature_left_17_75,feature_right_17_75,feature_alpha_17_75},{2,haar_17_76,feature_thres_17_76,feature_left_17_76,feature_right_17_76,feature_alpha_17_76},{2,haar_17_77,feature_thres_17_77,feature_left_17_77,feature_right_17_77,feature_alpha_17_77},{2,haar_17_78,feature_thres_17_78,feature_left_17_78,feature_right_17_78,feature_alpha_17_78},{2,haar_17_79,feature_thres_17_79,feature_left_17_79,feature_right_17_79,feature_alpha_17_79},{2,haar_17_80,feature_thres_17_80,feature_left_17_80,feature_right_17_80,feature_alpha_17_80},{2,haar_17_81,feature_thres_17_81,feature_left_17_81,feature_right_17_81,feature_alpha_17_81},{2,haar_17_82,feature_thres_17_82,feature_left_17_82,feature_right_17_82,feature_alpha_17_82},{2,haar_17_83,feature_thres_17_83,feature_left_17_83,feature_right_17_83,feature_alpha_17_83},{2,haar_17_84,feature_thres_17_84,feature_left_17_84,feature_right_17_84,feature_alpha_17_84},{2,haar_17_85,feature_thres_17_85,feature_left_17_85,feature_right_17_85,feature_alpha_17_85},{2,haar_17_86,feature_thres_17_86,feature_left_17_86,feature_right_17_86,feature_alpha_17_86},{2,haar_17_87,feature_thres_17_87,feature_left_17_87,feature_right_17_87,feature_alpha_17_87},{2,haar_17_88,feature_thres_17_88,feature_left_17_88,feature_right_17_88,feature_alpha_17_88},{2,haar_17_89,feature_thres_17_89,feature_left_17_89,feature_right_17_89,feature_alpha_17_89},{2,haar_17_90,feature_thres_17_90,feature_left_17_90,feature_right_17_90,feature_alpha_17_90},{2,haar_17_91,feature_thres_17_91,feature_left_17_91,feature_right_17_91,feature_alpha_17_91},{2,haar_17_92,feature_thres_17_92,feature_left_17_92,feature_right_17_92,feature_alpha_17_92},{2,haar_17_93,feature_thres_17_93,feature_left_17_93,feature_right_17_93,feature_alpha_17_93},{2,haar_17_94,feature_thres_17_94,feature_left_17_94,feature_right_17_94,feature_alpha_17_94},{2,haar_17_95,feature_thres_17_95,feature_left_17_95,feature_right_17_95,feature_alpha_17_95},{2,haar_17_96,feature_thres_17_96,feature_left_17_96,feature_right_17_96,feature_alpha_17_96},
};
HaarFeature MEMORY_TYPE haar_18_0[2] =
{
0,mvcvRect(4,3,11,9),-1.000000,mvcvRect(4,6,11,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,2,6,11),-1.000000,mvcvRect(3,2,3,11),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_0[2] =
{
-0.098691,0.062373,
};
int MEMORY_TYPE feature_left_18_0[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_0[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_0[3] =
{
0.399947,0.524778,0.819358,
};
HaarFeature MEMORY_TYPE haar_18_1[2] =
{
0,mvcvRect(13,0,4,5),-1.000000,mvcvRect(13,0,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,6,4),-1.000000,mvcvRect(12,7,3,2),2.000000,mvcvRect(9,9,3,2),2.000000,
};
float MEMORY_TYPE feature_thres_18_1[2] =
{
0.001950,-0.000891,
};
int MEMORY_TYPE feature_left_18_1[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_1[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_1[3] =
{
0.352982,0.585273,0.324598,
};
HaarFeature MEMORY_TYPE haar_18_2[2] =
{
0,mvcvRect(5,7,8,2),-1.000000,mvcvRect(9,7,4,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,8,15,1),-1.000000,mvcvRect(6,8,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_2[2] =
{
-0.000552,-0.001172,
};
int MEMORY_TYPE feature_left_18_2[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_2[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_2[3] =
{
0.389282,0.433505,0.652062,
};
HaarFeature MEMORY_TYPE haar_18_3[2] =
{
0,mvcvRect(4,12,12,2),-1.000000,mvcvRect(8,12,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,0,4,10),-1.000000,mvcvRect(15,0,2,5),2.000000,mvcvRect(13,5,2,5),2.000000,
};
float MEMORY_TYPE feature_thres_18_3[2] =
{
-0.000745,-0.002626,
};
int MEMORY_TYPE feature_left_18_3[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_3[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_3[3] =
{
0.404114,0.562498,0.396753,
};
HaarFeature MEMORY_TYPE haar_18_4[2] =
{
0,mvcvRect(9,9,2,2),-1.000000,mvcvRect(9,10,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,9,6,2),-1.000000,mvcvRect(6,9,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_4[2] =
{
-0.000397,0.003598,
};
int MEMORY_TYPE feature_left_18_4[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_4[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_4[3] =
{
0.385611,0.599789,0.424161,
};
HaarFeature MEMORY_TYPE haar_18_5[2] =
{
0,mvcvRect(8,17,4,3),-1.000000,mvcvRect(8,18,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,3,9,2),-1.000000,mvcvRect(11,3,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_5[2] =
{
0.005308,0.000963,
};
int MEMORY_TYPE feature_left_18_5[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_5[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_5[3] =
{
0.666017,0.448138,0.558349,
};
HaarFeature MEMORY_TYPE haar_18_6[2] =
{
0,mvcvRect(3,3,9,2),-1.000000,mvcvRect(6,3,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,0,9,14),-1.000000,mvcvRect(8,0,3,14),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_6[2] =
{
0.000508,0.003622,
};
int MEMORY_TYPE feature_left_18_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_6[3] =
{
0.353546,0.340981,0.542069,
};
HaarFeature MEMORY_TYPE haar_18_7[2] =
{
0,mvcvRect(7,3,7,10),-1.000000,mvcvRect(7,8,7,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,8,13,3),-1.000000,mvcvRect(4,9,13,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_7[2] =
{
-0.062061,0.000644,
};
int MEMORY_TYPE feature_left_18_7[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_7[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_7[3] =
{
0.193408,0.408363,0.549022,
};
HaarFeature MEMORY_TYPE haar_18_8[2] =
{
0,mvcvRect(3,12,14,4),-1.000000,mvcvRect(3,12,7,2),2.000000,mvcvRect(10,14,7,2),2.000000,0,mvcvRect(8,12,4,2),-1.000000,mvcvRect(8,13,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_8[2] =
{
0.026240,0.000819,
};
int MEMORY_TYPE feature_left_18_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_8[3] =
{
0.228571,0.464867,0.601736,
};
HaarFeature MEMORY_TYPE haar_18_9[2] =
{
0,mvcvRect(6,10,9,8),-1.000000,mvcvRect(6,14,9,4),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,12,2,8),-1.000000,mvcvRect(9,16,2,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_9[2] =
{
0.000238,-0.001587,
};
int MEMORY_TYPE feature_left_18_9[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_9[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_9[3] =
{
0.359804,0.425965,0.547643,
};
HaarFeature MEMORY_TYPE haar_18_10[2] =
{
0,mvcvRect(8,12,3,3),-1.000000,mvcvRect(8,13,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,4,10),-1.000000,mvcvRect(7,5,2,10),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_10[2] =
{
-0.006726,0.011006,
};
int MEMORY_TYPE feature_left_18_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_10[3] =
{
0.650724,0.514941,0.336298,
};
HaarFeature MEMORY_TYPE haar_18_11[2] =
{
0,mvcvRect(14,15,3,3),-1.000000,mvcvRect(14,16,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,6,13,3),-1.000000,mvcvRect(4,7,13,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_11[2] =
{
0.007145,-0.004723,
};
int MEMORY_TYPE feature_left_18_11[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_11[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_11[3] =
{
0.267293,0.565218,0.429814,
};
HaarFeature MEMORY_TYPE haar_18_12[2] =
{
0,mvcvRect(3,15,3,3),-1.000000,mvcvRect(3,16,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,9,4,2),-1.000000,mvcvRect(3,9,2,1),2.000000,mvcvRect(5,10,2,1),2.000000,
};
float MEMORY_TYPE feature_thres_18_12[2] =
{
0.009844,0.000015,
};
int MEMORY_TYPE feature_left_18_12[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_12[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_12[3] =
{
0.115189,0.437360,0.561213,
};
HaarFeature MEMORY_TYPE haar_18_13[2] =
{
0,mvcvRect(0,11,20,4),-1.000000,mvcvRect(10,11,10,2),2.000000,mvcvRect(0,13,10,2),2.000000,0,mvcvRect(8,15,4,3),-1.000000,mvcvRect(8,16,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_13[2] =
{
0.039909,0.005390,
};
int MEMORY_TYPE feature_left_18_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_13[3] =
{
0.520465,0.481347,0.636121,
};
HaarFeature MEMORY_TYPE haar_18_14[2] =
{
0,mvcvRect(0,11,20,4),-1.000000,mvcvRect(0,11,10,2),2.000000,mvcvRect(10,13,10,2),2.000000,0,mvcvRect(8,15,4,3),-1.000000,mvcvRect(8,16,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_14[2] =
{
-0.039909,0.005390,
};
int MEMORY_TYPE feature_left_18_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_14[3] =
{
0.150687,0.458169,0.620024,
};
HaarFeature MEMORY_TYPE haar_18_15[2] =
{
0,mvcvRect(10,13,1,6),-1.000000,mvcvRect(10,16,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,1,18,2),-1.000000,mvcvRect(11,1,9,1),2.000000,mvcvRect(2,2,9,1),2.000000,
};
float MEMORY_TYPE feature_thres_18_15[2] =
{
0.006701,-0.012624,
};
int MEMORY_TYPE feature_left_18_15[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_15[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_15[3] =
{
0.343224,0.308823,0.522674,
};
HaarFeature MEMORY_TYPE haar_18_16[2] =
{
0,mvcvRect(8,14,3,3),-1.000000,mvcvRect(8,15,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,1,6,1),-1.000000,mvcvRect(6,1,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_16[2] =
{
0.011807,-0.003426,
};
int MEMORY_TYPE feature_left_18_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_16[3] =
{
0.718794,0.312081,0.506584,
};
HaarFeature MEMORY_TYPE haar_18_17[2] =
{
0,mvcvRect(11,13,1,3),-1.000000,mvcvRect(11,14,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,5,2,12),-1.000000,mvcvRect(13,11,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_17[2] =
{
0.000394,0.034388,
};
int MEMORY_TYPE feature_left_18_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_17[3] =
{
0.475458,0.526166,0.335017,
};
HaarFeature MEMORY_TYPE haar_18_18[2] =
{
0,mvcvRect(1,14,18,6),-1.000000,mvcvRect(1,16,18,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,13,1,3),-1.000000,mvcvRect(8,14,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_18[2] =
{
-0.075010,0.000490,
};
int MEMORY_TYPE feature_left_18_18[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_18[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_18[3] =
{
0.171348,0.472580,0.595647,
};
HaarFeature MEMORY_TYPE haar_18_19[2] =
{
0,mvcvRect(7,13,6,3),-1.000000,mvcvRect(7,14,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,10,3,2),-1.000000,mvcvRect(9,11,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_19[2] =
{
-0.008553,0.000131,
};
int MEMORY_TYPE feature_left_18_19[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_19[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_19[3] =
{
0.655822,0.483540,0.558691,
};
HaarFeature MEMORY_TYPE haar_18_20[2] =
{
0,mvcvRect(5,1,3,3),-1.000000,mvcvRect(6,1,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,6,5),-1.000000,mvcvRect(8,5,3,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_20[2] =
{
0.004795,0.002012,
};
int MEMORY_TYPE feature_left_18_20[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_20[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_20[3] =
{
0.264571,0.365795,0.512477,
};
HaarFeature MEMORY_TYPE haar_18_21[2] =
{
0,mvcvRect(7,5,6,14),-1.000000,mvcvRect(7,12,6,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,16,6,2),-1.000000,mvcvRect(9,16,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_21[2] =
{
-0.117855,0.001558,
};
int MEMORY_TYPE feature_left_18_21[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_21[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_21[3] =
{
0.238565,0.549047,0.427475,
};
HaarFeature MEMORY_TYPE haar_18_22[2] =
{
0,mvcvRect(0,2,2,12),-1.000000,mvcvRect(1,2,1,12),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,0,5,3),-1.000000,mvcvRect(1,1,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_22[2] =
{
-0.015574,-0.002185,
};
int MEMORY_TYPE feature_left_18_22[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_22[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_22[3] =
{
0.693890,0.364599,0.509253,
};
HaarFeature MEMORY_TYPE haar_18_23[2] =
{
0,mvcvRect(12,4,3,3),-1.000000,mvcvRect(12,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,6,3,3),-1.000000,mvcvRect(12,7,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_23[2] =
{
0.002927,0.006466,
};
int MEMORY_TYPE feature_left_18_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_23[3] =
{
0.468581,0.497341,0.772610,
};
HaarFeature MEMORY_TYPE haar_18_24[2] =
{
0,mvcvRect(5,4,3,3),-1.000000,mvcvRect(5,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,6,3,3),-1.000000,mvcvRect(5,7,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_24[2] =
{
-0.007614,0.004151,
};
int MEMORY_TYPE feature_left_18_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_24[3] =
{
0.687747,0.478853,0.692166,
};
HaarFeature MEMORY_TYPE haar_18_25[2] =
{
0,mvcvRect(8,12,4,8),-1.000000,mvcvRect(10,12,2,4),2.000000,mvcvRect(8,16,2,4),2.000000,0,mvcvRect(2,17,18,2),-1.000000,mvcvRect(11,17,9,1),2.000000,mvcvRect(2,18,9,1),2.000000,
};
float MEMORY_TYPE feature_thres_18_25[2] =
{
0.002771,-0.012836,
};
int MEMORY_TYPE feature_left_18_25[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_25[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_25[3] =
{
0.548184,0.380016,0.520449,
};
HaarFeature MEMORY_TYPE haar_18_26[2] =
{
0,mvcvRect(9,3,2,2),-1.000000,mvcvRect(9,4,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,5,4,6),-1.000000,mvcvRect(8,7,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_26[2] =
{
-0.002438,0.002171,
};
int MEMORY_TYPE feature_left_18_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_26[3] =
{
0.258244,0.496116,0.321520,
};
HaarFeature MEMORY_TYPE haar_18_27[2] =
{
0,mvcvRect(9,0,8,6),-1.000000,mvcvRect(9,2,8,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,0,18,4),-1.000000,mvcvRect(7,0,6,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_27[2] =
{
0.000628,-0.009798,
};
int MEMORY_TYPE feature_left_18_27[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_27[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_27[3] =
{
0.546042,0.604654,0.493992,
};
HaarFeature MEMORY_TYPE haar_18_28[2] =
{
0,mvcvRect(0,0,4,8),-1.000000,mvcvRect(2,0,2,8),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,4,6,9),-1.000000,mvcvRect(2,4,2,9),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_28[2] =
{
0.007354,-0.014665,
};
int MEMORY_TYPE feature_left_18_28[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_28[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_28[3] =
{
0.529109,0.544612,0.356736,
};
HaarFeature MEMORY_TYPE haar_18_29[2] =
{
0,mvcvRect(1,4,18,2),-1.000000,mvcvRect(7,4,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,16,12,4),-1.000000,mvcvRect(14,16,6,2),2.000000,mvcvRect(8,18,6,2),2.000000,
};
float MEMORY_TYPE feature_thres_18_29[2] =
{
0.030245,-0.056660,
};
int MEMORY_TYPE feature_left_18_29[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_29[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_29[3] =
{
0.551833,0.693098,0.509339,
};
HaarFeature MEMORY_TYPE haar_18_30[2] =
{
0,mvcvRect(0,0,18,2),-1.000000,mvcvRect(0,0,9,1),2.000000,mvcvRect(9,1,9,1),2.000000,0,mvcvRect(3,0,3,18),-1.000000,mvcvRect(4,0,1,18),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_30[2] =
{
-0.005697,0.030807,
};
int MEMORY_TYPE feature_left_18_30[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_30[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_30[3] =
{
0.320153,0.498925,0.227705,
};
HaarFeature MEMORY_TYPE haar_18_31[2] =
{
0,mvcvRect(14,9,4,7),-1.000000,mvcvRect(14,9,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,14,2,2),-1.000000,mvcvRect(15,15,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_31[2] =
{
0.002275,0.002044,
};
int MEMORY_TYPE feature_left_18_31[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_31[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_31[3] =
{
0.481093,0.528387,0.325592,
};
HaarFeature MEMORY_TYPE haar_18_32[2] =
{
0,mvcvRect(2,9,4,7),-1.000000,mvcvRect(4,9,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,14,2,2),-1.000000,mvcvRect(3,15,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_32[2] =
{
-0.008628,0.000651,
};
int MEMORY_TYPE feature_left_18_32[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_32[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_32[3] =
{
0.626654,0.509714,0.319191,
};
HaarFeature MEMORY_TYPE haar_18_33[2] =
{
0,mvcvRect(11,0,6,6),-1.000000,mvcvRect(11,2,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,0,2,6),-1.000000,mvcvRect(15,0,1,3),2.000000,mvcvRect(14,3,1,3),2.000000,
};
float MEMORY_TYPE feature_thres_18_33[2] =
{
0.000882,-0.014595,
};
int MEMORY_TYPE feature_left_18_33[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_33[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_33[3] =
{
0.454959,0.264504,0.515387,
};
HaarFeature MEMORY_TYPE haar_18_34[2] =
{
0,mvcvRect(7,11,2,2),-1.000000,mvcvRect(7,11,1,1),2.000000,mvcvRect(8,12,1,1),2.000000,0,mvcvRect(7,10,2,2),-1.000000,mvcvRect(8,10,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_34[2] =
{
-0.001230,-0.000219,
};
int MEMORY_TYPE feature_left_18_34[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_34[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_34[3] =
{
0.619758,0.546920,0.420686,
};
HaarFeature MEMORY_TYPE haar_18_35[2] =
{
0,mvcvRect(9,14,2,6),-1.000000,mvcvRect(9,17,2,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,18,4,2),-1.000000,mvcvRect(12,19,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_35[2] =
{
-0.001091,0.000352,
};
int MEMORY_TYPE feature_left_18_35[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_35[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_35[3] =
{
0.414076,0.547661,0.415502,
};
HaarFeature MEMORY_TYPE haar_18_36[2] =
{
0,mvcvRect(8,17,4,3),-1.000000,mvcvRect(8,18,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,18,8,2),-1.000000,mvcvRect(2,19,8,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_36[2] =
{
-0.007256,0.001470,
};
int MEMORY_TYPE feature_left_18_36[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_36[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_36[3] =
{
0.716047,0.524081,0.372966,
};
HaarFeature MEMORY_TYPE haar_18_37[2] =
{
0,mvcvRect(2,9,16,3),-1.000000,mvcvRect(2,10,16,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,9,2,2),-1.000000,mvcvRect(9,10,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_37[2] =
{
0.000115,0.003051,
};
int MEMORY_TYPE feature_left_18_37[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_37[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_37[3] =
{
0.403380,0.526399,0.356009,
};
HaarFeature MEMORY_TYPE haar_18_38[2] =
{
0,mvcvRect(5,14,2,4),-1.000000,mvcvRect(5,14,1,2),2.000000,mvcvRect(6,16,1,2),2.000000,0,mvcvRect(8,9,4,2),-1.000000,mvcvRect(8,9,2,1),2.000000,mvcvRect(10,10,2,1),2.000000,
};
float MEMORY_TYPE feature_thres_18_38[2] =
{
0.000263,-0.003637,
};
int MEMORY_TYPE feature_left_18_38[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_38[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_38[3] =
{
0.456980,0.304257,0.586825,
};
HaarFeature MEMORY_TYPE haar_18_39[2] =
{
0,mvcvRect(9,5,2,5),-1.000000,mvcvRect(9,5,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,9,3,2),-1.000000,mvcvRect(10,9,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_39[2] =
{
-0.008489,0.005811,
};
int MEMORY_TYPE feature_left_18_39[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_39[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_39[3] =
{
0.491416,0.491853,0.626696,
};
HaarFeature MEMORY_TYPE haar_18_40[2] =
{
0,mvcvRect(8,9,3,2),-1.000000,mvcvRect(9,9,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,8,3,6),-1.000000,mvcvRect(9,8,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_40[2] =
{
0.000756,-0.002202,
};
int MEMORY_TYPE feature_left_18_40[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_40[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_40[3] =
{
0.563324,0.555392,0.382765,
};
HaarFeature MEMORY_TYPE haar_18_41[2] =
{
0,mvcvRect(8,12,4,8),-1.000000,mvcvRect(10,12,2,4),2.000000,mvcvRect(8,16,2,4),2.000000,0,mvcvRect(2,17,16,2),-1.000000,mvcvRect(10,17,8,1),2.000000,mvcvRect(2,18,8,1),2.000000,
};
float MEMORY_TYPE feature_thres_18_41[2] =
{
0.002791,-0.001823,
};
int MEMORY_TYPE feature_left_18_41[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_41[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_41[3] =
{
0.549870,0.438228,0.542403,
};
HaarFeature MEMORY_TYPE haar_18_42[2] =
{
0,mvcvRect(8,12,3,8),-1.000000,mvcvRect(9,12,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,10,1,3),-1.000000,mvcvRect(3,11,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_42[2] =
{
-0.007250,-0.000687,
};
int MEMORY_TYPE feature_left_18_42[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_42[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_42[3] =
{
0.288812,0.347266,0.507637,
};
HaarFeature MEMORY_TYPE haar_18_43[2] =
{
0,mvcvRect(9,14,10,6),-1.000000,mvcvRect(14,14,5,3),2.000000,mvcvRect(9,17,5,3),2.000000,0,mvcvRect(14,13,3,6),-1.000000,mvcvRect(14,15,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_43[2] =
{
0.002517,-0.010151,
};
int MEMORY_TYPE feature_left_18_43[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_43[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_43[3] =
{
0.466121,0.374478,0.529400,
};
HaarFeature MEMORY_TYPE haar_18_44[2] =
{
0,mvcvRect(1,19,18,1),-1.000000,mvcvRect(7,19,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,10,15,2),-1.000000,mvcvRect(7,10,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_44[2] =
{
-0.004140,-0.004708,
};
int MEMORY_TYPE feature_left_18_44[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_44[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_44[3] =
{
0.466049,0.417506,0.691631,
};
HaarFeature MEMORY_TYPE haar_18_45[2] =
{
0,mvcvRect(4,17,16,3),-1.000000,mvcvRect(4,18,16,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,6,4,9),-1.000000,mvcvRect(8,9,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_45[2] =
{
0.041981,-0.014273,
};
int MEMORY_TYPE feature_left_18_45[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_45[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_45[3] =
{
0.201822,0.751120,0.503208,
};
HaarFeature MEMORY_TYPE haar_18_46[2] =
{
0,mvcvRect(9,16,2,4),-1.000000,mvcvRect(9,16,1,2),2.000000,mvcvRect(10,18,1,2),2.000000,0,mvcvRect(5,5,10,8),-1.000000,mvcvRect(5,9,10,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_46[2] =
{
0.004087,0.001761,
};
int MEMORY_TYPE feature_left_18_46[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_46[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_46[3] =
{
0.250451,0.330140,0.521834,
};
HaarFeature MEMORY_TYPE haar_18_47[2] =
{
0,mvcvRect(13,1,4,2),-1.000000,mvcvRect(13,1,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,0,3,6),-1.000000,mvcvRect(14,2,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_47[2] =
{
0.000126,-0.002950,
};
int MEMORY_TYPE feature_left_18_47[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_47[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_47[3] =
{
0.461444,0.461995,0.524703,
};
HaarFeature MEMORY_TYPE haar_18_48[2] =
{
0,mvcvRect(6,7,2,2),-1.000000,mvcvRect(6,7,1,1),2.000000,mvcvRect(7,8,1,1),2.000000,0,mvcvRect(7,1,6,1),-1.000000,mvcvRect(9,1,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_48[2] =
{
-0.001131,-0.001698,
};
int MEMORY_TYPE feature_left_18_48[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_48[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_48[3] =
{
0.631437,0.340131,0.505553,
};
HaarFeature MEMORY_TYPE haar_18_49[2] =
{
0,mvcvRect(9,11,3,3),-1.000000,mvcvRect(9,12,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,9,3,3),-1.000000,mvcvRect(13,9,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_49[2] =
{
-0.011458,-0.008496,
};
int MEMORY_TYPE feature_left_18_49[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_49[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_49[3] =
{
0.494000,0.296545,0.519437,
};
HaarFeature MEMORY_TYPE haar_18_50[2] =
{
0,mvcvRect(8,11,3,3),-1.000000,mvcvRect(8,12,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,9,3,3),-1.000000,mvcvRect(6,9,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_50[2] =
{
0.011919,0.006442,
};
int MEMORY_TYPE feature_left_18_50[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_50[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_50[3] =
{
0.788700,0.510699,0.296715,
};
HaarFeature MEMORY_TYPE haar_18_51[2] =
{
0,mvcvRect(10,11,1,3),-1.000000,mvcvRect(10,12,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,9,6,4),-1.000000,mvcvRect(10,9,3,2),2.000000,mvcvRect(7,11,3,2),2.000000,
};
float MEMORY_TYPE feature_thres_18_51[2] =
{
-0.000879,-0.002031,
};
int MEMORY_TYPE feature_left_18_51[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_51[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_51[3] =
{
0.571437,0.448120,0.538491,
};
HaarFeature MEMORY_TYPE haar_18_52[2] =
{
0,mvcvRect(4,7,2,2),-1.000000,mvcvRect(4,7,1,1),2.000000,mvcvRect(5,8,1,1),2.000000,0,mvcvRect(5,7,3,1),-1.000000,mvcvRect(6,7,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_52[2] =
{
-0.001526,0.004286,
};
int MEMORY_TYPE feature_left_18_52[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_52[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_52[3] =
{
0.619357,0.433989,0.769730,
};
HaarFeature MEMORY_TYPE haar_18_53[2] =
{
0,mvcvRect(18,3,2,3),-1.000000,mvcvRect(18,4,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,1,4,2),-1.000000,mvcvRect(13,1,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_53[2] =
{
0.003501,0.012588,
};
int MEMORY_TYPE feature_left_18_53[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_53[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_53[3] =
{
0.317139,0.524670,0.424121,
};
HaarFeature MEMORY_TYPE haar_18_54[2] =
{
0,mvcvRect(3,1,4,2),-1.000000,mvcvRect(5,1,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,0,5,2),-1.000000,mvcvRect(3,1,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_54[2] =
{
0.000262,0.000045,
};
int MEMORY_TYPE feature_left_18_54[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_54[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_54[3] =
{
0.423190,0.417414,0.591960,
};
HaarFeature MEMORY_TYPE haar_18_55[2] =
{
0,mvcvRect(14,7,6,4),-1.000000,mvcvRect(17,7,3,2),2.000000,mvcvRect(14,9,3,2),2.000000,0,mvcvRect(4,8,16,2),-1.000000,mvcvRect(4,9,16,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_55[2] =
{
0.000781,0.000889,
};
int MEMORY_TYPE feature_left_18_55[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_55[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_55[3] =
{
0.427739,0.372016,0.522682,
};
HaarFeature MEMORY_TYPE haar_18_56[2] =
{
0,mvcvRect(2,11,5,6),-1.000000,mvcvRect(2,13,5,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,16,2,4),-1.000000,mvcvRect(5,16,1,2),2.000000,mvcvRect(6,18,1,2),2.000000,
};
float MEMORY_TYPE feature_thres_18_56[2] =
{
0.002337,0.001669,
};
int MEMORY_TYPE feature_left_18_56[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_56[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_56[3] =
{
0.547807,0.362868,0.615000,
};
HaarFeature MEMORY_TYPE haar_18_57[2] =
{
0,mvcvRect(15,6,2,12),-1.000000,mvcvRect(16,6,1,6),2.000000,mvcvRect(15,12,1,6),2.000000,0,mvcvRect(13,3,6,16),-1.000000,mvcvRect(15,3,2,16),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_57[2] =
{
0.000308,0.003462,
};
int MEMORY_TYPE feature_left_18_57[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_57[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_57[3] =
{
0.474708,0.458014,0.558568,
};
HaarFeature MEMORY_TYPE haar_18_58[2] =
{
0,mvcvRect(4,5,12,12),-1.000000,mvcvRect(4,5,6,6),2.000000,mvcvRect(10,11,6,6),2.000000,0,mvcvRect(5,1,10,13),-1.000000,mvcvRect(10,1,5,13),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_58[2] =
{
0.018961,0.173473,
};
int MEMORY_TYPE feature_left_18_58[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_58[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_58[3] =
{
0.529880,0.369839,0.849862,
};
HaarFeature MEMORY_TYPE haar_18_59[2] =
{
0,mvcvRect(11,5,2,2),-1.000000,mvcvRect(12,5,1,1),2.000000,mvcvRect(11,6,1,1),2.000000,0,mvcvRect(13,5,1,3),-1.000000,mvcvRect(13,6,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_59[2] =
{
0.000200,0.001097,
};
int MEMORY_TYPE feature_left_18_59[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_59[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_59[3] =
{
0.556566,0.479571,0.628626,
};
HaarFeature MEMORY_TYPE haar_18_60[2] =
{
0,mvcvRect(7,4,2,4),-1.000000,mvcvRect(7,4,1,2),2.000000,mvcvRect(8,6,1,2),2.000000,0,mvcvRect(7,5,6,4),-1.000000,mvcvRect(10,5,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_60[2] =
{
0.000151,-0.003446,
};
int MEMORY_TYPE feature_left_18_60[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_60[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_60[3] =
{
0.405241,0.617302,0.441426,
};
HaarFeature MEMORY_TYPE haar_18_61[2] =
{
0,mvcvRect(12,4,4,6),-1.000000,mvcvRect(14,4,2,3),2.000000,mvcvRect(12,7,2,3),2.000000,0,mvcvRect(12,11,7,6),-1.000000,mvcvRect(12,13,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_61[2] =
{
0.008518,-0.035812,
};
int MEMORY_TYPE feature_left_18_61[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_61[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_61[3] =
{
0.357057,0.315133,0.525270,
};
HaarFeature MEMORY_TYPE haar_18_62[2] =
{
0,mvcvRect(5,6,6,6),-1.000000,mvcvRect(7,6,2,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,8,2,2),-1.000000,mvcvRect(9,9,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_62[2] =
{
-0.021155,0.000899,
};
int MEMORY_TYPE feature_left_18_62[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_62[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_62[3] =
{
0.612472,0.516998,0.359627,
};
HaarFeature MEMORY_TYPE haar_18_63[2] =
{
0,mvcvRect(15,6,2,2),-1.000000,mvcvRect(16,6,1,1),2.000000,mvcvRect(15,7,1,1),2.000000,0,mvcvRect(14,7,4,4),-1.000000,mvcvRect(16,7,2,2),2.000000,mvcvRect(14,9,2,2),2.000000,
};
float MEMORY_TYPE feature_thres_18_63[2] =
{
-0.001561,0.000671,
};
int MEMORY_TYPE feature_left_18_63[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_63[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_63[3] =
{
0.491499,0.454621,0.539581,
};
HaarFeature MEMORY_TYPE haar_18_64[2] =
{
0,mvcvRect(5,5,6,2),-1.000000,mvcvRect(7,5,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,19,18,1),-1.000000,mvcvRect(7,19,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_64[2] =
{
-0.021597,-0.024947,
};
int MEMORY_TYPE feature_left_18_64[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_64[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_64[3] =
{
0.190313,0.697408,0.496772,
};
HaarFeature MEMORY_TYPE haar_18_65[2] =
{
0,mvcvRect(12,3,3,3),-1.000000,mvcvRect(12,4,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,0,2,3),-1.000000,mvcvRect(16,1,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_65[2] =
{
0.001873,0.006391,
};
int MEMORY_TYPE feature_left_18_65[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_65[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_65[3] =
{
0.474895,0.518018,0.292432,
};
HaarFeature MEMORY_TYPE haar_18_66[2] =
{
0,mvcvRect(5,3,3,3),-1.000000,mvcvRect(5,4,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,0,2,3),-1.000000,mvcvRect(2,1,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_66[2] =
{
-0.009155,0.002172,
};
int MEMORY_TYPE feature_left_18_66[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_66[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_66[3] =
{
0.766587,0.521555,0.336572,
};
HaarFeature MEMORY_TYPE haar_18_67[2] =
{
0,mvcvRect(15,6,2,2),-1.000000,mvcvRect(16,6,1,1),2.000000,mvcvRect(15,7,1,1),2.000000,0,mvcvRect(10,13,1,6),-1.000000,mvcvRect(10,16,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_67[2] =
{
0.001233,-0.000408,
};
int MEMORY_TYPE feature_left_18_67[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_67[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_67[3] =
{
0.626096,0.453351,0.538649,
};
HaarFeature MEMORY_TYPE haar_18_68[2] =
{
0,mvcvRect(0,7,10,2),-1.000000,mvcvRect(0,7,5,1),2.000000,mvcvRect(5,8,5,1),2.000000,0,mvcvRect(3,10,6,2),-1.000000,mvcvRect(3,11,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_68[2] =
{
0.000464,-0.000116,
};
int MEMORY_TYPE feature_left_18_68[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_68[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_68[3] =
{
0.410350,0.583039,0.430411,
};
HaarFeature MEMORY_TYPE haar_18_69[2] =
{
0,mvcvRect(12,18,4,2),-1.000000,mvcvRect(12,19,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,18,2,2),-1.000000,mvcvRect(13,18,1,1),2.000000,mvcvRect(12,19,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_18_69[2] =
{
-0.012719,0.000089,
};
int MEMORY_TYPE feature_left_18_69[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_69[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_69[3] =
{
0.213258,0.487289,0.545892,
};
HaarFeature MEMORY_TYPE haar_18_70[2] =
{
0,mvcvRect(6,19,2,1),-1.000000,mvcvRect(7,19,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,4,2,16),-1.000000,mvcvRect(0,4,1,8),2.000000,mvcvRect(1,12,1,8),2.000000,
};
float MEMORY_TYPE feature_thres_18_70[2] =
{
-0.000339,-0.018026,
};
int MEMORY_TYPE feature_left_18_70[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_70[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_70[3] =
{
0.397436,0.756855,0.504561,
};
HaarFeature MEMORY_TYPE haar_18_71[2] =
{
0,mvcvRect(16,1,4,9),-1.000000,mvcvRect(16,4,4,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,2,1,2),-1.000000,mvcvRect(10,3,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_71[2] =
{
0.006918,-0.000118,
};
int MEMORY_TYPE feature_left_18_71[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_71[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_71[3] =
{
0.396630,0.419808,0.543580,
};
HaarFeature MEMORY_TYPE haar_18_72[2] =
{
0,mvcvRect(4,14,4,6),-1.000000,mvcvRect(4,14,2,3),2.000000,mvcvRect(6,17,2,3),2.000000,0,mvcvRect(4,15,1,4),-1.000000,mvcvRect(4,17,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_72[2] =
{
-0.003947,0.000060,
};
int MEMORY_TYPE feature_left_18_72[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_72[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_72[3] =
{
0.636946,0.526957,0.381224,
};
HaarFeature MEMORY_TYPE haar_18_73[2] =
{
0,mvcvRect(0,2,20,4),-1.000000,mvcvRect(10,2,10,2),2.000000,mvcvRect(0,4,10,2),2.000000,0,mvcvRect(14,5,2,8),-1.000000,mvcvRect(14,9,2,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_73[2] =
{
0.009142,0.000213,
};
int MEMORY_TYPE feature_left_18_73[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_73[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_73[3] =
{
0.415676,0.352353,0.534945,
};
HaarFeature MEMORY_TYPE haar_18_74[2] =
{
0,mvcvRect(5,12,4,5),-1.000000,mvcvRect(7,12,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,13,9,6),-1.000000,mvcvRect(0,15,9,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_74[2] =
{
-0.000209,0.001313,
};
int MEMORY_TYPE feature_left_18_74[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_74[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_74[3] =
{
0.440332,0.605816,0.446822,
};
HaarFeature MEMORY_TYPE haar_18_75[2] =
{
0,mvcvRect(9,14,11,3),-1.000000,mvcvRect(9,15,11,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,14,7,3),-1.000000,mvcvRect(7,15,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_75[2] =
{
-0.002913,0.002965,
};
int MEMORY_TYPE feature_left_18_75[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_75[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_75[3] =
{
0.482571,0.483600,0.603928,
};
HaarFeature MEMORY_TYPE haar_18_76[2] =
{
0,mvcvRect(3,6,2,2),-1.000000,mvcvRect(3,6,1,1),2.000000,mvcvRect(4,7,1,1),2.000000,0,mvcvRect(6,7,2,7),-1.000000,mvcvRect(7,7,1,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_76[2] =
{
0.001777,-0.007714,
};
int MEMORY_TYPE feature_left_18_76[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_76[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_76[3] =
{
0.687183,0.284222,0.514543,
};
HaarFeature MEMORY_TYPE haar_18_77[2] =
{
0,mvcvRect(14,5,1,3),-1.000000,mvcvRect(14,6,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(13,4,4,3),-1.000000,mvcvRect(13,5,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_77[2] =
{
0.000510,0.001746,
};
int MEMORY_TYPE feature_left_18_77[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_77[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_77[3] =
{
0.602443,0.475661,0.572115,
};
HaarFeature MEMORY_TYPE haar_18_78[2] =
{
0,mvcvRect(2,7,4,4),-1.000000,mvcvRect(2,7,2,2),2.000000,mvcvRect(4,9,2,2),2.000000,0,mvcvRect(2,9,13,6),-1.000000,mvcvRect(2,12,13,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_78[2] =
{
0.000381,0.002823,
};
int MEMORY_TYPE feature_left_18_78[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_78[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_78[3] =
{
0.493107,0.331170,0.622760,
};
HaarFeature MEMORY_TYPE haar_18_79[2] =
{
0,mvcvRect(10,1,3,4),-1.000000,mvcvRect(11,1,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,8,5,2),-1.000000,mvcvRect(9,9,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_79[2] =
{
-0.005300,0.000045,
};
int MEMORY_TYPE feature_left_18_79[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_79[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_79[3] =
{
0.523209,0.399523,0.531480,
};
HaarFeature MEMORY_TYPE haar_18_80[2] =
{
0,mvcvRect(0,14,11,3),-1.000000,mvcvRect(0,15,11,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,11,2,8),-1.000000,mvcvRect(8,15,2,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_80[2] =
{
0.003275,-0.002816,
};
int MEMORY_TYPE feature_left_18_80[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_80[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_80[3] =
{
0.448162,0.390797,0.667164,
};
HaarFeature MEMORY_TYPE haar_18_81[2] =
{
0,mvcvRect(5,11,10,6),-1.000000,mvcvRect(5,14,10,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,13,15,5),-1.000000,mvcvRect(10,13,5,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_81[2] =
{
0.001411,0.008306,
};
int MEMORY_TYPE feature_left_18_81[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_81[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_81[3] =
{
0.535701,0.477097,0.557010,
};
HaarFeature MEMORY_TYPE haar_18_82[2] =
{
0,mvcvRect(8,10,1,10),-1.000000,mvcvRect(8,15,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,14,6,2),-1.000000,mvcvRect(6,14,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_82[2] =
{
0.002216,-0.004987,
};
int MEMORY_TYPE feature_left_18_82[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_82[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_82[3] =
{
0.494712,0.524131,0.251265,
};
HaarFeature MEMORY_TYPE haar_18_83[2] =
{
0,mvcvRect(7,14,7,3),-1.000000,mvcvRect(7,15,7,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,16,9,3),-1.000000,mvcvRect(7,17,9,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_83[2] =
{
-0.003666,-0.010581,
};
int MEMORY_TYPE feature_left_18_83[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_83[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_83[3] =
{
0.461955,0.630172,0.497303,
};
HaarFeature MEMORY_TYPE haar_18_84[2] =
{
0,mvcvRect(8,7,3,3),-1.000000,mvcvRect(8,8,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,5,1,6),-1.000000,mvcvRect(3,8,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_84[2] =
{
0.007337,-0.000393,
};
int MEMORY_TYPE feature_left_18_84[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_84[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_84[3] =
{
0.287097,0.425281,0.557925,
};
HaarFeature MEMORY_TYPE haar_18_85[2] =
{
0,mvcvRect(6,5,11,2),-1.000000,mvcvRect(6,6,11,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,0,3,2),-1.000000,mvcvRect(10,0,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_85[2] =
{
-0.008138,0.002481,
};
int MEMORY_TYPE feature_left_18_85[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_85[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_85[3] =
{
0.574732,0.520337,0.390357,
};
HaarFeature MEMORY_TYPE haar_18_86[2] =
{
0,mvcvRect(5,5,1,3),-1.000000,mvcvRect(5,6,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,3,2),-1.000000,mvcvRect(9,7,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_86[2] =
{
0.000887,-0.000422,
};
int MEMORY_TYPE feature_left_18_86[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_86[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_86[3] =
{
0.553432,0.533804,0.392584,
};
HaarFeature MEMORY_TYPE haar_18_87[2] =
{
0,mvcvRect(5,2,10,6),-1.000000,mvcvRect(10,2,5,3),2.000000,mvcvRect(5,5,5,3),2.000000,0,mvcvRect(8,4,6,4),-1.000000,mvcvRect(8,4,3,4),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_87[2] =
{
-0.007979,0.001144,
};
int MEMORY_TYPE feature_left_18_87[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_18_87[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_18_87[3] =
{
0.414432,0.470137,0.528174,
};
HaarFeature MEMORY_TYPE haar_18_88[2] =
{
0,mvcvRect(8,16,3,4),-1.000000,mvcvRect(9,16,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,13,2,6),-1.000000,mvcvRect(9,13,1,3),2.000000,mvcvRect(10,16,1,3),2.000000,
};
float MEMORY_TYPE feature_thres_18_88[2] =
{
0.007554,0.001029,
};
int MEMORY_TYPE feature_left_18_88[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_88[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_88[3] =
{
0.252726,0.560515,0.429786,
};
HaarFeature MEMORY_TYPE haar_18_89[2] =
{
0,mvcvRect(9,8,3,1),-1.000000,mvcvRect(10,8,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,5,18,15),-1.000000,mvcvRect(2,10,18,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_18_89[2] =
{
-0.001723,0.575867,
};
int MEMORY_TYPE feature_left_18_89[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_18_89[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_18_89[3] =
{
0.483968,0.511050,0.080489,
};
HaarClassifier MEMORY_TYPE haar_feature_class_18[90] =
{
{2,haar_18_0,feature_thres_18_0,feature_left_18_0,feature_right_18_0,feature_alpha_18_0},{2,haar_18_1,feature_thres_18_1,feature_left_18_1,feature_right_18_1,feature_alpha_18_1},{2,haar_18_2,feature_thres_18_2,feature_left_18_2,feature_right_18_2,feature_alpha_18_2},{2,haar_18_3,feature_thres_18_3,feature_left_18_3,feature_right_18_3,feature_alpha_18_3},{2,haar_18_4,feature_thres_18_4,feature_left_18_4,feature_right_18_4,feature_alpha_18_4},{2,haar_18_5,feature_thres_18_5,feature_left_18_5,feature_right_18_5,feature_alpha_18_5},{2,haar_18_6,feature_thres_18_6,feature_left_18_6,feature_right_18_6,feature_alpha_18_6},{2,haar_18_7,feature_thres_18_7,feature_left_18_7,feature_right_18_7,feature_alpha_18_7},{2,haar_18_8,feature_thres_18_8,feature_left_18_8,feature_right_18_8,feature_alpha_18_8},{2,haar_18_9,feature_thres_18_9,feature_left_18_9,feature_right_18_9,feature_alpha_18_9},{2,haar_18_10,feature_thres_18_10,feature_left_18_10,feature_right_18_10,feature_alpha_18_10},{2,haar_18_11,feature_thres_18_11,feature_left_18_11,feature_right_18_11,feature_alpha_18_11},{2,haar_18_12,feature_thres_18_12,feature_left_18_12,feature_right_18_12,feature_alpha_18_12},{2,haar_18_13,feature_thres_18_13,feature_left_18_13,feature_right_18_13,feature_alpha_18_13},{2,haar_18_14,feature_thres_18_14,feature_left_18_14,feature_right_18_14,feature_alpha_18_14},{2,haar_18_15,feature_thres_18_15,feature_left_18_15,feature_right_18_15,feature_alpha_18_15},{2,haar_18_16,feature_thres_18_16,feature_left_18_16,feature_right_18_16,feature_alpha_18_16},{2,haar_18_17,feature_thres_18_17,feature_left_18_17,feature_right_18_17,feature_alpha_18_17},{2,haar_18_18,feature_thres_18_18,feature_left_18_18,feature_right_18_18,feature_alpha_18_18},{2,haar_18_19,feature_thres_18_19,feature_left_18_19,feature_right_18_19,feature_alpha_18_19},{2,haar_18_20,feature_thres_18_20,feature_left_18_20,feature_right_18_20,feature_alpha_18_20},{2,haar_18_21,feature_thres_18_21,feature_left_18_21,feature_right_18_21,feature_alpha_18_21},{2,haar_18_22,feature_thres_18_22,feature_left_18_22,feature_right_18_22,feature_alpha_18_22},{2,haar_18_23,feature_thres_18_23,feature_left_18_23,feature_right_18_23,feature_alpha_18_23},{2,haar_18_24,feature_thres_18_24,feature_left_18_24,feature_right_18_24,feature_alpha_18_24},{2,haar_18_25,feature_thres_18_25,feature_left_18_25,feature_right_18_25,feature_alpha_18_25},{2,haar_18_26,feature_thres_18_26,feature_left_18_26,feature_right_18_26,feature_alpha_18_26},{2,haar_18_27,feature_thres_18_27,feature_left_18_27,feature_right_18_27,feature_alpha_18_27},{2,haar_18_28,feature_thres_18_28,feature_left_18_28,feature_right_18_28,feature_alpha_18_28},{2,haar_18_29,feature_thres_18_29,feature_left_18_29,feature_right_18_29,feature_alpha_18_29},{2,haar_18_30,feature_thres_18_30,feature_left_18_30,feature_right_18_30,feature_alpha_18_30},{2,haar_18_31,feature_thres_18_31,feature_left_18_31,feature_right_18_31,feature_alpha_18_31},{2,haar_18_32,feature_thres_18_32,feature_left_18_32,feature_right_18_32,feature_alpha_18_32},{2,haar_18_33,feature_thres_18_33,feature_left_18_33,feature_right_18_33,feature_alpha_18_33},{2,haar_18_34,feature_thres_18_34,feature_left_18_34,feature_right_18_34,feature_alpha_18_34},{2,haar_18_35,feature_thres_18_35,feature_left_18_35,feature_right_18_35,feature_alpha_18_35},{2,haar_18_36,feature_thres_18_36,feature_left_18_36,feature_right_18_36,feature_alpha_18_36},{2,haar_18_37,feature_thres_18_37,feature_left_18_37,feature_right_18_37,feature_alpha_18_37},{2,haar_18_38,feature_thres_18_38,feature_left_18_38,feature_right_18_38,feature_alpha_18_38},{2,haar_18_39,feature_thres_18_39,feature_left_18_39,feature_right_18_39,feature_alpha_18_39},{2,haar_18_40,feature_thres_18_40,feature_left_18_40,feature_right_18_40,feature_alpha_18_40},{2,haar_18_41,feature_thres_18_41,feature_left_18_41,feature_right_18_41,feature_alpha_18_41},{2,haar_18_42,feature_thres_18_42,feature_left_18_42,feature_right_18_42,feature_alpha_18_42},{2,haar_18_43,feature_thres_18_43,feature_left_18_43,feature_right_18_43,feature_alpha_18_43},{2,haar_18_44,feature_thres_18_44,feature_left_18_44,feature_right_18_44,feature_alpha_18_44},{2,haar_18_45,feature_thres_18_45,feature_left_18_45,feature_right_18_45,feature_alpha_18_45},{2,haar_18_46,feature_thres_18_46,feature_left_18_46,feature_right_18_46,feature_alpha_18_46},{2,haar_18_47,feature_thres_18_47,feature_left_18_47,feature_right_18_47,feature_alpha_18_47},{2,haar_18_48,feature_thres_18_48,feature_left_18_48,feature_right_18_48,feature_alpha_18_48},{2,haar_18_49,feature_thres_18_49,feature_left_18_49,feature_right_18_49,feature_alpha_18_49},{2,haar_18_50,feature_thres_18_50,feature_left_18_50,feature_right_18_50,feature_alpha_18_50},{2,haar_18_51,feature_thres_18_51,feature_left_18_51,feature_right_18_51,feature_alpha_18_51},{2,haar_18_52,feature_thres_18_52,feature_left_18_52,feature_right_18_52,feature_alpha_18_52},{2,haar_18_53,feature_thres_18_53,feature_left_18_53,feature_right_18_53,feature_alpha_18_53},{2,haar_18_54,feature_thres_18_54,feature_left_18_54,feature_right_18_54,feature_alpha_18_54},{2,haar_18_55,feature_thres_18_55,feature_left_18_55,feature_right_18_55,feature_alpha_18_55},{2,haar_18_56,feature_thres_18_56,feature_left_18_56,feature_right_18_56,feature_alpha_18_56},{2,haar_18_57,feature_thres_18_57,feature_left_18_57,feature_right_18_57,feature_alpha_18_57},{2,haar_18_58,feature_thres_18_58,feature_left_18_58,feature_right_18_58,feature_alpha_18_58},{2,haar_18_59,feature_thres_18_59,feature_left_18_59,feature_right_18_59,feature_alpha_18_59},{2,haar_18_60,feature_thres_18_60,feature_left_18_60,feature_right_18_60,feature_alpha_18_60},{2,haar_18_61,feature_thres_18_61,feature_left_18_61,feature_right_18_61,feature_alpha_18_61},{2,haar_18_62,feature_thres_18_62,feature_left_18_62,feature_right_18_62,feature_alpha_18_62},{2,haar_18_63,feature_thres_18_63,feature_left_18_63,feature_right_18_63,feature_alpha_18_63},{2,haar_18_64,feature_thres_18_64,feature_left_18_64,feature_right_18_64,feature_alpha_18_64},{2,haar_18_65,feature_thres_18_65,feature_left_18_65,feature_right_18_65,feature_alpha_18_65},{2,haar_18_66,feature_thres_18_66,feature_left_18_66,feature_right_18_66,feature_alpha_18_66},{2,haar_18_67,feature_thres_18_67,feature_left_18_67,feature_right_18_67,feature_alpha_18_67},{2,haar_18_68,feature_thres_18_68,feature_left_18_68,feature_right_18_68,feature_alpha_18_68},{2,haar_18_69,feature_thres_18_69,feature_left_18_69,feature_right_18_69,feature_alpha_18_69},{2,haar_18_70,feature_thres_18_70,feature_left_18_70,feature_right_18_70,feature_alpha_18_70},{2,haar_18_71,feature_thres_18_71,feature_left_18_71,feature_right_18_71,feature_alpha_18_71},{2,haar_18_72,feature_thres_18_72,feature_left_18_72,feature_right_18_72,feature_alpha_18_72},{2,haar_18_73,feature_thres_18_73,feature_left_18_73,feature_right_18_73,feature_alpha_18_73},{2,haar_18_74,feature_thres_18_74,feature_left_18_74,feature_right_18_74,feature_alpha_18_74},{2,haar_18_75,feature_thres_18_75,feature_left_18_75,feature_right_18_75,feature_alpha_18_75},{2,haar_18_76,feature_thres_18_76,feature_left_18_76,feature_right_18_76,feature_alpha_18_76},{2,haar_18_77,feature_thres_18_77,feature_left_18_77,feature_right_18_77,feature_alpha_18_77},{2,haar_18_78,feature_thres_18_78,feature_left_18_78,feature_right_18_78,feature_alpha_18_78},{2,haar_18_79,feature_thres_18_79,feature_left_18_79,feature_right_18_79,feature_alpha_18_79},{2,haar_18_80,feature_thres_18_80,feature_left_18_80,feature_right_18_80,feature_alpha_18_80},{2,haar_18_81,feature_thres_18_81,feature_left_18_81,feature_right_18_81,feature_alpha_18_81},{2,haar_18_82,feature_thres_18_82,feature_left_18_82,feature_right_18_82,feature_alpha_18_82},{2,haar_18_83,feature_thres_18_83,feature_left_18_83,feature_right_18_83,feature_alpha_18_83},{2,haar_18_84,feature_thres_18_84,feature_left_18_84,feature_right_18_84,feature_alpha_18_84},{2,haar_18_85,feature_thres_18_85,feature_left_18_85,feature_right_18_85,feature_alpha_18_85},{2,haar_18_86,feature_thres_18_86,feature_left_18_86,feature_right_18_86,feature_alpha_18_86},{2,haar_18_87,feature_thres_18_87,feature_left_18_87,feature_right_18_87,feature_alpha_18_87},{2,haar_18_88,feature_thres_18_88,feature_left_18_88,feature_right_18_88,feature_alpha_18_88},{2,haar_18_89,feature_thres_18_89,feature_left_18_89,feature_right_18_89,feature_alpha_18_89},
};
HaarFeature MEMORY_TYPE haar_19_0[2] =
{
0,mvcvRect(1,3,6,2),-1.000000,mvcvRect(4,3,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,6,6,2),-1.000000,mvcvRect(9,6,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_0[2] =
{
0.006664,0.008991,
};
int MEMORY_TYPE feature_left_19_0[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_0[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_0[3] =
{
0.382892,0.485843,0.735496,
};
HaarFeature MEMORY_TYPE haar_19_1[2] =
{
0,mvcvRect(8,17,4,3),-1.000000,mvcvRect(8,18,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,13,2,3),-1.000000,mvcvRect(10,14,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_1[2] =
{
0.005715,0.001126,
};
int MEMORY_TYPE feature_left_19_1[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_1[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_1[3] =
{
0.672322,0.442958,0.607078,
};
HaarFeature MEMORY_TYPE haar_19_2[2] =
{
0,mvcvRect(0,10,20,4),-1.000000,mvcvRect(0,12,20,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,7,6,4),-1.000000,mvcvRect(5,7,3,2),2.000000,mvcvRect(8,9,3,2),2.000000,
};
float MEMORY_TYPE feature_thres_19_2[2] =
{
-0.000918,-0.001049,
};
int MEMORY_TYPE feature_left_19_2[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_2[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_2[3] =
{
0.307635,0.559364,0.365102,
};
HaarFeature MEMORY_TYPE haar_19_3[2] =
{
0,mvcvRect(11,12,1,2),-1.000000,mvcvRect(11,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,10,2,3),-1.000000,mvcvRect(10,11,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_3[2] =
{
0.000035,0.000290,
};
int MEMORY_TYPE feature_left_19_3[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_3[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_3[3] =
{
0.427797,0.458355,0.528468,
};
HaarFeature MEMORY_TYPE haar_19_4[2] =
{
0,mvcvRect(9,5,2,2),-1.000000,mvcvRect(9,6,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,4,1,10),-1.000000,mvcvRect(4,9,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_4[2] =
{
0.000161,-0.000530,
};
int MEMORY_TYPE feature_left_19_4[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_4[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_4[3] =
{
0.379819,0.385044,0.593969,
};
HaarFeature MEMORY_TYPE haar_19_5[2] =
{
0,mvcvRect(11,18,4,2),-1.000000,mvcvRect(11,18,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,18,3,2),-1.000000,mvcvRect(12,19,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_5[2] =
{
0.000267,-0.000135,
};
int MEMORY_TYPE feature_left_19_5[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_5[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_5[3] =
{
0.412302,0.576060,0.423765,
};
HaarFeature MEMORY_TYPE haar_19_6[2] =
{
0,mvcvRect(0,6,16,6),-1.000000,mvcvRect(0,6,8,3),2.000000,mvcvRect(8,9,8,3),2.000000,0,mvcvRect(7,6,4,12),-1.000000,mvcvRect(7,12,4,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_6[2] =
{
-0.010842,0.012078,
};
int MEMORY_TYPE feature_left_19_6[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_6[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_6[3] =
{
0.392992,0.576192,0.278044,
};
HaarFeature MEMORY_TYPE haar_19_7[2] =
{
0,mvcvRect(11,18,4,2),-1.000000,mvcvRect(11,18,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,18,3,2),-1.000000,mvcvRect(12,19,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_7[2] =
{
0.002213,-0.015266,
};
int MEMORY_TYPE feature_left_19_7[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_7[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_7[3] =
{
0.479451,0.074056,0.515358,
};
HaarFeature MEMORY_TYPE haar_19_8[2] =
{
0,mvcvRect(8,12,1,2),-1.000000,mvcvRect(8,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,13,1,3),-1.000000,mvcvRect(8,14,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_8[2] =
{
0.000068,0.000176,
};
int MEMORY_TYPE feature_left_19_8[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_8[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_8[3] =
{
0.585874,0.356761,0.559896,
};
HaarFeature MEMORY_TYPE haar_19_9[2] =
{
0,mvcvRect(11,18,4,2),-1.000000,mvcvRect(11,18,2,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,12,4,6),-1.000000,mvcvRect(14,12,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_9[2] =
{
0.000813,0.003263,
};
int MEMORY_TYPE feature_left_19_9[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_9[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_9[3] =
{
0.534685,0.478254,0.545675,
};
HaarFeature MEMORY_TYPE haar_19_10[2] =
{
0,mvcvRect(6,0,3,4),-1.000000,mvcvRect(7,0,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,0,2,8),-1.000000,mvcvRect(4,0,1,4),2.000000,mvcvRect(5,4,1,4),2.000000,
};
float MEMORY_TYPE feature_thres_19_10[2] =
{
-0.003950,-0.000399,
};
int MEMORY_TYPE feature_left_19_10[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_10[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_10[3] =
{
0.283181,0.548522,0.415970,
};
HaarFeature MEMORY_TYPE haar_19_11[2] =
{
0,mvcvRect(11,17,9,3),-1.000000,mvcvRect(14,17,3,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,2,4,5),-1.000000,mvcvRect(16,2,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_11[2] =
{
-0.011433,0.005334,
};
int MEMORY_TYPE feature_left_19_11[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_11[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_11[3] =
{
0.563910,0.459698,0.593124,
};
HaarFeature MEMORY_TYPE haar_19_12[2] =
{
0,mvcvRect(0,2,5,9),-1.000000,mvcvRect(0,5,5,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,2,3,2),-1.000000,mvcvRect(8,2,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_12[2] =
{
0.008319,-0.000425,
};
int MEMORY_TYPE feature_left_19_12[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_12[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_12[3] =
{
0.323062,0.379529,0.540861,
};
HaarFeature MEMORY_TYPE haar_19_13[2] =
{
0,mvcvRect(11,17,9,3),-1.000000,mvcvRect(14,17,3,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(16,2,4,5),-1.000000,mvcvRect(16,2,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_13[2] =
{
-0.111894,-0.007555,
};
int MEMORY_TYPE feature_left_19_13[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_13[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_13[3] =
{
0.113230,0.633937,0.483877,
};
HaarFeature MEMORY_TYPE haar_19_14[2] =
{
0,mvcvRect(0,17,9,3),-1.000000,mvcvRect(3,17,3,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,2,4,5),-1.000000,mvcvRect(2,2,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_14[2] =
{
-0.007034,-0.014834,
};
int MEMORY_TYPE feature_left_19_14[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_14[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_14[3] =
{
0.566526,0.675142,0.414095,
};
HaarFeature MEMORY_TYPE haar_19_15[2] =
{
0,mvcvRect(5,11,10,9),-1.000000,mvcvRect(5,14,10,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,6,3,3),-1.000000,mvcvRect(9,7,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_15[2] =
{
0.008751,0.001665,
};
int MEMORY_TYPE feature_left_19_15[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_15[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_15[3] =
{
0.356126,0.534728,0.364978,
};
HaarFeature MEMORY_TYPE haar_19_16[2] =
{
0,mvcvRect(3,17,5,3),-1.000000,mvcvRect(3,18,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,5,4,7),-1.000000,mvcvRect(9,5,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_16[2] =
{
0.009490,0.001113,
};
int MEMORY_TYPE feature_left_19_16[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_16[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_16[3] =
{
0.275466,0.422599,0.562918,
};
HaarFeature MEMORY_TYPE haar_19_17[2] =
{
0,mvcvRect(9,8,2,5),-1.000000,mvcvRect(9,8,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,2,18,2),-1.000000,mvcvRect(2,3,18,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_17[2] =
{
0.009494,-0.001540,
};
int MEMORY_TYPE feature_left_19_17[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_17[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_17[3] =
{
0.490604,0.400705,0.538071,
};
HaarFeature MEMORY_TYPE haar_19_18[2] =
{
0,mvcvRect(2,8,15,6),-1.000000,mvcvRect(7,8,5,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,8,2,5),-1.000000,mvcvRect(10,8,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_18[2] =
{
0.134350,-0.009494,
};
int MEMORY_TYPE feature_left_19_18[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_18[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_18[3] =
{
0.221467,0.735316,0.500503,
};
HaarFeature MEMORY_TYPE haar_19_19[2] =
{
0,mvcvRect(12,10,4,6),-1.000000,mvcvRect(12,12,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,3,6,2),-1.000000,mvcvRect(14,4,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_19[2] =
{
0.020012,-0.001888,
};
int MEMORY_TYPE feature_left_19_19[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_19[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_19[3] =
{
0.332791,0.391529,0.540185,
};
HaarFeature MEMORY_TYPE haar_19_20[2] =
{
0,mvcvRect(5,5,2,3),-1.000000,mvcvRect(5,6,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,6,3,3),-1.000000,mvcvRect(4,7,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_20[2] =
{
0.007184,0.001698,
};
int MEMORY_TYPE feature_left_19_20[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_20[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_20[3] =
{
0.717660,0.452698,0.607691,
};
HaarFeature MEMORY_TYPE haar_19_21[2] =
{
0,mvcvRect(14,12,3,3),-1.000000,mvcvRect(14,13,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,12,11,3),-1.000000,mvcvRect(6,13,11,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_21[2] =
{
0.004922,0.011803,
};
int MEMORY_TYPE feature_left_19_21[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_21[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_21[3] =
{
0.256983,0.499964,0.595823,
};
HaarFeature MEMORY_TYPE haar_19_22[2] =
{
0,mvcvRect(1,2,3,6),-1.000000,mvcvRect(1,4,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,0,4,7),-1.000000,mvcvRect(3,0,2,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_22[2] =
{
-0.009770,0.002117,
};
int MEMORY_TYPE feature_left_19_22[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_22[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_22[3] =
{
0.345909,0.451513,0.582972,
};
HaarFeature MEMORY_TYPE haar_19_23[2] =
{
0,mvcvRect(9,8,3,4),-1.000000,mvcvRect(10,8,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,9,2,2),-1.000000,mvcvRect(10,10,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_23[2] =
{
0.009480,-0.002608,
};
int MEMORY_TYPE feature_left_19_23[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_23[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_23[3] =
{
0.480739,0.346222,0.520159,
};
HaarFeature MEMORY_TYPE haar_19_24[2] =
{
0,mvcvRect(8,8,3,4),-1.000000,mvcvRect(9,8,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,4,10,10),-1.000000,mvcvRect(4,9,10,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_24[2] =
{
-0.005725,-0.008233,
};
int MEMORY_TYPE feature_left_19_24[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_24[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_24[3] =
{
0.659985,0.282183,0.512528,
};
HaarFeature MEMORY_TYPE haar_19_25[2] =
{
0,mvcvRect(9,10,3,2),-1.000000,mvcvRect(10,10,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,10,3,2),-1.000000,mvcvRect(9,11,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_25[2] =
{
0.000896,-0.000150,
};
int MEMORY_TYPE feature_left_19_25[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_25[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_25[3] =
{
0.488382,0.482992,0.542872,
};
HaarFeature MEMORY_TYPE haar_19_26[2] =
{
0,mvcvRect(8,10,3,2),-1.000000,mvcvRect(9,10,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,4,14,12),-1.000000,mvcvRect(2,4,7,6),2.000000,mvcvRect(9,10,7,6),2.000000,
};
float MEMORY_TYPE feature_thres_19_26[2] =
{
0.000485,-0.096193,
};
int MEMORY_TYPE feature_left_19_26[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_26[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_26[3] =
{
0.443460,0.225664,0.595623,
};
HaarFeature MEMORY_TYPE haar_19_27[2] =
{
0,mvcvRect(10,12,1,6),-1.000000,mvcvRect(10,15,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,3,8,16),-1.000000,mvcvRect(11,3,4,8),2.000000,mvcvRect(7,11,4,8),2.000000,
};
float MEMORY_TYPE feature_thres_19_27[2] =
{
-0.001105,-0.102150,
};
int MEMORY_TYPE feature_left_19_27[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_27[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_27[3] =
{
0.452722,0.284435,0.518645,
};
HaarFeature MEMORY_TYPE haar_19_28[2] =
{
0,mvcvRect(5,6,8,10),-1.000000,mvcvRect(5,6,4,5),2.000000,mvcvRect(9,11,4,5),2.000000,0,mvcvRect(6,2,8,8),-1.000000,mvcvRect(6,2,4,4),2.000000,mvcvRect(10,6,4,4),2.000000,
};
float MEMORY_TYPE feature_thres_19_28[2] =
{
0.003015,0.007613,
};
int MEMORY_TYPE feature_left_19_28[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_28[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_28[3] =
{
0.380900,0.571870,0.426256,
};
HaarFeature MEMORY_TYPE haar_19_29[2] =
{
0,mvcvRect(10,5,4,2),-1.000000,mvcvRect(12,5,2,1),2.000000,mvcvRect(10,6,2,1),2.000000,0,mvcvRect(12,4,3,3),-1.000000,mvcvRect(12,5,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_29[2] =
{
0.001520,-0.014197,
};
int MEMORY_TYPE feature_left_19_29[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_29[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_29[3] =
{
0.594272,0.773110,0.499765,
};
HaarFeature MEMORY_TYPE haar_19_30[2] =
{
0,mvcvRect(4,19,12,1),-1.000000,mvcvRect(8,19,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,2,3,1),-1.000000,mvcvRect(9,2,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_30[2] =
{
-0.013819,-0.000507,
};
int MEMORY_TYPE feature_left_19_30[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_30[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_30[3] =
{
0.668114,0.330561,0.474997,
};
HaarFeature MEMORY_TYPE haar_19_31[2] =
{
0,mvcvRect(13,17,4,3),-1.000000,mvcvRect(13,18,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,14,6,3),-1.000000,mvcvRect(7,15,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_31[2] =
{
-0.009354,-0.009477,
};
int MEMORY_TYPE feature_left_19_31[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_31[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_31[3] =
{
0.286093,0.618888,0.484210,
};
HaarFeature MEMORY_TYPE haar_19_32[2] =
{
0,mvcvRect(9,14,2,3),-1.000000,mvcvRect(9,15,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,15,6,3),-1.000000,mvcvRect(7,16,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_32[2] =
{
0.001692,0.000587,
};
int MEMORY_TYPE feature_left_19_32[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_32[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_32[3] =
{
0.607025,0.378269,0.536820,
};
HaarFeature MEMORY_TYPE haar_19_33[2] =
{
0,mvcvRect(10,18,3,2),-1.000000,mvcvRect(11,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,12,2,3),-1.000000,mvcvRect(14,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_33[2] =
{
-0.002583,-0.002731,
};
int MEMORY_TYPE feature_left_19_33[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_33[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_33[3] =
{
0.369021,0.385711,0.531811,
};
HaarFeature MEMORY_TYPE haar_19_34[2] =
{
0,mvcvRect(4,10,4,6),-1.000000,mvcvRect(4,12,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,13,3,2),-1.000000,mvcvRect(4,14,3,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_34[2] =
{
0.021872,-0.000015,
};
int MEMORY_TYPE feature_left_19_34[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_34[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_34[3] =
{
0.232701,0.556072,0.430141,
};
HaarFeature MEMORY_TYPE haar_19_35[2] =
{
0,mvcvRect(9,16,2,3),-1.000000,mvcvRect(9,17,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,18,3,2),-1.000000,mvcvRect(11,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_35[2] =
{
0.005358,0.005006,
};
int MEMORY_TYPE feature_left_19_35[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_35[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_35[3] =
{
0.676764,0.519490,0.361285,
};
HaarFeature MEMORY_TYPE haar_19_36[2] =
{
0,mvcvRect(7,18,3,2),-1.000000,mvcvRect(8,18,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,10,4,2),-1.000000,mvcvRect(1,11,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_36[2] =
{
-0.001903,-0.007851,
};
int MEMORY_TYPE feature_left_19_36[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_36[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_36[3] =
{
0.323785,0.119485,0.499172,
};
HaarFeature MEMORY_TYPE haar_19_37[2] =
{
0,mvcvRect(12,4,6,3),-1.000000,mvcvRect(12,5,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,4,1,3),-1.000000,mvcvRect(14,5,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_37[2] =
{
-0.002709,0.001414,
};
int MEMORY_TYPE feature_left_19_37[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_37[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_37[3] =
{
0.485496,0.487232,0.590358,
};
HaarFeature MEMORY_TYPE haar_19_38[2] =
{
0,mvcvRect(2,4,6,3),-1.000000,mvcvRect(2,5,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,4,1,3),-1.000000,mvcvRect(5,5,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_38[2] =
{
0.009030,-0.000979,
};
int MEMORY_TYPE feature_left_19_38[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_38[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_38[3] =
{
0.654732,0.584927,0.455423,
};
HaarFeature MEMORY_TYPE haar_19_39[2] =
{
0,mvcvRect(14,12,3,3),-1.000000,mvcvRect(14,13,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,12,2,3),-1.000000,mvcvRect(15,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_39[2] =
{
0.001398,0.000834,
};
int MEMORY_TYPE feature_left_19_39[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_39[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_39[3] =
{
0.406463,0.539954,0.415281,
};
HaarFeature MEMORY_TYPE haar_19_40[2] =
{
0,mvcvRect(3,16,4,3),-1.000000,mvcvRect(3,17,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,0,4,2),-1.000000,mvcvRect(8,1,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_40[2] =
{
0.010551,0.000088,
};
int MEMORY_TYPE feature_left_19_40[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_40[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_40[3] =
{
0.179668,0.425186,0.541352,
};
HaarFeature MEMORY_TYPE haar_19_41[2] =
{
0,mvcvRect(0,0,20,1),-1.000000,mvcvRect(0,0,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,3,4),-1.000000,mvcvRect(10,7,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_41[2] =
{
-0.041022,0.007507,
};
int MEMORY_TYPE feature_left_19_41[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_41[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_41[3] =
{
0.522812,0.485374,0.609344,
};
HaarFeature MEMORY_TYPE haar_19_42[2] =
{
0,mvcvRect(0,0,20,1),-1.000000,mvcvRect(10,0,10,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,3,4),-1.000000,mvcvRect(9,7,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_42[2] =
{
0.041022,-0.000540,
};
int MEMORY_TYPE feature_left_19_42[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_42[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_42[3] =
{
0.220502,0.569273,0.446876,
};
HaarFeature MEMORY_TYPE haar_19_43[2] =
{
0,mvcvRect(1,6,19,3),-1.000000,mvcvRect(1,7,19,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,7,4,2),-1.000000,mvcvRect(12,8,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_43[2] =
{
-0.068696,-0.001845,
};
int MEMORY_TYPE feature_left_19_43[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_43[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_43[3] =
{
0.148331,0.621128,0.496660,
};
HaarFeature MEMORY_TYPE haar_19_44[2] =
{
0,mvcvRect(7,8,3,3),-1.000000,mvcvRect(7,9,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,7,3,3),-1.000000,mvcvRect(8,7,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_44[2] =
{
-0.006096,-0.004207,
};
int MEMORY_TYPE feature_left_19_44[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_44[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_44[3] =
{
0.229467,0.640709,0.474856,
};
HaarFeature MEMORY_TYPE haar_19_45[2] =
{
0,mvcvRect(2,9,16,3),-1.000000,mvcvRect(2,10,16,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,4,2,12),-1.000000,mvcvRect(9,8,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_45[2] =
{
-0.000713,0.117568,
};
int MEMORY_TYPE feature_left_19_45[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_45[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_45[3] =
{
0.535494,0.513698,0.010596,
};
HaarFeature MEMORY_TYPE haar_19_46[2] =
{
0,mvcvRect(7,3,2,5),-1.000000,mvcvRect(8,3,1,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,7,2,3),-1.000000,mvcvRect(9,8,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_46[2] =
{
0.000059,-0.006317,
};
int MEMORY_TYPE feature_left_19_46[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_46[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_46[3] =
{
0.371180,0.171207,0.506176,
};
HaarFeature MEMORY_TYPE haar_19_47[2] =
{
0,mvcvRect(9,14,4,3),-1.000000,mvcvRect(9,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,8,6,4),-1.000000,mvcvRect(10,8,3,2),2.000000,mvcvRect(7,10,3,2),2.000000,
};
float MEMORY_TYPE feature_thres_19_47[2] =
{
0.014941,-0.002079,
};
int MEMORY_TYPE feature_left_19_47[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_47[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_47[3] =
{
0.672912,0.441065,0.544403,
};
HaarFeature MEMORY_TYPE haar_19_48[2] =
{
0,mvcvRect(9,7,2,2),-1.000000,mvcvRect(10,7,1,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,5,6,6),-1.000000,mvcvRect(7,5,2,6),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_48[2] =
{
-0.000707,-0.003125,
};
int MEMORY_TYPE feature_left_19_48[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_48[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_48[3] =
{
0.556891,0.502387,0.356241,
};
HaarFeature MEMORY_TYPE haar_19_49[2] =
{
0,mvcvRect(9,1,3,6),-1.000000,mvcvRect(10,1,1,6),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,5,12,2),-1.000000,mvcvRect(8,5,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_49[2] =
{
-0.000789,0.010180,
};
int MEMORY_TYPE feature_left_19_49[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_49[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_49[3] =
{
0.545679,0.554514,0.462231,
};
HaarFeature MEMORY_TYPE haar_19_50[2] =
{
0,mvcvRect(4,2,6,4),-1.000000,mvcvRect(6,2,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,8,2),-1.000000,mvcvRect(4,8,8,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_50[2] =
{
-0.002751,0.010601,
};
int MEMORY_TYPE feature_left_19_50[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_50[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_50[3] =
{
0.494254,0.296123,0.596434,
};
HaarFeature MEMORY_TYPE haar_19_51[2] =
{
0,mvcvRect(3,6,14,6),-1.000000,mvcvRect(10,6,7,3),2.000000,mvcvRect(3,9,7,3),2.000000,0,mvcvRect(3,6,14,3),-1.000000,mvcvRect(3,6,7,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_51[2] =
{
0.005147,0.076321,
};
int MEMORY_TYPE feature_left_19_51[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_51[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_51[3] =
{
0.549523,0.517396,0.294022,
};
HaarFeature MEMORY_TYPE haar_19_52[2] =
{
0,mvcvRect(0,5,2,2),-1.000000,mvcvRect(0,6,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,13,4,3),-1.000000,mvcvRect(8,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_52[2] =
{
-0.001503,0.012267,
};
int MEMORY_TYPE feature_left_19_52[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_52[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_52[3] =
{
0.310630,0.465115,0.684661,
};
HaarFeature MEMORY_TYPE haar_19_53[2] =
{
0,mvcvRect(13,0,3,20),-1.000000,mvcvRect(14,0,1,20),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(10,8,10,3),-1.000000,mvcvRect(10,9,10,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_53[2] =
{
-0.031119,0.028906,
};
int MEMORY_TYPE feature_left_19_53[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_53[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_53[3] =
{
0.522606,0.518224,0.270543,
};
HaarFeature MEMORY_TYPE haar_19_54[2] =
{
0,mvcvRect(4,0,3,20),-1.000000,mvcvRect(5,0,1,20),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,8,10,3),-1.000000,mvcvRect(0,9,10,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_54[2] =
{
0.047598,0.030809,
};
int MEMORY_TYPE feature_left_19_54[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_54[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_54[3] =
{
0.110951,0.493863,0.140411,
};
HaarFeature MEMORY_TYPE haar_19_55[2] =
{
0,mvcvRect(12,5,3,4),-1.000000,mvcvRect(13,5,1,4),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,7,12,4),-1.000000,mvcvRect(10,7,4,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_55[2] =
{
-0.000213,0.078970,
};
int MEMORY_TYPE feature_left_19_55[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_55[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_55[3] =
{
0.439236,0.521655,0.229411,
};
HaarFeature MEMORY_TYPE haar_19_56[2] =
{
0,mvcvRect(1,14,6,6),-1.000000,mvcvRect(1,14,3,3),2.000000,mvcvRect(4,17,3,3),2.000000,0,mvcvRect(1,17,6,2),-1.000000,mvcvRect(1,18,6,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_56[2] =
{
-0.010258,0.001260,
};
int MEMORY_TYPE feature_left_19_56[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_56[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_56[3] =
{
0.617665,0.523622,0.332897,
};
HaarFeature MEMORY_TYPE haar_19_57[2] =
{
0,mvcvRect(14,8,6,12),-1.000000,mvcvRect(17,8,3,6),2.000000,mvcvRect(14,14,3,6),2.000000,0,mvcvRect(18,5,2,2),-1.000000,mvcvRect(18,6,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_57[2] =
{
-0.033490,-0.000592,
};
int MEMORY_TYPE feature_left_19_57[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_57[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_57[3] =
{
0.486619,0.411641,0.539564,
};
HaarFeature MEMORY_TYPE haar_19_58[2] =
{
0,mvcvRect(3,16,4,2),-1.000000,mvcvRect(3,16,2,1),2.000000,mvcvRect(5,17,2,1),2.000000,0,mvcvRect(2,16,6,2),-1.000000,mvcvRect(4,16,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_58[2] =
{
0.000030,-0.000544,
};
int MEMORY_TYPE feature_left_19_58[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_58[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_58[3] =
{
0.561074,0.562139,0.346120,
};
HaarFeature MEMORY_TYPE haar_19_59[2] =
{
0,mvcvRect(14,8,6,12),-1.000000,mvcvRect(17,8,3,6),2.000000,mvcvRect(14,14,3,6),2.000000,0,mvcvRect(18,5,2,2),-1.000000,mvcvRect(18,6,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_59[2] =
{
-0.033490,-0.000592,
};
int MEMORY_TYPE feature_left_19_59[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_59[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_59[3] =
{
0.489676,0.430540,0.534071,
};
HaarFeature MEMORY_TYPE haar_19_60[2] =
{
0,mvcvRect(5,16,9,2),-1.000000,mvcvRect(8,16,3,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,14,6,6),-1.000000,mvcvRect(3,14,3,3),2.000000,mvcvRect(6,17,3,3),2.000000,
};
float MEMORY_TYPE feature_thres_19_60[2] =
{
0.002055,-0.004435,
};
int MEMORY_TYPE feature_left_19_60[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_60[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_60[3] =
{
0.554500,0.603854,0.374659,
};
HaarFeature MEMORY_TYPE haar_19_61[2] =
{
0,mvcvRect(14,8,6,12),-1.000000,mvcvRect(17,8,3,6),2.000000,mvcvRect(14,14,3,6),2.000000,0,mvcvRect(11,7,2,12),-1.000000,mvcvRect(11,11,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_61[2] =
{
-0.084170,0.006742,
};
int MEMORY_TYPE feature_left_19_61[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_61[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_61[3] =
{
0.500735,0.529810,0.471615,
};
HaarFeature MEMORY_TYPE haar_19_62[2] =
{
0,mvcvRect(0,8,6,12),-1.000000,mvcvRect(0,8,3,6),2.000000,mvcvRect(3,14,3,6),2.000000,0,mvcvRect(7,7,2,12),-1.000000,mvcvRect(7,11,2,4),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_62[2] =
{
0.010278,0.005880,
};
int MEMORY_TYPE feature_left_19_62[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_62[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_62[3] =
{
0.626938,0.515483,0.381304,
};
HaarFeature MEMORY_TYPE haar_19_63[2] =
{
0,mvcvRect(14,12,1,2),-1.000000,mvcvRect(14,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,13,8,1),-1.000000,mvcvRect(12,13,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_63[2] =
{
-0.000007,0.000824,
};
int MEMORY_TYPE feature_left_19_63[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_63[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_63[3] =
{
0.444024,0.469753,0.548550,
};
HaarFeature MEMORY_TYPE haar_19_64[2] =
{
0,mvcvRect(0,3,16,6),-1.000000,mvcvRect(0,6,16,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,4,8,2),-1.000000,mvcvRect(1,4,4,1),2.000000,mvcvRect(5,5,4,1),2.000000,
};
float MEMORY_TYPE feature_thres_19_64[2] =
{
-0.005527,0.000961,
};
int MEMORY_TYPE feature_left_19_64[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_64[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_64[3] =
{
0.551360,0.361864,0.583846,
};
HaarFeature MEMORY_TYPE haar_19_65[2] =
{
0,mvcvRect(14,12,1,2),-1.000000,mvcvRect(14,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(15,12,2,3),-1.000000,mvcvRect(15,13,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_65[2] =
{
0.002481,-0.001048,
};
int MEMORY_TYPE feature_left_19_65[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_65[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_65[3] =
{
0.252322,0.411726,0.539300,
};
HaarFeature MEMORY_TYPE haar_19_66[2] =
{
0,mvcvRect(8,16,3,3),-1.000000,mvcvRect(8,17,3,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,12,1,2),-1.000000,mvcvRect(5,13,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_66[2] =
{
-0.006129,0.000117,
};
int MEMORY_TYPE feature_left_19_66[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_66[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_66[3] =
{
0.672633,0.504119,0.360773,
};
HaarFeature MEMORY_TYPE haar_19_67[2] =
{
0,mvcvRect(13,4,3,15),-1.000000,mvcvRect(14,4,1,15),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(17,3,2,6),-1.000000,mvcvRect(18,3,1,3),2.000000,mvcvRect(17,6,1,3),2.000000,
};
float MEMORY_TYPE feature_thres_19_67[2] =
{
-0.039909,0.001586,
};
int MEMORY_TYPE feature_left_19_67[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_67[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_67[3] =
{
0.156374,0.489198,0.577985,
};
HaarFeature MEMORY_TYPE haar_19_68[2] =
{
0,mvcvRect(4,4,3,15),-1.000000,mvcvRect(5,4,1,15),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,3,2,6),-1.000000,mvcvRect(1,3,1,3),2.000000,mvcvRect(2,6,1,3),2.000000,
};
float MEMORY_TYPE feature_thres_19_68[2] =
{
-0.022690,0.002092,
};
int MEMORY_TYPE feature_left_19_68[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_68[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_68[3] =
{
0.218688,0.477158,0.609923,
};
HaarFeature MEMORY_TYPE haar_19_69[2] =
{
0,mvcvRect(7,15,12,4),-1.000000,mvcvRect(7,17,12,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,0,19,3),-1.000000,mvcvRect(1,1,19,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_69[2] =
{
-0.024715,-0.013419,
};
int MEMORY_TYPE feature_left_19_69[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_69[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_69[3] =
{
0.346400,0.363069,0.525220,
};
HaarFeature MEMORY_TYPE haar_19_70[2] =
{
0,mvcvRect(3,17,10,2),-1.000000,mvcvRect(3,17,5,1),2.000000,mvcvRect(8,18,5,1),2.000000,0,mvcvRect(2,5,10,15),-1.000000,mvcvRect(2,10,10,5),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_70[2] =
{
-0.006063,-0.002092,
};
int MEMORY_TYPE feature_left_19_70[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_70[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_70[3] =
{
0.666632,0.339955,0.503570,
};
HaarFeature MEMORY_TYPE haar_19_71[2] =
{
0,mvcvRect(13,8,3,4),-1.000000,mvcvRect(13,10,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(19,13,1,2),-1.000000,mvcvRect(19,14,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_71[2] =
{
0.025962,0.000179,
};
int MEMORY_TYPE feature_left_19_71[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_71[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_71[3] =
{
0.503680,0.541853,0.431898,
};
HaarFeature MEMORY_TYPE haar_19_72[2] =
{
0,mvcvRect(4,8,3,4),-1.000000,mvcvRect(4,10,3,2),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,13,1,2),-1.000000,mvcvRect(0,14,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_72[2] =
{
-0.003155,-0.001140,
};
int MEMORY_TYPE feature_left_19_72[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_72[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_72[3] =
{
0.722103,0.332097,0.502443,
};
HaarFeature MEMORY_TYPE haar_19_73[2] =
{
0,mvcvRect(12,7,2,12),-1.000000,mvcvRect(12,13,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,7,2,2),-1.000000,mvcvRect(15,7,1,1),2.000000,mvcvRect(14,8,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_19_73[2] =
{
-0.047840,0.000416,
};
int MEMORY_TYPE feature_left_19_73[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_73[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_73[3] =
{
0.193877,0.480219,0.573071,
};
HaarFeature MEMORY_TYPE haar_19_74[2] =
{
0,mvcvRect(5,3,8,2),-1.000000,mvcvRect(5,4,8,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,2,2,6),-1.000000,mvcvRect(0,4,2,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_74[2] =
{
-0.000442,0.001448,
};
int MEMORY_TYPE feature_left_19_74[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_74[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_74[3] =
{
0.426252,0.571917,0.406415,
};
HaarFeature MEMORY_TYPE haar_19_75[2] =
{
0,mvcvRect(18,2,2,12),-1.000000,mvcvRect(19,2,1,6),2.000000,mvcvRect(18,8,1,6),2.000000,0,mvcvRect(18,1,1,2),-1.000000,mvcvRect(18,2,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_75[2] =
{
0.015702,0.000278,
};
int MEMORY_TYPE feature_left_19_75[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_75[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_75[3] =
{
0.499573,0.528929,0.458173,
};
HaarFeature MEMORY_TYPE haar_19_76[2] =
{
0,mvcvRect(0,2,2,12),-1.000000,mvcvRect(0,2,1,6),2.000000,mvcvRect(1,8,1,6),2.000000,0,mvcvRect(1,1,1,2),-1.000000,mvcvRect(1,2,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_76[2] =
{
-0.002901,0.000208,
};
int MEMORY_TYPE feature_left_19_76[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_76[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_76[3] =
{
0.601215,0.505798,0.359943,
};
HaarFeature MEMORY_TYPE haar_19_77[2] =
{
0,mvcvRect(16,4,4,14),-1.000000,mvcvRect(18,4,2,7),2.000000,mvcvRect(16,11,2,7),2.000000,0,mvcvRect(10,14,1,6),-1.000000,mvcvRect(10,17,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_77[2] =
{
-0.051530,0.000172,
};
int MEMORY_TYPE feature_left_19_77[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_77[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_77[3] =
{
0.499180,0.467547,0.537477,
};
HaarFeature MEMORY_TYPE haar_19_78[2] =
{
0,mvcvRect(0,4,4,14),-1.000000,mvcvRect(0,4,2,7),2.000000,mvcvRect(2,11,2,7),2.000000,0,mvcvRect(9,14,1,6),-1.000000,mvcvRect(9,17,1,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_78[2] =
{
0.023614,-0.000564,
};
int MEMORY_TYPE feature_left_19_78[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_78[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_78[3] =
{
0.658648,0.385330,0.519604,
};
HaarFeature MEMORY_TYPE haar_19_79[2] =
{
0,mvcvRect(9,14,4,3),-1.000000,mvcvRect(9,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,12,2),-1.000000,mvcvRect(8,7,4,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_79[2] =
{
0.006690,-0.004879,
};
int MEMORY_TYPE feature_left_19_79[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_79[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_79[3] =
{
0.600424,0.329323,0.524524,
};
HaarFeature MEMORY_TYPE haar_19_80[2] =
{
0,mvcvRect(0,8,4,3),-1.000000,mvcvRect(0,9,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,2,2),-1.000000,mvcvRect(4,7,1,1),2.000000,mvcvRect(5,8,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_19_80[2] =
{
-0.006854,0.000999,
};
int MEMORY_TYPE feature_left_19_80[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_80[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_80[3] =
{
0.256591,0.461549,0.594243,
};
HaarFeature MEMORY_TYPE haar_19_81[2] =
{
0,mvcvRect(13,7,2,1),-1.000000,mvcvRect(13,7,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,4,4,5),-1.000000,mvcvRect(11,4,2,5),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_81[2] =
{
-0.000134,0.001017,
};
int MEMORY_TYPE feature_left_19_81[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_81[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_81[3] =
{
0.548738,0.457836,0.542693,
};
HaarFeature MEMORY_TYPE haar_19_82[2] =
{
0,mvcvRect(4,8,3,3),-1.000000,mvcvRect(5,8,1,3),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(0,3,8,1),-1.000000,mvcvRect(4,3,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_82[2] =
{
0.000912,0.001008,
};
int MEMORY_TYPE feature_left_19_82[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_82[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_82[3] =
{
0.393946,0.404979,0.552070,
};
HaarFeature MEMORY_TYPE haar_19_83[2] =
{
0,mvcvRect(13,7,2,1),-1.000000,mvcvRect(13,7,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,7,3,2),-1.000000,mvcvRect(15,7,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_83[2] =
{
-0.000131,0.000552,
};
int MEMORY_TYPE feature_left_19_83[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_83[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_83[3] =
{
0.487909,0.484494,0.551283,
};
HaarFeature MEMORY_TYPE haar_19_84[2] =
{
0,mvcvRect(5,7,2,1),-1.000000,mvcvRect(6,7,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(3,7,3,2),-1.000000,mvcvRect(4,7,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_84[2] =
{
-0.000121,-0.000015,
};
int MEMORY_TYPE feature_left_19_84[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_84[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_84[3] =
{
0.436797,0.642596,0.488183,
};
HaarFeature MEMORY_TYPE haar_19_85[2] =
{
0,mvcvRect(18,5,2,2),-1.000000,mvcvRect(18,6,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,14,2,2),-1.000000,mvcvRect(13,14,1,1),2.000000,mvcvRect(12,15,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_19_85[2] =
{
-0.000401,-0.000658,
};
int MEMORY_TYPE feature_left_19_85[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_85[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_85[3] =
{
0.537210,0.583455,0.486908,
};
HaarFeature MEMORY_TYPE haar_19_86[2] =
{
0,mvcvRect(0,5,2,2),-1.000000,mvcvRect(0,6,2,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,14,2,2),-1.000000,mvcvRect(6,14,1,1),2.000000,mvcvRect(7,15,1,1),2.000000,
};
float MEMORY_TYPE feature_thres_19_86[2] =
{
0.000622,0.001466,
};
int MEMORY_TYPE feature_left_19_86[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_86[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_86[3] =
{
0.382464,0.481349,0.696674,
};
HaarFeature MEMORY_TYPE haar_19_87[2] =
{
0,mvcvRect(7,12,6,5),-1.000000,mvcvRect(9,12,2,5),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,17,5,2),-1.000000,mvcvRect(12,18,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_87[2] =
{
-0.049548,0.001302,
};
int MEMORY_TYPE feature_left_19_87[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_87[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_87[3] =
{
0.053928,0.533746,0.416075,
};
HaarFeature MEMORY_TYPE haar_19_88[2] =
{
0,mvcvRect(1,11,6,3),-1.000000,mvcvRect(4,11,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,9,6,3),-1.000000,mvcvRect(4,9,3,3),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_88[2] =
{
-0.004491,0.001659,
};
int MEMORY_TYPE feature_left_19_88[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_88[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_88[3] =
{
0.599744,0.372719,0.511563,
};
HaarFeature MEMORY_TYPE haar_19_89[2] =
{
0,mvcvRect(12,7,2,12),-1.000000,mvcvRect(12,13,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(8,7,5,3),-1.000000,mvcvRect(8,8,5,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_89[2] =
{
0.006470,0.004981,
};
int MEMORY_TYPE feature_left_19_89[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_89[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_89[3] =
{
0.525204,0.525672,0.393441,
};
HaarFeature MEMORY_TYPE haar_19_90[2] =
{
0,mvcvRect(6,7,2,12),-1.000000,mvcvRect(6,13,2,6),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,2,9,18),-1.000000,mvcvRect(4,2,3,18),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_90[2] =
{
-0.038537,-0.282757,
};
int MEMORY_TYPE feature_left_19_90[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_90[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_90[3] =
{
0.206192,0.061883,0.492506,
};
HaarFeature MEMORY_TYPE haar_19_91[2] =
{
0,mvcvRect(12,17,5,2),-1.000000,mvcvRect(12,18,5,1),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,12,2),-1.000000,mvcvRect(4,7,6,2),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_91[2] =
{
-0.009030,-0.043866,
};
int MEMORY_TYPE feature_left_19_91[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_91[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_91[3] =
{
0.315759,0.203368,0.516477,
};
HaarFeature MEMORY_TYPE haar_19_92[2] =
{
0,mvcvRect(6,7,6,1),-1.000000,mvcvRect(8,7,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(7,3,3,2),-1.000000,mvcvRect(8,3,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_92[2] =
{
-0.004570,-0.002336,
};
int MEMORY_TYPE feature_left_19_92[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_92[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_92[3] =
{
0.661118,0.280779,0.496288,
};
HaarFeature MEMORY_TYPE haar_19_93[2] =
{
0,mvcvRect(9,4,3,1),-1.000000,mvcvRect(10,4,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(11,11,3,1),-1.000000,mvcvRect(12,11,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_93[2] =
{
0.005396,-0.002630,
};
int MEMORY_TYPE feature_left_19_93[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_93[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_93[3] =
{
0.514639,0.628449,0.495559,
};
HaarFeature MEMORY_TYPE haar_19_94[2] =
{
0,mvcvRect(8,4,3,1),-1.000000,mvcvRect(9,4,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,11,3,1),-1.000000,mvcvRect(7,11,1,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_94[2] =
{
-0.003858,0.001396,
};
int MEMORY_TYPE feature_left_19_94[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_94[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_94[3] =
{
0.148675,0.470134,0.632097,
};
HaarFeature MEMORY_TYPE haar_19_95[2] =
{
0,mvcvRect(12,13,6,6),-1.000000,mvcvRect(12,15,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(14,13,1,6),-1.000000,mvcvRect(14,15,1,2),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_95[2] =
{
-0.008870,-0.000706,
};
int MEMORY_TYPE feature_left_19_95[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_95[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_95[3] =
{
0.528682,0.464837,0.533321,
};
HaarFeature MEMORY_TYPE haar_19_96[2] =
{
0,mvcvRect(2,13,6,6),-1.000000,mvcvRect(2,15,6,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(1,5,18,1),-1.000000,mvcvRect(7,5,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_96[2] =
{
0.004265,0.061572,
};
int MEMORY_TYPE feature_left_19_96[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_96[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_96[3] =
{
0.508488,0.362963,0.875716,
};
HaarFeature MEMORY_TYPE haar_19_97[2] =
{
0,mvcvRect(4,7,12,2),-1.000000,mvcvRect(10,7,6,1),2.000000,mvcvRect(4,8,6,1),2.000000,0,mvcvRect(6,1,8,10),-1.000000,mvcvRect(10,1,4,5),2.000000,mvcvRect(6,6,4,5),2.000000,
};
float MEMORY_TYPE feature_thres_19_97[2] =
{
-0.004538,-0.004088,
};
int MEMORY_TYPE feature_left_19_97[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_97[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_97[3] =
{
0.485670,0.458412,0.542024,
};
HaarFeature MEMORY_TYPE haar_19_98[2] =
{
0,mvcvRect(3,13,4,3),-1.000000,mvcvRect(3,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,13,4,3),-1.000000,mvcvRect(6,14,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_98[2] =
{
0.006431,0.007046,
};
int MEMORY_TYPE feature_left_19_98[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_98[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_98[3] =
{
0.270730,0.505749,0.702652,
};
HaarFeature MEMORY_TYPE haar_19_99[2] =
{
0,mvcvRect(9,14,4,3),-1.000000,mvcvRect(9,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,9,2,3),-1.000000,mvcvRect(12,10,2,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_99[2] =
{
-0.002325,0.000060,
};
int MEMORY_TYPE feature_left_19_99[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_99[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_99[3] =
{
0.482728,0.424725,0.550876,
};
HaarFeature MEMORY_TYPE haar_19_100[2] =
{
0,mvcvRect(7,14,4,3),-1.000000,mvcvRect(7,15,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(9,0,2,1),-1.000000,mvcvRect(10,0,1,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_100[2] =
{
0.018085,0.000847,
};
int MEMORY_TYPE feature_left_19_100[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_100[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_100[3] =
{
0.810480,0.515462,0.351438,
};
HaarFeature MEMORY_TYPE haar_19_101[2] =
{
0,mvcvRect(5,0,10,5),-1.000000,mvcvRect(5,0,5,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,6,8,7),-1.000000,mvcvRect(6,6,4,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_101[2] =
{
-0.026931,-0.004235,
};
int MEMORY_TYPE feature_left_19_101[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_101[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_101[3] =
{
0.488689,0.462238,0.538248,
};
HaarFeature MEMORY_TYPE haar_19_102[2] =
{
0,mvcvRect(5,0,10,5),-1.000000,mvcvRect(10,0,5,5),2.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(6,6,8,7),-1.000000,mvcvRect(10,6,4,7),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_102[2] =
{
0.026947,0.004645,
};
int MEMORY_TYPE feature_left_19_102[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_102[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_102[3] =
{
0.636660,0.536851,0.376543,
};
HaarFeature MEMORY_TYPE haar_19_103[2] =
{
0,mvcvRect(5,9,10,8),-1.000000,mvcvRect(10,9,5,4),2.000000,mvcvRect(5,13,5,4),2.000000,0,mvcvRect(10,0,4,10),-1.000000,mvcvRect(12,0,2,5),2.000000,mvcvRect(10,5,2,5),2.000000,
};
float MEMORY_TYPE feature_thres_19_103[2] =
{
-0.006958,0.000876,
};
int MEMORY_TYPE feature_left_19_103[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_103[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_103[3] =
{
0.423469,0.467241,0.535068,
};
HaarFeature MEMORY_TYPE haar_19_104[2] =
{
0,mvcvRect(1,4,8,3),-1.000000,mvcvRect(1,5,8,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,4,8,3),-1.000000,mvcvRect(4,5,8,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_104[2] =
{
0.001610,-0.001285,
};
int MEMORY_TYPE feature_left_19_104[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_104[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_104[3] =
{
0.573276,0.548180,0.378459,
};
HaarFeature MEMORY_TYPE haar_19_105[2] =
{
0,mvcvRect(9,7,4,3),-1.000000,mvcvRect(9,8,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(12,8,3,12),-1.000000,mvcvRect(12,14,3,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_105[2] =
{
0.010244,0.000269,
};
int MEMORY_TYPE feature_left_19_105[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_105[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_105[3] =
{
0.515591,0.535319,0.438715,
};
HaarFeature MEMORY_TYPE haar_19_106[2] =
{
0,mvcvRect(7,7,4,3),-1.000000,mvcvRect(7,8,4,1),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(5,8,3,12),-1.000000,mvcvRect(5,14,3,6),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_106[2] =
{
0.003790,-0.029370,
};
int MEMORY_TYPE feature_left_19_106[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_106[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_106[3] =
{
0.503200,0.587354,0.221545,
};
HaarFeature MEMORY_TYPE haar_19_107[2] =
{
0,mvcvRect(10,0,7,6),-1.000000,mvcvRect(10,2,7,2),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(2,1,18,1),-1.000000,mvcvRect(8,1,6,1),3.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_107[2] =
{
0.006074,-0.012711,
};
int MEMORY_TYPE feature_left_19_107[2] =
{
1,-1,
};
int MEMORY_TYPE feature_right_19_107[2] =
{
0,-2,
};
float MEMORY_TYPE feature_alpha_19_107[3] =
{
0.541703,0.605651,0.498518,
};
HaarFeature MEMORY_TYPE haar_19_108[2] =
{
0,mvcvRect(5,0,3,8),-1.000000,mvcvRect(6,0,1,8),3.000000,mvcvRect(0,0,0,0),0.000000,0,mvcvRect(4,7,4,2),-1.000000,mvcvRect(4,8,4,1),2.000000,mvcvRect(0,0,0,0),0.000000,
};
float MEMORY_TYPE feature_thres_19_108[2] =
{
-0.005945,-0.002893,
};
int MEMORY_TYPE feature_left_19_108[2] =
{
0,-1,
};
int MEMORY_TYPE feature_right_19_108[2] =
{
1,-2,
};
float MEMORY_TYPE feature_alpha_19_108[3] =
{
0.335207,0.692924,0.477822,
};
HaarClassifier MEMORY_TYPE haar_feature_class_19[109] =
{
{2,haar_19_0,feature_thres_19_0,feature_left_19_0,feature_right_19_0,feature_alpha_19_0},{2,haar_19_1,feature_thres_19_1,feature_left_19_1,feature_right_19_1,feature_alpha_19_1},{2,haar_19_2,feature_thres_19_2,feature_left_19_2,feature_right_19_2,feature_alpha_19_2},{2,haar_19_3,feature_thres_19_3,feature_left_19_3,feature_right_19_3,feature_alpha_19_3},{2,haar_19_4,feature_thres_19_4,feature_left_19_4,feature_right_19_4,feature_alpha_19_4},{2,haar_19_5,feature_thres_19_5,feature_left_19_5,feature_right_19_5,feature_alpha_19_5},{2,haar_19_6,feature_thres_19_6,feature_left_19_6,feature_right_19_6,feature_alpha_19_6},{2,haar_19_7,feature_thres_19_7,feature_left_19_7,feature_right_19_7,feature_alpha_19_7},{2,haar_19_8,feature_thres_19_8,feature_left_19_8,feature_right_19_8,feature_alpha_19_8},{2,haar_19_9,feature_thres_19_9,feature_left_19_9,feature_right_19_9,feature_alpha_19_9},{2,haar_19_10,feature_thres_19_10,feature_left_19_10,feature_right_19_10,feature_alpha_19_10},{2,haar_19_11,feature_thres_19_11,feature_left_19_11,feature_right_19_11,feature_alpha_19_11},{2,haar_19_12,feature_thres_19_12,feature_left_19_12,feature_right_19_12,feature_alpha_19_12},{2,haar_19_13,feature_thres_19_13,feature_left_19_13,feature_right_19_13,feature_alpha_19_13},{2,haar_19_14,feature_thres_19_14,feature_left_19_14,feature_right_19_14,feature_alpha_19_14},{2,haar_19_15,feature_thres_19_15,feature_left_19_15,feature_right_19_15,feature_alpha_19_15},{2,haar_19_16,feature_thres_19_16,feature_left_19_16,feature_right_19_16,feature_alpha_19_16},{2,haar_19_17,feature_thres_19_17,feature_left_19_17,feature_right_19_17,feature_alpha_19_17},{2,haar_19_18,feature_thres_19_18,feature_left_19_18,feature_right_19_18,feature_alpha_19_18},{2,haar_19_19,feature_thres_19_19,feature_left_19_19,feature_right_19_19,feature_alpha_19_19},{2,haar_19_20,feature_thres_19_20,feature_left_19_20,feature_right_19_20,feature_alpha_19_20},{2,haar_19_21,feature_thres_19_21,feature_left_19_21,feature_right_19_21,feature_alpha_19_21},{2,haar_19_22,feature_thres_19_22,feature_left_19_22,feature_right_19_22,feature_alpha_19_22},{2,haar_19_23,feature_thres_19_23,feature_left_19_23,feature_right_19_23,feature_alpha_19_23},{2,haar_19_24,feature_thres_19_24,feature_left_19_24,feature_right_19_24,feature_alpha_19_24},{2,haar_19_25,feature_thres_19_25,feature_left_19_25,feature_right_19_25,feature_alpha_19_25},{2,haar_19_26,feature_thres_19_26,feature_left_19_26,feature_right_19_26,feature_alpha_19_26},{2,haar_19_27,feature_thres_19_27,feature_left_19_27,feature_right_19_27,feature_alpha_19_27},{2,haar_19_28,feature_thres_19_28,feature_left_19_28,feature_right_19_28,feature_alpha_19_28},{2,haar_19_29,feature_thres_19_29,feature_left_19_29,feature_right_19_29,feature_alpha_19_29},{2,haar_19_30,feature_thres_19_30,feature_left_19_30,feature_right_19_30,feature_alpha_19_30},{2,haar_19_31,feature_thres_19_31,feature_left_19_31,feature_right_19_31,feature_alpha_19_31},{2,haar_19_32,feature_thres_19_32,feature_left_19_32,feature_right_19_32,feature_alpha_19_32},{2,haar_19_33,feature_thres_19_33,feature_left_19_33,feature_right_19_33,feature_alpha_19_33},{2,haar_19_34,feature_thres_19_34,feature_left_19_34,feature_right_19_34,feature_alpha_19_34},{2,haar_19_35,feature_thres_19_35,feature_left_19_35,feature_right_19_35,feature_alpha_19_35},{2,haar_19_36,feature_thres_19_36,feature_left_19_36,feature_right_19_36,feature_alpha_19_36},{2,haar_19_37,feature_thres_19_37,feature_left_19_37,feature_right_19_37,feature_alpha_19_37},{2,haar_19_38,feature_thres_19_38,feature_left_19_38,feature_right_19_38,feature_alpha_19_38},{2,haar_19_39,feature_thres_19_39,feature_left_19_39,feature_right_19_39,feature_alpha_19_39},{2,haar_19_40,feature_thres_19_40,feature_left_19_40,feature_right_19_40,feature_alpha_19_40},{2,haar_19_41,feature_thres_19_41,feature_left_19_41,feature_right_19_41,feature_alpha_19_41},{2,haar_19_42,feature_thres_19_42,feature_left_19_42,feature_right_19_42,feature_alpha_19_42},{2,haar_19_43,feature_thres_19_43,feature_left_19_43,feature_right_19_43,feature_alpha_19_43},{2,haar_19_44,feature_thres_19_44,feature_left_19_44,feature_right_19_44,feature_alpha_19_44},{2,haar_19_45,feature_thres_19_45,feature_left_19_45,feature_right_19_45,feature_alpha_19_45},{2,haar_19_46,feature_thres_19_46,feature_left_19_46,feature_right_19_46,feature_alpha_19_46},{2,haar_19_47,feature_thres_19_47,feature_left_19_47,feature_right_19_47,feature_alpha_19_47},{2,haar_19_48,feature_thres_19_48,feature_left_19_48,feature_right_19_48,feature_alpha_19_48},{2,haar_19_49,feature_thres_19_49,feature_left_19_49,feature_right_19_49,feature_alpha_19_49},{2,haar_19_50,feature_thres_19_50,feature_left_19_50,feature_right_19_50,feature_alpha_19_50},{2,haar_19_51,feature_thres_19_51,feature_left_19_51,feature_right_19_51,feature_alpha_19_51},{2,haar_19_52,feature_thres_19_52,feature_left_19_52,feature_right_19_52,feature_alpha_19_52},{2,haar_19_53,feature_thres_19_53,feature_left_19_53,feature_right_19_53,feature_alpha_19_53},{2,haar_19_54,feature_thres_19_54,feature_left_19_54,feature_right_19_54,feature_alpha_19_54},{2,haar_19_55,feature_thres_19_55,feature_left_19_55,feature_right_19_55,feature_alpha_19_55},{2,haar_19_56,feature_thres_19_56,feature_left_19_56,feature_right_19_56,feature_alpha_19_56},{2,haar_19_57,feature_thres_19_57,feature_left_19_57,feature_right_19_57,feature_alpha_19_57},{2,haar_19_58,feature_thres_19_58,feature_left_19_58,feature_right_19_58,feature_alpha_19_58},{2,haar_19_59,feature_thres_19_59,feature_left_19_59,feature_right_19_59,feature_alpha_19_59},{2,haar_19_60,feature_thres_19_60,feature_left_19_60,feature_right_19_60,feature_alpha_19_60},{2,haar_19_61,feature_thres_19_61,feature_left_19_61,feature_right_19_61,feature_alpha_19_61},{2,haar_19_62,feature_thres_19_62,feature_left_19_62,feature_right_19_62,feature_alpha_19_62},{2,haar_19_63,feature_thres_19_63,feature_left_19_63,feature_right_19_63,feature_alpha_19_63},{2,haar_19_64,feature_thres_19_64,feature_left_19_64,feature_right_19_64,feature_alpha_19_64},{2,haar_19_65,feature_thres_19_65,feature_left_19_65,feature_right_19_65,feature_alpha_19_65},{2,haar_19_66,feature_thres_19_66,feature_left_19_66,feature_right_19_66,feature_alpha_19_66},{2,haar_19_67,feature_thres_19_67,feature_left_19_67,feature_right_19_67,feature_alpha_19_67},{2,haar_19_68,feature_thres_19_68,feature_left_19_68,feature_right_19_68,feature_alpha_19_68},{2,haar_19_69,feature_thres_19_69,feature_left_19_69,feature_right_19_69,feature_alpha_19_69},{2,haar_19_70,feature_thres_19_70,feature_left_19_70,feature_right_19_70,feature_alpha_19_70},{2,haar_19_71,feature_thres_19_71,feature_left_19_71,feature_right_19_71,feature_alpha_19_71},{2,haar_19_72,feature_thres_19_72,feature_left_19_72,feature_right_19_72,feature_alpha_19_72},{2,haar_19_73,feature_thres_19_73,feature_left_19_73,feature_right_19_73,feature_alpha_19_73},{2,haar_19_74,feature_thres_19_74,feature_left_19_74,feature_right_19_74,feature_alpha_19_74},{2,haar_19_75,feature_thres_19_75,feature_left_19_75,feature_right_19_75,feature_alpha_19_75},{2,haar_19_76,feature_thres_19_76,feature_left_19_76,feature_right_19_76,feature_alpha_19_76},{2,haar_19_77,feature_thres_19_77,feature_left_19_77,feature_right_19_77,feature_alpha_19_77},{2,haar_19_78,feature_thres_19_78,feature_left_19_78,feature_right_19_78,feature_alpha_19_78},{2,haar_19_79,feature_thres_19_79,feature_left_19_79,feature_right_19_79,feature_alpha_19_79},{2,haar_19_80,feature_thres_19_80,feature_left_19_80,feature_right_19_80,feature_alpha_19_80},{2,haar_19_81,feature_thres_19_81,feature_left_19_81,feature_right_19_81,feature_alpha_19_81},{2,haar_19_82,feature_thres_19_82,feature_left_19_82,feature_right_19_82,feature_alpha_19_82},{2,haar_19_83,feature_thres_19_83,feature_left_19_83,feature_right_19_83,feature_alpha_19_83},{2,haar_19_84,feature_thres_19_84,feature_left_19_84,feature_right_19_84,feature_alpha_19_84},{2,haar_19_85,feature_thres_19_85,feature_left_19_85,feature_right_19_85,feature_alpha_19_85},{2,haar_19_86,feature_thres_19_86,feature_left_19_86,feature_right_19_86,feature_alpha_19_86},{2,haar_19_87,feature_thres_19_87,feature_left_19_87,feature_right_19_87,feature_alpha_19_87},{2,haar_19_88,feature_thres_19_88,feature_left_19_88,feature_right_19_88,feature_alpha_19_88},{2,haar_19_89,feature_thres_19_89,feature_left_19_89,feature_right_19_89,feature_alpha_19_89},{2,haar_19_90,feature_thres_19_90,feature_left_19_90,feature_right_19_90,feature_alpha_19_90},{2,haar_19_91,feature_thres_19_91,feature_left_19_91,feature_right_19_91,feature_alpha_19_91},{2,haar_19_92,feature_thres_19_92,feature_left_19_92,feature_right_19_92,feature_alpha_19_92},{2,haar_19_93,feature_thres_19_93,feature_left_19_93,feature_right_19_93,feature_alpha_19_93},{2,haar_19_94,feature_thres_19_94,feature_left_19_94,feature_right_19_94,feature_alpha_19_94},{2,haar_19_95,feature_thres_19_95,feature_left_19_95,feature_right_19_95,feature_alpha_19_95},{2,haar_19_96,feature_thres_19_96,feature_left_19_96,feature_right_19_96,feature_alpha_19_96},{2,haar_19_97,feature_thres_19_97,feature_left_19_97,feature_right_19_97,feature_alpha_19_97},{2,haar_19_98,feature_thres_19_98,feature_left_19_98,feature_right_19_98,feature_alpha_19_98},{2,haar_19_99,feature_thres_19_99,feature_left_19_99,feature_right_19_99,feature_alpha_19_99},{2,haar_19_100,feature_thres_19_100,feature_left_19_100,feature_right_19_100,feature_alpha_19_100},{2,haar_19_101,feature_thres_19_101,feature_left_19_101,feature_right_19_101,feature_alpha_19_101},{2,haar_19_102,feature_thres_19_102,feature_left_19_102,feature_right_19_102,feature_alpha_19_102},{2,haar_19_103,feature_thres_19_103,feature_left_19_103,feature_right_19_103,feature_alpha_19_103},{2,haar_19_104,feature_thres_19_104,feature_left_19_104,feature_right_19_104,feature_alpha_19_104},{2,haar_19_105,feature_thres_19_105,feature_left_19_105,feature_right_19_105,feature_alpha_19_105},{2,haar_19_106,feature_thres_19_106,feature_left_19_106,feature_right_19_106,feature_alpha_19_106},{2,haar_19_107,feature_thres_19_107,feature_left_19_107,feature_right_19_107,feature_alpha_19_107},{2,haar_19_108,feature_thres_19_108,feature_left_19_108,feature_right_19_108,feature_alpha_19_108},
};
HaarStageClassifier MEMORY_TYPE stageClassifier[20] =
{
{3,0.350692,haar_feature_class_0,-1,1,-1},{9,3.472178,haar_feature_class_1,-1,2,0},{14,5.984489,haar_feature_class_2,-1,3,1},{19,8.511786,haar_feature_class_3,-1,4,2},{19,8.468016,haar_feature_class_4,-1,5,3},{27,12.578500,haar_feature_class_5,-1,6,4},{31,14.546750,haar_feature_class_6,-1,7,5},{39,18.572250,haar_feature_class_7,-1,8,6},{45,21.578119,haar_feature_class_8,-1,9,7},{47,22.585291,haar_feature_class_9,-1,10,8},{53,25.609301,haar_feature_class_10,-1,11,9},{67,32.647129,haar_feature_class_11,-1,12,10},{63,30.672131,haar_feature_class_12,-1,13,11},{71,34.677078,haar_feature_class_13,-1,14,12},{75,36.726501,haar_feature_class_14,-1,15,13},{78,38.236038,haar_feature_class_15,-1,16,14},{91,44.682968,haar_feature_class_16,-1,17,15},{97,47.763451,haar_feature_class_17,-1,18,16},{90,44.251282,haar_feature_class_18,-1,19,17},{109,53.755569,haar_feature_class_19,-1,-1,18},
};
HaarClassifierCascade MEMORY_TYPE cascade_aux =
{
1112539136,
20,
mvcvCvSize(20, 20),
mvcvCvSize(0, 0),
0,
&stageClassifier[0],
0,
};
#endif /*__HAAR_DETECTOR_H*/