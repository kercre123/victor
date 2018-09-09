  /**
 * File: socketConstants.h
 *
 * Author: Paul Aluri
 * Created: 04/12/18
 *
 * Description: Contants for domain socket paths.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/
  
  #pragma once
  
namespace Anki {
namespace Victor {

  //
  // Local (unix-domain) socket paths.
  // RobotID will be appended to generate unique paths for each robot.
  //

  #ifdef SIMULATOR
  #define LOCAL_SOCKET_PATH                         "/tmp/"
  #else
  #define LOCAL_SOCKET_PATH                         "/dev/socket/"
  #endif

  constexpr char ANIM_ROBOT_SERVER_PATH[]           = LOCAL_SOCKET_PATH "_anim_robot_server_";
  constexpr char ANIM_ROBOT_CLIENT_PATH[]           = LOCAL_SOCKET_PATH "_anim_robot_client_";
  constexpr char ENGINE_ANIM_SERVER_PATH[]          = LOCAL_SOCKET_PATH "_engine_anim_server_";
  constexpr char ENGINE_ANIM_CLIENT_PATH[]          = LOCAL_SOCKET_PATH "_engine_anim_client_";
  constexpr char ENGINE_SWITCH_SERVER_PATH[]        = LOCAL_SOCKET_PATH "_engine_switch_server_";
  constexpr char ENGINE_SWITCH_CLIENT_PATH[]        = LOCAL_SOCKET_PATH "_engine_switch_client_";

  // basenames for per-robot socket servers
  constexpr char MIC_SERVER_BASE_PATH[]             = LOCAL_SOCKET_PATH "mic_sock";
  constexpr char AI_SERVER_BASE_PATH[]              = LOCAL_SOCKET_PATH "ai_sock";

  //
} // Victor
} // Anki
