/*
 * File:  cozmo_physics.h
 * Date:  09-25-2013        
 * Description: Webots physics plugin for drawing paths in the cozmo simulator. 
 *              Paths currently consist only of arcs and straights.
 *              This plugin receives messages from the CozmoBot class
 *              via an Emitter. For simplicity, a message is just an array of floats.
 *              The size of the array is determined by the message type.
 *              The message format is defined in cozmo_physics.h.
 *  
 * Author: Kevin Yoon       
 * Modifications: 
 */
#ifndef COZMO_PHYSICS_H
#define COZMO_PHYSICS_H



// Comm protocol
// Msg comes in the form of an array of floats.
// The first element of the array is the message type.
// 
// If you add a new message type, remember to create the corresponding params enum below.
enum MsgType {
  // Start high so that it doesn't collide with cozmoMsgProtocol.
  // This plugin listens on channel 0 so it might receive unintended messages by emitters
  // talking on CHANNEL_BROADCAST.
  PLUGIN_MSG_ERASE_PATH = 1000   
  ,PLUGIN_MSG_APPEND_LINE
  ,PLUGIN_MSG_APPEND_ARC
  ,PLUGIN_MSG_SHOW_PATH
  ,PLUGIN_MSG_SET_HEIGHT_OFFSET
};

// Common for all messages
enum CommonMsgParams {
  PLUGIN_MSG_ROBOT_ID = 1
  ,PLUGIN_MSG_PATH_ID
  ,LAST_PLUGIN_MSG_COMMON_PARAM
};



/////// Message-specific params ////////
// If you add a new message, create the corresponding enum here

enum ErasePathParams {
  ERASE_PATH_MSG_SIZE = LAST_PLUGIN_MSG_COMMON_PARAM
};

enum AppendLineParams {
  LINE_START_X = LAST_PLUGIN_MSG_COMMON_PARAM
  ,LINE_START_Y
  ,LINE_END_X
  ,LINE_END_Y
  ,LINE_MSG_SIZE
};

enum AppendArcParams {
  ARC_CENTER_X = LAST_PLUGIN_MSG_COMMON_PARAM
  ,ARC_CENTER_Y
  ,ARC_RADIUS
  ,ARC_START_RAD
  ,ARC_END_RAD
  ,ARC_MSG_SIZE
};

enum ShowPathParams {
  SHOW_PATH = LAST_PLUGIN_MSG_COMMON_PARAM
  ,SHOW_PATH_MSG_SIZE
};


enum SetHeightOffsetParams {
  HEIGHT_OFFSET = LAST_PLUGIN_MSG_COMMON_PARAM
  ,SET_HEIGHT_OFFSET_MSG_SIZE
};


#endif