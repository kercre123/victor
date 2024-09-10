/*
 * File: streamingWavePortalPlugIn.cpp
 *
 * Author: Jordan Rivas
 * Created: 7/11/2018
 *
 * Description: This wraps the StreamingWavePortalFx plug-in instance to provide Audio Data source for playback. This
 *              is intended to be interacted with at app level.
 *
 * Copyright: Anki, Inc. 2018
 */


#include "audioEngine/plugins/streamingWavePortalPlugIn.h"

#include "audioEngine/audioTools/streamingWaveDataInstance.h"
#include "audioEngine/plugins/wavePortalFxTypes.h"
#include "AK/SoundEngine/Common/AkTypes.h"
#include "AK/SoundEngine/Common/AkSoundEngine.h"
#include "audioEngine/audioDefines.h"
#include "streamingWavePortalFxFactory.h"
#include "streamingWavePortalFx.h"
#include "util/logging/logging.h"


namespace Anki {
namespace AudioEngine {
namespace PlugIns {

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StreamingWavePortalPlugIn::StreamingWavePortalPlugIn()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
StreamingWavePortalPlugIn::~StreamingWavePortalPlugIn()
{
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StreamingWavePortalPlugIn::RegisterPlugIn()
{
  // Add static CreateFx callback function
  StreamingWavePortalFx::PostCreateFxFunc = [this](StreamingWavePortalFx* plugin) { SetupEnginePlugInFx(plugin); };
  return true;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StreamingWavePortalPlugIn::AddDataInstance( const std::shared_ptr<StreamingWaveDataInstance>& dataInstance,
                                                 PluginId_t pluginId )
{
  std::lock_guard<std::mutex> lock(_dataInstanceMutex);
  const auto returnIt = _dataInstanceMap.emplace( pluginId, dataInstance );
  // Return true if added to _dataInstanceMap
  return returnIt.second;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
std::shared_ptr<StreamingWaveDataInstance> StreamingWavePortalPlugIn::GetDataInstance( PluginId_t pluginId ) const
{
  std::lock_guard<std::mutex> lock(_dataInstanceMutex);
  const auto findIt = _dataInstanceMap.find( pluginId );
  if ( findIt != _dataInstanceMap.end() ) {
    return findIt->second;
  }
  return nullptr;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingWavePortalPlugIn::ClearAudioData( PluginId_t pluginId )
{
  std::lock_guard<std::mutex> lock(_dataInstanceMutex);
  const auto it = _dataInstanceMap.find( pluginId );
  if ( it != _dataInstanceMap.end() ) {
    _dataInstanceMap.erase( it );
  }
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StreamingWavePortalPlugIn::HasAudioDataInfo( PluginId_t pluginId ) const
{
  std::lock_guard<std::mutex> lock(_dataInstanceMutex);
  const auto it = _dataInstanceMap.find( pluginId );
  if ( it != _dataInstanceMap.end() ) {
    return it->second->HasAudioData();
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
bool StreamingWavePortalPlugIn::IsPluginActive( PluginId_t pluginId ) const
{
  std::lock_guard<std::mutex> lock(_dataInstanceMutex);
  const auto it = _dataInstanceMap.find( pluginId );
  if ( it != _dataInstanceMap.end() ) {
    return it->second->IsPluginActive();
  }
  return false;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void StreamingWavePortalPlugIn::SetupEnginePlugInFx( StreamingWavePortalFx* plugIn )
{
  plugIn->SetInitCallback( [this] ( StreamingWavePortalFx* pluginInstance )
  {
    // Get data for plugin instance
    const auto pluginID = pluginInstance->GetPluginId();
    {
      std::lock_guard<std::mutex> lock(_dataInstanceMutex);
      const auto findIt = _dataInstanceMap.find( pluginID );
      if ( findIt != _dataInstanceMap.end() ) {
        findIt->second->SetIsPluginActive( true );
        pluginInstance->SetDataInstance( findIt->second );
      } else {
        // Can't find data instance for plugin Id
        LOG_ERROR("StreamingWavePortalPlugin.SetupEnginePlugInFx", "No data instance for pluginID %d", pluginID);
        pluginInstance->SetDataInstance( nullptr );
      }
    }

    if ( _initFunc != nullptr ) {
      _initFunc(this);
    }

  });

  plugIn->SetTerminateCallback( [this] ( StreamingWavePortalFx* pluginInstance )
  {
    // Remove local shared pointer
    {
      std::lock_guard<std::mutex> lock(_dataInstanceMutex);
      const auto findIt = _dataInstanceMap.find( pluginInstance->GetPluginId() );
      if ( findIt != _dataInstanceMap.end() ) {
        findIt->second->SetIsPluginActive( false );
        _dataInstanceMap.erase(findIt);
      }
    }

    if ( _termFunc != nullptr ) {
      _termFunc(this);
    }

  });
}


} // PlugIns
} // AudioEngine
} // Anki
