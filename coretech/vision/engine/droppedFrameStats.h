/**
 * File: droppedFrameStats.h
 *
 * Author: Andrew Stein
 * Date:   7/26/2016
 *
 * Description: Tracks total and recent frame drops and prints statistics for them.
 *
 * Copyright: Anki, Inc. 2016
 **/

#ifndef __Anki_Vision_Basestation_DroppedFrameStats_H__
#define __Anki_Vision_Basestation_DroppedFrameStats_H__

#include "util/logging/logging.h"

namespace Anki {
namespace Vision {

class DroppedFrameStats
{
public:

  DroppedFrameStats() { }

  // Defines how long the window is for computing "recent" drop stats,
  // in number of frames. Default is 100.
  void SetRecentWindowLength(u32 N) { _recentN = N; }

  // Set channel name for PRINT_CH_INFO messages. Default is "Performance".
  void SetChannelName(const char* channelName) { _channelName = channelName; }

  void Update(bool isDroppingFrame)
  {
    ++_numFrames;
    ++_numRecentFrames;

    if(isDroppingFrame)
    {
      ++_numTotalDrops;
      ++_numRecentDrops;
    }

    if(_numRecentFrames == _recentN)
    {
      // NOTE: Logging drop counts, putting associated frame count in Data field
      // So to get rate, divide s_val (count) by frame count (ddata)
      Util::sInfoF("robot.vision.dropped_frame_overall_count",
                   {{DDATA, std::to_string(_numFrames).c_str()}},
                   "%u", _numTotalDrops);

      Util::sInfoF("robot.vision.dropped_frame_recent_count",
                   {{DDATA, std::to_string(_numRecentFrames).c_str()}},
                   "%u", _numRecentDrops);

      if(_numRecentDrops > 0)
      {
        PRINT_CH_INFO(_channelName, "DroppedFrameStats",
                      "Dropped %u of %u total images (%.1f%%), %u of last %u (%.1f%%)",
                      _numTotalDrops, _numFrames,
                      (f32)_numTotalDrops/(f32)_numFrames * 100.f,
                      _numRecentDrops, _numRecentFrames,
                      (f32)_numRecentDrops / (f32)_numRecentFrames * 100.f);
      }

      _numRecentFrames = 0;
      _numRecentDrops = 0;
    }
  }

private:

  const char* _channelName = "Performance";

  u32  _recentN         = 100;
  u32  _numFrames       = 0;
  u32  _numRecentFrames = 0;
  u32  _numTotalDrops   = 0;
  u32  _numRecentDrops  = 0;

}; // class DroppedFrameStats


} // namespace Vector
} // namespace Anki

#endif // __Anki_Vision_Basestation_DroppedFrameStats_H__
