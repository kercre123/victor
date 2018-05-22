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
  constexpr char LOCAL_SOCKET_PATH[]  = "/tmp/";
  constexpr char ANIM_ROBOT_SERVER_PATH[]    = "/tmp/_anim_robot_server_";
  constexpr char ANIM_ROBOT_CLIENT_PATH[]    = "/tmp/_anim_robot_client_";
  constexpr char ENGINE_ANIM_SERVER_PATH[]   = "/tmp/_engine_anim_server_";
  constexpr char ENGINE_ANIM_CLIENT_PATH[]   = "/tmp/_engine_anim_client_";
  constexpr char ENGINE_SWITCH_SERVER_PATH[] = "/tmp/_engine_switch_server_";
  constexpr char ENGINE_SWITCH_CLIENT_PATH[] = "/tmp/_engine_switch_client_";
  constexpr char ENGINE_GATEWAY_SERVER_PATH[] = "/tmp/_engine_gateway_server_";
  constexpr char ENGINE_GATEWAY_CLIENT_PATH[] = "/tmp/_engine_gateway_client_";
  #else
  constexpr char LOCAL_SOCKET_PATH[]  = "/dev/";
  constexpr char ANIM_ROBOT_SERVER_PATH[]  = "/dev/socket/_anim_robot_server_";
  constexpr char ANIM_ROBOT_CLIENT_PATH[]  = "/dev/socket/_anim_robot_client_";
  constexpr char ENGINE_ANIM_SERVER_PATH[] = "/dev/socket/_engine_anim_server_";
  constexpr char ENGINE_ANIM_CLIENT_PATH[] = "/dev/socket/_engine_anim_client_";
  constexpr char ENGINE_SWITCH_SERVER_PATH[] = "/dev/socket/_engine_switch_server_";
  constexpr char ENGINE_SWITCH_CLIENT_PATH[] = "/dev/socket/_engine_switch_client_";
  constexpr char ENGINE_GATEWAY_SERVER_PATH[] = "/dev/socket/_engine_gateway_server_";
  constexpr char ENGINE_GATEWAY_CLIENT_PATH[] = "/dev/socket/_engine_gateway_client_";
  #endif

} // Victor
} // Anki