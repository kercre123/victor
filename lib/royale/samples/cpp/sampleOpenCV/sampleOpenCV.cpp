/****************************************************************************\
 * Copyright (C) 2017 Infineon Technologies & pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#include <royale.hpp>
#include <iostream>
#include <mutex>
#include <opencv2/opencv.hpp>

#include <sample_utils/PlatformResources.hpp>

using namespace royale;
using namespace sample_utils;
using namespace std;
using namespace cv;

// Linker errors for the OpenCV sample
//
// If this example gives linker errors about undefined references to cv::namedWindow and cv::imshow,
// or QFontEngine::glyphCache and qMessageFormatString (from OpenCV to Qt), it may be caused by a
// change in the compiler's C++ ABI.
//
// With Ubuntu and Debian's distribution packages, the libopencv packages that have 'v5' at the end
// of their name, for example libopencv-video2.4v5, are compatible with GCC 5 (and GCC 6), but
// incompatible with GCC 4.8 and GCC 4.9. The -dev packages don't have the postfix, but depend on
// the v5 (or non-v5) version of the corresponding lib package.  When Ubuntu moves to OpenCV 3.0,
// they're likely to drop the postfix (but the packages will be for GCC 5 or later).
//
// If you are manually installing OpenCV or Qt, you need to ensure that the binaries were compiled
// with the same version of the compiler.  The version number of the packages themselves doesn't say
// which ABI they use, it depends on which version of the compiler was used.

class MyListener : public IDepthDataListener
{

public :

    MyListener() :
        undistortImage (false)
    {
    }

    void onNewData (const DepthData *data)
    {
        // this callback function will be called for every new
        // depth frame

        std::lock_guard<std::mutex> lock (flagMutex);

        // create two images which will be filled afterwards
        // each image containing one 32Bit channel
        zImage.create (Size (data->width, data->height), CV_32FC1);
        grayImage.create (Size (data->width, data->height), CV_32FC1);

        // set the image to zero
        zImage = Scalar::all (0);
        grayImage = Scalar::all (0);

        int k = 0;
        for (int y = 0; y < zImage.rows; y++)
        {
            float *zRowPtr = zImage.ptr<float> (y);
            float *grayRowPtr = grayImage.ptr<float> (y);
            for (int x = 0; x < zImage.cols; x++, k++)
            {
                auto curPoint = data->points.at (k);
                if (curPoint.depthConfidence > 0)
                {
                    // if the point is valid, map the pixel from 3D world
                    // coordinates to a 2D plane (this will distort the image)
                    zRowPtr[x] = adjustZValue (curPoint.z);
                    grayRowPtr[x] = adjustGrayValue (curPoint.grayValue);
                }
            }
        }

        // create images to store the 8Bit version (some OpenCV
        // functions may only work on 8Bit images)
        zImage8.create (Size (data->width, data->height), CV_8UC1);
        grayImage8.create (Size (data->width, data->height), CV_8UC1);

        // convert images to the 8Bit version
        // This sample uses a fixed scaling of the values to (0, 255) to avoid flickering.
        // You can also replace this with an automatic scaling by using
        // normalize(zImage, zImage8, 0, 255, NORM_MINMAX, CV_8UC1)
        // normalize(grayImage, grayImage8, 0, 255, NORM_MINMAX, CV_8UC1)
        zImage.convertTo (zImage8, CV_8UC1);
        grayImage.convertTo (grayImage8, CV_8UC1);

        if (undistortImage)
        {
            // call the undistortion function on the z image
            Mat temp = zImage8.clone();
            undistort (temp, zImage8, cameraMatrix, distortionCoefficients);
        }

        // scale and display the depth image
        scaledZImage.create (Size (data->width * 4, data->height * 4), CV_8UC1);
        resize (zImage8, scaledZImage, scaledZImage.size());

        imshow ("Depth", scaledZImage);

        if (undistortImage)
        {
            // call the undistortion function on the gray image
            Mat temp = grayImage8.clone();
            undistort (temp, grayImage8, cameraMatrix, distortionCoefficients);
        }

        // scale and display the gray image
        scaledGrayImage.create (Size (data->width * 4, data->height * 4), CV_8UC1);
        resize (grayImage8, scaledGrayImage, scaledGrayImage.size());

        imshow ("Gray", scaledGrayImage);
    }

    void setLensParameters (const LensParameters &lensParameters)
    {
        // Construct the camera matrix
        // (fx   0    cx)
        // (0    fy   cy)
        // (0    0    1 )
        cameraMatrix = (Mat1d (3, 3) << lensParameters.focalLength.first, 0, lensParameters.principalPoint.first,
                        0, lensParameters.focalLength.second, lensParameters.principalPoint.second,
                        0, 0, 1);

        // Construct the distortion coefficients
        // k1 k2 p1 p2 k3
        distortionCoefficients = (Mat1d (1, 5) << lensParameters.distortionRadial[0],
                                  lensParameters.distortionRadial[1],
                                  lensParameters.distortionTangential.first,
                                  lensParameters.distortionTangential.second,
                                  lensParameters.distortionRadial[2]);
    }

    void toggleUndistort()
    {
        std::lock_guard<std::mutex> lock (flagMutex);
        undistortImage = !undistortImage;
    }

private:

    // adjust z value to fit fixed scaling, here max dist is 2.5m
    // the max dist here is used as an example and can be modified
    float adjustZValue (float zValue)
    {
        float clampedDist = std::min (2.5f, zValue);
        float newZValue = clampedDist / 2.5f * 255.0f;
        return newZValue;
    }

    // adjust gray value to fit fixed scaling, here max value is 180
    // the max value here is used as an example and can be modified
    float adjustGrayValue (uint16_t grayValue)
    {
        float clampedVal = std::min (180.0f, grayValue * 1.0f);
        float newGrayValue = clampedVal / 180.f * 255.0f;
        return newGrayValue;
    }

    // define images for depth and gray
    // and for their 8Bit and scaled versions
    Mat zImage, zImage8, scaledZImage;
    Mat grayImage, grayImage8, scaledGrayImage;

    // lens matrices used for the undistortion of
    // the image
    Mat cameraMatrix;
    Mat distortionCoefficients;

    std::mutex flagMutex;
    bool undistortImage;
};

int main (int argc, char *argv[])
{
    // Windows requires that the application allocate these, not the DLL.
    PlatformResources resources;

    // This is the data listener which will receive callbacks.  It's declared
    // before the cameraDevice so that, if this function exits with a 'return'
    // statement while the camera is still capturing, it will still be in scope
    // until the cameraDevice's destructor implicitly de-registers the listener.
    MyListener listener;

    // this represents the main camera device object
    std::unique_ptr<ICameraDevice> cameraDevice;

    // the camera manager will query for a connected camera
    {
        CameraManager manager;

        // check the number of arguments
        if (argc > 1)
        {
            // if the program was called with an argument try to open this as a file
            cout << "Trying to open : " << argv[1] << endl;
            cameraDevice = manager.createCamera (argv[1]);
        }
        else
        {
            // if no argument was given try to open the first connected camera
            royale::Vector<royale::String> camlist (manager.getConnectedCameraList());
            cout << "Detected " << camlist.size() << " camera(s)." << endl;

            if (!camlist.empty())
            {
                cameraDevice = manager.createCamera (camlist[0]);
            }
            else
            {
                cerr << "No suitable camera device detected." << endl
                     << "Please make sure that a supported camera is plugged in, all drivers are "
                     << "installed, and you have proper USB permission" << endl;
                return 1;
            }

            camlist.clear();
        }
    }
    // the camera device is now available and CameraManager can be deallocated here

    if (cameraDevice == nullptr)
    {
        // no cameraDevice available
        if (argc > 1)
        {
            cerr << "Could not open " << argv[1] << endl;
            return 1;
        }
        else
        {
            cerr << "Cannot create the camera device" << endl;
            return 1;
        }
    }

    // IMPORTANT: call the initialize method before working with the camera device
    auto status = cameraDevice->initialize();
    if (status != CameraStatus::SUCCESS)
    {
        cerr << "Cannot initialize the camera device, error string : " << getErrorString (status) << endl;
        return 1;
    }

    // retrieve the lens parameters from Royale
    LensParameters lensParameters;
    status = cameraDevice->getLensParameters (lensParameters);
    if (status != CameraStatus::SUCCESS)
    {
        cerr << "Can't read out the lens parameters" << endl;
        return 1;
    }

    listener.setLensParameters (lensParameters);

    // register a data listener
    if (cameraDevice->registerDataListener (&listener) != CameraStatus::SUCCESS)
    {
        cerr << "Error registering data listener" << endl;
        return 1;
    }

    // create two windows
    namedWindow ("Depth", WINDOW_AUTOSIZE);
    namedWindow ("Gray", WINDOW_AUTOSIZE);

    // start capture mode
    if (cameraDevice->startCapture() != CameraStatus::SUCCESS)
    {
        cerr << "Error starting the capturing" << endl;
        return 1;
    }

    int currentKey = 0;

    while (currentKey != 27)
    {
        // wait until a key is pressed
        currentKey = waitKey (0) & 255;

        if (currentKey == 'd')
        {
            // toggle the undistortion of the image
            listener.toggleUndistort();
        }
    }

    // stop capture mode
    if (cameraDevice->stopCapture() != CameraStatus::SUCCESS)
    {
        cerr << "Error stopping the capturing" << endl;
        return 1;
    }

    return 0;
}
