#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include <alloc.h> //must be after stdlib.h because it redefines realloc

#include <mat.h>
#include <imgproc.h>
#include <samplers.h>
#include <mvstl_utils.h>

namespace mvcv
{

#define FILT_2D_MAX_KERNEL_WIDTH 21
#define FILT_2D_MAX_KERNEL_HEIGHT 21
#define MED_FILT_MAX_KERNEL_WIDTH 15
#define MED_FILT_MAX_KERNEL_HEIGHT 15

int compare(const void* a, const void* b)
{
    return (*(u8*)a - *(u8*)b);
}

inline void absdiff_row(void* ts1, void* ts2, void* td, int w, int data_type)
{
    const float* fs1;
    const float* fs2;
    float* fd;
    const u8* us1;
    const u8* us2;
    u8* ud;

    //Move row pointers
    switch (data_type)
    {
    case CV_32F:
        fs1 = (const float*)ts1;
        fs2 = (const float*)ts2;
        fd = (float*)td;

        //Compute
        for (int j = 0; j < w; j++)
            fd[j] = fabs(fs1[j] - fs2[j]);
        break;

    case CV_8U:
        us1 = (const u8*)ts1;
        us2 = (const u8*)ts2;
        ud = (u8*)td;

        //Compute
        for (int j = 0; j < w; j++)
            ud[j] = fabs((float)(us1[j] - us2[j]));
        break;
    }
}

void absdiff(const Mat& src1, const Mat& src2, Mat& dst)
{
    int w = src1.cols;
    int h = src1.rows;

    //Check images have same size
    assert((src1.cols == src2.cols) && (src1.rows == src2.rows));
    assert((src2.cols == dst.cols) && (src2.rows == dst.rows));

    int data_type = CV_MAT_TYPE(src1.flags);

    for (int i = 0; i < h; i++)
    {
        void* ts1 = (void*)src1.ptr(i);
        void* ts2 = (void*)src2.ptr(i);
        void* td = (void*)dst.ptr(i);

        absdiff_row(ts1, ts2, td, w, data_type);
    }
}

Mat abs(Mat& src)
{
    float* s;
    int w = src.cols;
    int h = src.rows;

    for (int i = 0; i < h; i++)
    {
        s = (float*)src.ptr(i);

        for (int j = 0; j < w; j++)
            s[j] = fabs(s[j]);
    }

    return src;
}

inline void threshold_row(void* ts, void* td, int w, int thresh, int thresh_type, int data_type)
{
    const float* fs;
    float* fd;
    const u8* us;
    u8* ud;

    //Move row pointers
    switch (data_type)
    {
    case CV_32F:
        fs = (const float*)ts;
        fd = (float*)td;

        for (int j = 0; j < w; j++)
            switch (thresh_type)
            {
            case THRESH_TOZERO:
                if (fs[j] > thresh)
                    fd[j] = fs[j];
                else
                    fd[j] = 0.0;
            }
    break;

    case CV_8U:
        us = (const u8*)ts;
        ud = (u8*)td;

        for (int j = 0; j < w; j++)
            switch (thresh_type)
            {
            case THRESH_TOZERO:
                if (us[j] > thresh)
                    ud[j] = us[j];
                else
                    ud[j] = 0.0;
            }
    break;
    }
}

float threshold(const Mat& src, Mat& dst, float thresh, float maxval, int type)
{
    int w = src.cols;
    int h = src.rows;

    //Check images have same size
    assert((src.rows == dst.rows) && (src.cols == dst.cols));

    int data_type = CV_MAT_TYPE(src.flags);

    for (int i = 0; i < h; i++)
    {
        void* ts = (void*)src.ptr(i);
        void* td = (void*)dst.ptr(i);

        threshold_row(ts, td, w, thresh, type, data_type);
    }

    return thresh;
}

void (cv_sqrt)(const Mat& src, Mat& dst)
{
    const float* s;
    float* d;
    int w = src.cols;
    int h = src.rows;

    //Check images have same size
    assert((src.cols == dst.cols) && (src.rows == dst.rows));

    for (int i = 0; i < h; i++)
    {
        //Move row pointers
        s = (const float*)src.ptr(i);
        d = (float*)dst.ptr(i);

        //Compute
        for (int j = 0; j < w; j++)
            d[j] = sqrtf((float)s[j]);
    }
}


void divide(const Mat& src1, const Mat& src2, Mat& dst, float scale)
{
    const float* s1;
    const float* s2;
    float* d;
    int w = src1.cols;
    int h = src1.rows;

    //Check images have same size
    assert((src1.cols == src2.cols) && (src1.rows == src2.rows));
    assert((src2.cols == dst.cols) && (src2.rows == dst.rows));

    for (int i = 0; i < h; i++)
    {
        //Move row pointers
        s1 = (const float*)src1.ptr(i);
        s2 = (const float*)src2.ptr(i);
        d = (float*)dst.ptr(i);

        //Compute
        for (int j = 0; j < w; j++)
            d[j] = s1[j] * scale / s2[j];
    }
}

void minMaxLoc(const Mat& src, float* minVal, float* maxVal, Point* minLoc, Point* maxLoc, const Mat& mask)
{
    const float* s;
    const u8* m;
    int w = src.cols;
    int h = src.rows;
    float minV = 9999999;
    float maxV = 0;;
    Point minL;
    Point maxL;

    for (int i = 0; i < h; i++)
    {
        //Move row pointers
        s = (const float*)src.ptr(i);
        m = mask.ptr(i);

        //Compute
        for (int j = 0; j < w; j++)
        {
            if (m[j] != 0)
            {
                if (s[j] < minV)
                {
                    minV = s[j];
                    minL.x = j;
                    minL.y = i;
                }
                if (s[j] > maxV)
                {
                    maxV = s[j];
                    maxL.x = j;
                    maxL.y = i;
                }
            }
        }
    }

    *minVal = minV;
    if (maxVal != null)
        *maxVal = maxV;
    if (minLoc != null)
        *minLoc = minL;
    if (maxLoc != null)
        *maxLoc = maxL;
}

void filter2D(const Mat& src, Mat& dst, int ddepth, const Mat& kernel, Point anchor, float delta, int borderType)
{
    float* d;
    float* k[FILT_2D_MAX_KERNEL_WIDTH];
    float ls[FILT_2D_MAX_KERNEL_WIDTH];
    float* sr[FILT_2D_MAX_KERNEL_WIDTH];
    int w = src.cols;
    int h = src.rows;
    int kw = kernel.cols;
    int kh = kernel.rows;
    int kvl = kh / 2;
    float sum;
    float* brs[FILT_2D_MAX_KERNEL_WIDTH];
    int odd_even = kh & 1;

    //Initialize border buffer
    for (int i = 0; i < kvl; i++)
    {
        brs[i] = (float*)mv_malloc(w * sizeof(float));
        for (int j = 0; j < w; j++)
            brs[i][j] = 0.0;
    }

    //Initialize kernel rows
    for (int i = 0; i < kh; i++)
        k[i] = (float*)kernel.ptr(i);

    for (int i = 0; i < h; i++)
    {
        //Set first half of source rows
        for (int sri = 0; sri < kvl; sri++)
            sr[sri] = (i < kvl) ? brs[sri] : (float*)src.ptr(i + sri - kvl);

        //Set second half of source rows
        for (int sri = kvl; sri < kvl * 2 + odd_even; sri++)
            sr[sri] = (i + kvl > h) ? brs[i + kvl - h] : (float*)src.ptr(i + sri - kvl);

        d = (float*)dst.ptr(i);

        //Process every pixel on source current line
        for (int j = 0; j < w; j++)
        {
            //Reset line sums
            for (int lsi = 0; lsi < kvl * 2 + odd_even; lsi++)
                ls[lsi] = 0;

            //Convolve kernel with source image line by line
            for (int ki = 0; ki < kh; ki++)
                for (int kj = 0; kj < kw; kj++)
                    ls[kj] += sr[ki][j + kj] * k[ki][kj];

            //Sum up each partial sum
            sum = 0.0;
            for (int kj = 0; kj < kh; kj++)
                sum += ls[kj];

            d[j] = sum;
        }
    }

    // Free up memory
    for (int i = 0; i < kvl; i++)
    {
        mv_free(brs[i]);
    }
}

void make_border(u8* line_start, int line_width, int pad_width)
{
    u8* line_left = line_start;
    u8* line_right = line_start + line_width;

    for (int i = 0; i < pad_width; i++)
    {
        line_left[i] = 0;
        line_right[i] = 0;
    }
}

//#ifndef _WIN32
//extern "C" inline void median_row(u8* dst_row, u8* src_rows[], int line_width, int kernel_height);
//inline void median_row(u8* dst_row, u8* src_rows[], int line_width, int kernel_width = 5, int kernel_height = 5)
//{
//}
//#else
inline void median_row(u8* dst_row, u8* src_rows[], int line_width, int kernel_width = 3, int kernel_height = 3)
{
    u8 pixels[FILT_2D_MAX_KERNEL_HEIGHT * FILT_2D_MAX_KERNEL_WIDTH];
    //int kernel_width = 5;
    int src_start_pos = kernel_width / 2;
    int pixels_no =  kernel_width * kernel_height;

    for (int i = 0; i < line_width; i++)
    {
        for (int j = 0; j < kernel_height; j++)
            memcpy(pixels + j * kernel_width, src_rows[j] + src_start_pos + i, kernel_width);

        qsort(pixels, pixels_no, sizeof(u8), compare);

        dst_row[i] = pixels[pixels_no / 2];
    }
}
//#endif

void cvSmooth(const Mat& src, Mat& dst, int smoothtype, int kernel_width, int kernel_height, float param3, float param4, int slice_no, int lines_no)
{
    int kw = kernel_width; //kernel width
    int kh = kernel_height; //kernel height
    int pad_cols = kw / 2;
    int kh_center = kh / 2;
    int line_w = src.cols;
    int line_w_pad = src.cols + pad_cols * 2;
    u8* src_buff = (u8*)mv_malloc(line_w_pad * kh);
    u8* dst_buff = (u8*)mv_malloc(line_w);
    u8* line_ptrs[MED_FILT_MAX_KERNEL_HEIGHT];
    u8* src_rows[MED_FILT_MAX_KERNEL_HEIGHT];

    assert(kh <= MED_FILT_MAX_KERNEL_HEIGHT);

    //Prepare lines for first run
    for (int i = 0; i < kh; i++)
    {
        //Set line pointers into allocated local buffer
        line_ptrs[i] = src_buff + i * line_w_pad;

        if (slice_no * lines_no + i - kh_center < 0)
        {
            //Border padding for top rows
            memset(line_ptrs[i], 0, line_w_pad);
        }
        else
        {
            //Bring first source lines to local buffer
            dma_copy(line_ptrs[i] + pad_cols, (u8*)src.ptr(slice_no * lines_no + i - kh_center), line_w);
            make_border(line_ptrs[i], line_w, pad_cols);
        }
    }

    for (int i = 0; i < lines_no; i++)
    {
        //Copy row pointers in top-bottom order to pass as argument
        for (int j = 0; j < kh; j++)
            src_rows[j] = line_ptrs[(i + j) % kh];

        //Apply median filter on this scanline
        median_row(dst_buff, src_rows, line_w, kw, kh);

        //Evict result line to destination buffer
        dma_copy(dst.ptr(slice_no * lines_no + i), dst_buff, line_w);

        //Bring source line to local scratch buffer
        dma_copy(line_ptrs[(i + kh) % kh] + pad_cols, (u8*)src.ptr(slice_no * lines_no + i + kh_center), line_w);

        //Check if the kernel reached past the bottom of the frame and make bottom or lateral border
        if (slice_no * lines_no + i + kh_center > src.rows)
            memset(line_ptrs[(i + kh_center) % kh], 0, line_w_pad);
        else
            make_border(line_ptrs[i % kh], line_w, pad_cols);
    }

    mv_free(src_buff);
    mv_free(dst_buff);
}

/* lightweight convolution with 3x3 kernel */
void icvSepConvSmall3_32f( float* src, int src_step, float* dst, int dst_step,
            ClSize src_size, const float* kx, const float* ky, float* buffer )
{
    int  dst_width, buffer_step = 0;
    int  x, y;

    assert( src && dst && src_size.x > 2 && src_size.y > 2 &&
            (src_step & 3) == 0 && (dst_step & 3) == 0 &&
            (kx || ky) && (buffer || !kx || !ky));

    src_step /= sizeof(src[0]);
    dst_step /= sizeof(dst[0]);

    dst_width = src_size.x - 2;

    if( !kx )
    {
        /* set vars, so that vertical convolution
           will write results into destination ROI and
           horizontal convolution won't run */
        src_size.x = dst_width;
        buffer_step = dst_step;
        buffer = dst;
        dst_width = 0;
    }

    assert( src_step >= src_size.x && dst_step >= dst_width );

    src_size.y -= 3;
    if( !ky )
    {
        /* set vars, so that vertical convolution won't run and
           horizontal convolution will write results into destination ROI */
        src_size.y += 3;
        buffer_step = src_step;
        buffer = src;
        src_size.x = 0;
    }

    for( y = 0; y <= src_size.y; y++, src += src_step,
                                           dst += dst_step,
                                           buffer += buffer_step )
    {
        float* src2 = src + src_step;
        float* src3 = src + src_step*2;
        for( x = 0; x < src_size.x; x++ )
        {
            buffer[x] = (float)(ky[0]*src[x] + ky[1]*src2[x] + ky[2]*src3[x]);
        }

        for( x = 0; x < dst_width; x++ )
        {
            dst[x] = (float)(kx[0]*buffer[x] + kx[1]*buffer[x+1] + kx[2]*buffer[x+2]);
        }
    }
}
//This function isn't called anywhere in MDK and it should be moved to use the int4 types
//Since it's not called anywhere we jsut comment it out alltogether
#if 0
void cvFindCornerSubPix(const u8* srcarr, s32 srcStep, CvPoint2D32f* corners, int count, 
    ClSize win, ClSize zeroZone, CvTermCriteria criteria, bool inPlace)
{
    float* buffer;
    
    const int MAX_ITERS = 100;
    const float drv_x[] = { -1.f, 0.f, 1.f };
    const float drv_y[] = { 0.f, 0.5f, 0.f };
    float *maskX;
    float *maskY;
    float *mask;
    float *src_buffer;
    float *gx_buffer;
    float *gy_buffer;
    int win_w = win.x * 2 + 1, win_h = win.y * 2 + 1;
    int win_rect_size = (win_w + 4) * (win_h + 4);
    float coeff;
    ClSize size, src_buf_size;
    int i, j, k, pt_i;
    int max_iters = 10;
    float eps = 0;

    // TODO: replace checks with asserts

    const u8* src = srcarr;

    /*if( CV_MAT_TYPE( src->type ) != CV_8UC1 )
        CV_Error( CV_StsBadMask, "" );

    if( !corners )
        CV_Error( CV_StsNullPtr, "" );

    if( count < 0 )
        CV_Error( CV_StsBadSize, "" );

    if( count == 0 )
        return;

    if( win.width <= 0 || win.height <= 0 )
        CV_Error( CV_StsBadSize, "" );

    size = cvGetMatSize( src );

    if( size.width < win_w + 4 || size.height < win_h + 4 )
        CV_Error( CV_StsBadSize, "" );*/

    /* initialize variables, controlling loop termination */
    switch( criteria.type )
    {
    case CV_TERMCRIT_ITER:
        eps = 0.f;
        max_iters = criteria.max_iter;
        break;
    case CV_TERMCRIT_EPS:
        eps = criteria.epsilon;
        max_iters = MAX_ITERS;
        break;
    case CV_TERMCRIT_ITER | CV_TERMCRIT_EPS:
        eps = criteria.epsilon;
        max_iters = criteria.max_iter;
        break;
    default:
        assert( 0 );
    }

    eps = MAX( eps, 0 );
    eps *= eps;                 /* use square of error in comparsion operations. */

    max_iters = MAX( max_iters, 1 );
    max_iters = MIN( max_iters, MAX_ITERS );

    // TODO: replace with mv_malloc
    //buffer.allocate( win_rect_size * 5 + win_w + win_h + 32 );

    /* assign pointers */
    maskX = buffer;
    maskY = maskX + win_w + 4;
    mask = maskY + win_h + 4;
    src_buffer = mask + win_w * win_h;
    gx_buffer = src_buffer + win_rect_size;
    gy_buffer = gx_buffer + win_rect_size;

    coeff = 1. / (win.x * win.x);

    /* calculate mask */
    for( i = -win.x, k = 0; i <= win.x; i++, k++ )
    {
        maskX[k] = (float)exp( -i * i * coeff );
    }

    if( win.x == win.y )
    {
        maskY = maskX;
    }
    else
    {
        coeff = 1. / (win.y * win.y);
        for( i = -win.y, k = 0; i <= win.y; i++, k++ )
        {
            maskY[k] = (float) exp( -i * i * coeff );
        }
    }

    for( i = 0; i < win_h; i++ )
    {
        for( j = 0; j < win_w; j++ )
        {
            mask[i * win_w + j] = maskX[j] * maskY[i];
        }
    }


    /* make zero_zone */
    if( zeroZone.x >= 0 && zeroZone.y >= 0 &&
        zeroZone.x * 2 + 1 < win_w && zeroZone.y * 2 + 1 < win_h )
    {
        for( i = win.y - zeroZone.y; i <= win.y + zeroZone.y; i++ )
        {
            for( j = win.x - zeroZone.x; j <= win.x + zeroZone.x; j++ )
            {
                mask[i * win_w + j] = 0;
            }
        }
    }

    /* set sizes of image rectangles, used in convolutions */
    src_buf_size.x = win_w + 2;
    src_buf_size.y = win_h + 2;

    /* do optimization loop for all the points */
    for( pt_i = 0; pt_i < count; pt_i++ )
    {
        CvPoint2D32f cT = corners[pt_i], cI = cT;
        int iter = 0;
        float err;

        do
        {
            CvPoint2D32f cI2;
            float a, b, c, bb1, bb2;

            icvGetRectSubPix_8u32f_C1R_tuned(src, srcStep, size,
                                        (u8*)src_buffer, (win_w + 2) * sizeof( src_buffer[0] ),
                                        clSize( win_w + 2, win_h + 2 ), cI, inPlace);

            /* calc derivatives */
            icvSepConvSmall3_32f( src_buffer, src_buf_size.x * sizeof(src_buffer[0]),
                                  gx_buffer, win_w * sizeof(gx_buffer[0]),
                                  src_buf_size, drv_x, drv_y, buffer );

            icvSepConvSmall3_32f( src_buffer, src_buf_size.x * sizeof(src_buffer[0]),
                                  gy_buffer, win_w * sizeof(gy_buffer[0]),
                                  src_buf_size, drv_y, drv_x, buffer );

            a = b = c = bb1 = bb2 = 0;

            /* process gradient */
            for( i = 0, k = 0; i < win_h; i++ )
            {
                float py = i - win.y;

                for( j = 0; j < win_w; j++, k++ )
                {
                    float m = mask[k];
                    float tgx = gx_buffer[k];
                    float tgy = gy_buffer[k];
                    float gxx = tgx * tgx * m;
                    float gxy = tgx * tgy * m;
                    float gyy = tgy * tgy * m;
                    float px = j - win.x;

                    a += gxx;
                    b += gxy;
                    c += gyy;

                    bb1 += gxx * px + gxy * py;
                    bb2 += gxy * px + gyy * py;
                }
            }

            {
                float A[4];
                float InvA[4];
                CvMat matA, matInvA;

                A[0] = a;
                A[1] = A[2] = b;
                A[3] = c;

                /*cvInitMatHeader( &matA, 2, 2, CV_64F, A );
                cvInitMatHeader( &matInvA, 2, 2, CV_64FC1, InvA );

                cvInvert( &matA, &matInvA, CV_SVD );
                cI2.x = (float)(cI.x + InvA[0]*bb1 + InvA[1]*bb2);
                cI2.y = (float)(cI.y + InvA[2]*bb1 + InvA[3]*bb2);*/
            }

            err = (cI2.x - cI.x) * (cI2.x - cI.x) + (cI2.y - cI.y) * (cI2.y - cI.y);
            cI = cI2;
        }
        while( ++iter < max_iters && err > eps );

        /* if new point is too far from initial, it means poor convergence.
           leave initial point as the result */
        if( fabs( cI.x - cT.x ) > win.x || fabs( cI.y - cT.y ) > win.y )
        {
            cI = cT;
        }

        corners[pt_i] = cI;     /* store result */
    }

}
#endif
void thresholdKernel(u8** in, u8** out, u32 width, u32 height, u8 thresh, u32 thresh_type)

{
    u32 i, j;
    u8* in_1;

    in_1 = *in;


    u8 max_value = 0xff;

    for(i = 0; i < height ; i++)
    {
        for(j = 0; j < width; j++)
        {
            switch(thresh_type)
            {
                case Thresh_To_Zero:
                    if ((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = (in_1[j+i*width]);
                    }
                    else
                    {
                        out[i][j] = 0;
                    }
                    break;
                case Thresh_To_Zero_Inv:
                    if ((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = 0;
                    }
                    else
                    {
                        out[i][j] = (in_1[j+i*width]);
                    }
                    break;
                case Thresh_To_Binary:
                    if ((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = max_value;
                    }
                    else
                    {
                        out[i][j] = 0;
                    }
                    break;
                case Thresh_To_Binary_Inv:
                    if ((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = 0;
                    }
                    else
                    {
                        out[i][j] = max_value;
                    }
                    break;
                case Thresh_Trunc:
                    if((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = thresh;
                    }
                    else
                    {
                        out[i][j] = (in_1[j+i*width]);
                    }
                    break;
                default:
                    if ((in_1[j+i*width]) > thresh)
                    {
                        out[i][j] = thresh;
                    }
                    else
                    {
                        out[i][j] = (in_1[j+i*width]);
                    }
                    break;
            }
        }
    }

    return;
}

void AbsoluteDiff(u8** in1, u8** in2, u8** out, u32 width)

{
    u8* in_1;
    u8* in_2;
    in_1= *in1;
    in_2= *in2;
    //check if the two input images have the same size

    u32 j;
    if (sizeof(in1) != sizeof(in2))
    {
    	exit(1);
    }
    else
    {

    	for (j = 0; j < width; j++)
    	{
    		if (in_1[j] > in_2[j])
    		{
    			out[0][j] = in_1[j] - in_2[j];
    		}
    		else
    		{
    			out[0][j] = in_2[j] - in_1[j];
    		}
    	}

    }
    return;
}


// Dilate: The function dilates the source image using the specified structuring element.
// This is determined by the shape of a pixel neighborhood over which the maximum is taken.

/// Dilate kernel
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - array of pointers to output lines
/// @param[in]  kernel   - array of pointers to input kernel
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line
/// @param[in]  k        - kernel size
void Dilate(u8** src, u8** dst, u8** kernel, u32 width, u32 height, u32 k)
{
	int j, i1, j1;
	u8 max = 0;
	u8* row[15];

	for(j = 0;j < k; j++)
			row[j] = src[j];

	// Determine the anchor positions which must be replaces by a maximum value
	for ( j = 0; j < width; j++)
	{
		max = 0;
		// Determine the maximum number into a k*k square
		for (i1 = 0; i1 < k; i1++ )
		{
			for (j1 = 0; j1 < k; j1++)
			{
				if (kernel[i1][j1] && (max < row[i1][j1 + j]))
				{
						max = row[i1][j1 + j];

				}
			}
		}

		dst[0][j] = max;
	}

}


// Dilate3x3: The function dilates the source image using the specified structuring element.
// This is determined by the shape of a pixel neighborhood over which the maximum is taken.

/// Dilate3x3 kernel
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - array of pointers to output lines
/// @param[in]  kernel   - array of pointers to input kernel
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line
void Dilate3x3(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
{
	int j, i1, j1;
	u8 max = 0;
	u8* row[3];


	for(j = 0;j < 3; j++)
			row[j] = src[j];

	// Determine the anchor positions which must be replaces by a maximum value
	for ( j = 0; j < width; j++)
	{
		max = 0;
		// Determine the maximum number into a k*k square
		for(i1 = 0; i1 < 3; i1++ )
		{
			for (j1 = 0; j1 < 3; j1++)
			{
				if(kernel[i1][j1]==1)
				{
					if(max < row[i1][j1+j]){

						max = row[i1][j1+j];
					}
				}
			}
		}

		dst[0][j] =  max;
	}
}


// Dilate5x5: The function dilates the source image using the specified structuring element.
// This is determined by the shape of a pixel neighborhood over which the maximum is taken.

/// Dilate5x5 kernel
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - array of pointers to output lines
/// @param[in]  kernel   - array of pointers to input kernel
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line
void Dilate5x5(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
{
	int j,i1,j1;
	u8 max = 0;
	u8* row[5];


	for(j = 0;j < 5; j++)
			row[j] = src[j];

	// Determine the anchor positions which must be replaces by a maximum value
	for ( j = 0; j < width; j++)
	{
		max = 0;
		// Determine the maximum number into a k*k square
		for(i1 = 0; i1 < 5; i1++ )
		{
			for (j1 = 0; j1 < 5; j1++)
			{
				if(kernel[i1][j1]==1)
				{
					if(max < row[i1][j1+j]){

						max = row[i1][j1+j];
					}
				}
			}
		}

		dst[0][j] = max;
	}
}


// Dilate7x7: The function dilates the source image using the specified structuring element.
// This is determined by the shape of a pixel neighborhood over which the maximum is taken.

/// Dilate7x7 kernel
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - array of pointers to output lines
/// @param[in]  kernel   - array of pointers to input kernel
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line
void Dilate7x7(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
{
	int j, i1, j1;
	u8 max = 0;
	u8* row[7];


	for(j = 0;j < 7; j++)
			row[j] = src[j];

	// Determine the anchor positions which must be replaces by a maximum value
	for ( j = 0; j < width; j++)
	{
		max = 0;
		// Determine the maximum number into a k*k square
		for(i1 = 0; i1 < 7; i1++ )
		{
			for (j1 = 0; j1 < 7; j1++)
			{
				if(kernel[i1][j1]==1)
				{
					if(max < row[i1][j1+j]){

						max = row[i1][j1+j];
					}
				}
			}
		}

		dst[0][j] = max;
	}
}


// Erode: The function erodes the source image using the specified structuring element.
// This is determined by the shape of a pixel neighborhood over which the minimum is taken.

/// Dilate kernel
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - array of pointers to output lines
/// @param[in]  kernel   - array of pointers to input kernel
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line
/// @param[in]  k        - kernel size
void Erode(u8** src, u8** dst, u8** kernel, u32 width, u32 height, u32 k)
{
	int j, i1, j1;
	u8 min = 0xFF;
	u8* row[15];

	for(j = 0;j < k; j++)
			row[j] = src[j];

	// Determine the anchor positions which must be replaces by a maximum value
	for ( j = 0; j < width; j++)
	{
		min = 0xFF;
		// Determine the maximum number into a k*k square
		for(i1 = 0; i1 < k; i1++ )
		{
			for (j1 = 0; j1 < k; j1++)
			{
				if((kernel[i1][j1]==1) && (min > row[i1][j1+j]))
				{
						min = *(row[i1] + j1+j);
				}
			}
		}

		dst[0][j] = min;
	}

}

// Erode3x3: The function erodes the source image using the specified structuring element.
// This is determined by the shape of a pixel neighborhood over which the minimum is taken.

/// Erode3x3 kernel
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - array of pointers to output lines
/// @param[in]  kernel   - array of pointers to input kernel
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line
void Erode3x3(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
{
	int j,i1,j1;
	u8 computed;
	u8 min = 0xFF;
	u8* row[3];

	for(j = 0;j < 3; j++)
			row[j] = src[j];

	// Determine the anchor positions which must be replaces by a maximum value
	for ( j = 0; j < width; j++)
	{
		min = 0xFF;
		// Determine the minimum number into a k*k square
		for(i1 = 0; i1 < 3; i1++ )
		{
			for (j1 = 0; j1 < 3; j1++)
			{
				if(kernel[i1][j1]==1)
				{
					if(min > row[i1][j1+j]){

						min = row[i1][j1+j];
					}
				}
			}
		}

		dst[0][j] = min;
	}
}



// Erode5x5: The function erodes the source image using the specified structuring element.
// This is determined by the shape of a pixel neighborhood over which the minimum is taken.

/// Erode5x5 kernel
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - array of pointers to output lines
/// @param[in]  kernel   - array of pointers to input kernel
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line
void Erode5x5(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
{
	int j,i1,j1;
	u8 min = 0xFF;
	u8* row[5];


	for(j = 0;j < 5; j++)
		row[j] = src[j];

	// Determine the anchor positions which must be replaces by a maximum value
	for ( j = 0; j < width; j++)
	{
		min = 0xFF;
		// Determine the minimum number into a k*k square
		for(i1 = 0; i1 < 5; i1++ )
		{
			for (j1 = 0; j1 < 5; j1++)
			{
				if(kernel[i1][j1]==1)
				{
					if(min > row[i1][j1+j]){

						min = row[i1][j1+j];
					}
				}
			}
		}

		dst[0][j] = min;
	}
}


// Erode7x7: The function erodes the source image using the specified structuring element.
// This is determined by the shape of a pixel neighborhood over which the minimum is taken.

/// Erode7x7 kernel
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - array of pointers to output lines
/// @param[in]  kernel   - array of pointers to input kernel
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line
void Erode7x7(u8** src, u8** dst, u8** kernel, u32 width, u32 height)
{
	int j, i1, j1;
	u8 min = 0xFF;
	u8* row[7];


	for(j = 0;j < 7; j++)
		row[j] = src[j];

	// Determine the anchor positions which must be replaces by a maximum value
	for ( j = 0; j < width; j++)
	{
		min = 0xFF;
		// Determine the minimum number into a k*k square
		for(i1 = 0; i1 < 7; i1++ )
		{
			for (j1 = 0; j1 < 7; j1++)
			{
				if(kernel[i1][j1] == 1)
				{
					if(min > row[i1][j1 + j]){

						min = row[i1][j1 + j];
					}
				}
			}
		}

		dst[0][j] = min;
	}
}


/// boxfilter kernel that makes average on variable kernel size        
/// @param[in] in        - array of pointers to input lines      
/// @param[out] out      - array of pointers for output lines    
/// @param[in] k_size    -  kernel size for computing pixels. Eg. 3 for 3x3, 4 for 4x4  
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case  
/// @param[in] width     - width of input line
void boxfilter(u8** in, u8** out, u32 k_size, u32 normalize, u32 width)
{
    int i,x,y;
    u8* lines[15];
    float sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //initialize line pointers
    for(i=0;i<k_size;i++)
    {
        lines[i]=*(in+i);
    }

    //compute the actual out pixel
    for (i=0;i<width;i++)
    {
        sum=0.0f;
        for (x=0;x<k_size;x++)
        {
            for (y=0;y<k_size;y++)
            {
                sum+=(float)lines[x][y];
            }
            lines[x]++;
        }
        if(normalize == 1) *(*out+i)=(u8)(sum/((float)k_size*k_size));
        else *(aux+i)=(unsigned short)sum;
    }
    return;
}

/// boxfilter kernel that makes average on 3x3 kernel size
/// @param[in] in        - array of pointers to input lines
/// @param[out] out      - array of pointers for output lines
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in] width     - width of input line
void boxfilter3x3(u8** in, u8** out, u32 normalize, u32 width)
{
    int i,x,y;
    u8* lines[5];
    unsigned int sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //Initialize lines pointers
    lines[0] = *in;
    lines[1] = *(in+1);
    lines[2] = *(in+2);

    //Go on the whole line
    for (i = 0; i < width; i++)
    {
        sum = 0;
        for (y = 0; y < 3; y++)
        {
            for (x = -1; x <= 1; x++)
            {
                sum += (lines[y][x]);
            }
            lines[y]++;
        }

        if(normalize == 1)
        {
            *(*out+i)=(u8)(((half)(float)sum)*(half)0.1111);
        }
        else
        {
            *(aux+i)=(unsigned short)sum;
        }
    }
    return;
}

/// boxfilter kernel that makes average on 5x5 kernel size       
/// @param[in] in        - array of pointers to input lines      
/// @param[out] out      - array of pointers for output lines    
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in] width     - width of input line
void boxfilter5x5(u8** in, u8** out, u32 normalize, u32 width)
{
    int i,x,y;
    u8* lines[5];
    float sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //Initialize lines pointers
    lines[0]=*(in+0);
    lines[1]=*(in+1);
    lines[2]=*(in+2);
    lines[3]=*(in+3);
    lines[4]=*(in+4);

    //Go on the whole line
    for (i=0;i<width;i++){
        sum=0.0f;
        for (x=0;x<5;x++){
            for (y=0;y<5;y++){
                sum+=(float)lines[x][y];
            }
            lines[x]++;
        }
        if(normalize == 1) *(*out+i)=(u8)(sum/25.0f);
        else *(aux+i)=(unsigned short)sum;
    }
    return;
}


/// boxfilter kernel that makes average on 7x7 kernel size       
/// @param[in] in        - array of pointers to input lines      
/// @param[out] out      - array of pointers for output lines    
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in] width     - width of input line
void boxfilter7x7(u8** in, u8** out, u32 normalize, u32 width)
{
    int i,x,y;
    u8* lines[7];
    float sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //Initialize lines pointers
    lines[0]=*(in+0);
    lines[1]=*(in+1);
    lines[2]=*(in+2);
    lines[3]=*(in+3);
    lines[4]=*(in+4);
    lines[5]=*(in+5);
    lines[6]=*(in+6);

    //Go on the whole line
    for (i=0;i<width;i++){
        sum=0.0f;
        for (x=0;x<7;x++){
            for (y=0;y<7;y++){
                sum+=(float)lines[x][y];
            }
            lines[x]++;
        }
        if(normalize == 1) *(*out+i)=(u8)(sum/49.0f);
        else *(aux+i)=(unsigned short)sum;
    }
    return;
}

/// boxfilter kernel that makes average on 9x9 kernel size
/// @param[in] in        - array of pointers to input lines
/// @param[out] out      - array of pointers for output lines
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in] width     - width of input line
void boxfilter9x9(u8** in, u8** out, u32 normalize, u32 width)
{
    int i,x,y;
    u8* lines[9];
    float sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //Initialize lines pointers
    lines[0]=*(in+0);
    lines[1]=*(in+1);
    lines[2]=*(in+2);
    lines[3]=*(in+3);
    lines[4]=*(in+4);
    lines[5]=*(in+5);
    lines[6]=*(in+6);
    lines[7]=*(in+7);
    lines[8]=*(in+8);

    //Go on the whole line
    for (i=0;i<width;i++){
        sum=0.0f;
        for (x=0;x<9;x++){
            for (y=0;y<9;y++){
                sum+=(float)lines[x][y];
            }
            lines[x]++;
        }
        if(normalize == 1) *(*out+i)=(u8)(sum/81.0f);
        else *(aux+i)=(unsigned short)sum;
    }
    return;
}

/// boxfilter kernel that makes average on 11x11 kernel size
/// @param[in] in        - array of pointers to input lines
/// @param[out] out      - array of pointers for output lines
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in] width     - width of input line
void boxfilter11x11(u8** in, u8** out, u32 normalize, u32 width)
{
    int i,x,y;
    u8* lines[11];
    float sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //Initialize lines pointers
    lines[0]=*(in+0);
    lines[1]=*(in+1);
    lines[2]=*(in+2);
    lines[3]=*(in+3);
    lines[4]=*(in+4);
    lines[5]=*(in+5);
    lines[6]=*(in+6);
    lines[7]=*(in+7);
    lines[8]=*(in+8);
    lines[9]=*(in+9);
    lines[10]=*(in+10);

    //Go on the whole line
    for (i=0;i<width;i++){
        sum=0.0f;
        for (x=0;x<11;x++){
            for (y=0;y<11;y++){
                sum+=(float)lines[x][y];
            }
            lines[x]++;
        }
        if(normalize == 1) *(*out+i)=(u8)(sum/121.0f);
        else *(aux+i)=(unsigned short)sum;
    }
    return;
}

/// boxfilter kernel that makes average on 13x13 kernel size
/// @param[in] in        - array of pointers to input lines
/// @param[out] out      - array of pointers for output lines
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in] width     - width of input line
void boxfilter13x13(u8** in, u8** out, u32 normalize, u32 width)
{
    int i,x,y;
    u8* lines[13];
    float sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //Initialize lines pointers
    lines[0]=*(in+0);
    lines[1]=*(in+1);
    lines[2]=*(in+2);
    lines[3]=*(in+3);
    lines[4]=*(in+4);
    lines[5]=*(in+5);
    lines[6]=*(in+6);
    lines[7]=*(in+7);
    lines[8]=*(in+8);
    lines[9]=*(in+9);
    lines[10]=*(in+10);
    lines[11]=*(in+11);
	lines[12]=*(in+12);

    //Go on the whole line
    for (i=0;i<width;i++){
        sum=0.0f;
        for (x=0;x<13;x++){
            for (y=0;y<13;y++){
                sum+=(float)lines[x][y];
            }
            lines[x]++;
        }
        if(normalize == 1) *(*out+i)=(u8)(sum/169.0f);
        else *(aux+i)=(unsigned short)sum;
    }
    return;
}

/// boxfilter kernel that makes average on 15x15 kernel size
/// @param[in] in        - array of pointers to input lines
/// @param[out] out      - array of pointers for output lines
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in] width     - width of input line
void boxfilter15x15(u8** in, u8** out, u32 normalize, u32 width)
{
    int i,x,y;
    u8* lines[15];
    float sum;
    unsigned short* aux;
    aux=(unsigned short *)(*out);

    //Initialize lines pointers
    lines[0]=*(in+0);
    lines[1]=*(in+1);
    lines[2]=*(in+2);
    lines[3]=*(in+3);
    lines[4]=*(in+4);
    lines[5]=*(in+5);
    lines[6]=*(in+6);
    lines[7]=*(in+7);
    lines[8]=*(in+8);
    lines[9]=*(in+9);
    lines[10]=*(in+10);
    lines[11]=*(in+11);
	lines[12]=*(in+12);
	lines[13]=*(in+13);
	lines[14]=*(in+14);

    //Go on the whole line
    for (i=0;i<width;i++){
        sum=0.0f;
        for (x=0;x<15;x++){
            for (y=0;y<15;y++){
                sum+=(float)lines[x][y];
            }
            lines[x]++;
        }
        if(normalize == 1) *(*out+i)=(u8)(sum/225.0f);
        else *(aux+i)=(unsigned short)sum;
    }
    return;
}


/// Boxfilter 7x1 kernel
/// @param[in] in        - array of pointers to input lines
/// @param[out] out      - array of pointers for output lines
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in] width     - width of input line
void boxfilter7x1(u8 **in, u8 **out, u32 normalize, u32 width)
{
    u32   i, y;
    float sum = 0.0;
    unsigned short* aux;
    aux=(unsigned short *)(*out);
    u8    *output = *out;

    for (i = 0; i < width; i++)
    {
        sum = 0.0;

        for (y = 0; y < 7; y++)
        {
            sum += (float)(in[y][i]) ;
        }

        if (normalize == 1)
            output[i] =  (u8)(sum/7.0f);
        else
        	aux[i] = (unsigned short)(sum);
    }
    return;
}

/// boxfilter1x7 kernel
/// @param[in] in        - array of pointers to input lines
/// @param[out] out      - array of pointers for output lines
/// @param[in] normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in] width     - width of input line
void boxfilter1x7(u8 **in, u8 **out, u32 normalize, u32 width)
{
    u32   i, y;
    float sum = 0.0;
    u8    *input  = *in;
    unsigned short* aux;
    aux=(unsigned short *)(*out);
    u8    *output = *out;

    for (i = 0; i < width; i++)
    {
        sum = 0.0;

        for (y = 0; y < 7; y++)
        {
            sum += ((float)input[y]) ;
        }
        input++;

        if (normalize == 1)
            output[i] =  (u8)(sum/7.0f);
        else
        	aux[i] = (unsigned short)(sum);
    }
    return;
}

/// boxfilter7x7separable kernel
/// @param[in]  in        - array of pointers to input lines
/// @param[out] out       - array of pointers for output lines
/// @param[out] interim   - intermediate buffer for first step
/// @param[in]  normalize - parameter to check if we want to do a normalize boxfilter or not 1 for normalized values , 0 in the other case
/// @param[in]  width     - width of input line
void boxfilter7x7separable(u8 **in, u8 **out, u32 normalize, u32 width)
{
    u32   i, y;
    float sum = 0.0;
    unsigned short aux1[1920];
    u8    *output = *out;
    unsigned short* aux;
    unsigned short* aux2;
    aux=(unsigned short *)(*out);
    aux2 = aux1;
    for (i = 0; i < width+6; i++)
    {
        sum = 0.0;

        for (y = 0; y < 7; y++)
        {
            sum += (float)(in[y][i]) ;
        }

        	aux2[i] = (unsigned short)(sum);
    }


    for (i = 0; i < width; i++)
    {
        sum = 0.0;

        for (y = 0; y < 7; y++)
        {
            sum += ((float)aux2[y]) ;
        }
        aux2++;

        if (normalize == 1)
            output[i] =  (u8)(sum/49.0f);
        else
        	aux[i] = (unsigned short)(sum);
    }
    return;
}



void cvtColorKernelYUVToRGB(u8* yIn, u8* uIn, u8* vIn, u8* out, u32 width)
{
    u32 i, j;
    u32 uv_idx = 0;
    u32 out_idx_1 = 0;
    u32 out_idx_2 = 3 * width;
    int y[4], u, v, r, g, b;

    i = 0;
    while(i < width)
    {
        y[0] = yIn[i];
        y[1] = yIn[i + 1];
        y[2] = yIn[width + i];
        y[3] = yIn[width + i + 1];

        u = uIn[uv_idx] - 128;
        v = vIn[uv_idx] - 128;
        uv_idx++;

        for(j = 0; j < 4; j++)
        {
            r = y[j] + (int)(1.402f*v);
            g = y[j] - (int)(0.344f*u +0.714f*v);
            b = y[j] + (int)(1.772f*u);

            r = r>255? 255 : r<0 ? 0 : r;
            g = g>255? 255 : g<0 ? 0 : g;
            b = b>255? 255 : b<0 ? 0 : b;

            if(j  < 2)
            {
                out[out_idx_1++] = (u8)r;
                out[out_idx_1++] = (u8)g;
                out[out_idx_1++] = (u8)b;
            }
            else {
                out[out_idx_2++] = (u8)r;
                out[out_idx_2++] = (u8)g;
                out[out_idx_2++] = (u8)b;
            }
        }
        i += 2;
    }
}

void cvtColorKernelRGBToYUV(u8* in, u8* yOut, u8* uOut, u8* vOut, u32 width)
{
    u32 i;
    u32 r, g, b;
    int y, u, v;
    u32 uv_idx = 0;

    for(i = 0; i < width * 2; i+= 2)
    {
        r = in[i * 3];
        g = in[i * 3 + 1];
        b = in[i * 3 + 2];

        y = ceil(0.299f * r + 0.587f * g + 0.114f * b);
        yOut[i] = y > 255 ? 255 : (u8)y;

        if(i >= width)
        {
        	u = (int)(((float)b - y) * 0.564f) + 128;
        	v = (int)(((float)r - y) * 0.713f) + 128;
        	uOut[uv_idx] = (u8)u;
        	vOut[uv_idx] = (u8)v;
        	uv_idx++;
        }

        r = in[(i + 1) * 3];
        g = in[(i + 1) * 3 + 1];
        b = in[(i + 1)* 3 + 2];

        y = ceil(0.299f * r + 0.587f * g + 0.114f * b);
        yOut[i + 1] = y > 255 ? 255 : (u8)y;
    }
}

/// Performs color space conversion YUV422 to RGB for one line
    /// @param[in] input - pointer to the YUV422 interleaved line
    /// @param[out] rOut - pointer to the output line that contain R values
    /// @param[out] gOut- pointer to the output line that contain G values
    /// @param[out] bOut- pointer to the output line that contain B values
    /// @param[in] width - line width
void cvtColorKernelYUV422ToRGB(u8** input, u8** rOut, u8** gOut, u8** bOut, u32 width)
{	
	int j;
	int r, g, b, y1, y2, u, v;
	int out_index = 0;
	u8*  in = input[0];
	u8* outR = rOut[0];
	u8* outG = gOut[0];
	u8* outB = bOut[0];
	
	for(j = 0; j < width * 2; j+=4)
	{
		y1 = in[j];
		u  = in[j + 1] - 128;
		y2 = in[j + 2];
		v  = in[j + 3] - 128;
		
		r = y1 + (int)(1.402f * v);
        g = y1 - (int)(0.344f * u + 0.714f * v);
        b = y1 + (int)(1.772f * u);
		
		outR[out_index] = (u8) (r>255 ? 255 : r<0 ? 0 : r);
		outG[out_index] = (u8) (g>255 ? 255 : g<0 ? 0 : g);
		outB[out_index] = (u8) (b>255 ? 255 : b<0 ? 0 : b);
		out_index++;
	
		r = y2 + (int)(1.402f * v);
        g = y2 - (int)(0.344f * u + 0.714f * v);
        b = y2 + (int)(1.772f * u);

		outR[out_index] = (u8) (r>255 ? 255 : r<0 ? 0 : r);
		outG[out_index] = (u8) (g>255 ? 255 : g<0 ? 0 : g);
		outB[out_index] = (u8) (b>255 ? 255 : b<0 ? 0 : b);
		out_index++;
	}
}

/// Performs color space conversion RGB to YUV422 for one line
    /// @param[in] rIn - pointer to the input line that contain R values from RGB
    /// @param[in] gIn - pointer to the input line that contain G values from RGB
    /// @param[in] bIn - pointer to the input line that contain B values from RGB
    /// @param[out] output - pointer to the output line YUV422 interleaved
    /// @param[in] width - line width
void cvtColorKernelRGBToYUV422(u8** rIn, u8** gIn, u8** bIn, u8** output, u32 width)
{
	int j;
	float y, u1, v1, u2, v2, u_avr, v_avr;
	int out_index = 0;
	u8* inR = rIn[0];
	u8* inG = gIn[0];
	u8* inB = bIn[0];
	u8* out_yuyv = output[0];
	
	for(j = 0; j < width; j+=2)
	{
		y = 0.299f * inR[j] + 0.587f * inG[j] + 0.114f * inB[j];
		u1 = ((float)inB[j] - y) * 0.564f + 128;
        v1 = ((float)inR[j] - y) * 0.713f + 128;
		
		y = y>255 ? 255 : y<0 ? 0 : y;
		out_yuyv[out_index++] =(u8) y;

		y = 0.299f * inR[j + 1] + 0.587f * inG[j + 1] + 0.114f * inB[j + 1];
		y = y>255 ? 255 : y<0 ? 0 : y;
		u2 = ((float)inB[j + 1] - y) * 0.564f + 128;
        v2 = ((float)inR[j + 1] - y) * 0.713f + 128;

		u_avr = (u1 + u2) / 2;
		v_avr = (v1 + v2) / 2;
		
		out_yuyv[out_index++] =(u8) (u_avr>255 ? 255 : u_avr<0 ? 0 : u_avr);
		out_yuyv[out_index++] =(u8) y;	
		out_yuyv[out_index++] =(u8) (v_avr>255 ? 255 : v_avr<0 ? 0 : v_avr);		
	}
}

/// cvtColorKernel to conversion RGB to NV21
/// Performs color space conversion: RGB to NV21 
///@param[in] inR input R channel
///@param[in] inG input G channel
///@param[in] inB input B channel
///@param[out] yOut output Y channel
///@param[out] uvOut output UV channel interleaved
///@param[in] width - image width in pixels
void cvtColorRGBtoNV21(u8** inR, u8** inG, u8** inB, u8** yOut, u8** uvOut, u32 width, u32 k) 
{
	u32 i,j;
	
	u8* r = inR[0];
	u8* g = inG[0];
	u8* b = inB[0];
	u8* yo = yOut[0];
	u8* uvo = uvOut[0];
	
	int y,u1, u2, v1, v2, um, vm;
	u32 uv_idx = 0;
	
	for (i = 0; i < width; i+=2)
    {
        
        y = 0.299f * r[i] + 0.587f * g[i] + 0.114f * b[i];
        yo[i] = (u8) y > 255 ? 255 : y < 0 ? 0 : y;
		
		if (k % 2 == 0) {
           	u1 = (int)(((float)b[i] - y) * 0.564f) + 128;
        	v1 = (int)(((float)r[i] - y) * 0.713f) + 128;
        }
	//-------------------------------------------------------
		
		y = 0.299f * r[i+1] + 0.587f * g[i+1] + 0.114f * b[i+1];
		yo[i + 1] = (u8) y > 255 ? 255 : y < 0 ? 0 : y;
					
		if (k % 2 == 0) {
			u2 = (int)(((float)b[i+1] - y) * 0.564f) + 128;
			v2 = (int)(((float)r[i+1] - y) * 0.713f) + 128;
        
		um = (u1 + u2)/2;
        vm = (v1 + v2)/2;
        uvo[uv_idx] = (u8) um > 255 ? 255 : um < 0 ? 0 : um;
		uvo[uv_idx + 1] = (u8) vm > 255 ? 255 : vm < 0 ? 0 : vm;
		uv_idx = uv_idx + 2;
		}
		
 	}
	
}


/// cvtColorKernel to conversion NV21 to RGB
/// Performs color space conversion: NV21 to RGB 
///@param[in] yin input Y channel
///@param[in] uvin input UV channel interleaved
///@param[out] outR output R channel
///@param[out] outG output G channel
///@param[out] outB output B channel
///@param[in] width - image width in pixels
void cvtColorNV21toRGB(u8** yin, u8** uvin, u8** outR, u8** outG, u8** outB, u32 width) {
	u32 i;
	
	u8* y  = yin[0];
	u8* uv = uvin[0];
	u8* rr = outR[0];
	u8* gg = outG[0];
	u8* bb = outB[0];
	
	u32 uvidx = 0;
	int yy,u,v,r,g,b;
	for(i = 0; i < width; i+=2 )
	{
		yy = y[i];
		u = uv[uvidx] - 128;
		v = uv[uvidx+1] - 128;
		uvidx += 2;
			r =  yy + (int)(1.402f*v);
			rr[i] = (u8) (r > 255 ? 255 : r < 0 ? 0 : r);  
			g =  yy - (int)(0.344f*u + 0.714*v);
			gg[i] = (u8) (g > 255 ? 255 : g < 0 ? 0 : g);
			b =  yy + (int)(1.772f*u);
			bb[i] = (u8) (b > 255 ? 255 : b < 0 ? 0 : b);
	//----------------------------------------------
		yy = y[i + 1];
		r =  yy + (int)(1.402f*v);
		rr[i + 1] = (u8) (r > 255 ? 255 : r < 0 ? 0 : r);
		g =  yy - (int)(0.344f*u + 0.7148*v);
		gg[i + 1] = (u8) (g > 255 ? 255 : g < 0 ? 0 : g);
		b =  yy + (int)(1.772f*u);
		bb[i + 1] = (u8) (b > 255 ? 255 : b < 0 ? 0 : b);
	}
}

/// Laplacian3x3 kernel  
void Laplacian3x3(u8** in, u8** out, u32 inWidth)
{
    int i,x,y;
    u8* lines[3];
	float sum;
	int lapl[9]={ 0, -1, 0, -1, 4, -1, 0, -1, 0};
		
    //Initialize lines pointers
    lines[0] = in[0];
    lines[1] = in[1];
    lines[2] = in[2];
	
	//Go on the whole line
    for (i = 0; i < inWidth; i++){
	
		sum = 0.0f;
        for (x = 0; x < 3; x++){
            for (y = 0; y < 3; y++){
                sum += (float)(lines[x][y] * lapl[x * 3 + y]);
            }
            lines[x]++;
        }

        if (sum >= 255)
            sum = 255.0f;
        if (sum <= 0)
            sum = 0.0f;
        out[0][i] = (u8)(sum);
    }
    return;
}

///Laplacian5x5 kernel
void Laplacian5x5(u8** in, u8** out, u32 inWidth)
{
    int i,x,y;
    u8* lines[5];
	float sum;
	int lapl[25]={ 0, 0, -1, 0, 0, 0, -1, -2, -1, 0, -1, -2, 17, -2, -1, 0, -1, -2, -1, 0, 0, 0, -1, 0, 0};
		
    //Initialize lines pointers
    lines[0] = in[0];
    lines[1] = in[1];
    lines[2] = in[2];
    lines[3] = in[3];
    lines[4] = in[4];
	
	//Go on the whole line
    for (i = 0; i < inWidth; i++){
	
		sum = 0.0f;
        for (x = 0; x < 5; x++){
            for (y = 0; y < 5; y++){
                sum += (float)(lines[x][y] * lapl[x * 5 + y]);
            }
            lines[x]++;
        }

        if (sum >= 255)
            sum = 255.0f;
        if (sum <= 0)
            sum = 0.0f;
        out[0][i] = (u8)(sum);
    }
    return;
}

///Laplacian7x7 kernel
void Laplacian7x7(u8** in, u8** out, u32 inWidth)
{
    int i,x,y;
    u8* lines[7];
	float sum;
	int lapl[49]={ -10, -5, -2, -1, -2, -5, -10, -5, 0, 3, 4, 3, 0, -5,
				    -2, 3, 6, 7, 6, 3, -2, -1, 4, 7, 8, 7, 4, -1,
				    -2, 3, 6, 7, 6, 3, -2, -5, 0, 3, 4, 3, 0, -5,
					-10, -5, -2, -1, -2, -5, -10 };
    //Initialize lines pointers
    lines[0] = in[0];
    lines[1] = in[1];
    lines[2] = in[2];
    lines[3] = in[3];
    lines[4] = in[4];
    lines[5] = in[5];
    lines[6] = in[6];
	
	//Go on the whole line
    for (i = 0; i < inWidth; i++){
	
		sum = 0.0f;
        for (x = 0; x < 7; x++){
            for (y = 0; y < 7; y++){
                sum += (float)(lines[x][y] * lapl[x * 7 + y]);
            }
            lines[x]++;
        }

        if (sum >= 255)
            sum = 255.0f;
        if (sum <= 0)
            sum = 0.0f;
        out[0][i] = (u8)(sum);
    }
    return;
}

/// integral image kernel - this kernel makes the sum of all pixels before it and on the left of it's column  ( this particular case makes sum of pixels in u32 format)
/// @param[in] in         - array of pointers to input lines      
/// @param[out]out        - array of pointers for output lines    
/// @param[in] sum        - sum of previous pixels . for this parameter we must have an array of u32 declared as global and having the width of the line
/// @param[in] width      - width of input line
void integralimage_sum_u32(u8** in, u8** out, u32* sum, u32 width)
{
    int i,x,y;
    u8* in_line;
    u32* out_line;
    u32 s=0;
    //initialize a line pointer for one of u32 or f32
    in_line = *in;
    out_line = (u32 *)(*out);
    //go on the whole line and sum the pixels
    for(i=0;i<width;i++)
    {
        s=s+in_line[i]+sum[i];
        out_line[i]=s;
        sum[i]=sum[i]+in_line[i];
    }
    return;
}

/// integral image kernel - this kernel makes the sum of all pixels before it and on the left of it's column  ( this particular case makes sum of pixels in f32 format)
/// @param[in] in         - array of pointers to input lines      
/// @param[out]out        - array of pointers for output lines    
/// @param[in] sum        - sum of previous pixels . for this parameter we must have an array of u32 declared as global and having the width of the line
/// @param[in] width      - width of input line
void integralimage_sum_f32(u8** in, u8** out, u32* sum, u32 width)
{
    int i,x,y;
    u8* in_line;
    fp32* out_line;
    u32 s=0;
    //initialize a line pointer for one of u32 or f32
    in_line = *in;
    out_line = (fp32 *)(*out);

    //go on the whole line and sum the pixels
    for(i=0;i<width;i++)
    {
        s=s+in_line[i]+sum[i];
        out_line[i]=(fp32)s;
        sum[i]=sum[i]+in_line[i];
    }

    return;
}

/// integral image kernel - this kernel makes the sum of all pixels before it and on the left of it's column  ( this particular case makes square sum of pixels in u32 format)
/// @param[in] in         - array of pointers to input lines      
/// @param[out]out        - array of pointers for output lines    
/// @param[in] sum        - sum of previous pixels . for this parameter we must have an array of u32 declared as global and having the width of the line
/// @param[in] width      - width of input line
void integralimage_sqsum_u32(u8** in, u8** out, u32* sum, u32 width)
{
    int i,x,y;
    u8* in_line;
    u32* out_line;
    u32 s=0;
    //initialize a line pointer for one of u32 or f32
    in_line = *in;
    out_line = (u32 *)(*out);
    //go on the whole line and sum the pixels
    for(i=0;i<width;i++)
    {
        s=s+(in_line[i]*in_line[i])+sum[i];
        out_line[i]=s;
        sum[i]=sum[i]+in_line[i]*in_line[i];
    }

    return;
}

/// integral image kernel - this kernel makes the sum of all pixels before it and on the left of it's column  ( this particular case makes square sum of pixels in f32 format)
/// @param[in] in         - array of pointers to input lines      
/// @param[out]out        - array of pointers for output lines    
/// @param[in] sum        - sum of previous pixels . for this parameter we must have an array of u32 declared as global and having the width of the line
/// @param[in] width      - width of input line
void integralimage_sqsum_f32(u8** in, u8** out, fp32* sum, u32 width)
{
    int i,x,y;
    u8* in_line;
    fp32* out_line;
    fp32 s=0;
    //initialize a line pointer for one of u32 or f32
    in_line = *in;
    out_line = (fp32 *)(*out);
    //go on the whole line and sum the pixels
    for(i=0;i<width;i++)
    {
        s=s+(in_line[i]*in_line[i])+sum[i];
        out_line[i]=(fp32)s;
        sum[i]=sum[i]+in_line[i]*in_line[i];

    }
    return;
}


/// histogram kernel - makes a histogram on a given plane
/// @param[in] in         - array of pointers to input lines      
/// @param[out]hist       - array oh values from histogram
/// @param[in] width      - width of input line
void histogram(u8** in, u32 *hist, u32 width)
{
    u8 *in_line;
    u32 i;
    in_line  = *in;

    for (i=0;i<width; i++)
    {
        hist[in_line[i]]++;

    }
    return;
}

/// equalizehistogram kernel - makes an equalization trough an image with a given histogram
/// @param[in] in         - array of pointers to input lines
/// @param[out]out        - array of pointers to output lines
/// @param[in] hist       - pointer to an input array that indicates the cumulative histogram of the image
/// @param[in] width      - width of input line
void equalizeHist(u8** in, u8** out, u32 *hist, u32 width)
{
	u8 *in_line;
	u8 *out_line;
	u32 i;

	in_line   = *in;
	out_line  = *out;


	for(i=0;i<width;i++)
	{
		out_line[i]= (u8)((fp32)hist[in_line[i]] * ((fp32)255/(fp32)hist[255]));
	}

    return;
}

/// Convolution 1x5 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth    - width of input line
void Convolution1x5(u8 **in, u8 **out, float *conv, u32 inWidth)
{
    u32   i, y;
    float sum = 0.0;
    u8    *input  = *in;
    u8    *output = *out;

    for (i = 0; i < inWidth; i++)
    {
        sum = 0.0;

        for (y = 0; y < 5; y++)
        {
            sum += ((float)input[y]) * conv[y];
        }
        input++;

        if (sum >= 255.0f)
            sum = 255.0f;
        if (sum <= 0.0f)
            sum = 0.0f;
        output[i] = (u8)(sum);
    }
    return;
}

/// Convolution 1x7 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth    - width of input line
void Convolution1x7(u8 **in, u8 **out, float *conv, u32 inWidth)
{
    u32   i, y;
    float sum = 0.0;
    u8    *input  = *in;
    u8    *output = *out;

    for (i = 0; i < inWidth; i++)
    {
        sum = 0.0;

        for (y = 0; y < 7; y++)
        {
            sum += ((float)input[y]) * conv[y];
        }
        input++;

        if (sum >= 255.0f)
            sum = 255.0f;
        if (sum <= 0.0f)
            sum = 0.0f;
        output[i] = (u8)(sum);
    }
    return;
}

/// Convolution 1x9 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth    - width of input line
void Convolution1x9(u8 **in, u8 **out, float *conv, u32 inWidth)
{
    u32   i, y;
    float sum = 0.0;
    u8    *input  = *in;
    u8    *output = *out;

    for (i = 0; i < inWidth; i++)
    {
        sum = 0.0;

        for (y = 0; y < 9; y++)
        {
            sum += ((float)input[y]) * conv[y];
        }
        input++;

        if (sum >= 255.0f)
            sum = 255.0f;
        if (sum <= 0.0f)
            sum = 0.0f;
        output[i] = (u8)(sum);
    }
    return;
}

/// Convolution 1x15 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth    - width of input line
void Convolution1x15(u8 **in, u8 **out, float *conv, u32 inWidth)
{
    u32   i, y;
    float sum = 0.0;
    u8    *input  = *in;
    u8    *output = *out;

    for (i = 0; i < inWidth; i++)
    {
        sum = 0.0;

        for (y = 0; y < 15; y++)
        {
            sum += ((float)input[y]) * conv[y];
        }
        input++;

        if (sum >= 255.0f)
            sum = 255.0f;
        if (sum <= 0.0f)
            sum = 0.0f;
        output[i] = (u8)(sum);
    }
    return;
}

/// Convolution 5x1 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth    - width of input line
void Convolution5x1(u8 **in, u8 **out, float *conv, u32 inWidth)
{
    u32   i, y;
    float sum = 0.0;
    u8    *output = *out;

    for (i = 0; i < inWidth; i++)
    {
        sum = 0.0;

        for (y = 0; y < 5; y++)
        {
            sum += ((float)in[y][i]) * conv[y];
        }

        if (sum >= 255.0f)
            sum = 255.0f;
        if (sum <= 0.0f)
            sum = 0.0f;
        output[i] = (u8)(sum);
    }
    return;
}

/// Convolution 7x1 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth    - width of input line
void Convolution7x1(u8 **in, u8 **out, float *conv, u32 inWidth)
{
    u32   i, y;
    float sum = 0.0;
    u8    *output = *out;

    for (i = 0; i < inWidth; i++)
    {
        sum = 0.0;

        for (y = 0; y < 7; y++)
        {
            sum += ((float)in[y][i]) * conv[y];
        }

        if (sum >= 255.0f)
            sum = 255.0f;
        if (sum <= 0.0f)
            sum = 0.0f;
        output[i] = (u8)(sum);
    }
    return;
}

/// Convolution 9x1 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth    - width of input line
void Convolution9x1(u8 **in, u8 **out, float *conv, u32 inWidth)
{
    u32   i, y;
    float sum = 0.0;
    u8    *output = *out;

    for (i = 0; i < inWidth; i++)
    {
        sum = 0.0;

        for (y = 0; y < 9; y++)
        {
            sum += ((float)in[y][i]) * conv[y];
        }

        if (sum >= 255.0f)
            sum = 255.0f;
        if (sum <= 0.0f)
            sum = 0.0f;
        output[i] = (u8)(sum);
    }
    return;
}

/// Convolution 15x1 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth    - width of input line
void Convolution15x1(u8 **in, u8 **out, float *conv, u32 inWidth)
{
    u32   i, y;
    float sum = 0.0;
    u8    *output = *out;

    for (i = 0; i < inWidth; i++)
    {
        sum = 0.0;

        for (y = 0; y < 15; y++)
        {
            sum += ((float)in[y][i]) * conv[y];
        }

        if (sum >= 255.0f)
            sum = 255.0f;
        if (sum <= 0.0f)
            sum = 0.0f;
        output[i] = (u8)(sum);
    }
    return;
}

/// Convolution 3x3 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth      - width of input line
void Convolution3x3(u8** in, u8** out, float conv[9], u32 inWidth)
{
    int i,x,y;
    u8* lines[3];
    float sum;

    //Initialize lines pointers
    lines[0] = in[0];
    lines[1] = in[1];
    lines[2] = in[2];

    //Go on the whole line
    for (i = 0; i < inWidth; i++){
        sum = 0.0f;
        for (x = 0; x < 3; x++){
            for (y = 0; y < 3; y++){
                sum += (float)(lines[x][y] * conv[x * 3 + y]);
            }
            lines[x]++;
        }

        if (sum >= 255)
            sum = 255.0f;
        if (sum <= 0)
            sum = 0.0f;
        out[0][i] = (u8)(sum);
    }
    return;
}

/// Convolution 5x5 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth      - width of input line
void Convolution5x5(u8** in, u8** out, float conv[25], u32 inWidth)
{
    int x, y;
    unsigned int i;
    u8* lines[5];
    float sum;

    //Initialize lines pointers
    lines[0] = in[0];
    lines[1] = in[1];
    lines[2] = in[2];
    lines[3] = in[3];
    lines[4] = in[4];

    //Go on the whole line
    for (i = 0; i < inWidth; i++){
        sum = 0.0;
        for (x = 0; x < 5; x++)
        {
            for (y = 0; y < 5; y++)
            {
                sum += (float)(lines[x][y] * conv[x * 5 + y]);
            }
            lines[x]++;
        }
        if (sum >= 255)
            sum = 255.0;
        if (sum <= 0)
            sum = 0.0;
        out[0][i] = (u8)(sum);
    }
    return;
}

/// Convolution 7x7 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth      - width of input line
void Convolution7x7(u8** in, u8** out, float conv[49], u32 inWidth)
{
    int x, y;
    unsigned int i;
    u8* lines[7];
    float sum;

    //Initialize lines pointers
    lines[0] = in[0];
    lines[1] = in[1];
    lines[2] = in[2];
    lines[3] = in[3];
    lines[4] = in[4];
    lines[5] = in[5];
    lines[6] = in[6];

    //Go on the whole line
    for (i = 0; i < inWidth; i++){
        sum = 0.0;
        for (x = 0; x < 7; x++)
        {
            for (y = 0; y < 7; y++)
            {
                sum += (float)(lines[x][y] * conv[x * 7 + y]);
            }
            lines[x]++;
        }
        if (sum >= 255)
            sum = 255.0;
        if (sum <= 0)
            sum = 0.0;
        out[0][i] = (u8)(sum);

    }
    return;
}

/// Convolution 9x9 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth      - width of input line
void Convolution9x9(u8** in, u8** out, float conv[81], u32 inWidth)
{
    int x, y;
    unsigned int i;
    u8* lines[9];
    float sum;

    //Initialize lines pointers
    lines[0] = in[0];
    lines[1] = in[1];
    lines[2] = in[2];
    lines[3] = in[3];
    lines[4] = in[4];
    lines[5] = in[5];
    lines[6] = in[6];
    lines[7] = in[7];
    lines[8] = in[8];

    //Go on the whole line
    for (i = 0; i < inWidth; i++){
        sum = 0.0;
        for (x = 0; x < 9; x++)
        {
            for (y = 0; y < 9; y++)
            {
                sum += (float)(lines[x][y] * conv[x * 9 + y]);
            }
            lines[x]++;
        }
        if (sum >= 255)
            sum = 255.0;
        if (sum <= 0)
            sum = 0.0;
        out[0][i] = (u8)(sum);

    }
    return;
}

/// Convolution 11x11 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] conv       - array of values from convolution
/// @param[in] inWidth      - width of input line
void Convolution11x11(u8** in, u8** out, float conv[81], u32 inWidth)
{
    int x, y;
    unsigned int i;
    u8* lines[11];
    float sum;

    //Initialize lines pointers
    lines[0]  = in[0];
    lines[1]  = in[1];
    lines[2]  = in[2];
    lines[3]  = in[3];
    lines[4]  = in[4];
    lines[5]  = in[5];
    lines[6]  = in[6];
    lines[7]  = in[7];
    lines[8]  = in[8];
    lines[9]  = in[9];
    lines[10] = in[10];

    //Go on the whole line
    for (i = 0; i < inWidth; i++){
        sum = 0.0;
        for (x = 0; x < 11; x++)
        {
            for (y = 0; y < 11; y++)
            {
                sum += (float)(lines[x][y] * conv[x * 11 + y]);
            }
            lines[x]++;
        }
        if (sum >= 255)
            sum = 255.0;
        if (sum <= 0)
            sum = 0.0;
        out[0][i] = (u8)(sum);

    }
    return;
}

/// Convolution generic kernel
/// @param[in] in         array of pointers to input lines
/// @param[in] out        array of pointers to output lines
/// @param[in] kernelSize kernel size
/// @param[in] conv       pointer to convolution
/// @param[in] inWidth    width of input line
///
void Convolution(u8** in, u8** out, u32 kernelSize, float* conv, u32 inWidth)
{
    unsigned int i, x, y;
    u8* lines[15];
    float sum = 0.0;

    //Initialize lines pointers
    for (x = 0; x < kernelSize; x++)
    {
        lines[x] = *(in + x);
    }

    //Go on the whole line
    for (i = 0; i < inWidth; i++){

        sum = 0.0;

        for (x = 0; x < kernelSize; x++)
        {
            for (y = 0; y < kernelSize; y++)
            {
                sum += (float)(lines[x][y] * conv[x * kernelSize + y]);
            }
            lines[x]++;
        }

        if (sum >= 255)
            sum = 255.0;
        if (sum <= 0)
            sum = 0.0;
        out[0][i] = (u8)(sum);
    }
    return;
}

/// Convolution 5x5 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] interm     - array of pointers to intermediary line
/// @param[in] vconv      - array of values from vertical convolution
/// @param[in] hconv      - array of values from horizontal convolution
/// @param[in] inWidth    - width of input line
#ifdef __MOVICOMPILE__
void ConvolutionSeparable5x5(u8** in, u8** out, u8** interm, half vconv[5], half hconv[5], u32 inWidth)
{
    Convolution5x1_asm(&in[0], &interm[0], vconv, inWidth + 4);
    Convolution1x5_asm(&interm[0], out, hconv, inWidth);
}
#else
void ConvolutionSeparable5x5(u8** in, u8** out, u8** interm, float vconv[5], float hconv[5], u32 inWidth)
{
    Convolution5x1(&in[0], &interm[0], vconv, inWidth + 4);
    Convolution1x5(&interm[0], out, hconv, inWidth);
}
#endif

/// Convolution 7x7 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] interm     - array of pointers to intermediary line
/// @param[in] vconv      - array of values from vertical convolution
/// @param[in] hconv      - array of values from horizontal convolution
/// @param[in] inWidth    - width of input line
#ifdef __MOVICOMPILE__
void ConvolutionSeparable7x7(u8** in, u8** out, u8** interm, half vconv[7], half hconv[7], u32 inWidth)
{
    Convolution7x1_asm(&in[0], &interm[0], vconv, inWidth + 6);
    Convolution1x7_asm(&interm[0], out, hconv, inWidth);
}
#else
void ConvolutionSeparable7x7(u8** in, u8** out, u8** interm, float vconv[7], float hconv[7], u32 inWidth)
{
    Convolution7x1(&in[0], &interm[0], vconv, inWidth + 6);
    Convolution1x7(&interm[0], out, hconv, inWidth);
}
#endif

/// Convolution 9x9 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] interm     - array of pointers to intermediary line
/// @param[in] vconv      - array of values from vertical convolution
/// @param[in] hconv      - array of values from horizontal convolution
/// @param[in] inWidth    - width of input line
#ifdef __MOVICOMPILE__
void ConvolutionSeparable9x9(u8** in, u8** out, u8** interm, half vconv[9], half hconv[9], u32 inWidth)
{
    Convolution9x1_asm(&in[0], &interm[0], vconv, inWidth + 8);
    Convolution1x9_asm(&interm[0], out, hconv, inWidth);
}
#else
void ConvolutionSeparable9x9(u8** in, u8** out, u8** interm, float vconv[9], float hconv[9], u32 inWidth)
{
    Convolution9x1(&in[0], &interm[0], vconv, inWidth + 8);
    Convolution1x9(&interm[0], out, hconv, inWidth);
}
#endif

/// Convolution 15x15 kernel
/// @param[in] in         - array of pointers to input lines
/// @param[in] out        - array of pointers to output lines
/// @param[in] interm     - array of pointers to intermediary line
/// @param[in] vconv      - array of values from vertical convolution
/// @param[in] hconv      - array of values from horizontal convolution
/// @param[in] inWidth    - width of input line
#ifdef __MOVICOMPILE__
void ConvolutionSeparable15x15(u8** in, u8** out, u8** interm, half vconv[15], half hconv[15], u32 inWidth)
{
    Convolution15x1_asm(&in[0], &interm[0], vconv, inWidth + 14);
    Convolution1x15_asm(&interm[0], out, hconv, inWidth);
}
#else
void ConvolutionSeparable15x15(u8** in, u8** out, u8** interm, float vconv[15], float hconv[15], u32 inWidth)
{
    Convolution15x1(&in[0], &interm[0], vconv, inWidth + 14);
    Convolution1x15(&interm[0], out, hconv, inWidth);
}
#endif

/// integral image kernel - this kernel makes the sum of all pixels before it and on the left of it's column  ( this particular case makes square sum of pixels in f32 format)
/// @param[in] in         - array of pointers to input lines      
/// @param[out]out        - array of pointers for output lines    
/// @param[in] width      - width of input line
/// @param[in] plane      - number 0 to extract plane R, 1 for extracting G, 2 for extracting B
void channelExtract(u8** in, u8** out, u32 width, u32 plane)
{
	u8 *in_line;
	u8 *out_line;
	u32 i, c=0;
	in_line  = *in;
	out_line = *out;

	for (i=plane;i<width;i=i+3)
	{
		out_line[c]=in_line[i];
		c++;
	}
    return;
}

/// median blur filter - Filter, histogram based method for median calculation
/// @param[in] widthLine  - width of input line
/// @param[out]outLine    - array of pointers for output lines
/// @param[in] inLine     - array of pointers to input lines
void medianFilter3x3(u32 widthLine, u8 **outLine, u8 ** inLine)
{
	uint32_t i = 0;
	u8 *out;
    int j = 0;
    int histogram[256];
    int e = 0;
    u32 MED_HEIGHT = 3;
    u32 MED_WIDTH  = 3;
    u32 MED_LIMIT = 5;

    out = *outLine;

	for (i=0; i<256; i++) histogram[i] = 0;
	// build initial histogram
	for (i=0; i<MED_HEIGHT; i++)
	{
		for (j=0; j<MED_WIDTH; j++)
		{
			e =inLine[i][j-(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	for (i=0; i<widthLine; i++)
	{
		e = 0;
		j = 0;
		// use histogram
		while (j<256)
		{
			e+=histogram[j];
			if (e<MED_LIMIT)
			{
				j++;
			} else
			{
				out[i] = j;
				j = 256;
			}
		}
		// substract previous values from histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i-(MED_WIDTH>>1)];
			histogram[e]--;
		}
		// add next values to histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i+1+(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	return;
}

/// median blur filter - Filter, histogram based method for median calculation
/// @param[in] widthLine  - width of input line
/// @param[out]outLine    - array of pointers for output lines
/// @param[in] inLine     - array of pointers to input lines
void medianFilter5x5(u32 widthLine, u8 **outLine, u8 ** inLine)
{
	uint32_t i = 0;
	u8 *out;
    int j = 0;
    int histogram[256];
    int e = 0;
    u32 MED_HEIGHT = 5;
    u32 MED_WIDTH  = 5;
    u32 MED_LIMIT = (MED_WIDTH*MED_HEIGHT)/2 + 1;

    out = *outLine;

	for (i=0; i<256; i++) histogram[i] = 0;
	// build initial histogram
	for (i=0; i<MED_HEIGHT; i++)
	{
		for (j=0; j<MED_WIDTH; j++)
		{
			e =inLine[i][j-(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	for (i=0; i<widthLine; i++)
	{
		e = 0;
		j = 0;
		// use histogram
		while (j<256)
		{
			e+=histogram[j];
			if (e<MED_LIMIT)
			{
				j++;
			} else
			{
				out[i] = j;
				j = 256;
			}
		}
		// substract previous values from histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i-(MED_WIDTH>>1)];
			histogram[e]--;
		}
		// add next values to histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i+1+(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	return;
}

/// median blur filter - Filter, histogram based method for median calculation
/// @param[in] widthLine  - width of input line
/// @param[out]outLine    - array of pointers for output lines
/// @param[in] inLine     - array of pointers to input lines
void medianFilter7x7(u32 widthLine, u8 **outLine, u8 ** inLine)
{
	uint32_t i = 0;
	u8 *out;
    int j = 0;
    int histogram[256];
    int e = 0;
    u32 MED_HEIGHT = 7;
    u32 MED_WIDTH  = 7;
    u32 MED_LIMIT = (MED_WIDTH*MED_HEIGHT)/2 + 1;

    out = *outLine;

	for (i=0; i<256; i++) histogram[i] = 0;
	// build initial histogram
	for (i=0; i<MED_HEIGHT; i++)
	{
		for (j=0; j<MED_WIDTH; j++)
		{
			e =inLine[i][j-(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	for (i=0; i<widthLine; i++)
	{
		e = 0;
		j = 0;
		// use histogram
		while (j<256)
		{
			e+=histogram[j];
			if (e<MED_LIMIT)
			{
				j++;
			} else
			{
				out[i] = j;
				j = 256;
			}
		}
		// substract previous values from histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i-(MED_WIDTH>>1)];
			histogram[e]--;
		}
		// add next values to histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i+1+(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	return;
}

/// median blur filter - Filter, histogram based method for median calculation
/// @param[in] widthLine  - width of input line
/// @param[out]outLine    - array of pointers for output lines
/// @param[in] inLine     - array of pointers to input lines
void medianFilter9x9(u32 widthLine, u8 **outLine, u8 ** inLine)
{
	uint32_t i = 0;
	u8 *out;
    int j = 0;
    int histogram[256];
    int e = 0;
    u32 MED_HEIGHT = 9;
    u32 MED_WIDTH  = 9;
    u32 MED_LIMIT = (MED_WIDTH*MED_HEIGHT)/2 + 1;

    out = *outLine;

	for (i=0; i<256; i++) histogram[i] = 0;
	// build initial histogram
	for (i=0; i<MED_HEIGHT; i++)
	{
		for (j=0; j<MED_WIDTH; j++)
		{
			e =inLine[i][j-(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	for (i=0; i<widthLine; i++)
	{
		e = 0;
		j = 0;
		// use histogram
		while (j<256)
		{
			e+=histogram[j];
			if (e<MED_LIMIT)
			{
				j++;
			} else
			{
				out[i] = j;
				j = 256;
			}
		}
		// substract previous values from histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i-(MED_WIDTH>>1)];
			histogram[e]--;
		}
		// add next values to histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i+1+(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	return;
}

/// median blur filter - Filter, histogram based method for median calculation
/// @param[in] widthLine  - width of input line
/// @param[out]outLine    - array of pointers for output lines
/// @param[in] inLine     - array of pointers to input lines
void medianFilter11x11(u32 widthLine, u8 **outLine, u8 ** inLine)
{
	uint32_t i = 0;
	u8 *out;
    int j = 0;
    int histogram[256];
    int e = 0;
    u32 MED_HEIGHT = 11;
    u32 MED_WIDTH  = 11;
    u32 MED_LIMIT = (MED_WIDTH*MED_HEIGHT)/2 + 1;

    out = *outLine;

	for (i=0; i<256; i++) histogram[i] = 0;
	// build initial histogram
	for (i=0; i<MED_HEIGHT; i++)
	{
		for (j=0; j<MED_WIDTH; j++)
		{
			e =inLine[i][j-(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	for (i=0; i<widthLine; i++)
	{
		e = 0;
		j = 0;
		// use histogram
		while (j<256)
		{
			e+=histogram[j];
			if (e<MED_LIMIT)
			{
				j++;
			} else
			{
				out[i] = j;
				j = 256;
			}
		}
		// substract previous values from histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i-(MED_WIDTH>>1)];
			histogram[e]--;
		}
		// add next values to histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i+1+(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	return;
}

/// median blur filter - Filter, histogram based method for median calculation
/// @param[in] widthLine  - width of input line
/// @param[out]outLine    - array of pointers for output lines
/// @param[in] inLine     - array of pointers to input lines
void medianFilter13x13(u32 widthLine, u8 **outLine, u8 ** inLine)
{
	uint32_t i = 0;
	u8 *out;
    int j = 0;
    int histogram[256];
    int e = 0;
    u32 MED_HEIGHT = 13;
    u32 MED_WIDTH  = 13;
    u32 MED_LIMIT = (MED_WIDTH*MED_HEIGHT)/2 + 1;

    out = *outLine;

	for (i=0; i<256; i++) histogram[i] = 0;
	// build initial histogram
	for (i=0; i<MED_HEIGHT; i++)
	{
		for (j=0; j<MED_WIDTH; j++)
		{
			e =inLine[i][j-(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	for (i=0; i<widthLine; i++)
	{
		e = 0;
		j = 0;
		// use histogram
		while (j<256)
		{
			e+=histogram[j];
			if (e<MED_LIMIT)
			{
				j++;
			} else
			{
				out[i] = j;
				j = 256;
			}
		}
		// substract previous values from histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i-(MED_WIDTH>>1)];
			histogram[e]--;
		}
		// add next values to histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i+1+(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	return;
}

/// median blur filter - Filter, histogram based method for median calculation
/// @param[in] widthLine  - width of input line
/// @param[out]outLine    - array of pointers for output lines
/// @param[in] inLine     - array of pointers to input lines
void medianFilter15x15(u32 widthLine, u8 **outLine, u8 ** inLine)
{
	uint32_t i = 0;
	u8 *out;
    int j = 0;
    int histogram[256];
    int e = 0;
    u32 MED_HEIGHT = 15;
    u32 MED_WIDTH  = 15;
    u32 MED_LIMIT = (MED_WIDTH*MED_HEIGHT)/2 + 1;

    out = *outLine;

	for (i=0; i<256; i++) histogram[i] = 0;
	// build initial histogram
	for (i=0; i<MED_HEIGHT; i++)
	{
		for (j=0; j<MED_WIDTH; j++)
		{
			e =inLine[i][j-(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	for (i=0; i<widthLine; i++)
	{
		e = 0;
		j = 0;
		// use histogram
		while (j<256)
		{
			e+=histogram[j];
			if (e<MED_LIMIT)
			{
				j++;
			} else
			{
				out[i] = j;
				j = 256;
			}
		}
		// substract previous values from histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i-(MED_WIDTH>>1)];
			histogram[e]--;
		}
		// add next values to histogram
		for (j=0; j<MED_HEIGHT; j++)
		{
			e = inLine[j][i+1+(MED_WIDTH>>1)];
			histogram[e]++;
		}
	}
	return;
}


/// sobel filter - Filter, calculates magnitude
/// @param[in] in     - array of pointers to input lines
/// @param[out]out    - array of pointers for output lines
/// @param[in] width  - width of input line
void sobel(u8** in, u8** out, u32 width)
{
    int i,x,y;
    u8* lines[5], *outLine;
	fp32 sx=0, sy=0,s=0;
    float sum;
    unsigned char* aux;
    aux=(*out);

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


    //Initialize lines pointers
    lines[0]=*in;
    lines[1]=*(in+1);
    lines[2]=*(in+2);


    //Go on the whole line
    for (i=0;i<width;i++){
		sx = 	 VertSobel[0][0]*lines[0][i-1]+ VertSobel[0][1]*lines[0][i] +VertSobel[0][2]*lines[0][i+1]+
				 VertSobel[1][0]*lines[1][i-1]+ VertSobel[1][1]*lines[1][i] +VertSobel[1][2]*lines[1][i+1]+
				 VertSobel[2][0]*lines[2][i-1]+ VertSobel[2][1]*lines[2][i] +VertSobel[2][2]*lines[2][i+1];

		sy = 	 HorzSobel[0][0]*lines[0][i-1]+ HorzSobel[0][1]*lines[0][i] +HorzSobel[0][2]*lines[0][i+1]+
				 HorzSobel[1][0]*lines[1][i-1]+ HorzSobel[1][1]*lines[1][i] +HorzSobel[1][2]*lines[1][i+1]+
				 HorzSobel[2][0]*lines[2][i-1]+ HorzSobel[2][1]*lines[2][i] +HorzSobel[2][2]*lines[2][i+1];
		s= sqrtf(sx*sx+ sy*sy);
		if (s>255)
			aux[i]=255;
		else
			aux[i] = (u8)s;
    }
    return;
}


/// pyrdown filter - downsample even lines and even cols
/// @param[in] in     - array of pointers to input lines
/// @param[out]out    - array of pointers for output lines
/// @param[in] width  - width of input line
void pyrdown(u8 **inLine,u8 **out,int width)
{
	unsigned int gaus_matrix[3] = {16, 64,96 };
	int i, j;
	u8 outLine11[1920];
	u8 *outLine1;
	u8 *outLine;

	outLine1 = outLine11;
	outLine1[0] = 0;
	outLine1[1] = 0;
	outLine1 = outLine1 + 2;

	outLine=*out;

	for (i = 0; i < width; i++)
	{
        outLine1[i] = (((inLine[0][i] + inLine[4][i]) * gaus_matrix[0]) + ((inLine[1][i] + inLine[3][i]) * gaus_matrix[1]) + (inLine[2][i]  * gaus_matrix[2]))>>8;
	}

	outLine1[width] = 0;
	outLine1[width + 1] = 0;

    for (j = 0; j<width;j+=2)
    {
        outLine[j>>1] = (((outLine1[j-2] + outLine1[j+2]) * gaus_matrix[0]) + ((outLine1[j-1] + outLine1[j+1]) * gaus_matrix[1]) + (outLine1[j]  * gaus_matrix[2]))>>8;
    }

	return;
}


/// CvtColorBGRtoGray - Convert color image to gray
/// @param[in] src     - pointer to input image ( Color BGR )
/// @param[out]dst     - pointer to output image ( Gray )
void CvtColorBGRtoGray( Mat* src, Mat* dst)
{
    static const float coeffs[] = { 0.114, 0.587, 0.299 };
    float cb = coeffs[0], cg = coeffs[1], cr = coeffs[2];

    for (int i = 0; i < src->rows; i++)
        for (int j = 0; j < src->cols; j++)
        {
             u8 aux = cvRound( cb * src->data[i * src->step + 3 * j] +
                               cg * src->data[i * src->step + 3 * j + 1] +
                               cr * src->data[i * src->step + 3 * j + 2]);
             dst->data[i * dst->step + j] = aux > 255 ? (u8)255 : (u8)aux;
        }
}


/// gaussian filter
/// @param[in] in        - array of pointers to input lines
/// @param[out] out      - array of pointers for output lines
/// @param[in] width     - width of input line
void gauss(u8** inLine, u8** out, u32 width)
{
	fp32 gaus_matrix[3] = {1.4, 3.0, 3.8 };
	int i, j;
	fp32 outLine1[1920 + 4];
	u8 *outLine;


	outLine=*out;

	for (i = 0; i < width + 4; i++)
	{
        outLine1[i] = (((inLine[0][i] + inLine[4][i]) * gaus_matrix[0]) + ((inLine[1][i] + inLine[3][i]) * gaus_matrix[1]) + (inLine[2][i]  * gaus_matrix[2]));
	}

    for (j = 0; j<width;j++)
    {
        outLine[j] = (u8)((((outLine1[j] + outLine1[j+4]) * gaus_matrix[0]) + ((outLine1[j+1] + outLine1[j+3]) * gaus_matrix[1]) + (outLine1[j+2]  * gaus_matrix[2]))/159);
    }
    return;
}


/// SAD (sum of absolute differences) 5x5
///@param[in] in1		- array of pointers to input lines from the first image
///@param[in] in2		- array of pointers to input lines from the second image
///@param[out] out	- array of pointers for output line
///@param[in] width     	- width of input line

void sumOfAbsDiff5x5(u8** in1, u8** in2, u8** out, u32 width)
{
    int x, y;
    unsigned int i;
    u8 *lines1[5], *lines2[5];
    float sum;

	for(i = 0; i < 5; i++)
	{
		lines1[i] = in1[i];
		lines2[i] = in2[i];
	}

    for (i = 0; i < width; i++){
        sum = 0.0;
        for (x = 0; x < 5; x++)
        {
            for (y = 0; y < 5; y++)
            {
                sum += fabs((fp32)(lines1[x][y] - lines2[x][y]));
            }
            lines1[x]++;
            lines2[x]++;
        }
        if (sum >= 255)
            sum = 255.0;
        out[0][i] = (u8)(sum);
    }
    return;
}


/// SAD (sum of absolute differences) 11x11
///@param[in] in1		- array of pointers to input lines from the first image
///@param[in] in2		- array of pointers to input lines from the second image
///@param[out] out	- array of pointers for output line
///@param[in] width    	- width of input line

void sumOfAbsDiff11x11(u8** in1, u8** in2, u8** out, u32 width)
{
    int x, y;
    unsigned int i;
    u8 *lines1[11], *lines2[11];
    float sum;
	
	for(i = 0; i < 11; i++)
	{
		lines1[i] = in1[i];
		lines2[i] = in2[i];
	}

    for (i = 0; i < width; i++){
        sum = 0.0;
        for (x = 0; x < 11; x++)
        {
            for (y = 0; y < 11; y++)
            {
                sum += fabs((fp32)(lines1[x][y - 5] - lines2[x][y - 5]));
            }
            lines1[x]++;
            lines2[x]++;
        }
        if (sum >= 255)
            sum = 255.0;
        out[0][i] = (u8)(sum);
    }
    return;
}


/// SAD (sum of absolute differences) 21x21
///@param[in] in1		- array of pointers to input lines from the first image
///@param[in] in2		- array of pointers to input lines from the second image
///@param[out] out	- array of pointers for output line
///@param[in] width    	- width of input line

void sumOfAbsDiff21x21(u8** in1, u8** in2, u8** out, u32 width)
{
    int x, y;
    unsigned int i;
    u8 *lines1[21], *lines2[21];
    float sum;

	for(i = 0; i < 21; i++)
	{
		lines1[i] = in1[i];
		lines2[i] = in2[i];
	} 

    for (i = 0; i < width; i++){
        sum = 0.0;
        for (x = 0; x < 21; x++)
        {
            for (y = 0; y < 21; y++)
            {
                sum += fabs((fp32)(lines1[x][y] - lines2[x][y]));
            }
            lines1[x]++;
            lines2[x]++;
        }
        if (sum >= 255)
            sum = 255.0;
        out[0][i] = (u8)(sum);
    }
    return;
}

/// Convert from 12 bpp to 8 bpp
///@param[in]   in          - pointer to one input line
///@param[out]  out         - pointer to one output line
///@param[in]   width       - number of pixels of the input line
void convert12BppTo8Bpp(u8* out, u8* in, u32 width)
{
    u8* inLine = in;
    u8* outLine = out;
    
    for (u32 i = 0; i < width; i++)
    {
        outLine[i] = (inLine[i << 1] >> 4) | (inLine[(i << 1) + 1] << 4);
    }
}

}
