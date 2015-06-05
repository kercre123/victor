//
//  ios-binding.h
//  CozmoGame
//
//  Created by Greg Nagel on 2/3/15.
//
//

#include "ios-binding.h"
#include "../csharp-binding.h"

#include "anki/cozmo/game/cozmoGame.h"
#include "anki/cozmo/basestation/utils/parsingConstants/parsingConstants.h"
#include "anki/cozmo/shared/cozmoConfig.h"

#include "anki/common/basestation/jsonTools.h"
#include "anki/util/logging/logging.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace Anki;
using namespace Anki::Cozmo;
using namespace Anki::Cozmo::CSharpBinding;

CozmoGame* game = nullptr;

const char* ROBOT_ADVERTISING_HOST_IP = "127.0.0.1";
const char* VIZ_HOST_IP = "127.0.0.1";

void configure_game(Json::Value config)
{
  if(!config.isMember(AnkiUtil::kP_ADVERTISING_HOST_IP)) {
    config[AnkiUtil::kP_ADVERTISING_HOST_IP] = ROBOT_ADVERTISING_HOST_IP;
  }
  if(!config.isMember(AnkiUtil::kP_VIZ_HOST_IP)) {
    config[AnkiUtil::kP_VIZ_HOST_IP] = VIZ_HOST_IP;
  }
  if(!config.isMember(AnkiUtil::kP_ROBOT_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_ROBOT_ADVERTISING_PORT] = ROBOT_ADVERTISING_PORT;
  }
  if(!config.isMember(AnkiUtil::kP_UI_ADVERTISING_PORT)) {
    config[AnkiUtil::kP_UI_ADVERTISING_PORT] = UI_ADVERTISING_PORT;
  }
  if(!config.isMember(AnkiUtil::kP_AS_HOST)) {
    config[AnkiUtil::kP_AS_HOST] = true;
  }
  
  // Get engine playback mode mode
  CozmoGame::PlaybackMode playbackMode = CozmoGame::LIVE_SESSION_NO_RECORD;
  int pmInt;
  if(JsonTools::GetValueOptional(config, AnkiUtil::kP_ENGINE_PLAYBACK_MODE, pmInt)) {
    playbackMode = (CozmoGame::PlaybackMode)pmInt;
    assert(playbackMode <= CozmoGame::PLAYBACK_SESSION);
  }
  
  if (playbackMode != CozmoGame::PLAYBACK_SESSION) {
    
    // Wait for at least one robot and UI device to connect
    config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 1;
    config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 1;
    
  } else {
    
    config[AnkiUtil::kP_NUM_ROBOTS_TO_WAIT_FOR] = 0;
    config[AnkiUtil::kP_NUM_UI_DEVICES_TO_WAIT_FOR] = 0;
    
  } // if (bm != BM_PLAYBACK_SESSION)
}

int Anki::Cozmo::CSharpBinding::cozmo_game_create(const char* configuration_data)
{
    using namespace Cozmo;
  
    if (game != nullptr) {
        PRINT_STREAM_ERROR("Anki.Cozmo.CSharpBinding.cozmo_game_create", "Game already initialized.");
        return (int)RESULT_FAIL;
    }
    
    if (configuration_data == nullptr) {
        PRINT_STREAM_ERROR("Anki.Cozmo.CSharpBinding.cozmo_game_create", "Null pointer for configuration_data.");
        return (int)RESULT_FAIL_INVALID_PARAMETER;
    }
    
    Json::Reader reader;
    Json::Value config;
    if (!reader.parse(configuration_data, configuration_data + std::strlen(configuration_data), config)) {
        PRINT_STREAM_ERROR("Anki.Cozmo.CSharpBinding.cozmo_game_create", "json configuration parsing error: " << reader.getFormattedErrorMessages());
        return (int)RESULT_FAIL;
    }
  
    configure_game(config);
  
    CozmoGame* created_game = new CozmoGame();
  
    Result result = created_game->Init(config);
    if (result != RESULT_OK) {
      delete created_game;
      return (int)result;
    }
    
    game = created_game;
  
    return RESULT_OK;
}

int Anki::Cozmo::CSharpBinding::cozmo_game_destroy()
{
    if (game != nullptr) {
        delete game;
        game = nullptr;
    }
    return (int)RESULT_OK;
}

int Anki::Cozmo::CSharpBinding::cozmo_game_update(float current_time)
{
    if (game == nullptr) {
        PRINT_STREAM_ERROR("Anki.Cozmo.CSharpBinding.cozmo_game_update", "Game not initialized.");
        return (int)RESULT_FAIL;
    }
    game->Update(current_time);
    return RESULT_OK;
}
