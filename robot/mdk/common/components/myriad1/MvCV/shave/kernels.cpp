#include <kernels.h>

#include <mvcv_types.h>

namespace mvcv
{

CvPoint2D32f CalcBxBy(const u8* patchI, ClSize isz, const float* Ix, const float* Iy, CvPoint minI, 
	const u8* patchJ, ClSize jsz, CvPoint minJ, float& error)
{
	float bx = 0;
	float by = 0;
	
	error = 0;

	for (int y = 0; y < jsz.y; y++)
	{
		const u8* pi = patchI +
			(y + minJ.y - minI.y + 1) * isz.x + minJ.x - minI.x + 1;
		const u8* pj = patchJ + y * jsz.x;
		const float* ix = Ix +
			(y + minJ.y - minI.y) * (isz.x - 2) + minJ.x - minI.x;
		const float* iy = Iy + (ix - Ix);

		// Vectorize
		for (int x = 0; x < jsz.x; x++)
		{
			short t0 = pi[x] - pj[x];
			bx += t0 * ix[x];
			by += t0 * iy[x];

			error += t0 * t0;
		}
	}

	return cvPoint2D32f(bx, by);
}

void CalcG(const float* Ix, ClSize isz, CvPoint minI, const float* Iy, ClSize jsz, CvPoint minJ, Scalar& G)
{
	float Gxx = 0;
	float Gyy = 0;
	float Gxy = 0;
	float D = 0;

	//svu_assert(isz.x + minI.x <= 13 && isz.y + minI.y <= 13 && "isz wrong\n");
	//svu_assert(jsz.x + minJ.x <= 11 && jsz.y + minJ.y <= 11 && "jsz wrong\n");

	for (int y = 0; y < jsz.y; y++)
	{
		const float* ix = Ix +
			(y + minJ.y - minI.y) * (isz.x - 2) + minJ.x - minI.x;
		const float* iy = Iy + (ix - Ix);

		// Vectorize
		for(int x = 0; x < jsz.x; x++)
		{
			// Compute the gradients matrix (eq. 24)
			Gxx += ix[x] * ix[x];
			Gxy += ix[x] * iy[x];
			Gyy += iy[x] * iy[x];
		}
	}

	// Determinant of gradients matrix must not be 0 so the
	// matrix in (eq. 28) is invertible
	D = Gxx * Gyy - Gxy * Gxy;

	//return cvScalar(Gxx, Gyy, Gxy, D);
	G = cvScalar(Gxx, Gyy, Gxy, D);
}

void bilinearInterpolation16u(const u8* src, s32 src_step, u8* dst, s32 dst_step, ClSize win_size, CvPoint2D32f center)
{
	CvPoint ip;
	float  a12, a22, b1, b2;
	float a, b;
	float s = 0;
	const u8* src0;
	const u8* src1;

	// Translate center to top-left corner of the patch
	center.x -= (u32)((win_size.x - 1) * 0.5f);
	center.y -= (u32)((win_size.y - 1) * 0.5f);

	ip.x = cvFloor(center.x);
	ip.y = cvFloor(center.y);

	// Compute bilinear weights
	a = center.x - ip.x;
	b = center.y - ip.y;

	/*float c00 = (1.f - a) * (1.f - b);
	float c01 = (1.f - b) * a;
	float c10 = (1.f - a) * b;
	float c11 = a * b;*/

	u16 c00 = (u16)((1.f - a) * (1.f - b) * 256 + 0.5);
	u16 c01 = (u16)((1.f - b) * a * 256 + 0.5);
	u16 c10 = (u16)((1.f - a) * b * 256 + 0.5);
	u16 c11 = (u16)(a * b * 256 + 0.5);

	src0 = src;
	src1 = src0 + src_step;

	for (; win_size.y--; dst += dst_step, src0 = src1, src1 += src_step)
	{
		for (int j = 0; j < win_size.x; j++)
		{
			//float t = c00 * src0[j] + c01 * src0[j + 1] + c10 * src1[j] + c11 * src1[j + 1];
			u8 t = (u8)(((c00 * src0[j] + c01 * src0[j + 1] + c10 * src1[j] + c11 * src1[j + 1]) + 128) >> 8);
			dst[j] = t;
		}
	}
}

void conv3x3fp32_13x13( const u8* src, float* dstX, float* dstY, const float* smooth_k)
{
	const int src_width = 13;
	const int src_step  = 13;
	const int dst_step  = 11;
    int       height    = 11;
    u32 const  use_asm  = 1;
    int x;

    for( ; height--; src += src_step, dstX += dst_step, dstY += dst_step )
	{
            const u8* src2 = src + src_step;
            const u8* src3 = src + src_step*2;
            float b0[3];   // b0(x-2), b0(x-1), b0(x)
            float b1[3];   // b1(x-2), b1(x-1), b1(x)

#ifdef __MOVICOMPILE__
            if (use_asm) {
            	conv3x3fp32_13x13_asm(src, dstX, dstY, smooth_k);
            }
            else
#endif
            {
            	// calculate first couple of Y-axis scharr parts
				b0[2] = (src3[0] + src[0])*smooth_k[0] + src2[0]*smooth_k[1];
				b1[2] =  src3[0] - src[0];
				b0[1] = (src3[1] + src[1])*smooth_k[0] + src2[1]*smooth_k[1];
				b1[1] =  src3[1] - src[1];

				for( x = 2; x < src_width; x++ )
				{
					b0[0]      = (src3[x] + src[x])*smooth_k[0] + src2[x]*smooth_k[1];
					b1[0]      =  src3[x] - src[x];
					dstX[x-2]  = b0[0] - b0[2];
					dstY[x-2]  = (b1[2] +b1[0])*smooth_k[0] + b1[1]*smooth_k[1];

					b0[2] = b0[1];
					b0[1] = b0[0];
					b1[2] = b1[1];
					b1[1] = b1[0];
				}
            }
    }
}

/* compute dI/dx and dI/dy */
void conv3x3fp32(const u8* src, int src_step, float* dstX, float* dstY, int dst_step,
	ClSizeW src_size, const float* smooth_k, float* buffer0)
{
	int x;
	int src_width = src_size.x;
	int dst_width = src_size.x - 2;
	int height = src_size.y - 2;

	if (src_size.x == 13 && src_size.y == 13) {
		conv3x3fp32_13x13(src, dstX, dstY, smooth_k);
	}
	else
	{
		src_step /= sizeof(src[0]);
		dst_step /= sizeof(dstX[0]);

		for( ; height--; src += src_step, dstX += dst_step, dstY += dst_step )
		{
			const u8* src2 = src + src_step;
			const u8* src3 = src + src_step*2;
			float b0_m2, b0_m1, b0_m0;   // b0(x-2), b0(x-1), b0(x)
			float b1_m2, b1_m1, b1_m0;   // b1(x-2), b1(x-1), b1(x)

			// calculate first couple of Y-axis scharr parts
			b0_m2 = (src3[0] + src[0])*smooth_k[0] + src2[0]*smooth_k[1];
			b1_m2 =  src3[0] - src[0];
			b0_m1 = (src3[1] + src[1])*smooth_k[0] + src2[1]*smooth_k[1];
			b1_m1 =  src3[1] - src[1];

			for( x = 2; x < src_width; x++ )
			{
				b0_m0      = (src3[x] + src[x])*smooth_k[0] + src2[x]*smooth_k[1];
				b1_m0      =  src3[x] - src[x];
				dstX[x-2]  = b0_m0 - b0_m2;
				dstY[x-2]  = (b1_m2 +b1_m0)*smooth_k[0] + b1_m1*smooth_k[1];

				b0_m2 = b0_m1;
				b0_m1 = b0_m0;
				b1_m2 = b1_m1;
				b1_m1 = b1_m0;
			}
		}
	}
	// required?
	for( x = 0; x < src_width; x++)
	   buffer0[x] = 0;
}

}
