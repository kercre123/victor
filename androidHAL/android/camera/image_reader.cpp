/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Copied and modified from https://github.com/googlesamples/android-ndk/tree/camera-sample/camera/preview-read-image

#include "image_reader.h"
#include "utils/native_debug.h"
#include "util/logging/logging.h"

/*
 * MAX_BUF_COUNT:
 *   Max buffers in this ImageReader.
 */
#define MAX_BUF_COUNT 4

/*
 * ImageReader Listener: called by Camera for every frame captured
 * We pass the event to ImageReader class, so it could do some housekeeping about
 * the loaded queue. For example, we could keep a counter to track how many
 * buffers are full and idle in the queue. If camera almost has no buffer to capture
 * we could release ( skip ) some frames by AImageReader_getNextImage() and
 * AImageReader_delete().
 */
void OnImageCallback(void* ctx, AImageReader* reader) {
  reinterpret_cast<ImageReader*>(ctx)->ImageCallback(reader);
}

/*
 * Constructor
 */
ImageReader::ImageReader(ImageResolution_Android* res) :
  reader_(nullptr), imageWatermark_(0) {

  media_status_t status = AImageReader_new(res->width, res->height,
                                           AIMAGE_FORMAT_YUV_420_888,
                                           //AIMAGE_FORMAT_RGB_888,  // Not included in r14b API 24?
                                           MAX_BUF_COUNT, &reader_);
  ASSERT(reader_ && status == AMEDIA_OK, "Failed to create AImageReader");

  AImageReader_ImageListener listener {
      .context = this,
      .onImageAvailable = OnImageCallback,
  };
  AImageReader_setImageListener(reader_, &listener);
}

ImageReader::~ImageReader() {
    ASSERT(reader_, "NULL Pointer to %s", __FUNCTION__);
    AImageReader_delete(reader_);
}

bool ImageReader::IsReady(void) {
    return ((reader_ && imageWatermark_) ? true : false);
}
void ImageReader::ImageCallback(AImageReader* reader) {
  if (++imageWatermark_ >= (MAX_BUF_COUNT - 1)) {
    LOGI("Buffers are full, not pulling images fast enough (%d)", imageWatermark_);
  }
}

ANativeWindow* ImageReader::GetNativeWindow(void) {
  if (!reader_) return nullptr;
  ANativeWindow* nativeWindow;
  media_status_t status = AImageReader_getWindow(reader_, &nativeWindow);
  ASSERT(status == AMEDIA_OK, "Could not get ANativeWindow");

  return nativeWindow;
}

/*
 * GetLastImage()
 *   Retrieve the newest image inside ImageReader's bufferQueue, drop all BUT the last
 *   one to convert and present to display
 */
AImage* ImageReader::GetLastImage(void) {
  AImage* image;
  media_status_t status = AImageReader_acquireLatestImage(reader_, &image);
  if (status != AMEDIA_OK) {
    //        LOGI("NO IMAGE AVAILABLE");
    return nullptr;
  }
  imageWatermark_ = 1;
  return image;
}

/*
 * GetNextImage()
 *   Retrieve the next image in ImageReader's bufferQueue, NOT the last image so no image is
 *   skipped
 */
AImage* ImageReader::GetNextImage(void) {
  AImage* image;
  media_status_t status = AImageReader_acquireNextImage(reader_, &image);
  if (status != AMEDIA_OK) {
      //        LOGI("NO IMAGE AVAILABLE");
      return nullptr;
  }
  return image;
}

/*
 * Helper function for YUV_420 to RGB conversion. Courtesy of Tensorflow ImageClassifier Sample:
 * https://github.com/tensorflow/tensorflow/blob/master/tensorflow/examples/android/jni/yuv2rgb.cc
 * The difference is that here we have to swap UV plane when calling it.
 */
#ifndef MAX
#define MAX(a, b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b); _a > _b ? _a : _b; })
#define MIN(a, b) ({__typeof__(a) _a = (a); __typeof__(b) _b = (b); _a < _b ? _a : _b; })
#endif

// This value is 2 ^ 18 - 1, and is used to clamp the RGB values before their ranges
// are normalized to eight bits.
static const int kMaxChannelValue = 262143;

static inline void YUV2RGB(int nY, int nU, int nV, uint8_t& rByte, uint8_t& gByte, uint8_t& bByte) {
//static inline uint32_t YUV2RGB(int nY, int nU, int nV) {
  nY -= 16;
  nU -= 128;
  nV -= 128;
  if (nY < 0) nY = 0;

  // This is the floating point equivalent. We do the conversion in integer
  // because some Android devices do not have floating point in hardware.
  // nR = (int)(1.164 * nY + 1.596 * nV);
  // nG = (int)(1.164 * nY - 0.813 * nV - 0.391 * nU);
  // nB = (int)(1.164 * nY + 2.018 * nU);

  int nR = (int)(1192 * nY + 1634 * nV);
  int nG = (int)(1192 * nY - 833 * nV - 400 * nU);
  int nB = (int)(1192 * nY + 2066 * nU);

  nR = MIN(kMaxChannelValue, MAX(0, nR));
  nG = MIN(kMaxChannelValue, MAX(0, nG));
  nB = MIN(kMaxChannelValue, MAX(0, nB));

//  nR = (nR >> 10) & 0xff;
//  nG = (nG >> 10) & 0xff;
//  nB = (nB >> 10) & 0xff;
//  return 0xff000000 | (nR << 16) | (nG << 8) | nB;
  
  rByte = (nR >> 10) & 0xff;
  gByte = (nG >> 10) & 0xff;
  bByte = (nB >> 10) & 0xff;
}


bool ImageReader::GetLatestRGBImage(uint8_t* imageData, uint32_t& dataLength) {

  AImage* image = GetLastImage();
  if (image == nullptr) {
      return false;
  }

  if (imageData != nullptr) {
    ConvertImageToRGB(image, imageData, dataLength);
    //SaveDataToFile(imageData, dataLength);
  }

  --imageWatermark_;
  AImage_delete(image);
  return true;

}

void ImageReader::SaveDataToFile(uint8_t* buf, uint32_t dataLength) {
  
  // NOTE: The output file needs to already exist
  //       as app doesn't have permission to create file.
  FILE *f = fopen("/data/local/tmp/image.raw", "wb");
  if (f != nullptr) {
    LOGI("Saving image data to file");
    fwrite(buf, dataLength, 1, f);
    fflush(f);
    fclose(f);
  } else {
    LOGW("Failed to save image to file");
  }
}
                                 
                                 

void ImageReader::ConvertImageToRGB(AImage* image, uint8_t* buf, uint32_t& dataLength) {
  int srcFormat = -1;
  AImage_getFormat(image, &srcFormat);
  ASSERT(AIMAGE_FORMAT_YUV_420_888 == srcFormat, "Failed to get format");
  int srcPlanes = 0;
  AImage_getNumberOfPlanes(image, &srcPlanes);
  ASSERT(srcPlanes == 3, "Is not 3 planes");
  AImageCropRect srcRect;
  AImage_getCropRect(image, &srcRect);
  
  int yStride, uvStride;
  uint8_t *yPixel, *uPixel, *vPixel;
  int32_t yLen, uLen, vLen;
  AImage_getPlaneRowStride(image, 0, &yStride);
  AImage_getPlaneRowStride(image, 1, &uvStride);
  AImage_getPlaneData(image, 0, &yPixel, &yLen);
  AImage_getPlaneData(image, 1, &uPixel, &uLen);  
  AImage_getPlaneData(image, 2, &vPixel, &vLen);
  int32_t uvPixelStride;
  AImage_getPlanePixelStride(image, 1, &uvPixelStride);
  
  int height = srcRect.bottom - srcRect.top;
  int width = srcRect.right - srcRect.left;
  
  dataLength = height*width*3;
  
  int64_t timestampNs = 0;
  AImage_getTimestamp(image, &timestampNs);
  
//  PRINT_NAMED_INFO("ImageReader.ConvertImageToRGB",
//                   "%fms: %d x %d, format %d, numPlanes %d, yLength %d, pixelStride %d, rect (%d, %d, %d, %d)",
//                   0.000001 * timestampNs, width, height, srcFormat, srcPlanes, yLen, uvPixelStride, srcRect.left, srcRect.top, srcRect.right, srcRect.bottom);
  
  uint8_t *out = buf;
  for (int y = 0; y < height; y++) {
    const uint8_t *pY = yPixel + yStride * (y + srcRect.top) + srcRect.left;
    
    int uv_row_start = uvStride * ((y + srcRect.top) >> 1);
    const uint8_t *pU = uPixel + uv_row_start + (srcRect.left >> 1);
    const uint8_t *pV = vPixel + uv_row_start + (srcRect.left >> 1);
    
    for (int x = 0; x < width; x++) {
      const int uv_offset = (x >> 1) * uvPixelStride;
      YUV2RGB(pY[x], pU[uv_offset], pV[uv_offset], out[3*x], out[3*x+1], out[3*x+2]);
    }
    out += 3*width;
  }
  
}

