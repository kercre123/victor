/**
 * File: image.cpp
 *
 * Author: Andrew Stein
 * Date:   11/20/2014
 *
 * Description: Implements a container for images on the basestation.
 *
 * Copyright: Anki, Inc. 2014
 **/

#include "coretech/common/engine/math/point_impl.h"
#include "coretech/common/engine/math/polygon_impl.h"
#include "coretech/common/engine/math/quad_impl.h"
#include "coretech/vision/engine/cameraCalibration.h"
#include "coretech/vision/engine/image_impl.h"

#include "util/fileUtils/fileUtils.h"
#include "util/helpers/ankiDefines.h"
#include "util/helpers/boundedWhile.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/highgui.hpp"
#endif

namespace {
  
template <typename T>
void ResizeKeepAspectRatioHelper(const cv::Mat_<T>& src, cv::Mat_<T>& dest, s32 desiredCols, s32 desiredRows,
                                 int method, bool onlyReduceSize)
{

  const double ratio = double(src.rows) / double(src.cols); //without the double it's the nastiest bug!!
  const int newNumberCols = std::round(desiredCols * ratio);
  const int newNumberRows = std::round(desiredRows / ratio);
  cv::Size desiredSize;
  if (newNumberCols <= desiredRows) {
    desiredSize = {desiredCols, newNumberCols};
  }
  else {
    desiredSize = {newNumberRows, desiredRows};
  }
 
  if(onlyReduceSize && src.rows < desiredSize.height && src.cols < desiredSize.width)
  {
    // Source is already smaller than the desired size in both dimensions, and onlyReduceSize was specifed, so
    // don't resize (i.e. don't make small src bigger)
    dest = src;
  }
  else
  {
    try {
      cv::resize(src, dest, desiredSize, 0, 0, method);
    }
    catch (cv::Exception& e) {
      PRINT_NAMED_ERROR("ResizeKeepAspectRatioHelper.CvResizeException", "Error while resizing image: %s,"
                        "ratio %f, rows: %d, cols: %d, desiredSize: (%d, %d)",
                        e.what(), ratio, src.rows, src.cols, desiredSize.width, desiredSize.height);
    }
  }
}
  
} // anonymous namespace

namespace Anki {
namespace Vision {
  
#pragma mark --- ImageBase ---
  
  template<typename T>
  Result ImageBase<T>::Load(const std::string& filename)
  {
    const cv::Mat showableImage = cv::imread(filename, (GetNumChannels() == 1 ?
                                             CV_LOAD_IMAGE_GRAYSCALE :
                                             CV_LOAD_IMAGE_COLOR));
    
    SetFromShowableFormat(showableImage);

    if(IsEmpty()) {
      return RESULT_FAIL;
    } else {
      return RESULT_OK;
    }
  }
  
  template<typename T>
  Result ImageBase<T>::Save(const std::string &filename, s32 quality) const
  {
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_JPEG_QUALITY);
    compression_params.push_back(quality);
    
    // Convert color images to BGR(A) for saving (as assumed by imwrite)
    cv::Mat saveImg;
    ConvertToShowableFormat(saveImg);
    
    Util::FileUtils::CreateDirectory(filename, true, true);
    
    try {
      const bool success = imwrite(filename, saveImg, compression_params);
      if(!success) {
        PRINT_NAMED_WARNING("ImageBase.Save.ImwriteFailed",
                            "Failed writing %dx%d image to %s",
                            GetNumCols(), GetNumRows(), filename.c_str());
        return RESULT_FAIL;
      }
    }
    catch (cv::Exception& ex) {
      PRINT_NAMED_WARNING("ImageBase.Save.ImwriteException",
                          "Failed writing to %s: %s",
                          filename.c_str(), ex.what());
      return RESULT_FAIL;
    }
    
    return RESULT_OK;
  }
  
  template<typename T>
  ImageBase<T>& ImageBase<T>::operator= (const ImageBase<T> &other)
  {
    SetTimestamp(other.GetTimestamp());
    SetImageId(other.GetImageId());
    Array2d<T>::operator=(other);
    return *this;
  }
  
  template<typename T>
  void ImageBase<T>::Display(const char *windowName, s32 pauseTime_ms) const
  {
#   if !defined(ANKI_PLATFORM_OSX)
    {
      PRINT_NAMED_WARNING("ImageBase.Display.NoDisplay",
                          "Ignoring display request for %s", windowName);
      return;
    }
#   endif
    
    cv::Mat dispImg;
    ConvertToShowableFormat(dispImg);

    cv::imshow(windowName, dispImg);
    cv::waitKey(pauseTime_ms);
  }

  template<typename T>
  void ImageBase<T>::CloseDisplayWindow(const char *windowName)
  {
#   if !defined(ANKI_PLATFORM_OSX)
    {
      PRINT_NAMED_WARNING("ImageBase.CloseDisplayWindow.NoDisplay",
                          "Ignoring close display request for %s", windowName);
      return;
    }
#   endif
    
    cv::destroyWindow(windowName);
  }
  
  template<typename T>
  void ImageBase<T>::CloseAllDisplayWindows()
  {
#   if !defined(ANKI_PLATFORM_OSX)
    {
      // NOTE: Display should not be possible (via similar checks above), so
      // there's no real harm in calling this method, so just a debug, not
      // a warning.
      PRINT_NAMED_DEBUG("ImageBase.CloseAllDisplayWindows.NoDisplay", "");
      return;
    }
#   endif
    
    cv::destroyAllWindows();
  }

  template<typename T>
  inline cv::Scalar ImageBase<T>::GetCvColor(const ColorRGBA& color) const {
    return cv::Scalar(color.r(), color.g(), color.b(), 0);
  }

  template<typename T>
  void ImageBase<T>::DrawLine(const Point2f& start, const Point2f& end,
                              const ColorRGBA& color, const s32 thickness)
  {
    cv::line(this->get_CvMat_(), start.get_CvPoint_(), end.get_CvPoint_(), GetCvColor(color), thickness);
  }
  
  template<typename T>
  void ImageBase<T>::DrawCircle(const Point2f& center, const ColorRGBA& color, const s32 radius, const s32 thickness)
  {
    cv::circle(this->get_CvMat_(), center.get_CvPoint_(), radius, GetCvColor(color), thickness);
  }
  
  template<typename T>
  void ImageBase<T>::DrawFilledCircle(const Point2f& center, const ColorRGBA& color, const s32 radius)
  {
    cv::circle(this->get_CvMat_(), center.get_CvPoint_(), radius, GetCvColor(color), CV_FILLED);
  }
  
  template<typename T>
  void ImageBase<T>::DrawRect(const Rectangle<f32>& rect, const ColorRGBA& color, const s32 thickness)
  {
    cv::rectangle(this->get_CvMat_(), rect.get_CvRect_(), GetCvColor(color), thickness);
  }
  
  template<typename T>
  void ImageBase<T>::DrawFilledRect(const Rectangle<f32>& rect, const ColorRGBA& color)
  {
    cv::rectangle(this->get_CvMat_(), rect.get_CvRect_(), GetCvColor(color), CV_FILLED);
  }

  
  template<typename T>
  void ImageBase<T>::DrawRect(const Rectangle<s32>& rect, const ColorRGBA& color, const s32 thickness)
  {
    cv::rectangle(this->get_CvMat_(), rect.get_CvRect_(), GetCvColor(color), thickness);
  }
  
  template<typename T>
  void ImageBase<T>::DrawFilledRect(const Rectangle<s32>& rect, const ColorRGBA& color)
  {
    cv::rectangle(this->get_CvMat_(), rect.get_CvRect_(), GetCvColor(color), CV_FILLED);
  }

  template<typename T>
  void ImageBase<T>::DrawQuad(const Quad2f& quad, const ColorRGBA& color, const s32 thickness)
  {
    using namespace Quad;
    DrawLine(quad[CornerName::TopLeft], quad[CornerName::TopRight], color, thickness);
    DrawLine(quad[CornerName::TopLeft], quad[CornerName::BottomLeft], color, thickness);
    DrawLine(quad[CornerName::TopRight], quad[CornerName::BottomRight], color, thickness);
    DrawLine(quad[CornerName::BottomLeft], quad[CornerName::BottomRight], color, thickness);
  }

  template<typename T>
  static void DrawPolyHelper(ImageBase<T>* img, const cv::Mat& ptsMat,
                             const cv::Scalar& color, const s32 thickness,
                             const bool closed)
  {
    const cv::Point *pts = (const cv::Point*) ptsMat.data;
    int npts = ptsMat.rows;
    
    try {
      cv::polylines(img->get_CvMat_(), &pts, &npts, 1, closed, color, thickness);
    }
    catch (cv::Exception& e)
    {
      PRINT_NAMED_ERROR("ImageBase.DrawPolyHelper.OpenCvPolylinesFailed",
                        "%s", e.what());
      return;
    }
  }

  template<typename T>
  void ImageBase<T>::DrawPoly(const Poly2f& poly, const ColorRGBA& color, const s32 thickness, const bool closed)
  {
    cv::Mat ptsMat((int)poly.size(), 2, CV_32SC1);
    int row = 0;
    for(auto const& polyPt : poly)
    {
      s32 *ptsMat_row = ptsMat.ptr<s32>(row++);
      ptsMat_row[0] = (s32)std::round(polyPt.x());
      ptsMat_row[1] = (s32)std::round(polyPt.y());
    }
    
    DrawPolyHelper(this, ptsMat, GetCvColor(color), thickness, closed);
  }

  template<typename T>
  void ImageBase<T>::DrawPoly(const Poly2i& poly, const ColorRGBA& color, const s32 thickness, const bool closed)
  {
    cv::Mat ptsMat((int)poly.size(), 2, CV_32FC1);
    int row = 0;
    for(auto const& polyPt : poly)
    {
      f32 *ptsMat_row = ptsMat.ptr<f32>(row++);
      ptsMat_row[0] = polyPt.x();
      ptsMat_row[1] = polyPt.y();
    }
    
    DrawPolyHelper(this, ptsMat, GetCvColor(color), thickness, closed);
  }

  template<typename T>
  void ImageBase<T>::DrawFilledConvexPolygon(const std::vector<Point2i> points, const ColorRGBA& color)
  {
    std::vector<cv::Point> cvpts;
    cvpts.reserve(points.size());
    for (const auto& p: points) {
      cvpts.push_back(p.get_CvPoint_());
    }
    cv::fillConvexPoly(this->get_CvMat_(), cvpts, GetCvColor(color));
  }
  
  // Compile time "LUT" for converting from our resize method to OpenCV's
  static inline int GetOpenCvInterpMethod(ResizeMethod method)
  {
    switch(method)
    {
      case ResizeMethod::NearestNeighbor:
        return CV_INTER_NN;
        
      case ResizeMethod::Linear:
        return CV_INTER_LINEAR;
        
      case ResizeMethod::Cubic:
        return CV_INTER_CUBIC;
        
      case ResizeMethod::AverageArea:
        return CV_INTER_AREA;
    }
  }
  
  
  template<typename T>
  void ImageBase<T>::DrawText(const Point2f& position, const std::string& str,
                              const ColorRGBA& color, f32 scale, bool dropShadow,
                              int thickness, bool centered)
  {
    Point2f alignedPos(position);
    if(centered)
    {
      const cv::Size textSize = cv::getTextSize(str, CV_FONT_NORMAL, scale, thickness, nullptr);
      alignedPos.x() -= textSize.width/2;
    }
    
    if(dropShadow) {
      cv::Point shadowPos(alignedPos.get_CvPoint_());
      shadowPos.x += 1;
      shadowPos.y += 1;
      cv::putText(this->get_CvMat_(), str, shadowPos, CV_FONT_NORMAL, scale, GetCvColor(NamedColors::BLACK), thickness);
    }
    cv::putText(this->get_CvMat_(), str, alignedPos.get_CvPoint_(), CV_FONT_NORMAL, scale, GetCvColor(color), thickness);
  }
  
  template<typename T>
  Vec2f ImageBase<T>::GetTextSize(const std::string& str, f32 scale, int thickness)
  {
    auto sz = cv::getTextSize(str, CV_FONT_NORMAL, scale, thickness, nullptr);
    return Vec2f(sz.width, sz.height);
  }

  template<typename T>
  void ImageBase<T>::MakeTextFillImageWidth(const std::string& text,
					    int font,
					    int thickness,
					    int imageWidth,
					    cv::Size& textSize,
					    float& scale)
  {
    scale = 0.1f;
    cv::Size prevTextSize(0,0);
    // TODO Use binary search instead
    for(; scale < 3.f; scale += 0.05f)
    {
      textSize = cv::getTextSize(text, 
				 font,
				 scale, 
				 1, 
				 nullptr);

      if(textSize.width > imageWidth)
      {
	scale -= 0.05f;
	textSize = prevTextSize;
	break;
      }
      prevTextSize = textSize;
    }
  }

  template<typename T>
  void ImageBase<T>::DrawTextCenteredHorizontally(const std::string& text,
						  int font,
						  float scale,
						  int thickness,
						  const ColorRGBA& color,
						  int verticalPos,
						  bool drawTwiceToMaybeFillGaps)
  {
    int baseline = 0;
    cv::Size textSize = cv::getTextSize(text, 
					font,
					scale, 
					thickness, 
					&baseline);

    Point2f p((this->GetNumCols() - textSize.width)/2, verticalPos);

    this->DrawText(p,
		   text,
		   color,
		   scale,
		   false,
		   thickness);

    if(drawTwiceToMaybeFillGaps)
    {
      p.y() += 1;

      this->DrawText(p,
		     text,
		     color,
		     scale,
		     false,
		     thickness);
    }
  }


  template<typename T>
  void ImageBase<T>::Resize(f32 scaleFactor, ResizeMethod method)
  {
    cv::resize(this->get_CvMat_(), this->get_CvMat_(), cv::Size(), scaleFactor, scaleFactor,
               GetOpenCvInterpMethod(method));
  }
  
  template<typename T>
  void ImageBase<T>::Resize(s32 desiredRows, s32 desiredCols, ResizeMethod method)
  {
    if(desiredRows != GetNumRows() || desiredCols != GetNumCols()) {
      const cv::Size desiredSize(desiredCols, desiredRows);
      cv::resize(this->get_CvMat_(), this->get_CvMat_(), desiredSize, 0, 0,
                 GetOpenCvInterpMethod(method));
    }
  }
  
  template<typename T>
  void ImageBase<T>::Resize(ImageBase<T>& resizedImage, ResizeMethod method) const
  {
    if(resizedImage.IsEmpty()) {
      PRINT_NAMED_ERROR("Image.Resize.EmptyImage", "Output image should already have the desired size");
    }
    else {
      const cv::Size desiredSize(resizedImage.GetNumCols(), resizedImage.GetNumRows());
      cv::resize(this->get_CvMat_(), resizedImage.get_CvMat_(), desiredSize, 0, 0,
                 GetOpenCvInterpMethod(method));
      resizedImage.SetTimestamp(this->GetTimestamp());
      resizedImage.SetImageId(this->GetImageId());
    }
  }

  template <typename T>
  void ImageBase<T>::ResizeKeepAspectRatio(s32 desiredRows, s32 desiredCols, ResizeMethod method, bool onlyReduce)
  {

    if(desiredRows != GetNumRows() || desiredCols != GetNumCols()) {
      ResizeKeepAspectRatioHelper(this->get_CvMat_(), this->get_CvMat_(), desiredCols, desiredRows,
                                  GetOpenCvInterpMethod(method), onlyReduce);
    }
  }

  template<typename T>
  void ImageBase<T>::ResizeKeepAspectRatio(ImageBase<T>& resizedImage, ResizeMethod method) const
  {

    if(resizedImage.IsEmpty()) {
      PRINT_NAMED_ERROR("Image.ResizeKeepAspectRatio.EmptyImage", "Output image should already have the desired size");
      return;
    }

    const s32 desiredCols = resizedImage.GetNumCols();
    const s32 desiredRows = resizedImage.GetNumRows();
    ResizeKeepAspectRatioHelper(this->get_CvMat_(), resizedImage.get_CvMat_(), desiredCols, desiredRows, GetOpenCvInterpMethod(method), false);
    resizedImage.SetTimestamp(this->GetTimestamp());
    resizedImage.SetImageId(this->GetImageId());
  }

  template<typename T>
  void ImageBase<T>::CopyTo(ImageBase<T>& otherImage) const
  {
    Array2d<T>::CopyTo(otherImage);
    otherImage.SetTimestamp(GetTimestamp()); // Make sure timestamp gets copied too
    otherImage.SetImageId(GetImageId());
  }

  template<typename T>
  void ImageBase<T>::BoxFilter(ImageBase<T>& filtered, u32 size) const
  {
    DEV_ASSERT(size%2==0, "ImageBase.BoxFilter.SizeNotMultipleOf2");
    cv::boxFilter(this->get_CvMat_(), filtered.get_CvMat_(), -1, cv::Size(size, size));
  }
  
  template<typename T>
  void ImageBase<T>::Undistort(const CameraCalibration& calib, ImageBase<T>& undistortedImage) const
  {
    undistortedImage.Allocate(GetNumRows(), GetNumCols());
    const CameraCalibration scaledCalib = calib.GetScaled(GetNumRows(), GetNumCols());
    cv::undistort(this->get_CvMat_(), undistortedImage.get_CvMat_(),
                  scaledCalib.GetCalibrationMatrix().get_CvMatx_(),
                  scaledCalib.GetDistortionCoeffs());
  }
  
  // Explicit instantation for each image type:
  template class ImageBase<u8>;
  template class ImageBase<PixelRGB>;
  template class ImageBase<PixelRGBA>;
  template class ImageBase<PixelRGB565>;
  
#pragma mark --- Image ---
  
  Image::Image()
  : ImageBase<u8>()
  {
    
  }

  Image::~Image()
  {

  }
  
  Image::Image(s32 nrows, s32 ncols)
  : ImageBase<u8>(nrows, ncols)
  {
    
  }
  
  Image::Image(s32 nrows, s32 ncols, const u8& pixel)
  : ImageBase<u8>(nrows, ncols, pixel)
  {
    
  }
  
  Image::Image(s32 nrows, s32 ncols, const ColorRGBA& color)
  : ImageBase<u8>(nrows, ncols, color)
  {
    
  }

  Image::Image(s32 nrows, s32 ncols, u8* data)
  : ImageBase<u8>(nrows, ncols, data)
  {
    
  }
  
  Image::Image(const Array2d<u8>& array2d)
  : ImageBase<u8>(array2d)
  {
    
  }

# if ANKICORETECH_USE_OPENCV
  Image::Image(cv::Mat_<u8>& cvMat)
  : ImageBase<u8>(cvMat)
  {
    
  }
# endif
  
  Image& Image::Negate()
  {
    cv::bitwise_not(get_CvMat_(), get_CvMat_());
    return *this;
  }
 
  Image Image::GetNegative() const
  {
    Image output;
    cv::bitwise_not(get_CvMat_(), output.get_CvMat_());
    return output;
  }
  
  Image& Image::Threshold(u8 value)
  {
    get_CvMat_() = get_CvMat_() > value;
    return *this;
  }
  
  Image  Image::Threshold(u8 value) const
  {
    Image thresholdedImage;
    this->CopyTo(thresholdedImage);
    return thresholdedImage.Threshold(value);
  }

  s32 Image::GetConnectedComponents(Array2d<s32>& labelImage) const
  {
    const s32 count = cv::connectedComponents(this->get_CvMat_(), labelImage.get_CvMat_());
    return count;
  } // GetConnectedComponents()
  
  s32 Image::GetConnectedComponents(Array2d<s32>& labelImage, std::vector<ConnectedComponentStats>& stats) const
  {
    cv::Mat cvStats, cvCentroids;
    const s32 count = cv::connectedComponentsWithStats(this->get_CvMat_(), labelImage.get_CvMat_(), cvStats, cvCentroids);
    for(s32 iComp=0; iComp < count; ++iComp) 
    {
      const s32* compStats = cvStats.ptr<s32>(iComp);
      const f64* compCentroid = cvCentroids.ptr<f64>(iComp);

      ConnectedComponentStats stat{
        .area        = (size_t)compStats[cv::CC_STAT_AREA],
        .centroid    = Point2f(Util::Clamp(compCentroid[0], 0.0, (f64)(GetNumCols()-1)),
                               Util::Clamp(compCentroid[1], 0.0, (f64)(GetNumRows()-1))),
        .boundingBox = Rectangle<s32>(compStats[cv::CC_STAT_LEFT],  compStats[cv::CC_STAT_TOP],
                                      compStats[cv::CC_STAT_WIDTH], compStats[cv::CC_STAT_HEIGHT]),
      };
      
      stats.push_back(std::move(stat));
    }
    
    return count;
  }

  inline cv::Scalar Image::GetCvColor(const ColorRGBA& color) const {
    // Copied formula from Matlab's rgb2gray
    u8 gray = static_cast<u8>(0.2989f * color.r()) + (0.5870f * color.g()) + (0.1140f * color.b()); 
    return cv::Scalar(gray, gray, gray, 0);
  }

  void Image::ConvertToShowableFormat(cv::Mat& showImg) const {
    this->get_CvMat_().copyTo(showImg);
  }

  void Image::SetFromShowableFormat(const cv::Mat& showImg) {
    DEV_ASSERT(showImg.channels() == 1, "Image.SetFromShowableFormat.UnexpectedNumChannels");
    showImg.copyTo(this->get_CvMat_());
  }

  void Image::BoxFilter(ImageBase<u8>& filtered, u32 size) const
  {
    #if defined(MAC)
    #define IS_MAC 1
    #else
    #define IS_MAC 0
    #endif

    // Use base class's BoxFilter if the size is not 3 or this is mac
    // (will just use OpenCV's boxFilter)
    if(size != 3 || IS_MAC)
    {
      ImageBase<u8>::BoxFilter(filtered, size);
      return;
    }
      
    filtered.Allocate(GetNumRows(), GetNumCols());

    #ifdef __ARM_NEON__

      const uint16x8_t  kZeros = vdupq_n_u8(0);
      const float32x4_t kNorm  = vdupq_n_f32(1.f / 9.f);

      // Define neon implementation to filter a row of an image with a 3x3 box filter
      // All parameters are u8*
      #define FILTER_ROW_NEON(row1Ptr, row2Ptr, row3Ptr, outputPtr) \
        for(; c < ((GetNumCols()-1) - (6-1)); c += 6)               \
        {                                                           \
          uint8x8_t row1 = vld1_u8(row1Ptr);                        \
          row1Ptr += 6;                                             \
                                                                    \
          uint8x8_t row2 = vld1_u8(row2Ptr);                        \
          row2Ptr += 6;                                             \
                                                                    \
          uint8x8_t row3 = vld1_u8(row3Ptr);                        \
          row3Ptr += 6;                                             \
                                                                    \
          /* Sum the three rows vertically */                       \
          uint16x8_t sum = vaddl_u8(row1, row2);                    \
          sum = vaddw_u8(sum, row3);                                \
                                                                    \
          /* Shift the summed rows left once and then twice */      \
          /* and add in order to sum horizonatally */               \
          uint16x8_t shifted = vextq_u16(sum, kZeros, 1);           \
          uint16x8_t shifted2 = vextq_u16(sum, kZeros,  2);         \
          sum = vaddq_u16(sum, shifted);                            \
          sum = vaddq_u16(sum, shifted2);                           \
                                                                    \
          /* Expand and convert to float */                         \
          uint16x4_t sum16x4_1 = vget_low_u16(sum);                 \
          uint16x4_t sum16x4_2 = vget_high_u16(sum);                \
          uint32x4_t sum32x4_1 = vmovl_u16(sum16x4_1);              \
          uint32x4_t sum32x4_2 = vmovl_u16(sum16x4_2);              \
                                                                    \
          float32x4_t sumf_1 = vcvtq_f32_u32(sum32x4_1);            \
          float32x4_t sumf_2 = vcvtq_f32_u32(sum32x4_2);            \
                                                                    \
          /* Multiply by 1/9 and saturate convert back to u8*/      \
          sumf_1 = vmulq_f32(sumf_1, kNorm);                        \
          sumf_2 = vmulq_f32(sumf_2, kNorm);                        \
                                                                    \
          sum32x4_1 = vcvtq_u32_f32(sumf_1);                        \
          sum32x4_2 = vcvtq_u32_f32(sumf_2);                        \
                                                                    \
          sum16x4_1 = vqmovn_u32(sum32x4_1);                        \
          sum16x4_2 = vqmovn_u32(sum32x4_2);                        \
                                                                    \
          sum = vcombine_u16(sum16x4_1, sum16x4_2);                 \
                                                                    \
          uint8x8_t out = vqmovn_u16(sum);                          \
          vst1_u8(outputPtr, out);                                  \
          outputPtr += 6;                                           \
        }

    #else

      // Neon not available so increment row pointers to setup 
      // element by element filtering
      #define FILTER_ROW_NEON(row1Ptr, row2Ptr, row3Ptr, outputPtr) \
        row1Ptr++; \
        row2Ptr++; \
        row3Ptr++;
    
    #endif // #ifdef __ARM_NEON__

    // Macro to run a 3x3 box filter on a row of an image
    // Will use neon if on android
    // All arguments are u8*
    #define FILTER_ROW(row1Ptr, row2Ptr, row3Ptr, outputPtr)    \
      {                                                         \
        u32 c = 1;                                              \
        FILTER_ROW_NEON(row1Ptr, row2Ptr, row3Ptr, outputPtr);  \
                                                                \
        for(; c < GetNumCols()-1; c++)                          \
        {                                                       \
          /* Increment rowPtrs first due to how the neon */     \
          /* section works, starts at row[0] and writes to */   \
          /* output[1] so increment rowPtrs so this section */  \
          /* is starting at row[n] and writing to output[n] */  \
          row1Ptr++;                                            \
          row2Ptr++;                                            \
          row3Ptr++;                                            \
                                                                \
          *outputPtr = (u8)(((f32)*(row1Ptr-1) +                \
                             (f32)*(row1Ptr) +                  \
                             (f32)*(row1Ptr+1) +                \
                             (f32)*(row2Ptr-1) +                \
                             (f32)*(row2Ptr) +                  \
                             (f32)*(row2Ptr+1) +                \
                             (f32)*(row3Ptr-1) +                \
                             (f32)*(row3Ptr) +                  \
                             (f32)*(row3Ptr+1)) * (1/9.f));     \
                                                                \
          outputPtr++;                                          \
        }                                                       \
      }

    // Filter the first row using mirroring
    const u8* imageRow1 = GetRow(1);
    const u8* imageRow2 = GetRow(0);
    const u8* imageRow3 = GetRow(1);
    u8* output = filtered.GetRow(0) + 1;
    FILTER_ROW(imageRow1, imageRow2, imageRow3, output);

    // Filter the rest of the rows stopping before the last row
    u32 r = 1;
    for(; r < GetNumRows()-1; r++)
    {
      imageRow1 = GetRow(r-1);
      imageRow2 = GetRow(r);
      imageRow3 = GetRow(r+1);
      output = filtered.GetRow(r) + 1;

      FILTER_ROW(imageRow1, imageRow2, imageRow3, output);
    }

    // Filter the last row using mirroring
    const u32 kNumRows = GetNumRows();
    imageRow1 = GetRow(kNumRows - 2);
    imageRow2 = GetRow(kNumRows - 1);
    imageRow3 = GetRow(kNumRows - 2);
    output = filtered.GetRow(kNumRows - 1) + 1;
    FILTER_ROW(imageRow1, imageRow2, imageRow3, output);

    // Filter the left and right edge of the image using mirroring
    {
      const u8* prevRow = GetRow(0);
      const u8* curRow = GetRow(0);
      const u8* nextRow = GetRow(1);
      u8* filteredRow = filtered.GetRow(0);

      filteredRow[0] = (u8)(((f32)(curRow[0]) + 
                             (f32)(curRow[1]*2) + 
                             (f32)(nextRow[0]*2) + 
                             (f32)(nextRow[1]*4)) * (1/9.f));

      const u32 lastCols = GetNumCols() - 1;
      filteredRow[lastCols] = (u8)(((f32)(curRow[lastCols]) + 
                                    (f32)(curRow[lastCols-1]*2) + 
                                    (f32)(nextRow[lastCols]*2) + 
                                    (f32)(nextRow[lastCols-1]*4)) * (1/9.f));

      r = 1;
      for(; r < GetNumRows() - 1; r++)
      {
        prevRow = GetRow(r-1);
        curRow = GetRow(r);
        nextRow = GetRow(r+1);
        filteredRow = filtered.GetRow(r);

        filteredRow[0] = (u8)(((f32)(curRow[0]) + 
                               (f32)(prevRow[0]) + 
                               (f32)(nextRow[0]) + 
                               (f32)(curRow[1]*2) + 
                               (f32)(prevRow[1]*2) + 
                               (f32)(nextRow[1]*2)) * (1/9.f));

        filteredRow[lastCols] = (u8)(((f32)(curRow[lastCols]) + 
                                      (f32)(prevRow[lastCols]) + 
                                      (f32)(nextRow[lastCols]) + 
                                      (f32)(curRow[lastCols-1]*2) + 
                                      (f32)(prevRow[lastCols-1]*2) + 
                                      (f32)(nextRow[lastCols-1]*2)) * (1/9.f));
      }

      prevRow = GetRow(r - 1);
      curRow = GetRow(r);
      nextRow = GetRow(r);
      filteredRow = filtered.GetRow(r);

      filteredRow[0] = (u8)(((f32)(curRow[0]) + 
                             (f32)(curRow[1]*2) + 
                             (f32)(prevRow[0]*2) + 
                             (f32)(prevRow[1]*4)) * (1/9.f));

      filteredRow[lastCols] = (u8)(((f32)(curRow[lastCols]) + 
                                    (f32)(curRow[lastCols-1]*2) + 
                                    (f32)(prevRow[lastCols]*2) + 
                                    (f32)(prevRow[lastCols-1]*4)) * (1/9.f));
    }
  }

  void ImageRGB::ConvertHSV2RGB565(ImageRGB565& output)
  {
    output.Allocate(GetNumRows(), GetNumCols());

    const u32 kSizeOfPixelRGB = 3;
    u32 numRows = GetNumRows();
    u32 numCols = GetNumCols();

    if(this->IsContinuous() && output.IsContinuous())
    {
      numCols *= numRows;
      numRows = 1;
    }

    for(u32 r = 0; r < numRows; r++)
    {
      const u8* row = reinterpret_cast<u8*>(GetRow(r));
      u16* out = reinterpret_cast<u16*>(output.GetRow(r));

      u32 c = 0;

#ifdef __ARM_NEON__

      const float32x4_t kOne = vdupq_n_f32(1.f);

      // Vectors to hold LUT table to figure out which variables will
      // need to be set to r, g, and b
      const uint8x8_t rp = vcreate_u8(0x00000000FFFF0000);
      const uint8x8_t rq = vcreate_u8(0x000000000000FF00);
      const uint8x8_t rt = vcreate_u8(0x000000FF00000000);

      const uint8x8_t gp = vcreate_u8(0x0000FFFF00000000);
      const uint8x8_t gq = vcreate_u8(0x00000000FF000000);
      const uint8x8_t gt = vcreate_u8(0x00000000000000FF);

      const uint8x8_t bp = vcreate_u8(0x000000000000FFFF);
      const uint8x8_t bq = vcreate_u8(0x0000FF0000000000);
      const uint8x8_t bt = vcreate_u8(0x0000000000FF0000);

      const u32 kNumElementsProcessedPerLoop = 8;

      for(; c < numCols - (kNumElementsProcessedPerLoop - 1); c += kNumElementsProcessedPerLoop)
      {
        float32x4x3_t hsv1;
        float32x4x3_t hsv2;
        uint8x8x3_t hsv255 = vld3_u8(row);
        row += kNumElementsProcessedPerLoop*kSizeOfPixelRGB;

        // Expand H to float and convert from 0-255 to 0-360 and divide by 60
        uint16x8_t which16x8 = vmovl_u8(hsv255.val[0]);
        uint16x4_t which16x4_1 = vget_low_u16(which16x8);
        uint16x4_t which16x4_2 = vget_high_u16(which16x8);
        uint32x4_t which32x4_1 = vmovl_u16(which16x4_1);
        uint32x4_t which32x4_2 = vmovl_u16(which16x4_2);
        float32x4_t whichF32x4_1 = vcvtq_f32_u32(which32x4_1);
        float32x4_t whichF32x4_2 = vcvtq_f32_u32(which32x4_2);
        hsv1.val[0] = vmulq_n_f32(whichF32x4_1, (360/255.f) * (1/60.f));
        hsv2.val[0] = vmulq_n_f32(whichF32x4_2, (360/255.f) * (1/60.f));

        // Expand S to float and convert from 0-255 to 0-1
        which16x8 = vmovl_u8(hsv255.val[1]);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        whichF32x4_1 = vcvtq_f32_u32(which32x4_1);
        whichF32x4_2 = vcvtq_f32_u32(which32x4_2);
        hsv1.val[1] = vmulq_n_f32(whichF32x4_1, 1/255.f);
        hsv2.val[1] = vmulq_n_f32(whichF32x4_2, 1/255.f);

        // Expand V to float and convert from 0-255 to 0-1
        which16x8 = vmovl_u8(hsv255.val[2]);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        whichF32x4_1 = vcvtq_f32_u32(which32x4_1);
        whichF32x4_2 = vcvtq_f32_u32(which32x4_2);
        hsv1.val[2] = vmulq_n_f32(whichF32x4_1, 1/255.f);
        hsv2.val[2] = vmulq_n_f32(whichF32x4_2, 1/255.f);

        // Convert H to u32 and then back to float, this will floor(H)
        // i will be between [0, 5]
        float32x4_t i1 = vcvtq_f32_u32(vcvtq_u32_f32(hsv1.val[0]));
        float32x4_t i2 = vcvtq_f32_u32(vcvtq_u32_f32(hsv2.val[0]));
        
        // H - floor(H) will be the decimal portion of H
        float32x4_t f1 = vsubq_f32(hsv1.val[0], i1);
        float32x4_t f2 = vsubq_f32(hsv2.val[0], i2);
        
        // p = V * ( 1 - S )
        float32x4_t p1 = vsubq_f32(kOne, hsv1.val[1]);
        p1 = vmulq_f32(p1, hsv1.val[2]);
        float32x4_t p2 = vsubq_f32(kOne, hsv2.val[1]);
        p2 = vmulq_f32(p2, hsv2.val[2]);

        // q = V * ( 1 - (S * f) )
        float32x4_t q1 = vmulq_f32(hsv1.val[1], f1);
        q1 = vsubq_f32(kOne, q1);
        q1 = vmulq_f32(q1, hsv1.val[2]);
        float32x4_t q2 = vmulq_f32(hsv2.val[1], f2);
        q2 = vsubq_f32(kOne, q2);
        q2 = vmulq_f32(q2, hsv2.val[2]);

        // t = V * ( 1 - (S * ( 1 - f ) ) )
        float32x4_t t1 = vsubq_f32(kOne, f1);
        t1 = vmulq_f32(t1, hsv1.val[1]);
        t1 = vsubq_f32(kOne, t1);
        t1 = vmulq_f32(t1, hsv1.val[2]);
        float32x4_t t2 = vsubq_f32(kOne, f2);
        t2 = vmulq_f32(t2, hsv2.val[1]);
        t2 = vsubq_f32(kOne, t2);
        t2 = vmulq_f32(t2, hsv2.val[2]);

        // Convert i from f32 to u8
        uint32x4_t index32x4_1 = vcvtq_u32_f32(i1);
        uint16x4_t index16x4_1 = vmovn_u32(index32x4_1);
        uint32x4_t index32x4_2 = vcvtq_u32_f32(i2);
        uint16x4_t index16x4_2 = vmovn_u32(index32x4_2);
        uint16x8_t index16x8 = vcombine_u16(index16x4_1, index16x4_2);
        uint8x8_t index = vmovn_u16(index16x8);

        // The following block is repeated 3 times one for each color channel

        // Depending on which of the 6 sectors hue falls in r,g,b may either be set
        // from V, p, q, or t. index holds which sector each pixel's hue is in and is used
        // to index into the tables held by rp,rq,rt gp,gq,gt and bp,bq,bt. The result of
        // the look up will either be all 0s or all 1s. If all 1s, then for this sector the 
        // variable corresponding to the table should be used for this channel.
        // Ex: HSV = (0, 1, 1) which is sector 0 so r = V, g = t, b = p
        // rp[0] == 0s, rq[0] == 0s, rt[0] == 0s  r is set from V
        // gp[0] == 0s, gq[0] == 0s, gt[0] == 1s  g is set from t
        // bp[0] == 1s, bq[0] == 0s, bt[0] == 0s  b is set from p

        // Initialize to V
        float32x4_t r1 = hsv1.val[2];
        float32x4_t r2 = hsv2.val[2];

        // Check if r should be set from p
        uint8x8_t which = vtbl1_u8(rp, index);
        // Expand to f32 and multiply by 0xFFFFFFFF so 
        // elements will be all 1s or all 0s
        which16x8 = vmovl_u8(which);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        which32x4_1 = vmulq_n_u32(which32x4_1, 0xFFFFFFFF);
        which32x4_2 = vmulq_n_u32(which32x4_2, 0xFFFFFFFF);
        whichF32x4_1 = vreinterpretq_f32_u32(which32x4_1);
        whichF32x4_2 = vreinterpretq_f32_u32(which32x4_2);
        // If r should be set from p then whichF32x4 will be 1s which will
        // select values from p instead of r
        r1 = vbslq_f32(whichF32x4_1, p1, r1);
        r2 = vbslq_f32(whichF32x4_2, p2, r2);


        // Check if r should be set from q
        which = vtbl1_u8(rq, index);
        // Expand to f32 and multiply by 0xFFFFFFFF so 
        // elements will be all 1s or all 0s
        which16x8 = vmovl_u8(which);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        which32x4_1 = vmulq_n_u32(which32x4_1, 0xFFFFFFFF);
        which32x4_2 = vmulq_n_u32(which32x4_2, 0xFFFFFFFF);
        whichF32x4_1 = vreinterpretq_f32_u32(which32x4_1);
        whichF32x4_2 = vreinterpretq_f32_u32(which32x4_2);
        // If r should be set from q then whichF32x4 will be 1s which will
        // select values from q instead of r
        r1 = vbslq_f32(whichF32x4_1, q1, r1);
        r2 = vbslq_f32(whichF32x4_2, q2, r2);


        // Check if r should be set from t
        which = vtbl1_u8(rt, index);
        // Expand to f32 and multiply by 0xFFFFFFFF so 
        // elements will be all 1s or all 0s
        which16x8 = vmovl_u8(which);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        which32x4_1 = vmulq_n_u32(which32x4_1, 0xFFFFFFFF);
        which32x4_2 = vmulq_n_u32(which32x4_2, 0xFFFFFFFF);
        whichF32x4_1 = vreinterpretq_f32_u32(which32x4_1);
        whichF32x4_2 = vreinterpretq_f32_u32(which32x4_2);
        // If r should be set from t then whichF32x4 will be 1s which will
        // select values from t instead of r
        r1 = vbslq_f32(whichF32x4_1, t1, r1);
        r2 = vbslq_f32(whichF32x4_2, t2, r2);

        // Repeat for green

        float32x4_t g1 = hsv1.val[2];
        float32x4_t g2 = hsv2.val[2];

        which = vtbl1_u8(gp, index);
        which16x8 = vmovl_u8(which);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        which32x4_1 = vmulq_n_u32(which32x4_1, 0xFFFFFFFF);
        which32x4_2 = vmulq_n_u32(which32x4_2, 0xFFFFFFFF);
        whichF32x4_1 = vreinterpretq_f32_u32(which32x4_1);
        whichF32x4_2 = vreinterpretq_f32_u32(which32x4_2);
        g1 = vbslq_f32(whichF32x4_1, p1, g1);
        g2 = vbslq_f32(whichF32x4_2, p2, g2);

        which = vtbl1_u8(gq, index);
        which16x8 = vmovl_u8(which);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        which32x4_1 = vmulq_n_u32(which32x4_1, 0xFFFFFFFF);
        which32x4_2 = vmulq_n_u32(which32x4_2, 0xFFFFFFFF);
        whichF32x4_1 = vreinterpretq_f32_u32(which32x4_1);
        whichF32x4_2 = vreinterpretq_f32_u32(which32x4_2);
        g1 = vbslq_f32(whichF32x4_1, q1, g1);
        g2 = vbslq_f32(whichF32x4_2, q2, g2);

        which = vtbl1_u8(gt, index);
        which16x8 = vmovl_u8(which);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        which32x4_1 = vmulq_n_u32(which32x4_1, 0xFFFFFFFF);
        which32x4_2 = vmulq_n_u32(which32x4_2, 0xFFFFFFFF);
        whichF32x4_1 = vreinterpretq_f32_u32(which32x4_1);
        whichF32x4_2 = vreinterpretq_f32_u32(which32x4_2);
        g1 = vbslq_f32(whichF32x4_1, t1, g1);
        g2 = vbslq_f32(whichF32x4_2, t2, g2);

        // Repeat for blue

        float32x4_t b1 = hsv1.val[2];
        float32x4_t b2 = hsv2.val[2];

        which = vtbl1_u8(bp, index);
        which16x8 = vmovl_u8(which);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        which32x4_1 = vmulq_n_u32(which32x4_1, 0xFFFFFFFF);
        which32x4_2 = vmulq_n_u32(which32x4_2, 0xFFFFFFFF);
        whichF32x4_1 = vreinterpretq_f32_u32(which32x4_1);
        whichF32x4_2 = vreinterpretq_f32_u32(which32x4_2);
        b1 = vbslq_f32(whichF32x4_1, p1, b1);
        b2 = vbslq_f32(whichF32x4_2, p2, b2);

        which = vtbl1_u8(bq, index);
        which16x8 = vmovl_u8(which);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        which32x4_1 = vmulq_n_u32(which32x4_1, 0xFFFFFFFF);
        which32x4_2 = vmulq_n_u32(which32x4_2, 0xFFFFFFFF);
        whichF32x4_1 = vreinterpretq_f32_u32(which32x4_1);
        whichF32x4_2 = vreinterpretq_f32_u32(which32x4_2);
        b1 = vbslq_f32(whichF32x4_1, q1, b1);
        b2 = vbslq_f32(whichF32x4_2, q2, b2);

        which = vtbl1_u8(bt, index);
        which16x8 = vmovl_u8(which);
        which16x4_1 = vget_low_u16(which16x8);
        which16x4_2 = vget_high_u16(which16x8);
        which32x4_1 = vmovl_u16(which16x4_1);
        which32x4_2 = vmovl_u16(which16x4_2);
        which32x4_1 = vmulq_n_u32(which32x4_1, 0xFFFFFFFF);
        which32x4_2 = vmulq_n_u32(which32x4_2, 0xFFFFFFFF);
        whichF32x4_1 = vreinterpretq_f32_u32(which32x4_1);
        whichF32x4_2 = vreinterpretq_f32_u32(which32x4_2);
        b1 = vbslq_f32(whichF32x4_1, t1, b1);
        b2 = vbslq_f32(whichF32x4_2, t2, b2);

        // Convert from 0-1 to 0-255
        r1 = vmulq_n_f32(r1, 255.f);
        g1 = vmulq_n_f32(g1, 255.f);
        b1 = vmulq_n_f32(b1, 255.f);
        r2 = vmulq_n_f32(r2, 255.f);
        g2 = vmulq_n_f32(g2, 255.f);
        b2 = vmulq_n_f32(b2, 255.f);

        // Convert from f32x4 r,g,b to 16x8 rgb565
        uint32x4_t color32x4_1 = vcvtq_u32_f32(r1);
        uint16x4_t color16x4_1 = vqmovn_u32(color32x4_1);
        uint32x4_t color32x4_2= vcvtq_u32_f32(r2);
        uint16x4_t color16x4_2 = vqmovn_u32(color32x4_2);
        uint16x8_t colorR16x8 = vcombine_u16(color16x4_1, color16x4_2);
        // Shift red bits into top half of a u16
        colorR16x8 = vshlq_n_u16(colorR16x8, 8);

        color32x4_1 = vcvtq_u32_f32(g1);
        color16x4_1 = vqmovn_u32(color32x4_1);
        color32x4_2 = vcvtq_u32_f32(g2);
        color16x4_2 = vqmovn_u32(color32x4_2);
        uint16x8_t colorG16x8 = vcombine_u16(color16x4_1, color16x4_2);
        // Shift green bits into top half of u16
        colorG16x8 = vshlq_n_u16(colorG16x8, 8);

        // Shift green bits by 5 and insert into red vector this is the RG56 of RGB565
        uint16x8_t colorRG = vsriq_n_u16(colorR16x8, colorG16x8, 5);

        color32x4_1 = vcvtq_u32_f32(b1);
        color16x4_1 = vqmovn_u32(color32x4_1);
        color32x4_2 = vcvtq_u32_f32(b2);
        color16x4_2 = vqmovn_u32(color32x4_2);
        uint16x8_t colorB16x8 = vcombine_u16(color16x4_1, color16x4_2);
        // Shift blue bits into top half of a u16
        colorB16x8 = vshlq_n_u16(colorB16x8, 8);

        // Shift blue bits right by 11 and insert into red/green vector
        uint16x8_t rgb565 = vsriq_n_u16(colorRG, colorB16x8, 11);

        // Write into output
        vst1q_u16(out, rgb565);
        out += kNumElementsProcessedPerLoop;
      }

#endif

      for(; c < numCols; c++)
      {
        f32 h = (f32)row[0] * (360/256.f) * (1/60.f);
        f32 s = (f32)row[1] * (1/255.f);
        f32 v = (f32)row[2] * (1/255.f);

        static const int sector_data[][3]= {{1,3,0}, {1,0,2}, 
                                            {3,0,1}, {0,2,1}, 
                                            {0,1,3}, {2,1,0}};

        u32 i = floor(h);
        h -= i;

        float vpqt[4];
        vpqt[0] = v;
        vpqt[1] = v * (1.f - s);
        vpqt[2] = v * (1.f - (s * h));
        vpqt[3] = v * (1.f - (s * (1.f - h)));

        u16 b = cv::saturate_cast<u8>(vpqt[sector_data[i][0]] * 255);
        u16 g = cv::saturate_cast<u8>(vpqt[sector_data[i][1]] * 255);
        u16 r = cv::saturate_cast<u8>(vpqt[sector_data[i][2]] * 255);

        *out = (r << 8 & 0xF800) |
               (g << 3 & 0x07E0) |
               (b >> 3);

        row += kSizeOfPixelRGB;
        out++;
      }
    }
  }


  
#if 0
#pragma mark --- ImageRGBA ---
#endif
  
  ImageRGBA::ImageRGBA()
  : ImageBase<PixelRGBA>()
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols)
  : ImageBase<PixelRGBA>(nrows, ncols)
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols, const PixelRGBA& fillValue)
  : ImageBase<PixelRGBA>(nrows, ncols, fillValue)
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols, u32* data)
  : ImageBase<PixelRGBA>(nrows, ncols, reinterpret_cast<PixelRGBA*>(data))
  {
    
  }
  
  ImageRGBA::ImageRGBA(const ImageRGB& imageRGB, u8 alpha)
  : ImageRGBA(imageRGB.GetNumRows(), imageRGB.GetNumCols())
  {
    PixelRGBA* dataRGBA = GetDataPointer();
    const PixelRGB* dataRGB = imageRGB.GetDataPointer();
    
    for(s32 i=0; i<GetNumElements(); ++i)
    {
      dataRGBA[i].r() = dataRGB[i].r();
      dataRGBA[i].g() = dataRGB[i].g();
      dataRGBA[i].b() = dataRGB[i].b();
      dataRGBA[i].a() = alpha;
    }
    
    SetTimestamp(imageRGB.GetTimestamp());
    SetImageId(imageRGB.GetImageId());
  }
  
  Image ImageRGBA::ToGray() const
  {
    Image grayImage(GetNumRows(), GetNumCols());
    FillGray(grayImage);
    return grayImage;
  }
  
  void ImageRGBA::FillGray(Image& grayImage) const
  {
    grayImage.SetTimestamp(GetTimestamp()); // Make sure timestamp gets transferred!
    grayImage.SetImageId(GetImageId());
    cv::cvtColor(this->get_CvMat_(), grayImage.get_CvMat_(), CV_RGBA2GRAY);
  }

  ImageRGBA& ImageRGBA::SetFromGray(const Image& imageGray)
  {
    cv::cvtColor(imageGray.get_CvMat_(), this->get_CvMat_(), CV_GRAY2RGBA);
    SetTimestamp(imageGray.GetTimestamp());
    SetImageId(imageGray.GetImageId());
    return *this;
  }

  ImageRGBA& ImageRGBA::SetFromRGB565(const ImageRGB565& rgb565, const u8 alpha)
  {
    // Note: not equivalent to cv::cvtColor(rgb565.get_CvMat_(), this->get_CvMat_(), cv::COLOR_BGR5652RGBA);
    //       given differences in endian-ness.

    DEV_ASSERT(this->IsContinuous(), "ImageRGBA.IsNotContinuous");
    
    int nrows = rgb565.GetNumRows();
    int ncols = rgb565.GetNumCols();
    if(rgb565.IsContinuous()) {
      // pretend the data is a 1D array, nrows x ncols in length
      ncols *= nrows;
      nrows = 1;
    }

    uint32_t* dstPtr = (u32*)GetDataPointer();
    for(int i=0; i<nrows; i++) {
      auto * img_i = rgb565.GetRow(i); // get pointer to start of row i
      for(int j=0; j<ncols; j++) {
        *dstPtr++ = img_i[j].ToRGBA32(alpha);
      }
    }
    return *this;
  }

  void ImageRGBA::ConvertToShowableFormat(cv::Mat& showImg) const {
    cv::cvtColor(this->get_CvMat_(), showImg, cv::COLOR_RGBA2BGRA);
  }

  void ImageRGBA::SetFromShowableFormat(const cv::Mat& showImg) {
    DEV_ASSERT(showImg.channels() == 3, "ImageRGBA.SetFromShowableFormat.UnexpectedNumChannels");
    cv::cvtColor(showImg, this->get_CvMat_(), cv::COLOR_BGR2RGBA);
  }
  
#if 0 
#pragma mark --- ImageRGB ---
#endif 
  
  ImageRGB::ImageRGB()
  : ImageBase<PixelRGB>()
  {
    
  }
  
  ImageRGB::ImageRGB(s32 nrows, s32 ncols)
  : ImageBase<PixelRGB>(nrows, ncols)
  {
    
  }
  
  ImageRGB::ImageRGB(s32 nrows, s32 ncols, const PixelRGB& fillValue)
  : ImageBase<PixelRGB>(nrows, ncols, fillValue)
  {

  }

  ImageRGB::ImageRGB(s32 nrows, s32 ncols, u8* data)
  : ImageBase<PixelRGB>(nrows, ncols, reinterpret_cast<PixelRGB*>(data))
  {
    
  }
  
  ImageRGB::ImageRGB(const ImageRGBA& imageRGBA)
  : ImageBase<PixelRGB>(imageRGBA.GetNumRows(), imageRGBA.GetNumCols())
  {
    PixelRGB* dataRGB = GetDataPointer();
    const PixelRGBA* dataRGBA = imageRGBA.GetDataPointer();
    
    for(s32 i=0; i<GetNumElements(); ++i)
    {
      // TODO: Is this faster? memcpy(dataRGB+i, dataRGBA+i, 3);
      dataRGB[i].r() = dataRGBA[i].r();
      dataRGB[i].g() = dataRGBA[i].g();
      dataRGB[i].b() = dataRGBA[i].b();
    }
    SetTimestamp(imageRGBA.GetTimestamp());
    SetImageId(imageRGBA.GetImageId());
  }
  
  ImageRGB::ImageRGB(const ImageRGB565& rgb565)
  : ImageRGB(rgb565.GetNumRows(), rgb565.GetNumCols())
  {
    SetFromRGB565(rgb565);
  }
  
  ImageRGB::ImageRGB(const Image& imageGray)
  : ImageBase<PixelRGB>(imageGray.GetNumRows(), imageGray.GetNumCols())
  {
    SetFromGray(imageGray);
  }
  
  ImageRGB& ImageRGB::SetFromGray(const Image& imageGray)
  {
    cv::cvtColor(imageGray.get_CvMat_(), this->get_CvMat_(), CV_GRAY2RGB);
    SetTimestamp(imageGray.GetTimestamp());
    SetImageId(imageGray.GetImageId());
    return *this;
  }
  
  ImageRGB& ImageRGB::SetFromRGB565(const ImageRGB565 &rgb565)
  {
    // Similar to how COLOR_BGR5652BGR appears to be swapping R and B in ConvertToShowableFormat(),
    // COLOR_BGR5652RGB here appears not to, which is what we want.
    cv::cvtColor(rgb565.get_CvMat_(), this->get_CvMat_(), cv::COLOR_BGR5652RGB);
    return *this;
  }

  Image ImageRGB::ToGray() const
  {
    Image grayImage(GetNumRows(), GetNumCols());
    FillGray(grayImage);
    return grayImage;
  }
  
  void ImageRGB::FillGray(Image& grayImage) const
  {
    grayImage.SetTimestamp(GetTimestamp()); // Make sure timestamp gets transferred!
    grayImage.SetImageId(GetImageId());

    grayImage.Allocate(GetNumRows(), GetNumCols());

    u32 numRows = GetNumRows();
    u32 numCols = GetNumCols();

    if(IsContinuous() && grayImage.IsContinuous())
    {
      numCols *= numRows;
      numRows = 1;
    }

    for(u32 i = 0; i < numRows; i++)
    {
      const u8* imagePtr = reinterpret_cast<const u8*>(GetRow(i));
      u8* grayPtr = reinterpret_cast<u8*>(grayImage.GetRow(i));

      u32 j = 0;

#ifdef __ARM_NEON__
      const u32 kNumElementsProcessedPerLoop = 8;
      const u32 kSizeOfRGBElement = 3;
      const u32 kNumIterations = numCols - (kNumElementsProcessedPerLoop - 1);

      for(; j < kNumIterations; j += kNumElementsProcessedPerLoop)
      {
        uint8x8x3_t rgb = vld3_u8(imagePtr);
        imagePtr += kNumElementsProcessedPerLoop*kSizeOfRGBElement;

        uint16x8_t out = vaddl_u8(rgb.val[0], rgb.val[1]);
        out = vaddw_u8(out, rgb.val[1]);
        out = vaddw_u8(out, rgb.val[2]);
        uint8x8_t avg = vshrn_n_u16(out, 2); // Narrowing shift right (divide by 4 and convert to u8)

        vst1_u8(grayPtr, avg);
        grayPtr += kNumElementsProcessedPerLoop;
      }
#endif

      const Vision::PixelRGB* imageRGBPtr = reinterpret_cast<const Vision::PixelRGB*>(imagePtr);

      for(; j < numCols; j++)
      {
        *grayPtr = (((u16)imageRGBPtr->r() + (((u16)imageRGBPtr->g()) << 1) + (u16)imageRGBPtr->b()) >> 2);
        
        imageRGBPtr++;
        grayPtr++;
      }
    }
  }
  
  Image ImageRGB::Threshold(u8 value, bool anyChannel) const
  {
    std::function<u8(const PixelRGB&)> thresholdFcn = [value,anyChannel](const PixelRGB& p)
    {
      if(p.IsBrighterThan(value, anyChannel)) {
        return 255;
      } else {
        return 0;
      }
    };
    
    Image out(GetNumRows(), GetNumCols());
    ApplyScalarFunction(thresholdFcn, out);
    
    return out;
  }
  
  ImageRGB& ImageRGB::NormalizeColor(Array2d<s32>* workingArray)
  {
    GetNormalizedColor(*this, workingArray);
    return *this;
  }
  
  void ImageRGB::GetNormalizedColor(ImageRGB& imgNorm, Array2d<s32>* workingArray) const
  {
    this->CopyTo(imgNorm); // makes data continuous, which is required for reshape
    
    DEV_ASSERT(imgNorm.IsContinuous(), "ImageRGB.GetNormalizedColor.NotContinuous");
    
    // Wrap an Nx3 "header" around the original color data
    cv::Mat imageVector;
    try
    {
      imageVector = imgNorm.get_CvMat_().reshape(1, GetNumElements());
    }
    catch(cv::Exception& e)
    {
      PRINT_NAMED_ERROR("ImageRGB.GetNormalizedColor.OpenCvReshapeFailed",
                        "%s", e.what());
      return;
    }
    
    // Compute the sum along the rows, yielding an Nx1 vector
    cv::Mat_<s32> imageSum;
    if(workingArray != nullptr)
    {
      imageSum = workingArray->get_CvMat_();
    }
    try
    {
      cv::reduce(imageVector, imageSum, 1, CV_REDUCE_SUM, CV_32SC1);
    }
    catch (cv::Exception& e)
    {
      PRINT_NAMED_ERROR("ImageRGB.GetNormalizedColor.OpenCvReduceFailed",
                        "%s", e.what());
      return;
    }
    
    // Scale each row by 255 and divide by the sum, placing the result directly into output data
    // TODO: Avoid the repeat?
    try
    {
      cv::divide(imageVector, cv::repeat(imageSum, 1, imageVector.cols), imageVector, 255.0, CV_8UC1);
    }
    catch (cv::Exception& e)
    {
      PRINT_NAMED_ERROR("ImageRGB.GetNormalizedColor.OpenCvDivideFailed",
                        "%s", e.what());
      return;
    }
  }
  
  void ImageRGB::ConvertToShowableFormat(cv::Mat& showImg) const {
    cv::cvtColor(this->get_CvMat_(), showImg, cv::COLOR_RGB2BGR);
  }

  void ImageRGB::SetFromShowableFormat(const cv::Mat& showImg) {
    DEV_ASSERT(showImg.channels() == 3, "ImageRGB.SetFromShowableFormat.UnexpectedNumChannels");
    cv::cvtColor(showImg, this->get_CvMat_(), cv::COLOR_BGR2RGB);
  }

#if 0
#pragma mark --- ImageRGB565 ---
#endif


  ImageRGB565::ImageRGB565(const ImageRGB& imageRGB)
  : ImageRGB565(imageRGB.GetNumRows(), imageRGB.GetNumCols())
  {
    SetFromImageRGB(imageRGB);
  }
  
  ImageRGB565::ImageRGB565()
  : ImageBase<PixelRGB565>()
  {
    
  }
  
  ImageRGB565::ImageRGB565(s32 nrows, s32 ncols)
  : ImageBase<PixelRGB565>(nrows, ncols)
  {
    
  }

  ImageRGB565::ImageRGB565(s32 nrows, s32 ncols, const std::vector<u16>& pixels)
  : ImageBase<PixelRGB565>(nrows, ncols)
  {
    SetFromVector( pixels );
  }

  ImageRGB565& ImageRGB565::SetFromImage(const Image& image)
  {
    Allocate(image.GetNumRows(), image.GetNumCols());
    
    std::function<PixelRGB565(const u8&)> convertFcn = [](const u8& pix)
    {
      PixelRGB565 pixRGB565(pix,pix,pix);
      return pixRGB565;
    };
    
    image.ApplyScalarFunction(convertFcn, *this);
    
    return *this;
  }
  
  ImageRGB565& ImageRGB565::SetFromImageRGB(const ImageRGB& imageRGB)
  {
    // Similar to how COLOR_BGR5652BGR appears to be swapping R and B in ConvertToShowableFormat(),
    // COLOR_RGB2BGR565 here appears not to, which is what we want.
    cv::cvtColor(imageRGB.get_CvMat_(), this->get_CvMat_(), cv::COLOR_RGB2BGR565);
    return *this;
  }
  
  ImageRGB565& ImageRGB565::SetFromImageRGB(const ImageRGB& imageRGB, const std::array<u8, 256>& gammaLUT)
  {
    Allocate(imageRGB.GetNumRows(), imageRGB.GetNumCols());
    
    std::function<PixelRGB565(const PixelRGB&)> convertFcn = [&gammaLUT](const PixelRGB& pixRGB)
    {
      PixelRGB565 pixRGB565(gammaLUT[pixRGB.r()], gammaLUT[pixRGB.g()], gammaLUT[pixRGB.b()]);
      return pixRGB565;
    };
    
    imageRGB.ApplyScalarFunction(convertFcn, *this);
    
    return *this;
  }

  ImageRGB565& ImageRGB565::SetFromImageRGB565(const ImageRGB565& imageRGB565)
  {
    imageRGB565.CopyTo(*this);
    return *this;
  }

  ImageRGB565& ImageRGB565::SetFromVector(const std::vector<u16>& externalData)
  {
    u16* internalDataPtr = GetRawDataPointer();
    const size_t pixelCount = GetNumRows() * GetNumCols();
    DEV_ASSERT(externalData.size() == pixelCount, "ImageRGB565.SetFromShowableFormat.UnexpectedNumChannels");
    for(auto& i : externalData)
    {
      *internalDataPtr = i;
      ++internalDataPtr;
    }
    return *this;
  }

  cv::Scalar ImageRGB565::GetCvColor(const ColorRGBA& color) const {
    PixelRGB565 color565(color.r(), color.g(), color.b());

    // This is kinda weird that the low byte is expected first, but
    // it must be something about the way opencv is treating PixelRGB565
    // as a 2-channel u8 type.
    return cv::Scalar(color565.GetLowByte(), color565.GetHighByte(), 0, 0);
  }

  void ImageRGB565::ConvertToShowableFormat(cv::Mat& showImg) const {
    // Pixel format is actually RGB565 so by using COLOR_BGR5652RGB
    // it actually gets converted to BGR.
    // OR so I thought... It appears COLOR_BGR5652RGB does not swap
    // the R and B channels as expected. Oddly enough COLOR_BGR5652BGR does!
    // Chalking this up to an OpenCV bug!
    cv::cvtColor(this->get_CvMat_(), showImg, cv::COLOR_BGR5652BGR);
  }

  void ImageRGB565::SetFromShowableFormat(const cv::Mat& showImg) {
    DEV_ASSERT(showImg.channels() == 3, "ImageRGB565.SetFromShowableFormat.UnexpectedNumChannels");
    cv::cvtColor(showImg, this->get_CvMat_(), cv::COLOR_BGR2BGR565);
  }

} // namespace Vision
} // namespace Anki
