/****************************************************************************\
 * Copyright (C) 2017 pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

#include <ros/ros.h>
#include <rviz/panel.h>
#include <pluginlib/class_list_macros.h>

#include <std_msgs/String.h>
#include <std_msgs/UInt32.h>

#include <QLineEdit>
#include <QComboBox>
#include <QSlider>
#include <QLabel>
#include <QCheckBox>

#define MIN_FILTER 0
#define MAX_FILTER 750
#define MIN_DIVISOR 1
#define MAX_DIVISOR 2000

namespace royale_in_ros
{
    class RoyaleControl: public rviz::Panel
    {
        Q_OBJECT

    public:
        RoyaleControl (QWidget *parent = 0);

    private Q_SLOTS:
        void setUseCase (const QString &currentMode);
        void setExposureTime (int value);
        void setExposureMode (bool isAutomatic);
        void setMinFilter (int value);
        void setMaxFilter (int value);
        void setDivisor (int value);

        // The precise value can be entered directly via the text editor.
        void preciseExposureTimeSetting();
        void preciseMinFilterSetting();
        void preciseMaxFilterSetting();
        void preciseDivisorSetting();
        
    private:
        // Callback the camera initial settings after the UI is started
        void callbackInit (const std_msgs::String::ConstPtr &msg);

        // Callback the current parameters of exposure time
        // when the UI is started or user case is switched
        void callbackExpoTimeParam (const std_msgs::String::ConstPtr &msg);
        void callbackExpoTimeValue (const std_msgs::UInt32::ConstPtr &msg);

        // Callback the fps and display it
        void callbackFps (const std_msgs::String::ConstPtr &msg);

        QComboBox  *m_comboBoxUseCases;
        QLabel     *m_labelExpoTime;
        QSlider    *m_sliderExpoTime;
        QLineEdit  *m_lineEditExpoTime;
        QCheckBox  *m_checkBoxAutoExpo;
        QLabel     *m_labelEditFps;
        QSlider    *m_sliderDivisor;
        QLineEdit  *m_lineEditDivisor;
        QSlider    *m_sliderMinFilter;
        QLineEdit  *m_lineEditMinFilter;
        QSlider    *m_sliderMaxFilter;
        QLineEdit  *m_lineEditMaxFilter;

        ros::NodeHandle m_nh;
        ros::Publisher  m_pubIsInit;
        ros::Publisher  m_pubUseCase;
        ros::Publisher  m_pubExpoTime;
        ros::Publisher  m_pubExpoMode;
        ros::Publisher  m_pubMinFilter;
        ros::Publisher  m_pubMaxFilter;
        ros::Publisher  m_pubDivisor;
    
        ros::Subscriber m_subInit;
        ros::Subscriber m_subExpoTimeParam;
        ros::Subscriber m_subExpoTimeValue;
        ros::Subscriber m_subFps;

        bool  m_isInit;
        bool  m_isAutomatic;
        int   m_exposureTime;
        int   m_minETSlider;
        int   m_maxETSlider;
    };
}
