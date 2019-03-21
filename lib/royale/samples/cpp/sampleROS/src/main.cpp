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
#include <nodelet/loader.h>
#include <RoyaleInRos.hpp>

int main (int argc, char **argv)
{
    ros::init (argc, argv, "royale_camera");

    nodelet::Loader nodelet;
    nodelet::M_string remap (ros::names::getRemappings());
    nodelet::V_string nargv;
    std::string nodelet_name = ros::this_node::getName();
    nodelet.load (nodelet_name, "royale_in_ros/royale_nodelet", remap, nargv);

    ros::spin();
    return 0;
}
