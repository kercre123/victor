/****************************************************************************\
 * Copyright (C) 2017 pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#include <RoyaleInRos.hpp>

#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud2_iterator.h>
#include <sensor_msgs/Image.h>
#include <sensor_msgs/image_encodings.h>

#include <sstream>

using namespace std;
using namespace royale;

RoyaleInRos::RoyaleInRos() :
    IDepthDataListener(),
    IExposureListener(),
    m_cameraDevice (nullptr),
    m_frames (0),
    m_exposureTime (0),
    m_grayDivisor (100),
    m_minFilter (0.0f),
    m_maxFilter (7.5f),
    m_initPanel (false),
    m_autoExposure (false)
{
}

RoyaleInRos::~RoyaleInRos()
{
    stop();
}

void RoyaleInRos::onInit()
{
    // Retrieve the handle of the current node
    auto nodeHandle = getMTPrivateNodeHandle();

    // Advertise our point cloud topic and image topics
    m_pubCameraInfo = nodeHandle.advertise<sensor_msgs::CameraInfo> ("camera_info", 1);
    m_pubCloud = nodeHandle.advertise<sensor_msgs::PointCloud2> ("point_cloud", 1);
    m_pubDepth = nodeHandle.advertise<sensor_msgs::Image> ("depth_image", 1);
    m_pubGray = nodeHandle.advertise<sensor_msgs::Image> ("gray_image", 1);

    // Advertise the topics to initialize and update the settings of camera in the UI
    m_pubInit = nodeHandle.advertise<std_msgs::String> ("init_panel", 1);
    m_pubExpoTimeParam = nodeHandle.advertise<std_msgs::String> ("expo_time_param", 1);
    m_pubExpoTimeValue = nodeHandle.advertise<std_msgs::UInt32> ("expo_time_value", 1);
    m_pubFps = nodeHandle.advertise<std_msgs::String> ("update_fps", 1);

    // Subscribe the messages of UI to change the state of the camera
    m_subIsInit = nodeHandle.subscribe ("/is_init", 1, &RoyaleInRos::callbackIsInit, this);
    m_subUseCase = nodeHandle.subscribe ("/use_case", 1, &RoyaleInRos::callbackUseCase, this);
    m_subExpoTime = nodeHandle.subscribe ("/expo_time", 1, &RoyaleInRos::callbackExpoTime, this);
    m_subExpoMode = nodeHandle.subscribe ("/expo_mode", 1, &RoyaleInRos::callbackExpoMode, this);
    m_subMinFilter = nodeHandle.subscribe ("/min_filter", 1, &RoyaleInRos::callbackMinFiler, this);
    m_subMaxFilter = nodeHandle.subscribe ("/max_filter", 1, &RoyaleInRos::callbackMaxFiler, this);
    m_subDivisor = nodeHandle.subscribe ("/divisor", 1, &RoyaleInRos::callbackDivisor, this);
    
    start();
}

void RoyaleInRos::start()
{
    m_fpsProcess =  thread (&RoyaleInRos::fpsUpdate, this);

    // Create a camera manager and query available cameras
    CameraManager manager;
    Vector<String> cameraList (manager.getConnectedCameraList());

    if (cameraList.empty())
    {
        NODELET_ERROR_STREAM ("No suitable cameras found!");
        return;
    }

    // Create the first camera that was found, register a data listener
    // and start the capturing
    m_cameraDevice = manager.createCamera (cameraList[0]);

    String cameraName;
    m_cameraDevice->getCameraName (cameraName);
    NODELET_INFO_STREAM ("Opened camera : " << cameraName.c_str());

    if (m_cameraDevice->initialize() != CameraStatus::SUCCESS)
    {
        NODELET_ERROR_STREAM ("Error initializing the camera!");
        return;
    }

    if (!setCameraInfo())
    {
        NODELET_ERROR_STREAM ("Couldn't create camera info!");
        return;
    }

    if (m_cameraDevice->registerExposureListener (this) != CameraStatus::SUCCESS)
    {
        NODELET_ERROR_STREAM ("Couldn't register exposure listener!");
        return;
    }

    if (m_cameraDevice->registerDataListener (this) != CameraStatus::SUCCESS)
    {
        NODELET_ERROR_STREAM ("Couldn't register data listener!");
        return;
    }

    if (m_cameraDevice->startCapture() != CameraStatus::SUCCESS)
    {
        NODELET_ERROR_STREAM ("Error starting camera capture!");
        return;
    }

    // Record the use cases of camera and the parameters of exposure time
    std::stringstream ss;

    m_cameraDevice->getExposureLimits (m_limits);
    ss << to_string (m_limits.first) << "/" << to_string (m_limits.second);

    // The default exposure mode is manual mode (m_autoExposure is false)
    ss << "/" << 0;
    m_msgExpoTimeParam.data = ss.str();
    
    ss.str("");

    Vector<String> useCases;
    m_cameraDevice->getUseCases (useCases);    
    for (size_t i = 0; i < useCases.size(); ++i)
    {
        uint32_t streamCount;
        if (m_cameraDevice->getNumberOfStreams (useCases[i], streamCount) != CameraStatus::SUCCESS)
        {
            return;
        }
        else
        {
            if (streamCount == 1)
            {
                ss << "/" << useCases[i];
            }
        }
    }
    m_useCases = ss.str();
    ss.clear();

    initMsgUpdate();
}

void RoyaleInRos::stop()
{
    // Close the camera
    if (m_cameraDevice &&
            m_cameraDevice->stopCapture() != CameraStatus::SUCCESS)
    {
        NODELET_ERROR_STREAM ("Error stopping camera capture!");
        return;
    }

    m_fpsProcess.join();
}

void RoyaleInRos::onNewData (const DepthData *data)
{
    // Make sure the UI is reinitialized each time when rviz is started
    if (m_pubFps.getNumSubscribers() == 0)
    {
        m_initPanel = false;
    }

    // Initialize the UI
    if (!m_initPanel)
    {
        m_pubInit.publish (m_msgInitPanel);
    }

    m_frames++;

    // Create a standard header
    std_msgs::Header header;
    header.frame_id = "royale_camera_optical_frame";
    header.seq = 0;
    header.stamp.fromNSec (chrono::duration_cast<chrono::nanoseconds> (data->timeStamp).count());

    // Create camera info message
    sensor_msgs::CameraInfoPtr msgCameraInfo = sensor_msgs::CameraInfoPtr (new sensor_msgs::CameraInfo);

    *msgCameraInfo = m_cameraInfo;
    msgCameraInfo->header = header;
    msgCameraInfo->height = data->height;
    msgCameraInfo->width = data->width;

    // Create point cloud message ...
    sensor_msgs::PointCloud2Ptr msgPointCloud = sensor_msgs::PointCloud2Ptr (new sensor_msgs::PointCloud2);

    // ... where we want to save x,y,z
    msgPointCloud->fields.resize (3);

    msgPointCloud->fields[0].name = "x";
    msgPointCloud->fields[0].offset = 0;
    msgPointCloud->fields[0].datatype = sensor_msgs::PointField::FLOAT32;
    msgPointCloud->fields[0].count = 1;

    msgPointCloud->fields[1].name = "y";
    msgPointCloud->fields[1].offset = static_cast<uint32_t> (sizeof (float));
    msgPointCloud->fields[1].datatype = sensor_msgs::PointField::FLOAT32;
    msgPointCloud->fields[1].count = 1;

    msgPointCloud->fields[2].name = "z";
    msgPointCloud->fields[2].offset = 2u * static_cast<uint32_t> (sizeof (float));
    msgPointCloud->fields[2].datatype = sensor_msgs::PointField::FLOAT32;
    msgPointCloud->fields[2].count = 1;

    msgPointCloud->header = header;
    msgPointCloud->width = data->width;
    msgPointCloud->height = data->height;
    msgPointCloud->is_bigendian = false;
    msgPointCloud->is_dense = false;
    msgPointCloud->point_step = static_cast<uint32_t> (3 * sizeof (float));
    msgPointCloud->row_step = static_cast<uint32_t> (3 * sizeof (float) * data->width);

    // Reserve space for the actual data
    msgPointCloud->data.resize (3 * sizeof (float) * data->points.size());

    // Create a point cloud modifier
    sensor_msgs::PointCloud2Modifier modifier (*msgPointCloud);
    modifier.setPointCloud2FieldsByString (1, "xyz");

    // Create iterators for the three fields in our point cloud
    sensor_msgs::PointCloud2Iterator<float> iterX (*msgPointCloud, "x");
    sensor_msgs::PointCloud2Iterator<float> iterY (*msgPointCloud, "y");
    sensor_msgs::PointCloud2Iterator<float> iterZ (*msgPointCloud, "z");

    // Create Depth Image message
    sensor_msgs::ImagePtr msgDepthImage = sensor_msgs::ImagePtr (new sensor_msgs::Image);

    msgDepthImage->header = header;
    msgDepthImage->width = data->width;
    msgDepthImage->height = data->height;
    msgDepthImage->is_bigendian = false;
    msgDepthImage->encoding = sensor_msgs::image_encodings::TYPE_32FC1;
    msgDepthImage->step = static_cast<uint32_t> (sizeof (float) * data->width);
    msgDepthImage->data.resize (sizeof (float) * data->points.size());

    float *iterDepth = (float *) &msgDepthImage->data[0];

    // Create Depth Image message
    sensor_msgs::ImagePtr msgGrayImage = sensor_msgs::ImagePtr (new sensor_msgs::Image);

    msgGrayImage->header = header;
    msgGrayImage->width = data->width;
    msgGrayImage->height = data->height;
    msgGrayImage->is_bigendian = false;
    msgGrayImage->encoding = sensor_msgs::image_encodings::MONO16;
    msgGrayImage->step = static_cast<uint32_t> (sizeof (uint16_t) * data->width);
    msgGrayImage->data.resize (sizeof (uint16_t) * data->points.size());

    uint16_t *iterGray = (uint16_t *) &msgGrayImage->data[0];
    
    // Iterate over all the points we received in the callback
    for (auto currentPoint : data->points)
    {
        if (currentPoint.depthConfidence > 0 
         && currentPoint.z >= m_minFilter 
         && currentPoint.z <= m_maxFilter)
        {
            *iterX = currentPoint.x;
            *iterY = currentPoint.y;
            *iterZ = currentPoint.z;
            *iterDepth = currentPoint.z;
        }
        else
        {
            // If the confidence is 0 set this point to NaN
            // (according to http://www.ros.org/reps/rep-0117.html)
            *iterX = *iterY = *iterZ = numeric_limits<float>::quiet_NaN();
            *iterDepth = 0.0f;
        }

        // Set divisor of gray image to adjust the brightness
        uint16_t clampedVal = std::min (m_grayDivisor, currentPoint.grayValue);
        int newGrayValue = std::min (254, static_cast<int> (254 * 1.f * (float) clampedVal / (float) m_grayDivisor));

        if (newGrayValue < 0)
        {
            newGrayValue = 0;
        }

        *iterGray = static_cast<uint16_t> (newGrayValue);

        ++iterX;
        ++iterY;
        ++iterZ;
        ++iterDepth;
        ++iterGray;
    }

    // Publish the messages
    m_pubCameraInfo.publish (msgCameraInfo);
    m_pubCloud.publish (msgPointCloud);
    m_pubDepth.publish (msgDepthImage);
    m_pubGray.publish (msgGrayImage);
}

void RoyaleInRos::onNewExposure (const uint32_t newExposureTime)
{
    if (m_exposureTime == newExposureTime)
    {
        return;
    }
    m_exposureTime = newExposureTime;

    // Publish the new exposure time to the UI
    m_msgExpoTimeValue.data = newExposureTime;

    if (m_autoExposure)
    {
        m_pubExpoTimeValue.publish (m_msgExpoTimeValue);
    }
}

bool RoyaleInRos::setCameraInfo()
{
    LensParameters lensParams;
    if ((m_cameraDevice->getLensParameters (lensParams) == CameraStatus::SUCCESS))
    {
        if (lensParams.distortionRadial.size() != 3)
        {
            NODELET_ERROR_STREAM ("Unknown distortion model!");
            return false;
        }
        else
        {
            m_cameraInfo.distortion_model = "plumb_bob";
            m_cameraInfo.D.resize (5);
            m_cameraInfo.D[0] = lensParams.distortionRadial[0];
            m_cameraInfo.D[1] = lensParams.distortionRadial[1];
            m_cameraInfo.D[2] = lensParams.distortionTangential.first;
            m_cameraInfo.D[3] = lensParams.distortionTangential.second;
            m_cameraInfo.D[4] = lensParams.distortionRadial[2];
        }

        m_cameraInfo.K[0] = lensParams.focalLength.first;
        m_cameraInfo.K[1] = 0;
        m_cameraInfo.K[2] = lensParams.principalPoint.first;
        m_cameraInfo.K[3] = 0;
        m_cameraInfo.K[4] = lensParams.focalLength.second;
        m_cameraInfo.K[5] = lensParams.principalPoint.second;
        m_cameraInfo.K[6] = 0;
        m_cameraInfo.K[7] = 0;
        m_cameraInfo.K[8] = 1;

        m_cameraInfo.R[0] = 1;
        m_cameraInfo.R[1] = 0;
        m_cameraInfo.R[2] = 0;
        m_cameraInfo.R[3] = 0;
        m_cameraInfo.R[4] = 1;
        m_cameraInfo.R[5] = 0;
        m_cameraInfo.R[6] = 0;
        m_cameraInfo.R[7] = 0;
        m_cameraInfo.R[8] = 1;

        m_cameraInfo.P[0] = lensParams.focalLength.first;
        m_cameraInfo.P[1] = 0;
        m_cameraInfo.P[2] = lensParams.principalPoint.first;
        m_cameraInfo.P[3] = 0;
        m_cameraInfo.P[4] = 0;
        m_cameraInfo.P[5] = lensParams.focalLength.second;
        m_cameraInfo.P[6] = lensParams.principalPoint.second;
        m_cameraInfo.P[7] = 0;
        m_cameraInfo.P[8] = 0;
        m_cameraInfo.P[9] = 0;
        m_cameraInfo.P[10] = 1;
        m_cameraInfo.P[11] = 0;

        return true;
    }
    else
    {
        NODELET_ERROR_STREAM ("Couldn't get lens parameters!");
        return false;
    }
}

void RoyaleInRos::fpsUpdate()
{
    while (ros::ok())
    { 
        this_thread::sleep_for (chrono::seconds (1));

        // Publish the new fps to the UI
        std_msgs::String msg;
        msg.data = to_string (m_frames);
        m_pubFps.publish (msg);
        
        m_frames = 0;
    }
}

void RoyaleInRos::initMsgUpdate()
{
    std::stringstream ss;
    ss << to_string (m_minFilter) << "/";
    ss << to_string (m_maxFilter) << "/";
    ss << to_string (m_grayDivisor); 
    ss << m_useCases;

    m_msgInitPanel.data = ss.str();
    ss.clear();
}

void RoyaleInRos::callbackIsInit (const std_msgs::Bool::ConstPtr &msg)
{
    m_initPanel = msg->data;
}

void RoyaleInRos::callbackUseCase (const std_msgs::String::ConstPtr &msg)
{
    String currentMode = msg->data.c_str();
    if (m_cameraDevice->setUseCase (currentMode) != CameraStatus::SUCCESS)
    {
        NODELET_ERROR_STREAM ("Couldn't set use case!");
        return;
    }
    else
    {
        // Whenever user case is switched, get the current range and value of exposure time
        // and publish the message to the UI
        m_cameraDevice->getExposureLimits (m_limits);
        std::stringstream ss;
        ss << to_string (m_limits.first) << "/" << to_string (m_limits.second);

        ExposureMode expoMode;
        m_cameraDevice->getExposureMode (expoMode);
        if (expoMode == ExposureMode::AUTOMATIC)
        {
            m_autoExposure = true;
        }
        else
        {
            m_autoExposure = false;
        }
        ss << "/" << to_string (m_autoExposure);
        m_msgExpoTimeParam.data = ss.str();
        ss.clear();

        m_pubExpoTimeParam.publish (m_msgExpoTimeParam);
        m_pubExpoTimeValue.publish (m_msgExpoTimeValue);
    }
}

void RoyaleInRos::callbackExpoTime (const std_msgs::UInt32::ConstPtr &msg)
{
    m_exposureTime = msg->data;

    // If the device is busy we try to set the exposure "tries" times
    int tries = 5;
    CameraStatus ret;
    do
    {
        ret = m_cameraDevice->setExposureTime (m_exposureTime);
        if (ret == CameraStatus::DEVICE_IS_BUSY)
        {
            this_thread::sleep_for (chrono::milliseconds (200));
            tries--;
        }
    }
    while (tries > 0 && ret == CameraStatus::DEVICE_IS_BUSY);

    if (ret != CameraStatus::SUCCESS)
    {
        NODELET_WARN_STREAM ("Couldn't set exposure time!");
        return;
    }
}

void RoyaleInRos::callbackExpoMode (const std_msgs::Bool::ConstPtr &msg)
{
    m_autoExposure = msg->data;
    ExposureMode newMode;
    if (m_autoExposure)
    {
        newMode = ExposureMode::AUTOMATIC;
    }
    else
    {
        newMode = ExposureMode::MANUAL;
    }
    if (m_cameraDevice->setExposureMode (newMode) != CameraStatus::SUCCESS)
    {
        NODELET_ERROR_STREAM ("Couldn't set operation mode!");
        return;
    }
}

void RoyaleInRos::callbackMinFiler (const std_msgs::Float32::ConstPtr &msg)
{
    m_minFilter = msg->data;
    initMsgUpdate();
}

void RoyaleInRos::callbackMaxFiler (const std_msgs::Float32::ConstPtr &msg)
{
    m_maxFilter = msg->data;
    initMsgUpdate();
}

void RoyaleInRos::callbackDivisor (const std_msgs::UInt16::ConstPtr &msg)
{
    m_grayDivisor = msg->data;
    initMsgUpdate();
}

PLUGINLIB_EXPORT_CLASS (RoyaleInRos, nodelet::Nodelet)
