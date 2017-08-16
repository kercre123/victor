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

#ifndef CAMERA_IMAGE_READER_H
#define CAMERA_IMAGE_READER_H
#include <media/NdkImageReader.h>

/*
 * ImageResolution:
 *     A Data Structure to communicate resolution between camera and ImageReader
 */
struct ImageResolution_Android {
  int32_t  width;
  int32_t  height;
  int32_t  format;
};

class ImageReader {
  public:
    /*
     * Ctor and Dtor()
     */
    explicit ImageReader(ImageResolution_Android* res);

    ~ImageReader();

    /*
     * ImagerReader is ready and with Images in bufferQueue. User could begin to pull images
     */
    bool IsReady(void);

    /*
     * Retrieve ANativeWindow from ImageReader. Used to create camera capture
     * session output. ANativeWindow is bufferQueue, automatically handled
     * by camera ( producer ) and Reader; display engine pull image out to display
     * via function DisplayImage()
     */
    ANativeWindow * GetNativeWindow(void);

    /*
     * ImageReader callback handler: get called each frame is captured into Reader
     * This app does not do anything ( just place holder ). This function is for
     * ImageReader() internal usage
     */
    void ImageCallback(AImageReader* reader);

    /*
     * Retrieve buffer of RGB image data
     */
    bool GetLatestRGBImage(uint8_t* imageData, uint32_t& dataLength);

  private:
    AImageReader* reader_;
    volatile int32_t imageWatermark_;

    void ConvertImageToRGB(AImage* image, uint8_t* buf, uint32_t& dataLength);
    void SaveDataToFile(uint8_t* buf, uint32_t dataLength);
  
    AImage* GetNextImage(void);
    AImage* GetLastImage(void);
};

#endif //CAMERA_IMAGE_READER_H
