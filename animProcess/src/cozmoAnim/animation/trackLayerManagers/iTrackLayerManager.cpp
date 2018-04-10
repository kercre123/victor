/**
 * File: ITrackLayerManager.cpp
 *
 * Authors: Al Chaussee
 * Created: 06/28/2017
 *
 * Description:
 *
 *
 * Copyright: Anki, Inc. 2017
 *
 **/

#include "cozmoAnim/animation/trackLayerManagers/iTrackLayerManager.h"

#define LOG_CHANNEL    "TrackLayerManager"

#define DEBUG_FACE_LAYERING 0

namespace Anki {
namespace Cozmo {

template<class FRAME_TYPE>
ITrackLayerManager<FRAME_TYPE>::ITrackLayerManager(const Util::RandomGenerator& rng)
: _rng(rng)
{
  
}

template<class FRAME_TYPE>
bool ITrackLayerManager<FRAME_TYPE>::ApplyLayersToFrame(FRAME_TYPE& frame,
                                                        ApplyLayerFunc applyLayerFunc)
{
  if (DEBUG_FACE_LAYERING)
  {
    if (!_layers.empty())
    {
      LOG_DEBUG("AnimationStreamer.UpdateFace.ApplyingFaceLayers",
                        "NumLayers=%lu", (unsigned long)_layers.size());
    }
  }

  bool frameUpdated = false;
  
  std::list<std::string> layersToErase;
  
  for (auto layerIter = _layers.begin(); layerIter != _layers.end(); ++layerIter)
  {
    auto& layerName = layerIter->first;
    auto& layer = layerIter->second;
    
    // Apply the layer's track with frame
    frameUpdated |= applyLayerFunc(layer.track, layer.startTime_ms, layer.streamTime_ms, frame);
    
    layer.streamTime_ms += ANIM_TIME_STEP_MS;
    
    if (!layer.track.HasFramesLeft())
    {
      // This layer is done...
      if(layer.isPersistent)
      {
        if (layer.track.IsEmpty())
        {
          LOG_WARNING("AnimationStreamer.UpdateFace.EmptyPersistentLayer",
                      "Persistent face layer is empty - perhaps live frames were "
                      "used? (layer=%s)", layerName.c_str());
          layer.isPersistent = false;
        }
        else
        {
          //...but is marked persistent, so keep applying last frame
          layer.track.MoveToPrevKeyFrame(); // so we're not at end() anymore
          layer.streamTime_ms -= ANIM_TIME_STEP_MS;
          
          if (DEBUG_FACE_LAYERING)
          {
            LOG_DEBUG("AnimationStreamer.UpdateFace.HoldingLayer",
                      "Holding last frame of face layer %s",
                      layerName.c_str());
          }
          
          layer.sentOnce = true; // mark that it has been sent at least once
          
          // We no longer need anything but the last frame (which should now be
          // "current")
          layer.track.ClearUpToCurrent();
        }
      }
      else
      {
        //...and is not persistent, so delete it
        if (DEBUG_FACE_LAYERING)
        {
          LOG_DEBUG("AnimationStreamer.UpdateFace.RemovingFaceLayer",
                    "%s (Layers remaining=%lu)",
                    layerName.c_str(), (unsigned long)_layers.size()-1);
        }
        
        layersToErase.push_back(layerName);
      }
    }
  }
  
  // Actually erase elements from the map
  for (const auto& layerName : layersToErase)
  {
    _layers.erase(layerName);
  }
  
  return frameUpdated;
}

template<class FRAME_TYPE>
Result ITrackLayerManager<FRAME_TYPE>::AddLayer(const std::string& name,
                                                const Animations::Track<FRAME_TYPE>& track,
                                                TimeStamp_t delay_ms)
{
  if (_layers.find(name) != _layers.end()) {
    PRINT_NAMED_WARNING("TrackLayerManager.AddLayer.LayerAlreadyExists", "");
  }
  
  Result lastResult = RESULT_OK;
  
  Layer newLayer;
  newLayer.track = track; // COPY the track in
  newLayer.track.SetIsLive(true);
  newLayer.track.MoveToStart();
  newLayer.startTime_ms = delay_ms;
  newLayer.streamTime_ms = 0;
  newLayer.isPersistent = false;
  newLayer.sentOnce = false;
  
  _layers[name] = std::move(newLayer);
  
  return lastResult;
}

template<class FRAME_TYPE>
void ITrackLayerManager<FRAME_TYPE>::AddPersistentLayer(const std::string& name,
                                                        const Animations::Track<FRAME_TYPE>& track)
{
  if (_layers.find(name) != _layers.end()) {
    PRINT_NAMED_WARNING("TrackLayerManager.AddPersistentLayer.LayerAlreadyExists", "");
  }
  
  Layer newLayer;
  newLayer.track = track;
  newLayer.track.SetIsLive(false); // don't want keyframes to delete as they play
  newLayer.track.MoveToStart();
  newLayer.startTime_ms = 0;
  newLayer.streamTime_ms = 0;
  newLayer.isPersistent = true;
  newLayer.sentOnce = false;
  
  _layers[name] = std::move(newLayer);
}

template<class FRAME_TYPE>
void ITrackLayerManager<FRAME_TYPE>::AddToPersistentLayer(const std::string& layerName, FRAME_TYPE& keyframe)
{
  auto layerIter = _layers.find(layerName);
  if (layerIter != _layers.end())
  {
    auto& track = layerIter->second.track;
    assert(nullptr != track.GetLastKeyFrame());
    
    // Make keyframe trigger one sample length (plus any internal delay) past
    // the last keyframe's trigger time
    keyframe.SetTriggerTime(track.GetLastKeyFrame()->GetTriggerTime() +
                            ANIM_TIME_STEP_MS +
                            keyframe.GetTriggerTime());
    
    track.AddKeyFrameToBack(keyframe);
    layerIter->second.sentOnce = false;
  }
}

template<class FRAME_TYPE>
void ITrackLayerManager<FRAME_TYPE>::RemovePersistentLayer(const std::string& layerName, u32 duration_ms)
{
  auto layerIter = _layers.find(layerName);
  if (layerIter != _layers.end())
  {
    auto& layerName = layerIter->first;
    LOG_INFO("ITrackLayerManager.RemovePersistentLayer",
             "%s, (Layers remaining=%lu)",
             layerName.c_str(), (unsigned long)_layers.size()-1);
    
    
    // Add a layer that takes us back from where this persistent frame leaves
    // off to no adjustment at all.
    Animations::Track<FRAME_TYPE> track;
    track.SetIsLive(true);
    if (duration_ms > 0)
    {
      FRAME_TYPE firstFrame(layerIter->second.track.GetCurrentKeyFrame());
      firstFrame.SetTriggerTime(0);
      track.AddKeyFrameToBack(std::move(firstFrame));
    }
    FRAME_TYPE lastFrame;
    lastFrame.SetTriggerTime(duration_ms);
    track.AddKeyFrameToBack(std::move(lastFrame));
    
    AddLayer("Remove" + layerName, track);
    
    _layers.erase(layerIter);
  }
}

template<class FRAME_TYPE>
bool ITrackLayerManager<FRAME_TYPE>::HaveLayersToSend() const
{
  if (_layers.empty())
  {
    return false;
  }
  else
  {
    // There are layers, but we want to ignore any that are persistent that
    // have already been sent once
    for (const auto & layer : _layers)
    {
      if (!layer.second.isPersistent || !layer.second.sentOnce)
      {
        // There's at least one non-persistent layer, or a persistent layer
        // that has not been sent in its entirety at least once: return that there
        // are still layers to send
        return true;
      }
    }
    // All layers are persistent ones that have been sent, so no need to keep sending them
    // by themselves. They only need to be re-applied while there's something
    // else being sent
    return false;
  }
}

template<class FRAME_TYPE>
bool ITrackLayerManager<FRAME_TYPE>::HasLayer(const std::string& layerName) const
{
  return _layers.find(layerName) != _layers.end();
}

// Explicit instantiation of allowed templated classes
template class ITrackLayerManager<RobotAudioKeyFrame>;        // AudioLayerManager
template class ITrackLayerManager<BackpackLightsKeyFrame>;    // BackpackLayerManager
template class ITrackLayerManager<ProceduralFaceKeyFrame>;    // FaceLayerManager

}
}
