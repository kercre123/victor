#include <mv_types.h>
#include <feature.h>
#include <string.h>
#include "stdio.h"
#include <stdlib.h>
#include "math.h"

#ifdef __cplusplus
extern "C"
namespace mvcv{
#endif

void canny(u8** srcAddr, u8** dstAddr,  float threshold1, float threshold2,  u32 width)
{

	//declaration for all necesary data
	u8* in_lines[9];
	u8  in_aux1[width], in_aux2[width], in_aux3[width], in_aux4[width], in_aux5[width];
	u8* out_line;
	fp32 gradient1[width], gradient2[width], gradient3[width];
	fp32 tangent1[width], tangent2[width], tangent3[width];
	fp32 aux_line1[width], aux_line2[width], aux_line3[width];
	int i,j;
	float sum;


	//init data for using
	fp32 sx=0, sy=0,s=0;
    out_line  = *dstAddr;
	fp32 interval1 = 0.4142135623730950488016887242097; //22.5
	fp32 interval2 = 2.4142135623730950488016887242097; //67.5
	fp32 interval3 =-2.4142135623730950488016887242097; //112.5
	fp32 interval4 =-0.4142135623730950488016887242097; //157.5

	//sobel matrix
	int VertSobel[3][3]={
			{-1, 0, 1},
			{-2, 0, 2},
			{-1, 0, 1}
	};

	int HorzSobel[3][3]={
			{-1, -2, -1},
			{ 0,  0,  0},
			{ 1,  2,  1}
	};

	//blur matrix
	int Blur[5][5]={
			{2, 4, 5, 4, 2},
			{4, 9,12, 9, 4},
			{5,12,15,12, 5},
			{4, 9,12, 9, 4},
			{2, 4, 5, 4, 2},
	};


	//init line pointers
	for(i=0;i<9;i++)
	{
		in_lines[i]=*(srcAddr+i);
	}


	//first step -blur
	for (i=2;i<width-2;i++)
	{
		sum=0.0f;

		sum       = in_lines[0][i-2] * Blur[0][0] + in_lines[0][i-1] * Blur[0][1] + in_lines[0][i] * Blur[0][2] + in_lines[0][i+1] * Blur[0][3] + in_lines[0][i+2] * Blur[0][4] +
				    in_lines[1][i-2] * Blur[1][0] + in_lines[1][i-1] * Blur[1][1] + in_lines[1][i] * Blur[1][2] + in_lines[1][i+1] * Blur[1][3] + in_lines[1][i+2] * Blur[1][4] +
				    in_lines[2][i-2] * Blur[2][0] + in_lines[2][i-1] * Blur[2][1] + in_lines[2][i] * Blur[2][2] + in_lines[2][i+1] * Blur[2][3] + in_lines[2][i+2] * Blur[2][4] +
				    in_lines[3][i-2] * Blur[3][0] + in_lines[3][i-1] * Blur[3][1] + in_lines[3][i] * Blur[3][2] + in_lines[3][i+1] * Blur[3][3] + in_lines[3][i+2] * Blur[3][4] +
				    in_lines[4][i-2] * Blur[4][0] + in_lines[4][i-1] * Blur[4][1] + in_lines[4][i] * Blur[4][2] + in_lines[4][i+1] * Blur[4][3] + in_lines[4][i+2] * Blur[4][4];
		in_aux1[i] = (u8)(sum/159.0f);

		sum       = in_lines[1][i-2] * Blur[0][0] + in_lines[1][i-1] * Blur[0][1] + in_lines[1][i] * Blur[0][2] + in_lines[1][i+1] * Blur[0][3] + in_lines[1][i+2] * Blur[0][4] +
				    in_lines[2][i-2] * Blur[1][0] + in_lines[2][i-1] * Blur[1][1] + in_lines[2][i] * Blur[1][2] + in_lines[2][i+1] * Blur[1][3] + in_lines[2][i+2] * Blur[1][4] +
				    in_lines[3][i-2] * Blur[2][0] + in_lines[3][i-1] * Blur[2][1] + in_lines[3][i] * Blur[2][2] + in_lines[3][i+1] * Blur[2][3] + in_lines[3][i+2] * Blur[2][4] +
				    in_lines[4][i-2] * Blur[3][0] + in_lines[4][i-1] * Blur[3][1] + in_lines[4][i] * Blur[3][2] + in_lines[4][i+1] * Blur[3][3] + in_lines[4][i+2] * Blur[3][4] +
				    in_lines[5][i-2] * Blur[4][0] + in_lines[5][i-1] * Blur[4][1] + in_lines[5][i] * Blur[4][2] + in_lines[5][i+1] * Blur[4][3] + in_lines[5][i+2] * Blur[4][4];
		in_aux2[i] = (u8)(sum/159.0f);

		sum       = in_lines[2][i-2] * Blur[0][0] + in_lines[2][i-1] * Blur[0][1] + in_lines[2][i] * Blur[0][2] + in_lines[2][i+1] * Blur[0][3] + in_lines[2][i+2] * Blur[0][4] +
				    in_lines[3][i-2] * Blur[1][0] + in_lines[3][i-1] * Blur[1][1] + in_lines[3][i] * Blur[1][2] + in_lines[3][i+1] * Blur[1][3] + in_lines[3][i+2] * Blur[1][4] +
				    in_lines[4][i-2] * Blur[2][0] + in_lines[4][i-1] * Blur[2][1] + in_lines[4][i] * Blur[2][2] + in_lines[4][i+1] * Blur[2][3] + in_lines[4][i+2] * Blur[2][4] +
				    in_lines[5][i-2] * Blur[3][0] + in_lines[5][i-1] * Blur[3][1] + in_lines[5][i] * Blur[3][2] + in_lines[5][i+1] * Blur[3][3] + in_lines[5][i+2] * Blur[3][4] +
				    in_lines[6][i-2] * Blur[4][0] + in_lines[6][i-1] * Blur[4][1] + in_lines[6][i] * Blur[4][2] + in_lines[6][i+1] * Blur[4][3] + in_lines[6][i+2] * Blur[4][4];
		in_aux3[i] = (u8)(sum/159.0f);


		sum       = in_lines[3][i-2] * Blur[0][0] + in_lines[3][i-1] * Blur[0][1] + in_lines[3][i] * Blur[0][2] + in_lines[3][i+1] * Blur[0][3] + in_lines[3][i+2] * Blur[0][4] +
				    in_lines[4][i-2] * Blur[1][0] + in_lines[4][i-1] * Blur[1][1] + in_lines[4][i] * Blur[1][2] + in_lines[4][i+1] * Blur[1][3] + in_lines[4][i+2] * Blur[1][4] +
				    in_lines[5][i-2] * Blur[2][0] + in_lines[5][i-1] * Blur[2][1] + in_lines[5][i] * Blur[2][2] + in_lines[5][i+1] * Blur[2][3] + in_lines[5][i+2] * Blur[2][4] +
				    in_lines[6][i-2] * Blur[3][0] + in_lines[6][i-1] * Blur[3][1] + in_lines[6][i] * Blur[3][2] + in_lines[6][i+1] * Blur[3][3] + in_lines[6][i+2] * Blur[3][4] +
				    in_lines[7][i-2] * Blur[4][0] + in_lines[7][i-1] * Blur[4][1] + in_lines[7][i] * Blur[4][2] + in_lines[7][i+1] * Blur[4][3] + in_lines[7][i+2] * Blur[4][4];
		in_aux4[i] = (u8)(sum/159.0f);

		sum       = in_lines[4][i-2] * Blur[0][0] + in_lines[4][i-1] * Blur[0][1] + in_lines[4][i] * Blur[0][2] + in_lines[4][i+1] * Blur[0][3] + in_lines[4][i+2] * Blur[0][4] +
				    in_lines[5][i-2] * Blur[1][0] + in_lines[5][i-1] * Blur[1][1] + in_lines[5][i] * Blur[1][2] + in_lines[5][i+1] * Blur[1][3] + in_lines[5][i+2] * Blur[1][4] +
				    in_lines[6][i-2] * Blur[2][0] + in_lines[6][i-1] * Blur[2][1] + in_lines[6][i] * Blur[2][2] + in_lines[6][i+1] * Blur[2][3] + in_lines[6][i+2] * Blur[2][4] +
				    in_lines[7][i-2] * Blur[3][0] + in_lines[7][i-1] * Blur[3][1] + in_lines[7][i] * Blur[3][2] + in_lines[7][i+1] * Blur[3][3] + in_lines[7][i+2] * Blur[3][4] +
				    in_lines[8][i-2] * Blur[4][0] + in_lines[8][i-1] * Blur[4][1] + in_lines[8][i] * Blur[4][2] + in_lines[8][i+1] * Blur[4][3] + in_lines[8][i+2] * Blur[4][4];
		in_aux5[i] = (u8)(sum/159.0f);

	}

	//second step -calculate gradient and tangent
	for(i=3;i<width-3;i++)
	{
		sx = 	 VertSobel[0][0]*in_aux1[i-1]+ VertSobel[0][1]*in_aux1[i] +VertSobel[0][2]*in_aux1[i+1]+
				 VertSobel[1][0]*in_aux2[i-1]+ VertSobel[1][1]*in_aux2[i] +VertSobel[1][2]*in_aux2[i+1]+
				 VertSobel[2][0]*in_aux3[i-1]+ VertSobel[2][1]*in_aux3[i] +VertSobel[2][2]*in_aux3[i+1];

		sy = 	 HorzSobel[0][0]*in_aux1[i-1]+ HorzSobel[0][1]*in_aux1[i] +HorzSobel[0][2]*in_aux1[i+1]+
				 HorzSobel[1][0]*in_aux2[i-1]+ HorzSobel[1][1]*in_aux2[i] +HorzSobel[1][2]*in_aux2[i+1]+
				 HorzSobel[2][0]*in_aux3[i-1]+ HorzSobel[2][1]*in_aux3[i] +HorzSobel[2][2]*in_aux3[i+1];

		s= sqrtf(sx*sx+ sy*sy);

		gradient1[i]=s;

		if (sy == 0) tangent1[i] = 2.5f;
		else tangent1[i] = sx/sy;


		//**************************************//
		sx = 	 VertSobel[0][0]*in_aux2[i-1]+ VertSobel[0][1]*in_aux2[i] +VertSobel[0][2]*in_aux2[i+1]+
				 VertSobel[1][0]*in_aux3[i-1]+ VertSobel[1][1]*in_aux3[i] +VertSobel[1][2]*in_aux3[i+1]+
				 VertSobel[2][0]*in_aux4[i-1]+ VertSobel[2][1]*in_aux4[i] +VertSobel[2][2]*in_aux4[i+1];

		sy = 	 HorzSobel[0][0]*in_aux2[i-1]+ HorzSobel[0][1]*in_aux2[i] +HorzSobel[0][2]*in_aux2[i+1]+
				 HorzSobel[1][0]*in_aux3[i-1]+ HorzSobel[1][1]*in_aux3[i] +HorzSobel[1][2]*in_aux3[i+1]+
				 HorzSobel[2][0]*in_aux4[i-1]+ HorzSobel[2][1]*in_aux4[i] +HorzSobel[2][2]*in_aux4[i+1];

		s= sqrtf(sx*sx+ sy*sy);

		gradient2[i]=s;

		if (sy == 0) tangent2[i] = 2.5f;
		else tangent2[i] = sx/sy;

		//**************************************//
		sx = 	 VertSobel[0][0]*in_aux3[i-1]+ VertSobel[0][1]*in_aux3[i] +VertSobel[0][2]*in_aux3[i+1]+
				 VertSobel[1][0]*in_aux4[i-1]+ VertSobel[1][1]*in_aux4[i] +VertSobel[1][2]*in_aux4[i+1]+
				 VertSobel[2][0]*in_aux5[i-1]+ VertSobel[2][1]*in_aux5[i] +VertSobel[2][2]*in_aux5[i+1];

		sy = 	 HorzSobel[0][0]*in_aux3[i-1]+ HorzSobel[0][1]*in_aux3[i] +HorzSobel[0][2]*in_aux3[i+1]+
				 HorzSobel[1][0]*in_aux4[i-1]+ HorzSobel[1][1]*in_aux4[i] +HorzSobel[1][2]*in_aux4[i+1]+
				 HorzSobel[2][0]*in_aux5[i-1]+ HorzSobel[2][1]*in_aux5[i] +HorzSobel[2][2]*in_aux5[i+1];

		s= sqrtf(sx*sx+ sy*sy);

		gradient3[i]=s;

		if (sy == 0) tangent3[i] = 2.5f;
		else tangent3[i] = sx/sy;

	}





	//third step - Non-Maximum Surpression
	for(i=4;i<width-4;i++)
	{
		//first line **************************************************************
		if (((tangent2[i]>=interval4)&&(tangent2[i]<=interval1)))
		{
			if (!((gradient2[i]>gradient1[i])&&(gradient2[i]>gradient3[i])))
			{
				gradient2[i]=0;
			}
		}

		if (((tangent2[i]>interval1)&&(tangent2[i]<interval2)))
		{
			if (!((gradient2[i]>gradient1[i+1])&&(gradient2[i]>gradient3[i-1])))
			{
				gradient2[i]=0;
			}
		}

		if (((tangent2[i]>=interval2)||(tangent2[i]<=interval3)))
		{
			if (!((gradient2[i]>gradient2[i+1])&&(gradient2[i]>gradient2[i-1])))
			{
				gradient2[i]=0;
			}
		}

		if (((tangent2[i]>interval3)&&(tangent2[i]<interval4)))
		{
			if(!((gradient2[i]>gradient1[i-1])&&(gradient2[i]>gradient3[i+1])))
			{
				gradient2[i]=0;
			}
		}

	}




	//forth step - Edge tracking by hysteresis
	for (i=4;i<width-4;i++)
	{
		if (gradient2[i]>=threshold2)
		{
			out_line[i]=255;
		}
		if ((gradient2[i]<threshold2)&&(gradient2[i]>threshold1))
		{
			if ((gradient1[i-1]>=threshold2)||(gradient1[i]>=threshold2)||(gradient1[i+1]>=threshold2)||(gradient2[i-1]>=threshold2)||(gradient2[i+1]>=threshold2)||(gradient3[i-1]>=threshold2)||(gradient3[i]>=threshold2)||(gradient3[i+1]>=threshold2))
			{
				out_line[i]=255;
			}
			else
				out_line[i]=0;
		}
		if(gradient2[i]<=threshold1)
		{
			out_line[i]=0;
		}
	}
out_line[0]=0;
out_line[1]=0;
out_line[2]=0;
out_line[3]=0;
out_line[width]=0;
out_line[width-1]=0;
out_line[width-2]=0;
out_line[width-3]=0;

	return;
}


/// CornerMinEigenVal_patched filter - is 5x5 kernel size
/// @param[in]  input_buffer   - pointer to input pixel
/// @param[in]  posy           - position on line
/// @param[out] out_pix        - pointer to output pixel    
/// @param[in]  width          - width of line
/// @param[in]  stride         -  if it not exists we can put it to 0

void CornerMinEigenVal_patched(u8 **in_lines, u32 posy, u8 *out_pix, u32 width)
{
	 fp32 dx1[5], dx2[5], dx3[5];
	 fp32 dy1[5], dy2[5], dy3[5];
	 fp32 dxx1[5], dxx2[5], dxx3[5];
	 fp32 dxy1[5], dxy2[5], dxy3[5];
	 fp32 dyy1[5], dyy2[5], dyy3[5];
	 fp32 a, b, c, xu, x, y, number;
	 u32 i, j;

		//sobel matrix
		fp32 VertSobel[3][3]={
				{-1, 0, 1},
				{-2, 0, 2},
				{-1, 0, 1}
		};

		fp32 HorzSobel[3][3]={
				{-1, -2, -1},
				{ 0,  0,  0},
				{ 1,  2,  1}
		};

		j=0;
		for (i=posy-1; i<posy + 2; i++)
		{
			dx1[j] =    VertSobel[0][0]*in_lines[0][i-1]+ VertSobel[0][1]*in_lines[0][i-0] +VertSobel[0][2]*in_lines[0][i+1]+
						VertSobel[1][0]*in_lines[1][i-1]+ VertSobel[1][1]*in_lines[1][i-0] +VertSobel[1][2]*in_lines[1][i+1]+
						VertSobel[2][0]*in_lines[2][i-1]+ VertSobel[2][1]*in_lines[2][i-0] +VertSobel[2][2]*in_lines[2][i+1];

			dy1[j] = 	HorzSobel[0][0]*in_lines[0][i-1]+ HorzSobel[0][1]*in_lines[0][i-0] +HorzSobel[0][2]*in_lines[0][i+1]+
					    HorzSobel[1][0]*in_lines[1][i-1]+ HorzSobel[1][1]*in_lines[1][i-0] +HorzSobel[1][2]*in_lines[1][i+1]+
					    HorzSobel[2][0]*in_lines[2][i-1]+ HorzSobel[2][1]*in_lines[2][i-0] +HorzSobel[2][2]*in_lines[2][i+1];

			dx2[j] =    VertSobel[0][0]*in_lines[1][i-1]+ VertSobel[0][1]*in_lines[1][i-0] +VertSobel[0][2]*in_lines[1][i+1]+
						VertSobel[1][0]*in_lines[2][i-1]+ VertSobel[1][1]*in_lines[2][i-0] +VertSobel[1][2]*in_lines[2][i+1]+
						VertSobel[2][0]*in_lines[3][i-1]+ VertSobel[2][1]*in_lines[3][i-0] +VertSobel[2][2]*in_lines[3][i+1];

			dy2[j] = 	HorzSobel[0][0]*in_lines[1][i-1]+ HorzSobel[0][1]*in_lines[1][i-0] +HorzSobel[0][2]*in_lines[1][i+1]+
					    HorzSobel[1][0]*in_lines[2][i-1]+ HorzSobel[1][1]*in_lines[2][i-0] +HorzSobel[1][2]*in_lines[2][i+1]+
					    HorzSobel[2][0]*in_lines[3][i-1]+ HorzSobel[2][1]*in_lines[3][i-0] +HorzSobel[2][2]*in_lines[3][i+1];

			dx3[j] =    VertSobel[0][0]*in_lines[2][i-1]+ VertSobel[0][1]*in_lines[2][i-0] +VertSobel[0][2]*in_lines[2][i+1]+
						VertSobel[1][0]*in_lines[3][i-1]+ VertSobel[1][1]*in_lines[3][i-0] +VertSobel[1][2]*in_lines[3][i+1]+
						VertSobel[2][0]*in_lines[4][i-1]+ VertSobel[2][1]*in_lines[4][i-0] +VertSobel[2][2]*in_lines[4][i+1];

			dy3[j] = 	HorzSobel[0][0]*in_lines[2][i-1]+ HorzSobel[0][1]*in_lines[2][i-0] +HorzSobel[0][2]*in_lines[2][i+1]+
					    HorzSobel[1][0]*in_lines[3][i-1]+ HorzSobel[1][1]*in_lines[3][i-0] +HorzSobel[1][2]*in_lines[3][i+1]+
					    HorzSobel[2][0]*in_lines[4][i-1]+ HorzSobel[2][1]*in_lines[4][i-0] +HorzSobel[2][2]*in_lines[4][i+1];
			dxx1[j] = dx1[j]*dx1[j];
			dxx2[j] = dx2[j]*dx2[j];
			dxx3[j] = dx3[j]*dx3[j];

			dyy1[j] = dy1[j]*dy1[j];
			dyy2[j] = dy2[j]*dy2[j];
			dyy3[j] = dy3[j]*dy3[j];


			dxy1[j] = dx1[j]* dy1[j];
			dxy2[j] = dx2[j]* dy2[j];
			dxy3[j] = dx3[j]* dy3[j];
			j++;
		}
	
		i = 1;
		a    = ((dxx1[i-1] + dxx1[i+0] + dxx1[i+1] +
			     dxx2[i-1] + dxx2[i+0] + dxx2[i+1] +
			     dxx3[i-1] + dxx3[i+0] + dxx3[i+1])/9.0f) * 0.5f;
	    b    =  (dxy1[i-1] + dxy1[i+0] + dxy1[i+1] +
			     dxy2[i-1] + dxy2[i+0] + dxy2[i+1] +
			     dxy3[i-1] + dxy3[i+0] + dxy3[i+1])/9.0f;
	    c    = ((dyy1[i-1] + dyy1[i+0] + dyy1[i+1] +
			     dyy2[i-1] + dyy2[i+0] + dyy2[i+1] +
			     dyy3[i-1] + dyy3[i+0] + dyy3[i+1])/9.0f)* 0.5f;
	    number = (a - c)*(a - c) + b*b;
		x = number * 0.5F;
		y  = number;
		j  = * ( u32 * ) &y;
		j  = 0x5f3759df - ( j >> 1 );
		y  = * ( float * ) &j;
		y  = y * ( 1.5 - ( x * y * y ) );
		y  = y * ( 1.5 - ( x * y * y ) );
		number =  number * y;
		xu = (a + c) - number;
		xu = xu > 255 ? 255 : xu;
		*out_pix = (u8)(xu);




	return;
}

/// CornerMinEigenVal filter - is 5x5 kernel size
/// @param[in]   input_lines   - pointer to input pixel
/// @param[out]  output_line   - position on line   
/// @param[in]   width         - width of line
void CornerMinEigenVal(u8 **in_lines, u8 **out_line, u32 width)
{
	
	 fp32 dx1[1280], dx2[1280], dx3[1280];
	 fp32 dy1[1280], dy2[1280], dy3[1280];
	 fp32 dxx1[1280], dxx2[1280], dxx3[1280];
	 fp32 dxy1[1280], dxy2[1280], dxy3[1280];
	 fp32 dyy1[1280], dyy2[1280], dyy3[1280];
	 fp32 a, b, c, xu, x, y, number;
	 u8    *output = *out_line;
	 u32 i, j;
	 
	 
	
	//sobel matrix
	fp32 VertSobel[3][3]={
			{-1, 0, 1},
			{-2, 0, 2},
			{-1, 0, 1}
	};

	fp32 HorzSobel[3][3]={
			{-1, -2, -1},
			{ 0,  0,  0},
			{ 1,  2,  1}
	};
	
	
	for (i=1; i<width-1; i++)
	{
		dx1[i] =    VertSobel[0][0]*in_lines[0][i-1]+ VertSobel[0][1]*in_lines[0][i-0] +VertSobel[0][2]*in_lines[0][i+1]+
					VertSobel[1][0]*in_lines[1][i-1]+ VertSobel[1][1]*in_lines[1][i-0] +VertSobel[1][2]*in_lines[1][i+1]+
					VertSobel[2][0]*in_lines[2][i-1]+ VertSobel[2][1]*in_lines[2][i-0] +VertSobel[2][2]*in_lines[2][i+1];
                                                                                                                 
		dy1[i] = 	HorzSobel[0][0]*in_lines[0][i-1]+ HorzSobel[0][1]*in_lines[0][i-0] +HorzSobel[0][2]*in_lines[0][i+1]+
				    HorzSobel[1][0]*in_lines[1][i-1]+ HorzSobel[1][1]*in_lines[1][i-0] +HorzSobel[1][2]*in_lines[1][i+1]+
				    HorzSobel[2][0]*in_lines[2][i-1]+ HorzSobel[2][1]*in_lines[2][i-0] +HorzSobel[2][2]*in_lines[2][i+1];
		                                                                                                         
		dx2[i] =    VertSobel[0][0]*in_lines[1][i-1]+ VertSobel[0][1]*in_lines[1][i-0] +VertSobel[0][2]*in_lines[1][i+1]+
					VertSobel[1][0]*in_lines[2][i-1]+ VertSobel[1][1]*in_lines[2][i-0] +VertSobel[1][2]*in_lines[2][i+1]+
					VertSobel[2][0]*in_lines[3][i-1]+ VertSobel[2][1]*in_lines[3][i-0] +VertSobel[2][2]*in_lines[3][i+1];
                                                                                                                 
		dy2[i] = 	HorzSobel[0][0]*in_lines[1][i-1]+ HorzSobel[0][1]*in_lines[1][i-0] +HorzSobel[0][2]*in_lines[1][i+1]+
				    HorzSobel[1][0]*in_lines[2][i-1]+ HorzSobel[1][1]*in_lines[2][i-0] +HorzSobel[1][2]*in_lines[2][i+1]+
				    HorzSobel[2][0]*in_lines[3][i-1]+ HorzSobel[2][1]*in_lines[3][i-0] +HorzSobel[2][2]*in_lines[3][i+1];
		                                                                                                         
		dx3[i] =    VertSobel[0][0]*in_lines[2][i-1]+ VertSobel[0][1]*in_lines[2][i-0] +VertSobel[0][2]*in_lines[2][i+1]+
					VertSobel[1][0]*in_lines[3][i-1]+ VertSobel[1][1]*in_lines[3][i-0] +VertSobel[1][2]*in_lines[3][i+1]+
					VertSobel[2][0]*in_lines[4][i-1]+ VertSobel[2][1]*in_lines[4][i-0] +VertSobel[2][2]*in_lines[4][i+1];
                                                                                                                 
		dy3[i] = 	HorzSobel[0][0]*in_lines[2][i-1]+ HorzSobel[0][1]*in_lines[2][i-0] +HorzSobel[0][2]*in_lines[2][i+1]+
				    HorzSobel[1][0]*in_lines[3][i-1]+ HorzSobel[1][1]*in_lines[3][i-0] +HorzSobel[1][2]*in_lines[3][i+1]+
				    HorzSobel[2][0]*in_lines[4][i-1]+ HorzSobel[2][1]*in_lines[4][i-0] +HorzSobel[2][2]*in_lines[4][i+1];	
		dxx1[i] = dx1[i]*dx1[i];
		dxx2[i] = dx2[i]*dx2[i];
		dxx3[i] = dx3[i]*dx3[i];
		
		dyy1[i] = dy1[i]*dy1[i];
		dyy2[i] = dy2[i]*dy2[i];
		dyy3[i] = dy3[i]*dy3[i];
		
		dxy1[i] = dx1[i]* dy1[i];
		dxy2[i] = dx2[i]* dy2[i];
		dxy3[i] = dx3[i]* dy3[i];
	}

	for (i=2; i<width-2;i++)
	{
		a    = ((dxx1[i-1] + dxx1[i+0] + dxx1[i+1] +
			     dxx2[i-1] + dxx2[i+0] + dxx2[i+1] +	
			     dxx3[i-1] + dxx3[i+0] + dxx3[i+1])/9.0f) * 0.5f;
	    b    =  (dxy1[i-1] + dxy1[i+0] + dxy1[i+1] +
			     dxy2[i-1] + dxy2[i+0] + dxy2[i+1] +	
			     dxy3[i-1] + dxy3[i+0] + dxy3[i+1])/9.0f;
	    c    = ((dyy1[i-1] + dyy1[i+0] + dyy1[i+1] +
			     dyy2[i-1] + dyy2[i+0] + dyy2[i+1] +	
			     dyy3[i-1] + dyy3[i+0] + dyy3[i+1])/9.0f)* 0.5f;
	    number = (a - c)*(a - c) + b*b;
		x = number * 0.5F;
		y  = number;
		j  = * ( u32 * ) &y;
		j  = 0x5f3759df - ( j >> 1 );
		y  = * ( float * ) &j;
		y  = y * ( 1.5 - ( x * y * y ) );
		y  = y * ( 1.5 - ( x * y * y ) );
		number =  number * y;
		xu = (a + c) - number;
		xu = xu > 255 ? 255 : xu;
		output[i] = (u8)(xu);
		
	}
	output[0] = 0;
	output[1] = 0;
	output[width-1] = 0;
	output[width-2]   = 0;
	return;
}

/// Fast9 - corner detection
/// @param[in]  in_lines   - array of pointers to input lines
/// @param[out] score      - pointer to corner score buffer
/// @param[out] base       - pointer to corner candidates buffer ; first element is the number of candidates, the rest are the position of coordinates
/// @param[in]  posy       - y position
/// @param[in]  thresh     - threshold
/// @param[in]  thresh     - number of pixels to process
#define adiff(a,b) ((a)>(b)?((a)-(b)):((b)-(a)))

void fast9(u8** in_lines, u8* score,  unsigned int *base, u32 posy, u32 thresh, u32 width)
{
	unsigned int counter, i, posx, offset;
	unsigned char  *row[7];
	unsigned int  *out;
	unsigned char sample[16], origin;
	int loLimit, hiLimit, loCount, hiCount, loScore, hiScore;
	unsigned int  bitMask, bitFlag;

	row[0]  = *(in_lines+0);
	row[1]  = *(in_lines+1);
	row[2]  = *(in_lines+2);
	row[3]  = *(in_lines+3);
	row[4]  = *(in_lines+4);
	row[5]  = *(in_lines+5);
	row[6]  = *(in_lines+6);
	
	offset  = 0;
	thresh  = thresh&0xFF;
	counter = 0;
	*base   = counter;
	out     = base; 
	out++;

	for (posx=offset; posx<(offset+width); posx++)
	{
		origin     = row[3][posx];
		sample[0]  = row[0][posx-1];
		sample[1]  = row[0][posx];
		sample[2]  = row[0][posx+1];
		sample[3]  = row[1][posx+2];
		sample[4]  = row[2][posx+3];
		sample[5]  = row[3][posx+3];
		sample[6]  = row[4][posx+3];
		sample[7]  = row[5][posx+2];
		sample[8]  = row[6][posx+1];
		sample[9]  = row[6][posx];
		sample[10] = row[6][posx-1];
		sample[11] = row[5][posx-2];
		sample[12] = row[4][posx-3];
		sample[13] = row[3][posx-3];
		sample[14] = row[2][posx-3];
		sample[15] = row[1][posx-2];

		score[posx-offset] = 0;

		if ( (adiff(origin,sample[1]) < (thresh+1)) && (adiff(origin,sample[9]) < (thresh+1)) )
		{
			continue; 
		}
		if ( (adiff(origin,sample[5]) < (thresh+1)) && (adiff(origin,sample[13]) < (thresh+1)) )
		{
			continue;
		}
		loLimit = origin-thresh; 
		hiLimit = origin+thresh;
		bitMask = 0;
		bitFlag = 0x01FF;
		loCount = 0;
		loScore = 0;
		hiCount = 0;
		hiScore = 0;
		for (i=0; i<16; i++)
		{
			if (sample[i] > hiLimit)
			{
				bitMask |= (1<<(16+i));
				hiScore += (sample[i]-hiLimit);
				hiCount++;
			} else
			if (sample[i] < loLimit)
			{
				bitMask |= (1<<i);
				loScore += (loLimit-sample[i]);
				loCount++;
			}

		}
		if (hiCount >loCount)
		{
			bitMask = bitMask>>16;
			loScore = hiScore;
		}else
		{
			bitMask = bitMask&0xFFFF;
		}
		loCount = 0;
		for (i=0; i<16; i++)
		{
			if ((bitMask&bitFlag) == bitFlag) loCount++;
			if (bitFlag&0x8000) bitFlag = ((bitFlag&0x7FFF)<<1)+1; else bitFlag = bitFlag<<1;
		}
		if (loCount != 0)
		{
			*out = posx | (posy<<16);
			out++;
			counter++;
			*base = counter;
			score[posx-offset] = (loScore>>4);
		}

	}

	return;
}

#ifdef __cplusplus
}
#endif
