/*********************************************************************************
 * CozmoCommsTranslator
 * 
 * A utility for bidirectionally moving raw bytes between a serial COM port and a TCP socket.
 * Made to be used with a physical robot that uses in a serial port (in lieu of a BTLE radio)
 * to communicate with a basestation. 
 * 
 * Setup:
 * - Connect robot to serial port on Windows PC.
 * - Start Webots with a world containing a robot_advertisement_controller on Mac. 
 * - Start basestation separately or as a controller in the Webots world (on the Mac). 
 * - Start this utility with proper arguments: 
 *      CozmoCommsTranslator robotID serialCOMPort advertisementServiceIP advertisementServicePort 
 *  
 *     robotID:                            Single byte number representing the ID of the robot
 *     serialCOMPort:                  The X in COMX at which the physical robot is attached
 *     advertisementServiceIP:     The IP address of the advertisement service (which is wherever Webots is running)
 *     advertisementServicePort:  The registration port of the advertisement service
 * 
 *********************************************************************************/
