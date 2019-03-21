/****************************************************************************\
 * Copyright (C) 2017 pmdtechnologies ag
 *
 * THIS CODE AND INFORMATION ARE PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
 * PARTICULAR PURPOSE.
 *
 \****************************************************************************/

 #include <stdio.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QString>

#include <std_msgs/Bool.h>
#include <std_msgs/UInt16.h>
#include <std_msgs/Float32.h>

#include "RoyaleControl.hpp"

namespace royale_in_ros
{
    RoyaleControl::RoyaleControl (QWidget *parent) : 
        rviz::Panel (parent),
        m_isInit (false),
        m_isAutomatic (false)
    {
        // Set UI
        QVBoxLayout *mainLayout = new QVBoxLayout;

        // Use Case
        mainLayout->addWidget (new QLabel ("Use Case:"));
        m_comboBoxUseCases = new QComboBox;
        mainLayout->addWidget (m_comboBoxUseCases);

        // Exposure Time
        m_labelExpoTime = new QLabel;
        mainLayout->addWidget (m_labelExpoTime);
        QHBoxLayout *exTimeLayout = new QHBoxLayout;

        m_sliderExpoTime = new QSlider (Qt::Horizontal);
        m_sliderExpoTime->setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        exTimeLayout->addWidget (m_sliderExpoTime);

        m_lineEditExpoTime = new QLineEdit;
        m_lineEditExpoTime->setSizePolicy (QSizePolicy::Maximum, QSizePolicy::Fixed);
        exTimeLayout->addWidget (m_lineEditExpoTime);

        mainLayout->addLayout (exTimeLayout);

        // Auto Exposure
        mainLayout->addWidget (new QLabel ("Auto Exposure:"));
        m_checkBoxAutoExpo = new QCheckBox;
        mainLayout->addWidget (m_checkBoxAutoExpo);

        m_labelEditFps = new QLabel;
        mainLayout->addWidget (m_labelEditFps);

        // Filter Minimum
        QString minF= QString::number ((float) MIN_FILTER / 100.0f, 10, 2);
        QString maxF= QString::number ((float) MAX_FILTER / 100.0f, 10, 2);

        mainLayout->addWidget (new QLabel ("Filter Minimum (" + minF + " - " + maxF + "m):"));
        QHBoxLayout *minFilterLayout = new QHBoxLayout;

        m_sliderMinFilter = new QSlider (Qt::Horizontal);
        m_sliderMinFilter->setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        m_sliderMinFilter->setRange (MIN_FILTER, MAX_FILTER);
        minFilterLayout->addWidget (m_sliderMinFilter);

        m_lineEditMinFilter = new QLineEdit;
        m_lineEditMinFilter->setSizePolicy (QSizePolicy::Maximum, QSizePolicy::Fixed);
        minFilterLayout->addWidget (m_lineEditMinFilter);

        mainLayout->addLayout (minFilterLayout);
        
        // Filter Maximum
        mainLayout->addWidget (new QLabel ("Filter Maximum (" + minF + " - " + maxF + "m):"));
        QHBoxLayout *maxFilterLayout = new QHBoxLayout;

        m_sliderMaxFilter = new QSlider (Qt::Horizontal);
        m_sliderMaxFilter->setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        m_sliderMaxFilter->setRange (MIN_FILTER, MAX_FILTER);
        maxFilterLayout->addWidget (m_sliderMaxFilter);

        m_lineEditMaxFilter = new QLineEdit;
        m_lineEditMaxFilter->setSizePolicy (QSizePolicy::Maximum, QSizePolicy::Fixed);
        maxFilterLayout->addWidget (m_lineEditMaxFilter);

        mainLayout->addLayout (maxFilterLayout);

        // Gray Divisor
        QString minD= QString::number (MIN_DIVISOR);
        QString maxD= QString::number (MAX_DIVISOR);

        mainLayout->addWidget (new QLabel ("Gray Divisor (" + minD + " - " + maxD + "):"));
        QHBoxLayout *divisorLayout = new QHBoxLayout;

        m_sliderDivisor = new QSlider (Qt::Horizontal);
        m_sliderDivisor->setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
        m_sliderDivisor->setRange (MIN_DIVISOR, MAX_DIVISOR);
        divisorLayout->addWidget (m_sliderDivisor);

        m_lineEditDivisor = new QLineEdit;
        m_lineEditDivisor->setSizePolicy (QSizePolicy::Maximum, QSizePolicy::Fixed);
        divisorLayout->addWidget (m_lineEditDivisor);

        mainLayout->addLayout (divisorLayout);

        setLayout (mainLayout);
 
        // Advertise the topics to publish the message of the changed settings in the UI
        m_pubIsInit = m_nh.advertise<std_msgs::Bool> ("is_init", 1);
        m_pubUseCase = m_nh.advertise<std_msgs::String> ("use_case", 1);
        m_pubExpoTime = m_nh.advertise<std_msgs::UInt32> ("expo_time", 1);
        m_pubExpoMode = m_nh.advertise<std_msgs::Bool> ("expo_mode", 1);
        m_pubMinFilter = m_nh.advertise<std_msgs::Float32> ("min_filter", 1);
        m_pubMaxFilter = m_nh.advertise<std_msgs::Float32> ("max_filter", 1);
        m_pubDivisor = m_nh.advertise<std_msgs::UInt16> ("divisor", 1);

        // Subscribe the messages to show the changed state of the camera
        m_subInit = m_nh.subscribe ("royale_camera_driver/init_panel", 1, &RoyaleControl::callbackInit, this);
        m_subExpoTimeParam = m_nh.subscribe ("royale_camera_driver/expo_time_param", 1, &RoyaleControl::callbackExpoTimeParam, this);
        m_subExpoTimeValue =  m_nh.subscribe ("royale_camera_driver/expo_time_value", 1, &RoyaleControl::callbackExpoTimeValue, this);
        m_subFps = m_nh.subscribe ("royale_camera_driver/update_fps", 1, &RoyaleControl::callbackFps, this);

        connect (m_comboBoxUseCases, SIGNAL (currentTextChanged (const QString)), this, SLOT (setUseCase (const QString)));
        connect (m_sliderExpoTime, SIGNAL (valueChanged (int)), this, SLOT (setExposureTime  (int))); 
        connect (m_checkBoxAutoExpo, SIGNAL (toggled (bool)), this, SLOT (setExposureMode (bool)));
        connect (m_sliderMinFilter, SIGNAL (valueChanged (int)), this, SLOT (setMinFilter (int)));
        connect (m_sliderMaxFilter, SIGNAL (valueChanged (int)), this, SLOT (setMaxFilter (int)));
        connect (m_sliderDivisor, SIGNAL (valueChanged (int)), this, SLOT (setDivisor (int)));
        
        connect (m_lineEditExpoTime, SIGNAL (editingFinished()), this, SLOT (preciseExposureTimeSetting()));
        connect (m_lineEditMinFilter, SIGNAL (editingFinished()), this, SLOT (preciseMinFilterSetting()));
        connect (m_lineEditMaxFilter, SIGNAL (editingFinished()), this, SLOT (preciseMaxFilterSetting()));
        connect (m_lineEditDivisor, SIGNAL (editingFinished()), this, SLOT (preciseDivisorSetting()));
    }

    void RoyaleControl::callbackInit (const std_msgs::String::ConstPtr &msg)
    {
        // Will be called (only once) whenever the UI is started
        if (!m_isInit)
        {
            QString str = msg->data.c_str();
            QStringList initList = str.split ('/');

            float min = initList.at (0).toFloat();
            m_sliderMinFilter->blockSignals (true);
            m_sliderMinFilter->setValue ((int) (min * 100));
            m_sliderMinFilter->blockSignals (false);
            m_lineEditMinFilter->setText (QString::number (min, 10, 2));
            
            float max = initList.at (1).toFloat();
            m_sliderMaxFilter->blockSignals (true);
            m_sliderMaxFilter->setValue ((int) (max * 100));
            m_sliderMaxFilter->blockSignals (false);
            m_lineEditMaxFilter->setText (QString::number (max, 10, 2));

            m_sliderDivisor->blockSignals (true);
            m_sliderDivisor->setValue (initList.at (2).toInt());
            m_sliderDivisor->blockSignals (false);
            m_lineEditDivisor->setText (initList.at (2));

            for (int i = 3; i < initList.size(); ++i)
            {
                m_comboBoxUseCases->addItem (initList.at (i));
            }

            // Publish the message to indicate that the initialization is complete
            m_isInit = true;
            std_msgs::Bool msg;
            msg.data = m_isInit;
            m_pubIsInit.publish (msg);
        }
    }

    void RoyaleControl::callbackExpoTimeParam (const std_msgs::String::ConstPtr &msg)
    {
        QString str = msg->data.c_str();
        m_minETSlider = str.section ('/', 0, 0).toInt();
        m_maxETSlider = str.section ('/', 1, 1).toInt();
        m_sliderExpoTime->blockSignals (true);
        m_sliderExpoTime->setRange (m_minETSlider, m_maxETSlider);
        m_sliderExpoTime->blockSignals (false);

        m_labelExpoTime->setText ("Exposure Time (" + QString::number (m_minETSlider) + " - " + QString::number (m_maxETSlider) + "):");

        m_isAutomatic = str.section ('/', 2, 2).toInt();
        m_checkBoxAutoExpo->blockSignals (true);
        m_checkBoxAutoExpo->setChecked (m_isAutomatic);
        m_checkBoxAutoExpo->blockSignals (false);
        m_lineEditExpoTime->setEnabled (!m_isAutomatic);
    }

    void RoyaleControl::callbackExpoTimeValue (const std_msgs::UInt32::ConstPtr &msg)
    {
        int value = static_cast<int> (msg->data);
        m_sliderExpoTime->blockSignals (true);
        m_sliderExpoTime->setValue (value);
        m_sliderExpoTime->blockSignals (false);
        
        m_exposureTime = value;
        m_lineEditExpoTime->setText (QString::number (m_exposureTime));
    }

    void RoyaleControl::callbackFps (const std_msgs::String::ConstPtr &msg)
    {
        QString fps = msg->data.c_str();
        m_labelEditFps->setText ("Frames per Second: " + fps);
    }

    void RoyaleControl::setUseCase (const QString &currentMode)
    {
        std_msgs::String msg;
        msg.data = currentMode.toStdString();
        m_pubUseCase.publish (msg);
    }

    void RoyaleControl::setExposureTime (int value)
    {
        if (m_isAutomatic)
        {
            // The exposure time can not be manually changed in the auto exposure mode
            m_sliderExpoTime->blockSignals (true);
            m_sliderExpoTime->setValue (m_exposureTime);
            m_sliderExpoTime->blockSignals (false);
        }
        else
        {
            m_sliderExpoTime->blockSignals (true);
            m_sliderExpoTime->setValue (value);
            
            // Publish the message to change the exposure time
            std_msgs::UInt32 msg;
            msg.data = static_cast<uint32_t> (value);
            m_pubExpoTime.publish (msg);
            

            m_exposureTime = value;
            m_lineEditExpoTime->setText (QString::number (m_exposureTime));
   
            m_sliderExpoTime->blockSignals (false);
        }
    }

    void RoyaleControl::setExposureMode (bool isAutomatic)
    {
        // Publish the message to change the exposure mode
        m_isAutomatic = isAutomatic;
        std_msgs::Bool msg;
        msg.data = isAutomatic;
        m_pubExpoMode.publish (msg);

        // The exposure time can not be manually changed in the auto exposure mode
        m_lineEditExpoTime->setEnabled (!isAutomatic);
    }

    void RoyaleControl::setMinFilter (int value)
    {
        // The minimum value should not be more than the maximum value
        int maxMinFilter = MAX_FILTER - 1;
        if (value <= maxMinFilter)
        {
            m_sliderMinFilter->blockSignals (true);
            m_sliderMinFilter->setValue (value);

            // Publish the message to change the min filter
            float min = (float) value / 100.0f;
            std_msgs::Float32 msg;
            msg.data = min;
            m_pubMinFilter.publish (msg);

            m_lineEditMinFilter->setText (QString::number (min, 10, 2));
    
            if (m_sliderMaxFilter->value() < value)
            {
                setMaxFilter (value + 1);
            }
            m_sliderMinFilter->blockSignals (false);
        }
        else
        {
            setMinFilter (maxMinFilter);
        }
    }

    void RoyaleControl::setMaxFilter (int value)
    {
        // The maximum value should not be less than the minimum value
        int minMaxFilter = MIN_FILTER + 1;
        if (value >= minMaxFilter)
        {
            m_sliderMaxFilter->blockSignals (true);
            m_sliderMaxFilter->setValue (value);

            // Publish the message to change the max filter
            float max = (float) value / 100.0f;
            std_msgs::Float32 msg;
            msg.data = max;
            m_pubMaxFilter.publish (msg);

            m_lineEditMaxFilter->setText (QString::number (max, 10, 2));

            if (m_sliderMinFilter->value() > value)
            {
                setMinFilter (value - 1);
            }
            m_sliderMaxFilter->blockSignals (false);
        }
        else
        {
            setMaxFilter (minMaxFilter);
        }
    }

    void RoyaleControl::setDivisor (int value)
    {
        m_sliderDivisor->blockSignals (true);
        m_sliderDivisor->setValue (value);

        // Publish the message to change the gray divisor
        std_msgs::UInt16 msg;
        msg.data = static_cast<uint16_t> (value);
        m_pubDivisor.publish (msg);

        m_lineEditDivisor->setText (QString::number (value));
        m_sliderDivisor->blockSignals (false);
    }

    void RoyaleControl::preciseExposureTimeSetting()
    {
        int value = m_lineEditExpoTime->text().toInt();
        // The input is confirmed by the Enter key, otherwise it has not changed
        // The input which is out of range input is not allowed
        if (value >= m_minETSlider && value <= m_maxETSlider && m_lineEditExpoTime->hasFocus())
        {
            setExposureTime (value);
        }
        else
        {
            m_lineEditExpoTime->setText (QString::number (m_exposureTime));
        }
    }

    void RoyaleControl::preciseMinFilterSetting()
    {
        int value = (int) (m_lineEditMinFilter->text().toFloat() * 100);
        // The input is confirmed by the Enter key, otherwise it has not changed
        // The input which is out of range input is not allowed
        if (value >= MIN_FILTER && m_lineEditMinFilter->hasFocus())
        {
            setMinFilter (value);
        }
        else
        {
            float min = (float) m_sliderMinFilter->value() / 100.0f;
            m_lineEditMinFilter->setText (QString::number (min, 10, 2));
        }
    }

    void RoyaleControl::preciseMaxFilterSetting()
    {
        int value = (int) (m_lineEditMaxFilter->text().toFloat() * 100);
        // The input is confirmed by the Enter key, otherwise it has not changed
        // The input which is out of range input is not allowed
        if (value <= MAX_FILTER && m_lineEditMaxFilter->hasFocus())
        {
            setMaxFilter (value);
        }
        else
        {
            float max = (float) m_sliderMaxFilter->value() / 100.0f;
            m_lineEditMaxFilter->setText (QString::number (max, 10, 2));
        }
    }

    void RoyaleControl::preciseDivisorSetting()
    {
        int value = m_lineEditDivisor->text().toInt();
        // The input is confirmed by the Enter key, otherwise it has not changed
        // The input which is out of range input is not allowed
        if (value >= MIN_DIVISOR && value <= MAX_DIVISOR && m_lineEditDivisor->hasFocus())
        {
            setDivisor (value);
        }
        else
        {
            m_lineEditDivisor->setText (QString::number (m_sliderDivisor->value()));
        }
    }
}

PLUGINLIB_EXPORT_CLASS (royale_in_ros::RoyaleControl, rviz::Panel)
