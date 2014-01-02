///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved
///            For License Warranty see: common/license.txt
///
/// @brief     ImgProc kernels definitions
///
/// This is the API to the MvCV kernels library
///

#ifndef __IMGPROC_H__
#define __IMGPROC_H__

#include "mat.h"

#ifdef __cplusplus
namespace mvcv
{
#endif

//Various thresholding methods
enum 
{
    //THRESH_BINARY        = CV_THRESH_BINARY,
    //THRESH_BINARY_INV    = CV_THRESH_BINARY_INV,
    //THRESH_TRUNC        = CV_THRESH_TRUNC,
    THRESH_TOZERO        = CV_THRESH_TOZERO
    //THRESH_TOZERO_INV    = CV_THRESH_TOZERO_INV,
    //THRESH_MASK        = CV_THRESH_MASK,
    //THRESH_OTSU        = CV_THRESH_OTSU
};

//Various border interpolation methods
enum 
{ 
    //BORDER_REPLICATE    = IPL_BORDER_REPLICATE,
    //BORDER_CONSTANT        = IPL_BORDER_CONSTANT,
    //BORDER_REFLECT        = IPL_BORDER_REFLECT,
    //BORDER_WRAP            = IPL_BORDER_WRAP,
    BORDER_REFLECT_101    = IPL_BORDER_REFLECT_101,
    //BORDER_REFLECT101    = BORDER_REFLECT_101,
    //BORDER_TRANSPARENT    = IPL_BORDER_TRANSPARENT,
    BORDER_DEFAULT        = BORDER_REFLECT_101
    //BORDER_ISOLATED        = 16
};

enum
{
    CV_MEDIAN = 0
};

///Computes per-element absolute difference between 2 arrays or between array and a scalar.
///@param[in]  src1 The first input array
///@param[in]  src2 The second input array; Must be the same size and same type as src1
///@param[out] dst  The destination array; it will have the same size and same type as src1
void absdiff(const Mat& src1, const Mat& src2, Mat& dst);

///Computes absolute value of each matrix element
///@param src matrix or matrix expression
Mat abs(Mat& src);

/*///Applies a fixed-level threshold to each array element
///@param[in] src  Source array (single-channel, 8-bit of 32-bit floating point)
///@param[out] dst Destination array; will have the same size and the same type as src
///@param[in] thresh Threshold value
///@param[in] maxval Maximum value to use with THRESH_BINARY and THRESH_BINARY_INV thresholding types
///@param[in] type Thresholding type
///@return threshold value applied*/
float threshold(const Mat& src, Mat& dst, float thresh, float maxval, int type = THRESH_TOZERO);

///Calculates square root of array elements
///@param src The source floating-point array
///@param dst The destination array; will have the same size and the same type as src
void (cv_sqrt)(const Mat& src, Mat& dst);

///Performs per-element division of two arrays or a scalar by an array.
///@param src1  The first source array
///@param src2  The second source array; should have the same size and same type as src1
///@param dst   The destination array; will have the same size and same type as src2
///@param scale Scale factor
void divide(const Mat& src1, const Mat& src2, Mat& dst, float scale = 1);

///Finds global minimum and maximum in a whole array or sub-array
///@param[in]  src    The source single-channel array
///@param[out] minVal Pointer to returned minimum value; NULL if not required
///@param[out] maxVal Pointer to returned maximum value; NULL if not required
///@param[out] minLoc Pointer to returned minimum location (in 2D case); NULL if not required
///@param[out] maxLoc Pointer to returned maximum location (in 2D case); NULL if not required
///@param[in]  mask   The optional mask used to select a sub-array
void minMaxLoc(const Mat& src, float* minVal, float* maxVal = 0, Point* minLoc = 0, Point* maxLoc = 0, const Mat& mask = Mat());

///Convolves an image with the kernel
///@param[in] src The source image
///@param[out] dst The destination image. It will have the same size and the same number of channels as src
///@param[in] ddepth The desired depth of the destination image. 
///@param[in] kernel Convolution kernel (or rather a correlation kernel), a single-channel floating point matrix. 
///@param[in] anchor The anchor of the kernel that indicates the relative position of a filtered point within the kernel. The anchor should lie within the kernel. The special default value (-1,-1) means that the anchor is at the kernel center
///@param[in] delta The optional value added to the filtered pixels before storing them in dst
///@param[in] borderType The pixel extrapolation method
void filter2D(const Mat& src, Mat& dst, int ddepth, const Mat& kernel, Point anchor = cvPoint(-1, -1), float delta = 0, int borderType = BORDER_DEFAULT);

///Apply median filter over an image
///@param[in]  src The source image
///@param[out] dst The destination image. It will have the same size and the same number of channels as src
///@param[in]  smoothtype Type of the smoothing(by default: CV_MEDIAN)
///@param[in]  kernel_width Kernel width(by default: 3)
///@param[in]  kernel_height Kernel height(by default: 3)
///@param[in]  param3 the third parameter of smoothing operation
///@param[in]  param4 the fourth parameter of smoothing operation
///@param[in]  slice_no number of slices
///@param[in]  lines_no number of lines
void cvSmooth(const Mat& src, Mat& dst, int smoothtype = CV_MEDIAN, int kernel_width = 3, int kernel_height = 3, float param3 = 0, float param4 = 0, int slice_no = 0, int lines_no = 0);

enum
{
    Thresh_To_Zero         = 0,
    Thresh_To_Zero_Inv   = 1,
    Thresh_To_Binary     = 2,
    Thresh_To_Binary_Inv = 3,
    Thresh_Trunc         = 4
    //THRESH_MASK        = CV_THRESH_MASK,
    //THRESH_OTSU        = CV_THRESH_OTSU
};

//!@{
/// threshold kernel  computes the output image based on a threshold value and a threshold type
/// @param[in] in             - array of pointers to input lines
/// @param[out] out           - array of pointers to output lines
/// @param[in] width          - width of the input line
/// @param[in] height         - height of the input line
/// @param[in] thresh         - threshold value
/// @param[in] thresh_type    - one of the 5 available thresholding types:
///                           - Thresh_To_Zero: values below threshold are zeroed
///                           - Thresh_To_Zero_Inv: opposite of Thresh_To_Zero
///                           - Thresh_To_Binary: values below threshold are zeroed and all others are saturated to pixel max value
///                           - Thresh_To_Binary_Inv: opposite of Thresh_To_Binary
///                           - Thresh_Trunc: values above threshold are given threshold value
///                           - default mode: Thresh_Trunc
/// @return    Nothing
#ifdef __PC__

    #define thresholdKernel_asm thresholdKernel
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void thresholdKernel_asm(u8** in, u8** out, u32 width, u32 height, u8 thresh, u32 thresh_type);
#endif

    void thresholdKernel(u8** in, u8** out, u32 width, u32 height, u8 thresh, u32 thresh_type);
//!@}

//!@{
/// AbsoluteDiff kernel  computes the absolute difference of two images given as parameters(used to estimate motion)
/// @param[in] in1             - array of pointers to input lines of the first image
/// @param[in] in2             - array of pointers to input lines of the second image
/// @param[out] out            - array of pointers to output line
/// @param[in] width           - width of the input lines
/// @return    Nothing
#ifdef __PC__

    #define AbsoluteDiff_asm AbsoluteDiff
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void AbsoluteDiff_asm(u8** in1, u8** in2, u8** out, u32 width);
#endif

#ifdef __cplusplus
extern "C"
#endif
    void AbsoluteDiff(u8** in1, u8** in2, u8** out, u32 width);
//!@}

//!@{
/// The function dilates the source image using the specified structuring element.
/// This is determined by the shape of a pixel neighborhood over which the maximum is taken.
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - pointer to output line
/// @param[in]  kernel   - kernel filled with 1s and 0s which determines the image area to dilate on
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line (defaulted to one line)
/// @param[in]  k        - kernel size
#ifdef __PC__

    #define Dilate_asm Dilate
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void Dilate_asm(u8** src, u8** dst, u8** kernel, u32 width, u32 height, u32 k);
#endif
#ifdef __cplusplus
extern "C"
#endif
    void Dilate(u8** src, u8** dst, u8** kernel, u32 width, u32 height, u32 k);
//!@}

//!@{
/// The function dilates the source image using the specified structuring element.
/// This is determined by the shape of a pixel neighborhood over which the maximum is taken.
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - array of pointers to output lines
/// @param[in]  kernel   - kernel filled with 1s and 0s which determines the image area to dilate on
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line

#ifdef __PC__

    #define Dilate3x3_asm Dilate3x3
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void Dilate3x3_asm(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
#endif
#ifdef __cplusplus
extern "C"
#endif
    void Dilate3x3(u8** src, u8** dst, u8** kernel, u32 width, u32 height);


#ifdef __PC__

    #define Dilate5x5_asm Dilate5x5
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void Dilate5x5_asm(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
#endif
#ifdef __cplusplus
extern "C"
#endif
    void Dilate5x5(u8** src, u8** dst, u8** kernel, u32 width, u32 height);


#ifdef __PC__

    #define Dilate7x7_asm Dilate7x7
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void Dilate7x7_asm(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
#endif
#ifdef __cplusplus
extern "C"
#endif
    void Dilate7x7(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
//!@}

//!@{
/// The function erodes the source image using the specified structuring element.
/// This is determined by the shape of a pixel neighborhood over which the minimum is taken.
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - pointer to output line
/// @param[in]  kernel   - kernel filled with 1s and 0s which determines the image area to erode on
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line
/// @param[in]  k        - kernel size
#ifdef __PC__

    #define Erode_asm Erode
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void Erode_asm(u8** src, u8** dst, u8** kernel, u32 width, u32 height, u32 k);
#endif
#ifdef __cplusplus
extern "C"
#endif
    void Erode(u8** src, u8** dst, u8** kernel, u32 width, u32 height, u32 k);
//!@}

//!@{
/// The function erodes the source image using the specified structuring element.
/// This is determined by the shape of a pixel neighborhood over which the minimum is taken.
/// @param[in]  src      - array of pointers to input lines of the input image
/// @param[out] dst      - pointer to output line
/// @param[in]  kernel   - kernel filled with 1s and 0s which determines the image area to erode on
/// @param[in]  width    - width  of the input line
/// @param[in]  height   - height of the input line
#ifdef __PC__
    #define Erode3x3_asm Erode3x3
#endif
#ifdef __MOVICOMPILE__
    extern "C"
    void Erode3x3_asm(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
#endif
#ifdef __cplusplus
extern "C"
#endif
    void Erode3x3(u8** src, u8** dst, u8** kernel, u32 width, u32 height);

#ifdef __PC__
    #define Erode5x5_asm Erode5x5
#endif
#ifdef __MOVICOMPILE__
    extern "C"
    void Erode5x5_asm(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
#endif
#ifdef __cplusplus
extern "C"
#endif
    void Erode5x5(u8** src, u8** dst, u8** kernel, u32 width, u32 height);

#ifdef __PC__
    #define Erode7x7_asm Erode7x7
#endif
#ifdef __MOVICOMPILE__
    extern "C"
    void Erode7x7_asm(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
#endif
#ifdef __cplusplus
extern "C"
#endif
    void Erode7x7(u8** src, u8** dst, u8** kernel, u32 width, u32 height);
//!@}
	
//!@{		
/// boxfilter kernel that makes average on variable kernel size        
/// @param[in] in        - array of pointers to input lines      
/// @param[out] out      - array of pointers to output lines    
/// @param[in] k_size    -  kernel size for computing pixels. Eg. 3 for 3x3, 4 for 4x4  
/// @param[in] normalize - 1 to normalize to kernel size, 0 otherwise 
/// @param[in] width     - width of input line
#ifdef __PC__

    #define boxfilter_asm boxfilter
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void boxfilter_asm(u8** in, u8** out, u32 k_size, u32 normalize, u32 width);
#endif
    void boxfilter(u8** in, u8** out, u32 k_size, u32 normalize, u32 width);
//!@}

//!@{	
/// boxfilter kernel that makes average on 3x3 kernel size       
/// @param[in] in        - array of pointers to input lines      
/// @param[out] out      - array of pointers to output lines    
/// @param[in] normalize - 1 to normalize to kernel size, 0 otherwise 
/// @param[in] width     - width of input line
#ifdef __PC__

    #define boxfilter3x3_asm boxfilter3x3
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void boxfilter3x3_asm(u8** in, u8** out, u32 normalize, u32 width);
#endif
    void boxfilter3x3(u8** in, u8** out, u32 normalize, u32 width);
//!@}
	
//!@{	
/// boxfilter kernel that makes average on 5x5 kernel size       
/// @param[in] in        - array of pointers to input lines      
/// @param[out] out      - array of pointers to output lines    
/// @param[in] normalize - 1 to normalize to kernel size, 0 otherwise 
/// @param[in] width     - width of input line
#ifdef __PC__

    #define boxfilter5x5_asm boxfilter5x5
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void boxfilter5x5_asm(u8** in, u8** out, u32 normalize, u32 width);
#endif
    void boxfilter5x5(u8** in, u8** out, u32 normalize, u32 width);
//!@}
	
//!@{	
/// boxfilter kernel that makes average on 7x7 kernel size       
/// @param[in] in        - array of pointers to input lines      
/// @param[out] out      - array of pointers to output lines    
/// @param[in] normalize - 1 to normalize to kernel size, 0 otherwise 
/// @param[in] width     - width of input line
#ifdef __PC__

    #define boxfilter7x7_asm boxfilter7x7
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void boxfilter7x7_asm(u8** in, u8** out, u32 normalize, u32 width);
#endif
    void boxfilter7x7(u8** in, u8** out, u32 normalize, u32 width);
//!@}
	
//!@{
    /// boxfilter kernel that makes average on 9x9 kernel size
    /// @param[in] in        - array of pointers to input lines
    /// @param[out] out      - array of pointers to output lines
    /// @param[in] normalize - 1 to normalize to kernel size, 0 otherwise 
    /// @param[in] width     - width of input line
    #ifdef __PC__

        #define boxfilter9x9_asm boxfilter9x9
    #endif

    #ifdef __MOVICOMPILE__
        extern "C"
        void boxfilter9x9_asm(u8** in, u8** out, u32 normalize, u32 width);
    #endif
        void boxfilter9x9(u8** in, u8** out, u32 normalize, u32 width);
//!@}
//!@{
    /// boxfilter kernel that makes average on 11x11 kernel size
    /// @param[in] in        - array of pointers to input lines
    /// @param[out] out      - array of pointers to output lines
    /// @param[in] normalize - 1 to normalize to kernel size, 0 otherwise 
    /// @param[in] width     - width of input line
    #ifdef __PC__

        #define boxfilter11x11_asm boxfilter11x11
    #endif

    #ifdef __MOVICOMPILE__
        extern "C"
        void boxfilter11x11_asm(u8** in, u8** out, u32 normalize, u32 width);
    #endif
        void boxfilter11x11(u8** in, u8** out, u32 normalize, u32 width);
//!@}
//!@{
    /// boxfilter kernel that makes average on 13x13 kernel size
    /// @param[in] in        - array of pointers to input lines
    /// @param[out] out      - array of pointers to output lines
    /// @param[in] normalize - 1 to normalize to kernel size, 0 otherwise 
    /// @param[in] width     - width of input line
    #ifdef __PC__

        #define boxfilter13x13_asm boxfilter13x13
    #endif

    #ifdef __MOVICOMPILE__
        extern "C"
        void boxfilter13x13_asm(u8** in, u8** out, u32 normalize, u32 width);
    #endif
        void boxfilter13x13(u8** in, u8** out, u32 normalize, u32 width);
//!@}
//!@{
    /// boxfilter kernel that makes average on 15x15 kernel size
    /// @param[in] in        - array of pointers to input lines
    /// @param[out] out      - array of pointers to output lines
    /// @param[in] normalize - 1 to normalize to kernel size, 0 otherwise  
    /// @param[in] width     - width of input line
    #ifdef __PC__

        #define boxfilter15x15_asm boxfilter15x15
    #endif

    #ifdef __MOVICOMPILE__
        extern "C"
        void boxfilter15x15_asm(u8** in, u8** out, u32 normalize, u32 width);
    #endif
        void boxfilter15x15(u8** in, u8** out, u32 normalize, u32 width);
//!@}

//!@{
    /// boxfilter kernel that makes average on 7x1 kernel size
    /// @param[in] in        - array of pointers to input lines
    /// @param[out] out      - array of pointers to output lines
    /// @param[in] normalize - 1 to normalize to kernel size, 0 otherwise 
    /// @param[in] width     - width of input line
    #ifdef __PC__

        #define boxfilter7x1_asm boxfilter7x1
    #endif

    #ifdef __MOVICOMPILE__
        extern "C"
        void boxfilter7x1_asm(u8** in, u8** out, u32 normalize, u32 width);
    #endif
        void boxfilter7x1(u8** in, u8** out, u32 normalize, u32 width);
//!@}

//!@{
    /// boxfilter kernel that makes average on 1x7 kernel size
    /// @param[in] in        - array of pointers to input lines
    /// @param[out] out      - array of pointers to output lines
    /// @param[in] normalize - 1 to normalize to kernel size, 0 otherwise 
    /// @param[in] width     - width of input line
    #ifdef __PC__

       #define boxfilter1x7_asm boxfilter1x7
    #endif

    #ifdef __MOVICOMPILE__
       extern "C"
       void boxfilter1x7_asm(u8** in, u8** out, u32 normalize, u32 width);
    #endif
       void boxfilter1x7(u8** in, u8** out, u32 normalize, u32 width);
//!@}

//!@{
    /// boxfilter7x7separable  (monolithic implementation of 7x1 + 1x7 kernels) kernel
    /// @param[in]  in        - array of pointers to input lines
    /// @param[out] out       - array of pointers to output lines
    /// @param[out] interim   - intermediate buffer for first step
    /// @param[in]  normalize - 1 to normalize, 0 otherwise 
    /// @param[in]  width     - width of input line
    #ifdef __PC__

       #define boxfilter7x7separable_asm boxfilter7x7separable
    #endif

    #ifdef __MOVICOMPILE__
       extern "C"
       void boxfilter7x7separable_asm(u8 **in, u8 **out, u32 normalize, u32 width);
    #endif
       void boxfilter7x7separable(u8 **in, u8 **out, u32 normalize, u32 width);
//!@}


#ifdef __PC__

    #define cvtColorKernelYUVToRGB_asm cvtColorKernelYUVToRGB
    #define cvtColorKernelRGBToYUV_asm cvtColorKernelRGBToYUV
#endif

#ifdef __MOVICOMPILE__
    /// Performs color space conversion YUV420p to RGB for two consecutive lines in an image
    /// @param[in] yIn - pointer to the first of the two lines of y plane from the YUV image
    /// @param[in] uIn - pointer to the first of the two lines of u plane from the YUV image
    /// @param[in] vIn - pointer to the first of the two lines of v plane from the YUV image
    /// @param[out] out - pointer to the output line of RGB image
    /// @param[in] width - line width in pixels
    /// @return    Nothing
    ///
    extern "C"
    void cvtColorKernelYUVToRGB_asm(u8* yIn, u8* uIn, u8* vIn, u8* out, u32 width);
    /// Performs color space conversion RGB to YUV420p for two consecutive lines in an image
    /// @param[in] in - pointer to the first of the two lines from input RGB image
    /// @param[out] yOut - pointer to the y plane in the first of the two lines from the output YUV image
    /// @param[out] uOut - pointer to the u plane in the first of the two lines from the output YUV image
    /// @param[out] vOut - pointer to the v plane in the first of the two lines from the output YUV image
    /// @param[in] width - line width in pixels
    /// @return    Nothing
    ///
    extern "C"
    void cvtColorKernelRGBToYUV_asm(u8* in, u8* yOut, u8* uOut, u8* vOut, u32 width);
#endif

#ifdef __cplusplus
extern "C"
#endif
	/// Performs color space conversion: YUV420p to RGB 
	///@param[in] yIn input Y channel
	///@param[in] uIn input U channel
	///@param[in] vIn input V channel
	///@param[out] out output RGB channel
	///@param[in] width - image width in pixels
    void cvtColorKernelYUVToRGB(u8* yIn, u8* uIn, u8* vIn, u8* out, u32 width);
	
	/// Performs color space conversion: RGB to YUV420p 
	///@param[in] in input RGB channel
	///@param[out] yOut output Y channel
	///@param[out] uOut output U channel
	///@param[out] vOut output V channel
	///@param[in] width - image width in pixels
    void cvtColorKernelRGBToYUV(u8* in, u8* yOut, u8* uOut, u8* vOut, u32 width);


#ifdef __PC__

    #define cvtColorKernelYUV422ToRGB_asm cvtColorKernelYUVT422oRGB
    #define cvtColorKernelRGBToYUV422_asm cvtColorKernelRGBToYUV422
#endif	
	
	
#ifdef __MOVICOMPILE__
	/// Performs color space conversion YUV422 to RGB for one line
    /// @param[in] input - pointer to the YUV422 interleaved line
    /// @param[out] rOut - pointer to the output line that contain R values
    /// @param[out] gOut- pointer to the output line that contain G values
    /// @param[out] bOut- pointer to the output line that contain B values
    /// @param[in] width - line width
    /// @return    Nothing
	extern "C"
	void cvtColorKernelYUV422ToRGB_asm(u8** input, u8** rOut, u8** gOut, u8** bOut, u32 width);
	
	/// Performs color space conversion RGB to YUV422 for one line
    /// @param[in] rIn - pointer to the input line that contain R values from RGB
    /// @param[in] gIn - pointer to the input line that contain G values from RGB
    /// @param[in] bIn - pointer to the input line that contain B values from RGB
    /// @param[out] output - pointer to the output line - YUV422 interleaved
    /// @param[in] width - line width
    /// @return    Nothing
	extern "C"
	void cvtColorKernelRGBToYUV422_asm(u8** rIn, u8** gIn, u8** bIn, u8** output, u32 width);
#endif	

#ifdef __cplusplus
extern "C"
#endif

	/// Performs color space conversion YUV422 to RGB for one line
    /// @param[in] input - pointer to the YUV422 interleaved line
    /// @param[out] rOut - pointer to the output line that contain R values
    /// @param[out] gOut - pointer to the output line that contain G values
    /// @param[out] bOut - pointer to the output line that contain B values
    /// @param[in] width - line width

    void cvtColorKernelYUV422ToRGB(u8** input, u8** rOut, u8** gOut, u8** bOut, u32 width);
	
	/// Performs color space conversion RGB to YUV422 for one line
    /// @param[in] rIn     - pointer to the input line that contain R values from RGB
    /// @param[in] gIn     - pointer to the input line that contain G values from RGB
    /// @param[in] bIn     - pointer to the input line that contain B values from RGB
    /// @param[out] output - pointer to the output line YUV422 interleaved
    /// @param[in] width   - line width
	void cvtColorKernelRGBToYUV422(u8** rIn, u8** gIn, u8** bIn, u8** output, u32 width);		




//!@{	
 /// Performs color space conversion RGB to NV21 for one line in an image
    /// @param[in]  inR     - pointer to the first line from input R plane 
	/// @param[in]  inG     - pointer to the first line from input G plane
	/// @param[in]  in      - pointer to the first line from input B plane 	
    /// @param[Out] yOut    - pointer to the y plane in the first of the line the output NV21 image
    /// @param[out]  uvOut  - pointer to the uv plane in the first of line from the output NV21 image
    /// @param[in]   width  - line width in pixels
    /// @return     Nothing
    ///

#ifdef __PC__

     #define cvtColorRGBtoNV21_asm cvtColorRGBtoNV21
#endif

#ifdef __MOVICOMPILE__
   
    extern "C"
    void cvtColorRGBtoNV21_asm(u8** inR, u8** inG, u8** inB, u8** yOut, u8** uvOut, u32 width, u32 k);
#endif

#ifdef __cplusplus
extern "C"
#endif	
	 void cvtColorRGBtoNV21(u8** inR, u8** inG, u8** inB, u8** yOut, u8** uvOut, u32 width, u32 k);

//!@}

//!@{	
#ifdef __PC__

     #define cvtColorNV21toRGB_asm cvtColorNV21toRGB
#endif
    /// Performs color space conversion NV21 to RGB for one lines in an image
    /// @param[in] yin   - pointer to the first line from input Y plane
	/// @param[in] uvin  - pointer to the first line from input UV plane interleaved
    /// @param[out] outR - pointer to the R plane in the first line from the output RGB image
    /// @param[out] outG - pointer to the G plane in the first line from the output RGB image
    /// @param[out] outB - pointer to the B plane in the first line from the output RGB image
    /// @param[in] width - line width in pixels
    /// @return    Nothing
    ///
#ifdef __MOVICOMPILE__
   

    extern "C"
    void cvtColorNV21toRGB_asm(u8** yin, u8** uvin, u8** outR, u8** outG, u8** outB, u32 width);
#endif

#ifdef __cplusplus
extern "C"
#endif	
    void cvtColorNV21toRGB(u8** yin, u8** uvin, u8** outR, u8** outG, u8** outB, u32 width);	
//!@}


//!@{
/// Laplacian filter  - applies a Laplacian filter with custom size (see http://en.wikipedia.org/wiki/Discrete_Laplace_operator ) 
/// @param[in] in     - array of pointers to input lines
/// @param[out] out   - pointer to output line
/// @param[in] width  - width of input line
#ifdef __PC__

    #define Laplacian3x3_asm Laplacian3x3
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void Laplacian3x3_asm(u8** in, u8** out, u32 inWidth);
#endif
    void Laplacian3x3(u8** in, u8** out, u32 inWidth);

#ifdef __PC__

    #define Laplacian5x5_asm Laplacian3x3
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void Laplacian5x5_asm(u8** in, u8** out, u32 inWidth);
#endif
    void Laplacian5x5(u8** in, u8** out, u32 inWidth);

#ifdef __PC__

    #define Laplacian7x7_asm Laplacian3x3
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void Laplacian7x7_asm(u8** in, u8** out, u32 inWidth);
#endif
    void Laplacian7x7(u8** in, u8** out, u32 inWidth);
//!@}
	
//!@{	
/// integral image kernel - this kernel makes the sum of all pixels before current pixel (columns to the left and rows above) - output is u32 format
/// @param[in] in         - array of pointers to input lines      
/// @param[out] out       - array of pointers to output lines    
/// @param[in] sum        - sum of previous pixels - for this parameter we must have an array of u32 declared as global and having the width of the line
/// @param[in] width      - width of input line
#ifdef __PC__

    #define integralimage_sum_u32_asm integralimage_sum_u32
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void integralimage_sum_u32_asm(u8** in, u8** out, u32* sum, u32 width);
#endif
    void integralimage_sum_u32(u8** in, u8** out, u32* sum, u32 width);
//!@}
	
//!@{	
/// integral image kernel - this kernel makes the sum of all pixels before current pixel (columns to the left and rows above) - output is fp32 format
/// @param[in] in         - array of pointers to input lines      
/// @param[out] out       - array of pointers for output lines    
/// @param[in] sum        - sum of previous pixels - for this parameter we must have an array of fp32 declared as global and having the width of the line
/// @param[in] width      - width of input line
#ifdef __PC__

    #define integralimage_sum_f32_asm integralimage_sum_f32
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void integralimage_sum_f32_asm(u8** in, u8** out, u32* sum, u32 width);
#endif
    void integralimage_sum_f32(u8** in, u8** out, u32* sum, u32 width);
//!@}
	
//!@{
/// integral image kernel - this kernel makes the sum of all squared pixels before current pixel (columns to the left and rows above) - output is u32 format
/// @param[in] in         - array of pointers to input lines      
/// @param[out] out       - array of pointers for output lines    
/// @param[in] sum        - sum of previous pixels - for this parameter we must have an array of u32 declared as global and having the width of the line
/// @param[in] width      - width of input line
#ifdef __PC__

    #define integralimage_sqsum_u32_asm integralimage_sqsum_u32
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void integralimage_sqsum_u32_asm(u8** in, u8** out, u32* sum, u32 width);
#endif
    void integralimage_sqsum_u32(u8** in, u8** out, u32* sum, u32 width);
//!@}
	
//!@{
/// integral image kernel - this kernel makes the sum of all squared pixels before current pixel (columns to the left and rows above) - output is fp32 format
/// @param[in] in         - array of pointers to input lines      
/// @param[out] out       - array of pointers for output lines    
/// @param[in] sum        - sum of previous pixels - for this parameter we must have an array of fp32 declared as global and having the width of the line
/// @param[in] width      - width of input line
#ifdef __PC__

    #define integralimage_sqsum_f32_asm integralimage_sqsum_f32
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void integralimage_sqsum_f32_asm(u8** in, u8** out, fp32* sum, u32 width);
#endif
    void integralimage_sqsum_f32(u8** in, u8** out, fp32* sum, u32 width);
//!@}
	
//!@{
/// histogram kernel  - computes a histogram on a given line (has to be applied to all lines of an image)
/// @param[in] in     - pointer to input line      
/// @param[out] hist  - array of values from histogram
/// @param[in] width  - width of input line
#ifdef __PC__

    #define histogram_asm histogram
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void histogram_asm(u8** in, u32 *hist, u32 width);
#endif
    void histogram(u8** in, u32 *hist, u32 width);
//!@}
	
//!@{	
/// equalizehistogram kernel - makes an equalization through an input line with a given histogram
/// @param[in] in         - array of pointers to input line
/// @param[out] out       - array of pointers to output line
/// @param[in] hist       - pointer to an input array that indicates the cumulative histogram of the image
/// @param[in] width      - width of input line
#ifdef __PC__

	#define equalizeHist_asm equalizeHist
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void equalizeHist_asm(u8** in, u8** out, u32 *hist, u32 width);
#endif
    void equalizeHist(u8** in, u8** out, u32 *hist, u32 width);
//!@}
	

#ifdef __PC__
    #define Convolution3x3_asm Convolution3x3
    #define Convolution5x5_asm Convolution5x5
#endif

#ifdef __MOVICOMPILE__

	/// Convolution kernel (variable size)
	/// @param[in] in         - array of pointers to input lines
	/// @param[in] out        - pointer to output line
	/// @param[in] kernelSize - kernel size
	/// @param[in] conv       - array of values from convolution
	/// @param[in] inWidth    - width of input line
    extern "C"
    void Convolution_asm(u8** in, u8** out, u32 kernelSize, half* conv, u32 inWidth);
//!{
	/// Convolution kernel (custom size)
	/// @param[in] in         - array of pointers to input lines
	/// @param[in] out        - pointer to output line
	/// @param[in] conv       - array of values from convolution
	/// @param[in] inWidth    - width of input line
    extern "C"
    void Convolution3x3_asm(u8** in, u8** out, half conv[9], u32 inWidth);
    extern "C"
    void Convolution5x5_asm(u8** in, u8** out, half conv[25], u32 inWidth);
    extern "C"
    void Convolution7x7_asm(u8** in, u8** out, half conv[49], u32 inWidth);
    extern "C"
    void Convolution9x9_asm(u8** in, u8** out, half conv[81], u32 inWidth);
    extern "C"
    void Convolution11x11_asm(u8** in, u8** out, half conv[121], u32 inWidth);
    extern "C"
    void Convolution7x7Fp16ToU8_asm(u8** in, u8** out, half conv[49], u32 inWidth);
    extern "C"
    void Convolution7x7Fp16ToU16_asm(u8** in, half** out, half conv[49], u32 inWidth);
    extern "C"
    void Convolution1x5_asm(u8** in, u8** out, half* conv, u32 inWidth);
    extern "C"
    void Convolution1x7_asm(u8** in, u8** out, half* conv, u32 inWidth);
    extern "C"
    void Convolution1x9_asm(u8** in, u8** out, half* conv, u32 inWidth);
    extern "C"
    void Convolution1x15_asm(u8** in, u8** out, half* conv, u32 inWidth);
    extern "C"
    void Convolution5x1_asm(u8** in, u8** out, half* conv, u32 inWidth);
    extern "C"
    void Convolution7x1_asm(u8** in, u8** out, half* conv, u32 inWidth);
    extern "C"
    void Convolution9x1_asm(u8** in, u8** out, half* conv, u32 inWidth);
    extern "C"
    void Convolution15x1_asm(u8** in, u8** out, half* conv, u32 inWidth);
//!}
#endif

	/// Convolution kernel (variable size)
	/// @param[in] in         - array of pointers to input lines
	/// @param[in] out        - pointer to output line
	/// @param[in] kernelSize - kernel size
	/// @param[in] conv       - array of values from convolution
	/// @param[in] inWidth    - width of input line
    void Convolution(u8** in, u8** out, u32 kernelSize, float* conv, u32 inWidth);
//!@{
	/// Convolution kernel (custom size)
	/// @param[in] in         - array of pointers to input lines
	/// @param[in] out        - pointer to output line
	/// @param[in] conv       - array of values from convolution
	/// @param[in] inWidth    - width of input line
    void Convolution3x3(u8** in, u8** out, float conv[9], u32 inWidth);
    void Convolution5x5(u8** in, u8** out, float conv[25], u32 inWidth);
    void Convolution7x7(u8** in, u8** out, float conv[49], u32 inWidth);
    void Convolution9x9(u8** in, u8** out, float conv[81], u32 inWidth);
    void Convolution11x11(u8** in, u8** out, float conv[121], u32 inWidth);
    void Convolution1x5(u8** in, u8** out, float* conv, u32 inWidth);
    void Convolution1x7(u8** in, u8** out, float* conv, u32 inWidth);
    void Convolution1x9(u8** in, u8** out, float* conv, u32 inWidth);
    void Convolution1x15(u8** in, u8** out, float* conv, u32 inWidth);
    void Convolution5x1(u8** in, u8** out, float* conv, u32 inWidth);
    void Convolution7x1(u8** in, u8** out, float* conv, u32 inWidth);
    void Convolution9x1(u8** in, u8** out, float* conv, u32 inWidth);
    void Convolution15x1(u8** in, u8** out, float* conv, u32 inWidth);
//!@}
	
//!@{
    /// Convolution separable kernel (separable NxN convolution = Nx1 convolution + 1xN convolution)
    /// @param[in] in         - array of pointers to input lines
    /// @param[in] out        - pointer to output line
    /// @param[in] interm     - pointer to intermediary line
    /// @param[in] vconv      - array of values from vertical convolution
    /// @param[in] hconv      - array of values from horizontal convolution
    /// @param[in] inWidth    - width of input line
#ifdef __MOVICOMPILE__
    extern "C"
    void ConvolutionSeparable5x5(u8** in, u8** out, u8** interm, half vconv[5], half hconv[5], u32 inWidth);
    extern "C"
    void ConvolutionSeparable7x7(u8** in, u8** out, u8** interm, half vconv[7], half hconv[7], u32 inWidth);
    extern "C"
    void ConvolutionSeparable9x9(u8** in, u8** out, u8** interm, half vconv[9], half hconv[9], u32 inWidth);
    extern "C"
    void ConvolutionSeparable15x15(u8** in, u8** out, u8** interm, half vconv[15], half hconv[15], u32 inWidth);
#else
    extern "C"
    void ConvolutionSeparable5x5(u8** in, u8** out, u8** interm, float vconv[5], float hconv[5], u32 inWidth);
    extern "C"
    void ConvolutionSeparable7x7(u8** in, u8** out, u8** interm, float vconv[7], float hconv[7], u32 inWidth);
    extern "C"
    void ConvolutionSeparable9x9(u8** in, u8** out, u8** interm, float vconv[9], float hconv[9], u32 inWidth);
    extern "C"
    void ConvolutionSeparable15x15(u8** in, u8** out, u8** interm, float vconv[15], float hconv[15], u32 inWidth);
#endif
//!@}

//!@{	
/// channel extract kernel - extracts one of the R, G, B, plane from an interleaved RGB line 
/// @param[in] in         - pointers to input line      
/// @param[out]out        - pointers for output line    
/// @param[in] width      - width of input line
/// @param[in] plane      - number 0 to extract plane R, 1 for extracting G, 2 for extracting B
#ifdef __PC__

    #define channelExtract_asm channelExtract
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void channelExtract_asm(u8** in, u8** out, u32 width, u32 plane);
#endif
    void channelExtract(u8** in, u8** out, u32 width, u32 plane);
//!@}
	
//!@{
/// median blur filter   - Filter, histogram based method for median calculation
/// @param[in] widthLine - width of input line
/// @param[out] outLine  - pointer to output line
/// @param[in] inLine    - array of pointers to input lines
#ifdef __PC__

    #define medianFilter3x3_asm medianFilter3x3
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void medianFilter3x3_asm(u32 widthLine, u8 **outLine, u8 ** inLine);
#endif
    void medianFilter3x3(u32 widthLine, u8 **outLine, u8 ** inLine);
	
#ifdef __PC__

    #define medianFilter5x5_asm medianFilter5x5
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void medianFilter5x5_asm(u32 widthLine, u8 **outLine, u8 ** inLine);
#endif
    void medianFilter5x5(u32 widthLine, u8 **outLine, u8 ** inLine);

#ifdef __PC__

    #define medianFilter7x7_asm medianFilter7x7
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void medianFilter7x7_asm(u32 widthLine, u8 **outLine, u8 ** inLine);
#endif
    void medianFilter7x7(u32 widthLine, u8 **outLine, u8 ** inLine);

#ifdef __PC__

    #define medianFilter9x9_asm medianFilter9x9
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void medianFilter9x9_asm(u32 widthLine, u8 **outLine, u8 ** inLine);
#endif
    void medianFilter9x9(u32 widthLine, u8 **outLine, u8 ** inLine);

#ifdef __PC__

    #define medianFilter11x11_asm medianFilter11x11
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void medianFilter11x11_asm(u32 widthLine, u8 **outLine, u8 ** inLine);
#endif
    void medianFilter11x11(u32 widthLine, u8 **outLine, u8 ** inLine);

#ifdef __PC__

    #define medianFilter13x13_asm medianFilter13x13
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void medianFilter13x13_asm(u32 widthLine, u8 **outLine, u8 ** inLine);
#endif
    void medianFilter13x13(u32 widthLine, u8 **outLine, u8 ** inLine);

#ifdef __PC__

    #define medianFilter15x15_asm medianFilter15x15
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void medianFilter15x15_asm(u32 widthLine, u8 **outLine, u8 ** inLine);
#endif
        void medianFilter15x15(u32 widthLine, u8 **outLine, u8 ** inLine);
//!@}
		
//!@{
/// sobel filter      - Filter, calculates magnitude (see http://en.wikipedia.org/wiki/Sobel_operator)
/// @param[in] in     - array of pointers to input lines
/// @param[out] out   - pointer to output line
/// @param[in] width  - width of input line
#ifdef __PC__

    #define sobel_asm sobel
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void sobel_asm(u8** in, u8** out, u32 width);
#endif
    void sobel(u8** in, u8** out, u32 width);
//!@}
	
//!@{
/// pyrdown filter - downsample even lines and even cols (monolithic implementation of GaussVx2 + GaussHx2 kernels) 
/// @param[in] inLine     - array of pointers to input lines
/// @param[out] outLine   - pointer to output lines
/// @param[in] width      - width of input line
#ifdef __PC__

    #define pyrdown_asm pyrdown
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void pyrdown_asm(u8 **inLine,u8 **outLine,int width);
#endif
    void pyrdown(u8 **inLine,u8 **outLine,int width);
//!@}
	
//!@{
/// gaussian filter - apply a gaussian blur (see http://softwarebydefault.files.wordpress.com/2013/06/guassianblur5x5kernel.jpg ).
/// @param[in] inLine     - array of pointers to input lines
/// @param[out] out       - pointer to output line
/// @param[in] width      - width of input line
#ifdef __PC__

    #define gauss_asm gauss
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void gauss_asm(u8 **inLine,u8 **out, u32 width);
#endif
    void gauss(u8 **inLine,u8 **out, u32 width);
//!@}


/// CvtColorBGRtoGray - Convert color image to gray
/// @param[in] src     - pointer to input image ( Color BGR )
/// @param[out] dst     - pointer to output image ( Gray )
    void CvtColorBGRtoGray( Mat* src, Mat* dst);

//!@{
/// SAD (sum of absolute differences) custom size
///@param[in] in1		- array of pointers to input lines from the first image
///@param[in] in2		- array of pointers to input lines from the second image
///@param[out] out	- pointer for output line
///@param[in] width    	- width of input line

#ifdef __PC__

    #define sumOfAbsDiff5x5_asm sumOfAbsDiff5x5
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void sumOfAbsDiff5x5_asm(u8** in1, u8** in2, u8** out, u32 width);
#endif
	void sumOfAbsDiff5x5(u8** in1, u8** in2, u8** out, u32 width);


#ifdef __PC__

    #define sumOfAbsDiff11x11_asm sumOfAbsDiff11x11
#endif

#ifdef __MOVICOMPILE__
    extern "C"
	///                              WARNING:  this is an untested kernel  !!!
    void sumOfAbsDiff11x11_asm(u8** in1, u8** in2, u8** out, u32 width);
#endif
	void sumOfAbsDiff11x11(u8** in1, u8** in2, u8** out, u32 width);

//!@}	

//!@ { 
/// Convert from 12 bpp to 8 bpp
///@param[in]   in          - pointer to one input line
///@param[out]  out         - pointer to one output line
///@param[in]   width       - number of pixels of the input line
#ifdef __PC__

    #define convert12BppTo8Bpp_asm convert12BppTo8Bpp
#endif

#ifdef __MOVICOMPILE__
    extern "C"
    void convert12BppTo8Bpp_asm(u8* out, u8* in, u32 width);
#endif
    void convert12BppTo8Bpp(u8* out, u8* in, u32 width);
//!@}

#ifdef __cplusplus
}
#endif
#endif