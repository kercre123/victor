sampleROS
=========


Description
-----------

This tool is a interface of Royale in ROS.

It contains the following features:
- provide a panel of rviz as the UI
- support to switch use cases
- support to set exposure time and auto exposure
- display the FPS
- provide filter function to determine the displayed distance range
- publish ROS topics: `camera_info`, `point_cloud`, `depth_image` and `gray_image`

ROS topics:
- `camera_info` : provide camera information
- `point_cloud` : PointCloud2 of ROS with 3 channels (x, y and z of Royale DepthData)
- `depth_image` : TYPE_32FC1 image. It also supports point cloud which is called "DepthCloud" in this topic.
- `gray_image`  : MONO16 image. The brightness of gray image can be adjusted by `gray_divisor`.


Note
----

Treating the depth data as an array of (data->width * data->height) z coordinates will lose the lens calibration.
The 3D depth points are not arranged in a rectilinear projection, and the discarded x and y coordinates from the
depth points account for the optical projection (or optical distortion) of the camera.
This means that you will see a distortion in the `depth_image`, but not in the `point_cloud`.


Dependencies
------------

- Ubuntu Trusty (14.04 LTS), Ubuntu Xenial (16.04 LTS) or higher
- ROS Indigo (for Ubuntu Trusty), ROS Kinetic (for Ubuntu Xenial) or higher
- Royale SDK for LINUX or OSX


Install and run the program
---------------------------

1. Install and configure ROS (e.g. Kinetic). (http://wiki.ros.org/kinetic/Installation/Ubuntu)
  - upper right button in Ubuntu --> `System Settings...` --> `Software & Updates` --> make sure the options 
    (`restricted`, `universe` and `multiverse`) are selected.
  - add source:
	`$ sudo sh -c 'echo "deb http://packages.ros.org/ros/ubuntu $(lsb_release -sc) main" > /etc/apt/sources.list.d/ros-latest.list'`
  - set the key:
	`$ sudo apt-key adv --keyserver hkp://ha.pool.sks-keyservers.net:80 --recv-key 0xB01FA116`
  - make sure the system software is in the latest version:
	`$ sudo apt-get update`
  - install ROS:
	`$ sudo apt-get install ros-kinetic-desktop-full`
  - initialize rosdep:
	`$ sudo rosdep init`
	`$ rosdep update`
  - initialize environment
	`$ echo "source /opt/ros/kinetic/setup.bash" >> ~/.bashrc`
	`$ source ~/.bashrc`
  - install the commonly used plugins, e.g.:
	`$ sudo apt-get install python-rosinstall`

2. Creating a workspace for catkin. (http://wiki.ros.org/catkin/Tutorials/create_a_workspace)
  - create a catkin workspace:
	`$ mkdir -p ~/catkin_ws/src`
  - build the workspace:
	`$ cd ~/catkin_ws/`
	`$ catkin_make`
  - add the directory into ROS_PACKAGE_PATH environment variable:
	`$ echo $ROS_PACKAGE_PATH`
	/home/youruser/catkin_ws/src:/opt/ros/kinetic/share

3. Download the Royale SDK for LINUX and extract it

4. Copy `sampleROS` from `<path_of_extracted_royale_sdk>/libroyale-<version_number>-LINUX-64Bit/samples/cpp/`
   to `~/catkin_ws/src/sampleROS`.

5. Install the udev rules provided by the SDK
	`$ cd <path_of_extracted_royale_sdk>`
	`$ sudo cp libroyale-<version_number>-LINUX-64Bit/driver/udev/10-royale-ubuntu.rules /etc/udev/rules.d/`

6. Build the example
	`$ cd ~/catkin_ws/`
	`$ catkin_make`

7. Connect camera device

8. Source the setup.bash file
	`$ source devel/setup.bash`

9. Run ROS
	`$ roslaunch royale_in_ros camera_driver.launch`

10. Open a new Terminal to start rviz (3D visualization tool for ROS)
    (Note: Sometimes `$ source devel/setup.bash` is also needed to be set in new Terminal before running the rviz
     to ensure that the new panel of rviz below can be added smoothly.)
	`$ rosrun rviz rviz`

11. Add the topics in rviz
  - point_cloud topic:
	a. Set the `Fixed frame` to `royale_camera_link`;
	b. Add `PointCloud2` from `By display type`, set `Topic` to `royale_camera_driver/point_cloud` and set `Channel Name` to `z`;
	   Or add `PointCloud2` from `By topic` and set `Channel Name` to `z`.
  - image topics:
	a. Add `Image` from `By display type` and set `Image Topic`;
	   Or add topics from `By topic`;
	b. The both image topics support for sub-topics `Camera` and `DepthCloud` too. 
	   Before adding these two sub-topics, the `Fixed frame` has to been set earlier.

12. Add the panel (UI) to control the camera in rviz:
  - The menu of rviz --> `Panels` --> `Add New Panel` --> `royale_in_ros` --> `RoyaleControl` --> `OK`. 
  - The precise value of slider can be entered directly in the text editor and confirmed with the Enter key.
  - In auto exposure mode, the exposure time can not be manually changed.

