/**
 * File: audioLayerManager.cpp
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

#include "engine/animations/trackLayerManagers/iTrackLayerManager.h"

#define DEBUG_FACE_LAYERING 0

namespace Anki {
namespace Cozmo {

template<class FRAME_TYPE>
ITrackLayerManager<FRAME_TYPE>::ITrackLayerManager(const Util::RandomGenerator& rng)
: _rng(rng)
{
  
}

template<class FRAME_TYPE>
void ITrackLayerManager<FRAME_TYPE>::IncrementLayerTagCtr()
{
  // Increment the tag counter and keep it from being the "special"
  // value used to indicate "not animating" or any existing
  // layer tag in use
  ++_layerTagCtr;
  while(_layerTagCtr == NotAnimatingTag ||
        _layers.find(_layerTagCtr) != _layers.end())
  {
    ++_layerTagCtr;
  }
}

template<class FRAME_TYPE>
bool ITrackLayerManager<FRAME_TYPE>::ApplyLayersToFrame(FRAME_TYPE& frame,
                                                        ApplyLayerFunc applyLayerFunc)
{
  if(DEBUG_FACE_LAYERING)
  {
    if(!_layers.empty())
    {
      PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.ApplyingFaceLayers",
                        "NumLayers=%lu", (unsigned long)_layers.size());
    }
  }

  bool frameUpdated = false;
  
  std::list<AnimationTag> tagsToErase;
  
  for(auto layerIter = _layers.begin(); layerIter != _layers.end(); ++layerIter)
  {
    auto& layer = layerIter->second;
    
    // Apply the layer's track with frame
    frameUpdated |= applyLayerFunc(layer.track, layer.startTime_ms, layer.streamTime_ms, frame);
    
    layer.streamTime_ms += RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
    
    if(!layer.track.HasFramesLeft())
    {
      // This layer is done...
      if(layer.isPersistent)
      {
        if(layer.track.IsEmpty())
        {
          PRINT_NAMED_WARNING("AnimationStreamer.UpdateFace.EmptyPersistentLayer",
                              "Persistent face layer is empty - perhaps live frames were "
                              "used? (tag=%d)", layer.tag);
          layer.isPersistent = false;
        }
        else
        {
          //...but is marked persistent, so keep applying last frame
          layer.track.MoveToPrevKeyFrame(); // so we're not at end() anymore
          layer.streamTime_ms -= RobotAudioKeyFrame::SAMPLE_LENGTH_MS;
          
          if(DEBUG_FACE_LAYERING)
          {
            PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.HoldingLayer",
                              "Holding last frame of face layer %s with tag %d",
                              layer.name.c_str(), layer.tag);
          }
          
          // We no longer need anything but the last frame (which should now be
          // "current")
          layer.track.ClearUpToCurrent();
        }
      }
      else
      {
        //...and is not persistent, so delete it
        if(DEBUG_FACE_LAYERING)
        {
          PRINT_NAMED_DEBUG("AnimationStreamer.UpdateFace.RemovingFaceLayer",
                            "%s, Tag = %d (Layers remaining=%lu)",
                            layer.name.c_str(), layer.tag, (unsigned long)_layers.size()-1);
        }
        
        tagsToErase.push_back(layerIter->first);
      }
    }
  }
  
  // Actually erase elements from the map
  for(auto tag : tagsToErase)
  {
    _layers.erase(tag);
  }
  
  return frameUpdated;
}

template<class FRAME_TYPE>
Result ITrackLayerManager<FRAME_TYPE>::AddLayer(const std::string& name,
                                                const Animations::Track<FRAME_TYPE>& track,
                                                TimeStamp_t delay_ms)
{
  Result lastResult = RESULT_OK;
  
  IncrementLayerTagCtr();
  
  Layer newLayer;
  newLayer.tag = _layerTagCtr;
  newLayer.track = track; // COPY the track in
  newLayer.track.SetIsLive(true);
  newLayer.track.Init();
  newLayer.startTime_ms = delay_ms;
  newLayer.streamTime_ms = 0;
  newLayer.isPersistent = false;
  newLayer.name = name;
  
  _layers[_layerTagCtr] = std::move(newLayer);
  
  return lastResult;
}

template<class FRAME_TYPE>
AnimationTag ITrackLayerManager<FRAME_TYPE>::AddPersistentLayer(const std::string& name,
                                                                const Animations::Track<FRAME_TYPE>& track)
{
  IncrementLayerTagCtr();
  
  Layer newLayer;
  newLayer.tag = _layerTagCtr;
  newLayer.track = track;
  newLayer.track.SetIsLive(false); // don't want keyframes to delete as they play
  newLayer.track.Init();
  newLayer.startTime_ms = 0;
  newLayer.streamTime_ms = 0;
  newLayer.isPersistent = true;
  newLayer.name = name;
  
  _layers[_layerTagCtr] = std::move(newLayer);
  
  return _layerTagCtr;
}

template<class FRAME_TYPE>
void ITrackLayerManager<FRAME_TYPE>::AddToPersistentLayer(AnimationTag tag, FRAME_TYPE& keyframe)
{
  auto layerIter = _layers.find(tag);
  if(layerIter != _layers.end())
  {
    auto& track = layerIter->second.track;
    assert(nullptr != track.GetLastKeyFrame());
    
    // Make keyframe trigger one sample length (plus any internal delay) past
    // the last keyframe's trigger time
    keyframe.SetTriggerTime(track.GetLastKeyFrame()->GetTriggerTime() +
                            IKeyFrame::SAMPLE_LENGTH_MS +
                            keyframe.GetTriggerTime());
    
    track.AddKeyFrameToBack(keyframe);
  }
}

// Specialization for AudioSample keyframes since they don't have trigger times (always sent)
template<>
void ITrackLayerManager<AnimKeyFrame::AudioSample>::AddToPersistentLayer(AnimationTag tag, AnimKeyFrame::AudioSample& keyframe)
{
  auto layerIter = _layers.find(tag);
  if(layerIter != _layers.end())
  {
    auto& track = layerIter->second.track;
    assert(nullptr != track.GetLastKeyFrame());
    
    track.AddKeyFrameToBack(keyframe);
  }
}

template<class FRAME_TYPE>
void ITrackLayerManager<FRAME_TYPE>::RemovePersistentLayer(AnimationTag tag, s32 duration_ms)
{
  auto layerIter = _layers.find(tag);
  if(layerIter != _layers.end())
  {
    PRINT_NAMED_INFO("ITrackLayerManager.RemovePersistentLayer",
                     "%s, Tag = %d (Layers remaining=%lu)",
                     layerIter->second.name.c_str(), layerIter->first, (unsigned long)_layers.size()-1);
    
    
    // Add a layer that takes us back from where this persistent frame leaves
    // off to no adjustment at all.
    Animations::Track<FRAME_TYPE> track;
    track.SetIsLive(true);
    if(duration_ms > 0)
    {
      FRAME_TYPE firstFrame(layerIter->second.track.GetCurrentKeyFrame());
      firstFrame.SetTriggerTime(0);
      track.AddKeyFrameToBack(std::move(firstFrame));
    }
    FRAME_TYPE lastFrame;
    lastFrame.SetTriggerTime(duration_ms);
    track.AddKeyFrameToBack(std::move(lastFrame));
    
    AddLayer("Remove" + layerIter->second.name, track);
    
    _layers.erase(layerIter);
  }
}

// Specialization for AudioSample keyframes since they don't have trigger times
template<>
void ITrackLayerManager<AnimKeyFrame::AudioSample>::RemovePersistentLayer(AnimationTag tag, s32 duration_ms)
{
  auto layerIter = _layers.find(tag);
  if(layerIter != _layers.end())
  {
    PRINT_NAMED_INFO("ITrackLayerManager.RemovePersistentLayer",
                     "%s, Tag = %d (Layers remaining=%lu)",
                     layerIter->second.name.c_str(), layerIter->first, (unsigned long)_layers.size()-1);
    
    
    // Add a layer that takes us back from where this persistent frame leaves
    // off to no adjustment at all.
    Animations::Track<AnimKeyFrame::AudioSample> track;
    track.SetIsLive(true);
    if(duration_ms > 0)
    {
      AnimKeyFrame::AudioSample firstFrame(layerIter->second.track.GetCurrentKeyFrame());
      track.AddKeyFrameToBack(std::move(firstFrame));
    }
    AnimKeyFrame::AudioSample lastFrame;
    track.AddKeyFrameToBack(std::move(lastFrame));
    
    AddLayer("Remove" + layerIter->second.name, track);
    
    _layers.erase(layerIter);
  }
}

template<class FRAME_TYPE>
bool ITrackLayerManager<FRAME_TYPE>::HaveLayersToSend() const
{
  return !_layers.empty();
}

template<class FRAME_TYPE>
bool ITrackLayerManager<FRAME_TYPE>::HasLayerWithName(const std::string& name) const
{
  for(const auto& layer : _layers)
  {
    if(layer.second.name == name)
    {
      return true;
    }
  }
  
  return false;
}

template<class FRAME_TYPE>
bool ITrackLayerManager<FRAME_TYPE>::HasLayerWithTag(AnimationTag tag) const
{
  return _layers.find(tag) != _layers.end();
}

// Explicit instantiation of allowed templated classes
template class ITrackLayerManager<AnimKeyFrame::AudioSample>; // AudioLayerManager
template class ITrackLayerManager<BackpackLightsKeyFrame>;    // BackpackLayerManager
template class ITrackLayerManager<ProceduralFaceKeyFrame>;    // FaceLayerManager

}
}
