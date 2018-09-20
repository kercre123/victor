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

  // clients defined in golang
  constexpr char ENGINE_GATEWAY_SERVER_PATH[]       = LOCAL_SOCKET_PATH "_engine_gateway_server_";
  constexpr char ENGINE_GATEWAY_PROTO_SERVER_PATH[] = LOCAL_SOCKET_PATH "_engine_gateway_proto_server_";
  constexpr char SWITCH_GATEWAY_SERVER_PATH[]       = LOCAL_SOCKET_PATH "_switchboard_gateway_server_";

  // basenames for per-robot socket servers
  constexpr char MIC_SERVER_BASE_PATH[]             = LOCAL_SOCKET_PATH "mic_sock";
  constexpr char AI_SERVER_BASE_PATH[]              = LOCAL_SOCKET_PATH "ai_sock";

  // servers defined in golang
  constexpr char TOKEN_SERVER_PATH[]                = LOCAL_SOCKET_PATH "token_server";
  constexpr char TOKEN_SWITCHBOARD_CLIENT_PATH[]    = LOCAL_SOCKET_PATH "_token_switchboard_client_";
  constexpr char JDOCS_SERVER_PATH[]                = LOCAL_SOCKET_PATH "jdocs_server";
  constexpr char JDOCS_ENGINE_CLIENT_PATH[]         = LOCAL_SOCKET_PATH "_jdocs_engine_client_";
  constexpr char LOGCOLLECTOR_CLIENT_PATH[]         = LOCAL_SOCKET_PATH "logcollector_client_";
  constexpr char LOGCOLLECTOR_SERVER_PATH[]         = LOCAL_SOCKET_PATH "logcollector_server";

  //
} // Victor
} // Anki
