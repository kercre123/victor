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

#include "anki/common/basestation/math/point_impl.h"

#include "anki/vision/basestation/image_impl.h"

#if ANKICORETECH_USE_OPENCV
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#endif

// Uncomment this to test cropping the 4:3 image to 16:9 format by simply
// adding block bars to the top and bottom 12.5% of the image
//#define TEST_16_9_ASPECT_RATIO

namespace Anki {
namespace Vision {
  
  
  Image::Image()
  : ImageBase<u8>()
  {
    
  }
  
  Image::Image(s32 nrows, s32 ncols)
  : ImageBase<u8>(nrows, ncols)
  {
    
  }
  
  Image::Image(s32 nrows, s32 ncols, u8* data)
  : ImageBase<u8>(nrows,ncols,data)
  {
    
  }
  
#if ANKICORETECH_USE_OPENCV
  Image::Image(cv::Mat_<u8>& cvMat)
  : ImageBase<u8>(cvMat)
  {
    
  }
#endif
  
  template<typename T>
  void ImageBase<T>::Display(const char *windowName, bool pause) const
  {
#   if ANKICORETECH_USE_OPENCV
    cv::imshow(windowName, this->get_CvMat_());
    if(pause) {
      cv::waitKey();
    }
#   endif
  }

  s32 Image::GetConnectedComponents(Array2d<s32>& labelImage,
                                    std::vector<std::vector< Point2<s32> > >& regionPoints) const
  {
    // Until we start using OpenCV 3, which has an actual connected components implementation,
    // this is adapted from here:
    //    http://nghiaho.com/uploads/code/opencv_connected_component/blob.cpp
    //
    
    regionPoints.clear();
    
    // Fill the label_image with the blobs
    // 0  - background
    // 1  - unlabelled foreground
    // 2+ - labelled foreground
    
    int labelCount = 2; // starts at 2 because 0,1 are used already
    
    for(int y=0; y < labelImage.GetNumRows(); y++) {
      s32 *row = labelImage.GetRow(y); // (s32*)labelImage.ptr(y);
      for(int x=0; x < labelImage.GetNumCols(); x++) {
        if(row[x] != 1) {
          continue;
        }
        
        cv::Rect rect;
        cv::floodFill(labelImage.get_CvMat_(), cv::Point(x,y), labelCount, &rect, 0, 0, 4);
        
        std::vector<Point2<s32> > blob;
        
        for(int i=rect.y; i < (rect.y+rect.height); i++) {
          int *row2 = (int*)labelImage.GetRow(i);
          for(int j=rect.x; j < (rect.x+rect.width); j++) {
            if(row2[j] != labelCount) {
              continue;
            }
            
            blob.emplace_back(j,i);
          }
        }
        
        regionPoints.emplace_back(blob);
        
        ++labelCount;
      }
    }
    
    return labelCount-2;
    
  } // GetConnectedComponents()
  
  
  void Image::Resize(f32 scaleFactor)
  {
    cv::resize(this->get_CvMat_(), this->get_CvMat_(), cv::Size(), scaleFactor, scaleFactor, CV_INTER_LINEAR);
  }
  
  void Image::Resize(s32 desiredRows, s32 desiredCols)
  {
    if(desiredRows != GetNumRows() || desiredCols != GetNumCols()) {
      const cv::Size desiredSize(desiredCols, desiredRows);
      cv::resize(this->get_CvMat_(), this->get_CvMat_(), desiredSize, 0, 0, CV_INTER_LINEAR);
    }
  }
  
  void Image::Resize(Image& resizedImage) const
  {
    if(resizedImage.IsEmpty()) {
      printf("Image::Resize - Output image should already be the desired size.\n");
    } else {
      const cv::Size desiredSize(resizedImage.GetNumCols(), resizedImage.GetNumRows());
      cv::resize(this->get_CvMat_(), resizedImage.get_CvMat_(), desiredSize, 0, 0, CV_INTER_LINEAR);
    }
  }
  
  
  
#if 0
#pragma mark --- ImageRGBA ---
#endif
  
  ImageRGBA::ImageRGBA()
  : ImageBase<u32>()
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols)
  : ImageBase<u32>(nrows, ncols)
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols, u32* data)
  : ImageBase<u32>(nrows, ncols, data)
  {
    
  }
  
  ImageRGBA::ImageRGBA(s32 nrows, s32 ncols, u8* data24)
  : ImageRGBA(nrows,ncols)
  {
    s32 index=0;
    u32* data32 = GetDataPointer();
    for(s32 i=0; i<nrows*ncols; ++i, index+=3) {
      data32[i] = ((static_cast<u32>(data24[index])  <<24) +
                   (static_cast<u32>(data24[index+1])<<16) +
                   (static_cast<u32>(data24[index+2])<<8));
    }
  }
  
  Image ImageRGBA::ToGray() const
  {
    // Create a little lambda wrapper for converting a pixel to gray, in the
    // std::function form req'd by ApplyScalarFunction
    std::function<void(const u32&, u8&)> convertToGrayHelper = [](const u32& rgbPixel, u8& grayValue) {
      grayValue = ImageRGBA::GetGray(rgbPixel);
    };
    
    // Call the grayscale conversion on every pixel of this color image
    Image grayImage(GetNumRows(), GetNumCols());
    ApplyScalarFunction(convertToGrayHelper, grayImage);
    
    return grayImage;
  }
  
  
#if 0
#pragma mark --- ImageDeChunker ---
#endif
  
  ImageDeChunker::ImageDeChunker()
  : _imgID(0)
  , _imgBytes(0)
  , _imgWidth(0)
  , _imgHeight(0)
  , _expectedChunkId(0)
  , _isImgValid(false)
  {
    
  }
  
  // Turn a fully assembled MINIPEG_GRAY image into a JPEG with header and footer
  // This is a port of C# code from Nathan.
  static void miniGrayToJpeg(const std::vector<u8>& bufferIn, const s32 bufferLength, std::vector<u8>& bufferOut)
  {
    // Fetch quality to decide which header to use
    const int quality = bufferIn[0];
    
    // Pre-baked JPEG header for grayscale, Q50
    static const u8 header50[] = {
      0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
      0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, // 0x19 = QTable
      0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28, 0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23,
      0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40,
      0x44, 0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57, 0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D,
      
      //0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0xF0, // 0x5E = Height x Width
      0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x01, 0x28, // 0x5E = Height x Width
      
      //0x01, 0x40, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
      0x01, 0x90, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
      
      0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
      0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
      0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
      0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
      0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
      0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
      0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
      0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01,
      0x00, 0x00, 0x3F, 0x00
    };

    // Pre-baked JPEG header for grayscale, Q80
    static const u8 header80[] = {
      0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01,
      0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x06, 0x04, 0x05, 0x06, 0x05, 0x04, 0x06,
      0x06, 0x05, 0x06, 0x07, 0x07, 0x06, 0x08, 0x0A, 0x10, 0x0A, 0x0A, 0x09, 0x09, 0x0A, 0x14, 0x0E,
      0x0F, 0x0C, 0x10, 0x17, 0x14, 0x18, 0x18, 0x17, 0x14, 0x16, 0x16, 0x1A, 0x1D, 0x25, 0x1F, 0x1A,
      0x1B, 0x23, 0x1C, 0x16, 0x16, 0x20, 0x2C, 0x20, 0x23, 0x26, 0x27, 0x29, 0x2A, 0x29, 0x19, 0x1F,
      0x2D, 0x30, 0x2D, 0x28, 0x30, 0x25, 0x28, 0x29, 0x28, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0xF0,
      0x01, 0x40, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01,
      0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04,
      0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03,
      0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
      0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08,
      0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16,
      0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
      0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
      0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
      0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
      0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6,
      0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4,
      0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA,
      0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01,
      0x00, 0x00, 0x3F, 0x00
    };
    
    const u8* header = nullptr;
    int headerLength = 0;
    switch(quality)
    {
      case 50:
        header = header50;
        headerLength = sizeof(header50);
        break;
      case 80:
        header = header80;
        headerLength = sizeof(header80);
        break;
      default:
        PRINT_NAMED_ERROR("miniGrayToJpeg", "No header for quality of %d\n", quality);
        return;
    }
    
    assert(header != nullptr);
    assert(headerLength > 0);
    
    // Allocate enough space for worst case expansion
    bufferOut.resize(bufferLength*2 + headerLength);
    
    int off = headerLength;
    std::copy(header, header+headerLength, bufferOut.begin());
    
    
    
    // Add byte stuffing - one 0 after each 0xff
    for (int i = 1; i < bufferLength; i++)
    {
      bufferOut[off++] = bufferIn[i];
      if (bufferIn[i] == 0xff) {
        bufferOut[off++] = 0;
      }
    }
    
    bufferOut[off++] = 0xFF;
    bufferOut[off++] = 0xD9;
    
    bufferOut.resize(off);
  
  } // miniGrayToJpeg()
  
  bool ImageDeChunker::AppendChunk(u32 newImageId, u32 frameTimeStamp, u16 nrows, u16 ncols,
                                   ImageEncoding_t encoding, u8 totalChunkCount,
                                   u8 chunkId, const u8* data, u32 chunkSize)
  {
    if(chunkSize > ImageDeChunker::CHUNK_SIZE) {
      PRINT_NAMED_ERROR("ImageDeChunker.AppendChunk", "Expecting chunks of size no more than %d, got %d.\n",
                        ImageDeChunker::CHUNK_SIZE, chunkSize);
      return false;
    }
    
    // If msgID has changed, then start over.
    if (newImageId != _imgID) {
      _imgID = newImageId;
      _imgBytes = 0;
      _imgWidth  = ncols;
      _imgHeight = nrows;
      _imgData.resize(_imgWidth*_imgHeight*3);
      _isImgValid = chunkId == 0;
      _expectedChunkId = 0;
    }
    
    // Check if a chunk was received out of order
    if (chunkId != _expectedChunkId) {
      PRINT_NAMED_INFO("MessageImageChunk.ChunkDropped",
                       "Expected chunk %d, got %d\n", _expectedChunkId, chunkId);
      _isImgValid = false;
    }
    
    _expectedChunkId = chunkId + 1;
    
    // We've received all data when the msg chunkSize is less than the max
    const bool isLastChunk =  chunkId == totalChunkCount-1;
    
    if (!_isImgValid) {
      if (isLastChunk) {
        PRINT_NAMED_INFO("MessageImageChunk.IncompleteImage",
                         "Received last chunk of invalidated image\n");
      }
      return false;
    }
    
    // Msgs are guaranteed to be received in order (with TCP) so just append data to array
    std::copy(data, data + chunkSize, _imgData.begin() + _imgBytes);
    _imgBytes += chunkSize;
    
    if (isLastChunk) {
      
      switch(encoding)
      {
        case IE_JPEG_COLOR:
        {
          _img = cv::imdecode(_imgData, CV_LOAD_IMAGE_COLOR);
          cvtColor(_img, _img, CV_BGR2RGB);
          break;
        }
          
        case IE_MINIPEG_GRAY:
        {
          std::vector<u8> buffer;
          miniGrayToJpeg(_imgData, _imgBytes, buffer);
          _img = cv::imdecode(buffer, CV_LOAD_IMAGE_GRAYSCALE);
          break;
        }

        case IE_JPEG_GRAY:
        {
          _img = cv::imdecode(_imgData, CV_LOAD_IMAGE_GRAYSCALE);
          break;
        }
        case IE_RAW_GRAY:
        {
          // Already decompressed.
          // Raw image bytes is the same as total received bytes.
          _img = cv::Mat_<u8>(_imgHeight, _imgWidth, &(_imgData[0]));
          break;
        }
        case IE_RAW_RGB:
          // Already decompressed.
          // Raw image bytes is the same as total received bytes.
          _img = cv::Mat(_imgHeight, _imgWidth, CV_8UC3, &(_imgData[0]));
          break;
        case IE_JPEG_CHW:
        {
          _img = cv::imdecode(_imgData, CV_LOAD_IMAGE_COLOR);
          cvtColor(_img, _img, CV_BGR2RGB);
          cv::copyMakeBorder(_img, _img, 0, 0, 160, 160, cv::BORDER_CONSTANT, 0);          
          break;
        }
        default:
          printf("***ERRROR - ProcessVizImageChunkMessage: Encoding %d not yet supported for decoding image chunks.\n", encoding);
          return false;
      } // switch(encoding)
      
      if(_img.rows != _imgHeight || _img.cols != _imgWidth) {
        printf("***ERROR - ImageDeChunker.AppendChunk: expected %dx%d image after decoding, got %dx%d.\n", _imgWidth, _imgHeight, _img.cols, _img.rows);
      }
      
#     ifdef TEST_16_9_ASPECT_RATIO
      // TEST 16:9 format by blacking out 25% of the rows (12.5% at top and bottom)
      _img(cv::Range(0,_img.rows*.125),cv::Range::all()).setTo(0);
      _img(cv::Range(0.875*_img.rows,_img.rows),cv::Range::all()).setTo(0);
#     endif
      
    } // if(isLastChunk)
    
    return isLastChunk;
    
  }
  
  bool ImageDeChunker::AppendChunk(u32 newImageId, u32 frameTimeStamp, u16 nrows, u16 ncols,
                                   ImageEncoding_t encoding, u8 totalChunkCount,
                                   u8 chunkId, const std::array<u8, CHUNK_SIZE>& data, u32 chunkSize)
  {
    return AppendChunk(newImageId, frameTimeStamp, nrows, ncols, encoding, totalChunkCount,
                       chunkId, &(data[0]), chunkSize);
  } // AppendChunk()
  
  /*
  Image ImageDeChunker::GetImage() const
  {
    if(_img.channels() == 3) {
      Image grayImage(_img.rows, _img.cols);
      cvtColor(_img, grayImage.get_CvMat_(), CV_RGB2GRAY);
    } else {
      return Vision::Image(_img.rows, _img.cols, _img.data);
    }
  }
  
  ImageRGBA ImageDeChunker::GetImageRGBA() const
  {
    // TODO Fill this in!
  }
  */
} // namespace Vision
} // namespace Anki
