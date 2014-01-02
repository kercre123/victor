#include <samplers.h>

#include <alloc.h>

#include <mvcv_types.h>
#include <kernels.h>
#include <mvstl_utils.h>

namespace mvcv
{

static inline const void*
	icvAdjustRect_tuned( const void* srcptr, int src_step, int pix_size,
	ClSizeW src_size, ClSizeW win_size,
	CvPointW ip, CvRect* pRect )
{
	CvRect rect;
	const char* src = (const char*)srcptr;

	if( ip.x >= 0 )
	{
		src += ip.x*pix_size;
		rect.x = 0;
	}
	else
	{
		rect.x = -ip.x;
		if( rect.x > win_size.x )
			rect.x = win_size.x;
	}

	if( ip.x + win_size.x < src_size.x )
		rect.width = win_size.x;
	else
	{
		rect.width = src_size.x - ip.x - 1;
		if( rect.width < 0 )
		{
			src += rect.width*pix_size;
			rect.width = 0;
		}
	}

	if( ip.y >= 0 )
	{
		src += ip.y * src_step;
		rect.y = 0;
	}
	else
		rect.y = -ip.y;

	if( ip.y + win_size.y < src_size.y )
		rect.height = win_size.y;
	else
	{
		rect.height = src_size.y - ip.y - 1;
		if( rect.height < 0 )
		{
			src += rect.height*src_step;
			rect.height = 0;
		}
	}

	*pRect = rect;
	return src - rect.x*pix_size;
}

//#define bilinearInterpolation16u bilinearInterpolation16u_asm

int icvGetRectSubPix_8u32f_C1R_tuned
	( const u8* src, int src_step, ClSizeW src_size,
	u8* dst, int dst_step, ClSizeW win_size, CvPoint2D32fW rect_center, bool inPlace)
{
	CvPointW ip;
	CvPoint2D32fW center;
	float  a12, a22, b1, b2;
	float a, b;
	float s = 0;
	int j,i;
	int capture = 0;
	int dstH = 0;
	int dstW = 0;
	u8* capDst = dst;

	center = rect_center;

	center.x -= (win_size.x-1)*0.5f;
	center.y -= (win_size.y-1)*0.5f;

	ip.x = cvFloor( center.x );
	ip.y = cvFloor( center.y );

	if( win_size.x <= 0 || win_size.y <= 0 )
		return CV_BADRANGE_ERR;

	src_step /= sizeof(src[0]);
	dst_step /= sizeof(dst[0]);

	//u8* local_src_buff = (u8*)mv_malloc((win_size.x + 1) * (win_size.y + 1));
	u8 local_src_buff[14 * 14]; // (win_size.x + 1) * (win_size.y + 1) to account for borders
	u8* src0;
	u8* src1;
	u8* src2;

	src0 = local_src_buff;
	src1 = local_src_buff + win_size.x + 1;
	src2 = local_src_buff + (win_size.x + 1) * 2;

	if( 0 <= ip.x && ip.x + win_size.x < src_size.x &&
		0 <= ip.y && ip.y + win_size.y < src_size.y )
	{
		//Extracted rectangle is completely inside the image

		src0 = local_src_buff;
		src1 = local_src_buff;

		src += ip.y * src_step + ip.x;
		 
		if (!inPlace)
		{
			// Copy full patch + 1 pixel border to the right and bottom for proper sampling
			smartCopy(src0, (u8*)src, (win_size.y + 1) * (win_size.x + 1), win_size.x + 1, src_step - win_size.x - 1);
			scDmaWaitFinished();

			rect_center.x = rect_center.x - (u32)(rect_center.x) + (win_size.x >> 1);
			rect_center.y = rect_center.y - (u32)(rect_center.y) + (win_size.y >> 1);

			if (win_size.x >= 5 && win_size.y >=5 ){
				ClSize aux;
				CvPoint2D32f auxf;
				aux.x=win_size.x;aux.y=win_size.y;
				auxf.x=rect_center.x;auxf.y=rect_center.y;
				bilinearInterpolation16u_asm(src1, win_size.x + 1, dst, dst_step, aux, auxf);
			}
			else{
				ClSize aux;
				CvPoint2D32f auxf;
				aux.x=win_size.x;aux.y=win_size.y;
				auxf.x=rect_center.x;auxf.y=rect_center.y;
				bilinearInterpolation16u(src1, win_size.x + 1, dst, dst_step, aux, auxf);
			}
		}
		else
		{
			if (win_size.x >= 5 && win_size.y >=5 ){
				ClSize aux;
				CvPoint2D32f auxf;
				aux.x=win_size.x;aux.y=win_size.y;
				auxf.x=rect_center.x;auxf.y=rect_center.y;
				bilinearInterpolation16u_asm(src, src_step, dst, dst_step, aux, auxf);
			}
			else{
				ClSize aux;
				CvPoint2D32f auxf;
				aux.x=win_size.x;aux.y=win_size.y;
				auxf.x=rect_center.x;auxf.y=rect_center.y;
				bilinearInterpolation16u(src, src_step, dst, dst_step, aux, auxf);
			}
		}
	}
	//The corner coordinates should take into account the restriction above
	else
	{
		CvRect r;

		src = (const u8*)icvAdjustRect_tuned( src, src_step*sizeof(*src),
			sizeof(*src), src_size, win_size,ip, &r);

		smartCopy(src0, (u8*)src, win_size.x  + 1);
		src += src_step;
		scDmaWaitFinished();
		smartCopy(src1, (u8*)src, win_size.x  + 1);

		// Compute weights before starting processing
		a = center.x - ip.x;
		b = center.y - ip.y;
		a = MAX(a,0.0001f);
		a12 = a*(1.f-b);
		a22 = a*b;
		b1 = 1.f - b;
		b2 = b;
		s = (1.f - a)/a;

		for( i = 0; i < win_size.y; i++, dst += dst_step )
		{
			const u8 *src_next = src + src_step;

			if( i < r.y || i >= r.height )
				src_next -= src_step;

			scDmaWaitFinished();
			smartCopy(src2, (u8*)src_next, win_size.x + 1);

			for( j = 0; j < r.x; j++ )
			{
				//float s0 = (src[r.x])*b1 + (src2[r.x])*b2;
				float s0 = src0[r.x] * b1 + src1[r.x] * b2;

				dst[j] = (u8)s0;
			}

			if( j < r.width )
			{
				float prev = (1 - a)*(b1*(src[j]) + b2*(src2[j]));

				for( ; j < r.width; j++ )
				{
					//float t = a12*(src[j+1]) + a22*(src2[j+1]);
					float t = a12 * src0[j+1] + a22 * src1[j+1];
					dst[j] = (u8)(prev + t);
					prev = t * s;
				}
			}

			for( ; j < win_size.x; j++ )
			{
				//float s0 = (src[r.width])*b1 + (src2[r.width])*b2;
				float s0 = src0[r.width] * b1 + src1[r.width] * b2;

				dst[j] = (u8)s0;
			}

			if( i < r.height )
				src = src_next;

			//Swizzle buffers
			u8* tmp = src0;
			src0 = src1;
			src1 = src2;
			src2 = tmp;
		}
	}

	//mv_free(local_src_buff);

	return CV_OK;
}

}
