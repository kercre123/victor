/**
 * File: image.h
 *
 * Author: Andrew Stein
 * Date:   11/20/2014
 *
 * Description: Defines a container for images on the basestation.
 *
 * Copyright: Anki, Inc. 2014
 **/

#ifndef ANKI_CORETECH_VISION_BASESTATION_IMAGE_H
#define ANKI_CORETECH_VISION_BASESTATION_IMAGE_H

#include "anki/common/types.h"

#include "anki/common/basestation/array2d.h"
#include "anki/common/basestation/colorRGBA.h"
#include "anki/common/basestation/math/point.h"

#include "anki/vision/CameraSettings.h"


namespace Anki {
namespace Vision {

  template<typename T>
  class ImageBase : public Array2d<T>
  {
  public:
    ImageBase() : Array2d<T>() { }
    ImageBase(s32 nrows, s32 ncols) : Array2d<T>(nrows, ncols) { }
    ImageBase(s32 nrows, s32 ncols, T* data) : Array2d<T>(nrows, ncols, data) { }
    
#   if ANKICORETECH_USE_OPENCV
    // Construct from a cv::Mat_<T>
    ImageBase(cv::Mat_<T>& cvMat) : Array2d<T>(cvMat) { }
#   endif
    
    void SetTimestamp(TimeStamp_t ts);
    TimeStamp_t GetTimestamp() const;
    
    void Display(const char *windowName, bool pause = false) const;

    using Array2d<T>::GetDataPointer;
    using Array2d<T>::IsEmpty;
    
  protected:
    TimeStamp_t     _timeStamp;
    
  }; // class ImageBase
  
  
  class Image : public ImageBase<u8>
  {
  public:
    
    // Create an empty image
    Image();
    
    // Allocate a new image
    Image(s32 nrows, s32 ncols);
    
    // Wrap image "header" around given data pointer: no allocation.
    Image(s32 nrows, s32 ncols, u8* data);
    
#   if ANKICORETECH_USE_OPENCV
    // Construct from a cv::Mat_<u8>
    Image(cv::Mat_<u8>& cvMat);
    using Array2d<u8>::get_CvMat_;
#   endif

    s32 GetConnectedComponents(Array2d<s32>& labelImage,
                               std::vector<std::vector< Point2<s32> > >& regionPoints) const;
    
    // Resize in place by scaleFactor
    void Resize(f32 scaleFactor);
    
    // Resize in place to a specific size
    void Resize(s32 desiredRows, s32 desiredCols);
    
    // Resize into the new image (which is already the desired size)
    void Resize(Image& resizedImage) const;
    
    using Array2d<u8>::GetDataPointer;
    using Array2d<u8>::IsEmpty;
  }; // class Image
  
  
  class ImageRGBA : public ImageBase<u32>
  {
  public:
    ImageRGBA();
    ImageRGBA(s32 nrows, s32 ncols); // allocates
    
    // No allocation, just wraps a header around given data.
    ImageRGBA(s32 nrows, s32 ncols, u32* data_32bpp);
    
    // Allocates and expands given 24bpp RGB data to into internal 32bpp RGBA format.
    // Assumes data buffer is nrows*ncols*3 bytes in length!
    ImageRGBA(s32 nrows, s32 ncols, u8* data_24bpp);
    
#   if ANKICORETECH_USE_OPENCV
    ImageRGBA(cv::Mat& cvMat);
#   endif
    
    // Access specific channel of a specific pixel
    u8 GetR(s32 row, s32 col) const;
    u8 GetG(s32 row, s32 col) const;
    u8 GetB(s32 row, s32 col) const;
    u8 GetAlpha(s32 row, s32 col) const;
    
    u8 GetGray(s32 row, s32 col) const;
    
    static u8 GetGray(u32 colorPixel);
    
    ColorRGBA GetColor(s32 row, s32 col) const;
    
    /*
    void SetR(s32 row, s32 col, u8 r);
    void SetG(s32 row, s32 col, u8 r);
    void SetB(s32 row, s32 col, u8 r);
    */
    
    Image ToGray() const;
    
  }; // class ImageRGBA
  
  
  
  class ImageDeChunker
  {
  public:
    static const u32 CHUNK_SIZE = 1400;
    
    ImageDeChunker();
    
    // Return true if image just completed and is available, false otherwise.
    bool AppendChunk(u32 imageId, u32 frameTimeStamp, u16 nrows, u16 ncols,
                     ImageEncoding_t encoding, u8 totalChunkCount,
                     u8 chunkId, const std::array<u8, CHUNK_SIZE>& data, u32 chunkSize);
    
    bool AppendChunk(u32 imageId, u32 frameTimeStamp, u16 nrows, u16 ncols,
                     ImageEncoding_t encoding, u8 totalChunkCount,
                     u8 chunkId, const u8* data, u32 chunkSize);
    
    cv::Mat GetImage() { return _img; }
    const std::vector<u8>& GetRawData() const { return _imgData; }
    
    //void SetImageCompleteCallback(std::function<void(const Vision::Image&)>callback) const;
    
  private:

    u32              _imgID;
    std::vector<u8>  _imgData;
    u32              _imgBytes;
    u32              _imgWidth, _imgHeight;
    u8               _expectedChunkId;
    bool             _isImgValid;
    
    cv::Mat          _img;
    
  }; // class ImageDeChunker
  
  
  
  //
  // Inlined implemenations
  //
  template<typename T>
  inline void ImageBase<T>::SetTimestamp(TimeStamp_t ts) {
    _timeStamp = ts;
  }
  
  template<typename T>
  inline TimeStamp_t ImageBase<T>::GetTimestamp() const {
    return _timeStamp;
  }
  
  // Helper macros for getting RGB from a 32bit value
# define EXTRACT_R(__PIXEL__) ((__PIXEL__ & 0xff000000) >> 24)
# define EXTRACT_G(__PIXEL__) ((__PIXEL__ & 0x00ff0000) >> 16)
# define EXTRACT_B(__PIXEL__) ((__PIXEL__ & 0x0000ff00) >>  8)
# define EXTRACT_A(__PIXEL__)  (__PIXEL__ & 0x000000ff)
  
  inline u8 ImageRGBA::GetR(s32 row, s32 col) const {
    const u32 pixel = operator()(row,col);
    return static_cast<u8>(EXTRACT_R(pixel));
  }

  inline u8 ImageRGBA::GetG(s32 row, s32 col) const {
    const u32 pixel = operator()(row,col);
    return static_cast<u8>(EXTRACT_G(pixel));
  }
  
  inline u8 ImageRGBA::GetB(s32 row, s32 col) const {
    const u32 pixel = operator()(row,col);
    return static_cast<u8>(EXTRACT_B(pixel));
  }
  
  inline u8 ImageRGBA::GetAlpha(s32 row, s32 col) const {
    const u32 pixel = operator()(row,col);
    return static_cast<u8>(EXTRACT_A(pixel));
  }
  
  inline u8 ImageRGBA::GetGray(u32 pixel)
  {
    u32 average = EXTRACT_B(pixel);  // start with blue
    average += EXTRACT_G(pixel);     // add green
    average += EXTRACT_R(pixel);     // add red
    average /= 3;
    return static_cast<u8>(average);
  }
  
  inline u8 ImageRGBA::GetGray(s32 row, s32 col) const {
    return ImageRGBA::GetGray( operator()(row,col) );
  }
  
# undef EXTRACT_R
# undef EXTRACT_G
# undef EXTRAC_B
  
  inline ColorRGBA ImageRGBA::GetColor(s32 row, s32 col) const {
    return ColorRGBA(operator()(row,col));
  }

} // namespace Vision
} // namespace Anki

#endif // ANKI_CORETECH_VISION_BASESTATION_IMAGE_H
