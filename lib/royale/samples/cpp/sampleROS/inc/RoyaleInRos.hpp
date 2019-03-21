/****************************************************************************\
 * Copyright (C) 2017 pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#include <royale.hpp>

#include <thread>

#include <ros/ros.h>
#include <nodelet/nodelet.h>
#include <pluginlib/class_list_macros.h>

#include <sensor_msgs/CameraInfo.h>
#include <std_msgs/String.h>
#include <std_msgs/Bool.h>
#include <std_msgs/UInt16.h>
#include <std_msgs/UInt32.h>
#include <std_msgs/Float32.h>

class RoyaleInRos :
    public nodelet::Nodelet, public royale::IDepthDataListener, public royale::IExposureListener
{
public:
    RoyaleInRos();
    ~RoyaleInRos();

    // Overridden from Nodelet
    void onInit() override;

    // Starting and stopping the camera
    void start();
    void stop();

private:
    // Called by Royale for every new frame
    void onNewData (const royale::DepthData *data) override;

    // Will be called when the newly calculated exposure time deviates from currently set exposure time of the current UseCase
    void onNewExposure (const uint32_t newExposureTime) override;

    // Create cameraInfo, return true if the setting is successful, otherwise false
    bool setCameraInfo();

    // Calculate the FPS
    void fpsUpdate();

    // The required parameters for UI's initialization
    void initMsgUpdate();

    // Callback a bool value to determine whether the UI is initialized 
    void callbackIsInit (const std_msgs::Bool::ConstPtr &msg);

    // Callback the changed settings from UI and set the changed settings
    void callbackUseCase (const std_msgs::String::ConstPtr &msg);
    void callbackExpoTime (const std_msgs::UInt32::ConstPtr &msg);
    void callbackExpoMode (const std_msgs::Bool::ConstPtr &msg);
    void callbackMinFiler (const std_msgs::Float32::ConstPtr &msg);
    void callbackMaxFiler (const std_msgs::Float32::ConstPtr &msg);
    void callbackDivisor (const std_msgs::UInt16::ConstPtr &msg);

    sensor_msgs::CameraInfo m_cameraInfo;
    std_msgs::String        m_msgInitPanel;
    std_msgs::String        m_msgExpoTimeParam;
    std_msgs::UInt32        m_msgExpoTimeValue;

    ros::Publisher  m_pubCameraInfo;
    ros::Publisher  m_pubCloud;
    ros::Publisher  m_pubDepth;
    ros::Publisher  m_pubGray;

    ros::Publisher  m_pubInit;
    ros::Publisher  m_pubExpoTimeParam;
    ros::Publisher  m_pubExpoTimeValue;
    ros::Publisher  m_pubFps;

    ros::Subscriber m_subIsInit;
    ros::Subscriber m_subUseCase;
    ros::Subscriber m_subExpoTime;
    ros::Subscriber m_subExpoMode;
    ros::Subscriber m_subMinFilter;
    ros::Subscriber m_subMaxFilter;
    ros::Subscriber m_subDivisor;

    std::unique_ptr<royale::ICameraDevice> m_cameraDevice;
    royale::Pair<uint32_t, uint32_t>       m_limits;

    std::string     m_useCases;
    std::thread     m_fpsProcess;
    uint64_t        m_frames;
    uint32_t        m_exposureTime;
    uint16_t        m_grayDivisor;
    float           m_minFilter;
    float           m_maxFilter;
    bool            m_initPanel;
    bool            m_autoExposure;
};
