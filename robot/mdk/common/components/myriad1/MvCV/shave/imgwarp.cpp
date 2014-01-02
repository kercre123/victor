#include <math.h>

#include "imgwarp.h"
#include "mvcv.h"

namespace mvcv
{

Mat getRotationMatrix2D(Point2f center, float angle, float scale)
{
    angle *= (float)CV_PI/180;
    float alpha = cos(angle) * scale;
    float beta = sin(angle) * scale;

	Mat M(2, 3, CV_32F);
    float* m = (float*)M.data;

    m[0] = alpha;
    m[1] = beta;
    m[2] = (1 - alpha) * center.x - beta * center.y;
    m[3] = -beta;
    m[4] = alpha;
    m[5] = beta * center.x + (1 - alpha) * center.y;

    return M;
}

/// WarpAffineTransformBilinear kernel
/// @param[in] in         - address of pointer to input frame
/// @param[in] out        - address of pointer to input frame
/// @param[in] in_width   - width of input frame
/// @param[in] in_height  - height of input frame
/// @param[in] out_width  - width of output frame
/// @param[in] out_height - height of output frame
/// @param[in] in_stride  - stride of input frame
/// @param[in] out_stride - stride of output frame
/// @param[in] fill_color - pointer to filled color value (if NULL no fill color)
/// @param[in] mat        - inverted matrix from warp transformation
void WarpAffineTransformBilinear(u8** in, u8** out, u32 in_width, u32 in_height, u32 out_width, u32 out_height, u32 in_stride, u32 out_stride, u8* fill_color, float imat[2][3])
{
    float xstart_r, ystart_r;
    float xstep_c, ystep_c; /* change in src x,y as we step across target cols */
    float xstep_r, ystep_r; /* change in src x,y as we step down target rows */
    float x, y;
    int   row, col;
    float xf, yf;
    float S0, S1, S2, S3;
    float cx, cy;
    u8    *input  = *in;
    u8    *output = *out;
    u8    *output_aux;

    xstep_c  = imat[0][0];
    xstep_r  = imat[0][1];
    xstart_r = imat[0][2];
    ystep_c  = imat[1][0];
    ystep_r  = imat[1][1];
    ystart_r = imat[1][2];

    for (row = 0; row < out_height; row++)
    {
        output_aux = output;
        x = xstart_r; y = ystart_r;
        for (col = 0; col < out_width; col++)
        {
            int xi, yi;
            /* compute input addresses */
            xf = floor(x);
            yf = floor(y);
            xi = (int)xf;
            yi = (int)yf;
            if(fill_color != NULL)
            {
                S0 = (float)(*fill_color);
            }
            if((xi >= -1) && (xi <= (in_width - 1)) && (yi >= -1) && (yi <= (in_height - 1)))
            {
                /* Fractional portions of source location are the interpolating coefficients */
                cx = x - xf;
                cy = y - yf;

                /* Get nearest four neighbors around source location. */
                S0 = (float)input[yi*in_stride             + xi    ];
                S1 = (float)input[yi*in_stride             + xi + 1];
                S2 = (float)input[yi*in_stride + in_stride + xi    ];
                S3 = (float)input[yi*in_stride + in_stride + xi + 1];

                /* Bi-linear interpolation. */
                S0 = (1.0 - cx)*S0 + cx*S1;
                S2 = (1.0 - cx)*S2 + cx*S3;
                S0 = (1.0 - cy)*S0 + cy*S2;
            }
            if(fill_color != NULL)
            {
                *output_aux = (u8)((int)S0);
            }
            output_aux++;
            x += xstep_c;
            y += ystep_c;
        }
        output += out_stride;
        xstart_r += xstep_r; ystart_r += ystep_r;
    }
}

/// WarpAffineTransformNN kernel
/// @param[in] in         - address of pointer to input frame
/// @param[in] out        - address of pointer to input frame
/// @param[in] in_width   - width of input frame
/// @param[in] in_height  - height of input frame
/// @param[in] out_width  - width of output frame
/// @param[in] out_height - height of output frame
/// @param[in] in_stride  - stride of input frame
/// @param[in] out_stride - stride of output frame
/// @param[in] fill_color - pointer to filled color value (if NULL no fill color)
/// @param[in] mat        - inverted matrix from warp transformation
void WarpAffineTransformNN(u8** in, u8** out, u32 in_width, u32 in_height, u32 out_width, u32 out_height, u32 in_stride, u32 out_stride, u8* fill_color, float imat[2][3])
{
    float xstart_r, ystart_r;
    float xstep_c, ystep_c; /* change in src x,y as we step across target cols */
    float xstep_r, ystep_r; /* change in src x,y as we step down target rows */
    float x, y;
    int   row, col;
    float xf, yf;
    u8    S0;
    u8    *input  = *in;
    u8    *output = *out;
    u8    *output_aux;

    xstep_c  = imat[0][0];
    xstep_r  = imat[0][1];
    xstart_r = imat[0][2];
    ystep_c  = imat[1][0];
    ystep_r  = imat[1][1];
    ystart_r = imat[1][2];

    for (row = 0; row < out_height; row++)
    {
        output_aux = output;
        x = xstart_r; y = ystart_r;
        for (col = 0; col < out_width; col++)
        {
            int xi, yi;
            /* compute input addresses */
            xf = floor(x);
            yf = floor(y);
            xi = (int)xf;
            yi = (int)yf;
            if(fill_color != NULL)
            {
                S0 = (*fill_color);
            }
            if((xi >= -1) && (xi <= (in_width - 1)) && (yi >= -1) && (yi <= (in_height - 1)))
            {
                /* Get nearest four neighbors around source location. */
                S0 = input[yi*in_stride + xi];
            }
            if(fill_color != NULL)
            {
                *output_aux = S0;
            }
            output_aux++;
            x += xstep_c;
            y += ystep_c;
        }
        output += out_stride;
        xstart_r += xstep_r; ystart_r += ystep_r;
    }
}

}