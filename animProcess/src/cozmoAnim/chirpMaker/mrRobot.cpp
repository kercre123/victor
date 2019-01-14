/**
 * File: because it had to be done
 *
 */

#include "cozmoAnim/chirpMaker/mrRobot.h"
#include "clad/types/chirpTypes.h"


namespace Anki {
namespace Vector {
  
namespace {
  //  const float A = ;
  //  const float Bb = ;
  //const float G = 391.995f;
  const float F = 349.228f;
  const float Gb = 369.994f;
  const float C = 261.626f;
  const float Db = 277.183f;
  const float D = 293.665f;
  const float Eb = 311.127f;
  
//  const float G_Hz = 391.995f;
//  const float D_Hz = 293.665f;
//  const float E_Hz = 329.628f;
//  const float Fsharp_Hz = 369.994f;
  
  enum Note {
    EIGHTH=1,
    QUARTER=2,
    DOTTED_QUARTER=3,
    HALF=4,
    DOTTED_HALF=6,
    WHOLE=8,
    
    QUARTER_REST=20,
    EIGHTH_REST=21,
    WHOLE_REST=22,
  };
}
  
  constexpr int kEighth = 250;
  
int GetPlayedDuration(Note note) {
  // EIGHTH = 250ms
  return int(note)*kEighth - 20;
}
int GetMeasuredDuration(Note note) {
  return int(note)*kEighth;
}
  
void MrRoboto::BasePart(std::vector<Chirp>& output, uint64_t startTime_ms)
{
  uint64_t nextStart = startTime_ms;
  auto add = [&](Note note, float pitch=0.0f) {
    if( note == QUARTER_REST ) {
      nextStart += kEighth*2;
    } else if( note == EIGHTH_REST ) {
      nextStart += kEighth;
    } else if( note == WHOLE_REST ) {
      nextStart += kEighth*8;
    } else {
      Chirp chirp;
      chirp.startTime_ms = nextStart;
      chirp.duration_ms = GetPlayedDuration(note);
      chirp.pitch0_Hz = pitch;
      chirp.pitch1_Hz = pitch;
      chirp.volume = 1.0f;
      nextStart += GetMeasuredDuration(note);
      output.push_back(std::move(chirp));
    }
  };
  
  // intro
  
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( QUARTER, F );
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( EIGHTH_REST );
  add( QUARTER, F );
  add( EIGHTH,   F );
  add( QUARTER, F );
  add( QUARTER, F );
  
  // repeat, this time with other plater
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( QUARTER, F );
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( EIGHTH_REST );
  add( QUARTER, F );
  add( EIGHTH,   F );
  add( QUARTER, F );
  add( QUARTER, F );
  
  add( QUARTER, Gb );
  add( QUARTER, Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( Note(9), Gb );
  
  // repeat again
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( QUARTER, F );
  add( EIGHTH,   F );
  add( EIGHTH,   F );
  add( EIGHTH_REST );
  add( QUARTER, F );
  add( EIGHTH,   F );
  add( QUARTER, F );
  add( QUARTER, F );
  
  add( QUARTER, Gb );
  add( QUARTER, Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( DOTTED_QUARTER, Gb );
  add( QUARTER, Gb );
  add( QUARTER, Gb );
  add( QUARTER, Gb );
  add( QUARTER, Gb );
  add( QUARTER, Gb );
  add( QUARTER, Gb );
  
}
  
void MrRoboto::TreblePart(std::vector<Chirp>& output, uint64_t startTime_ms)
{
  uint64_t nextStart = startTime_ms;
  auto add = [&](Note note, float pitch=0.0f) {
    if( note == QUARTER_REST ) {
      nextStart += kEighth*2;
    } else if( note == EIGHTH_REST ) {
      nextStart += kEighth;
    } else if( note == WHOLE_REST ) {
      nextStart += kEighth*8;
    } else {
      Chirp chirp;
      chirp.startTime_ms = nextStart;
      chirp.duration_ms = GetPlayedDuration(note);
      chirp.pitch0_Hz = pitch;
      chirp.pitch1_Hz = pitch;
      chirp.volume = 1.0f;
      nextStart += GetMeasuredDuration(note);
      output.push_back(std::move(chirp));
    }
  };
  
  add( WHOLE_REST );
  add( WHOLE_REST );
  
  add( EIGHTH,   Eb );
  add( EIGHTH,   Eb );
  add( EIGHTH,   Eb );
  add( EIGHTH,   Eb );
  add( EIGHTH,   Eb );
  add( EIGHTH,   D );
  add( EIGHTH_REST );
  add( EIGHTH,   Eb );
  add( EIGHTH_REST );
  add( QUARTER, Eb );
  add( EIGHTH,   D );
  add( QUARTER, Eb );
  add( QUARTER, D );
  add( QUARTER, Eb );
  add( QUARTER, Db );
  add( EIGHTH,   C );
  add( EIGHTH,   Db );
  add( EIGHTH,   Eb );
  add( Note(9), Db );
  
  // repeat
  add( EIGHTH,   Eb );
  add( EIGHTH,   Eb );
  add( EIGHTH,   Eb );
  add( EIGHTH,   Eb );
  add( EIGHTH,   Eb );
  add( EIGHTH,   D );
  add( EIGHTH_REST );
  add( EIGHTH,   Eb );
  add( EIGHTH_REST );
  add( QUARTER, Eb );
  add( EIGHTH,   D );
  add( QUARTER, Eb );
  add( QUARTER, D );
  add( QUARTER, Eb );
  add( QUARTER, Db );
  add( EIGHTH,   C );
  add( EIGHTH,   Db );
  add( EIGHTH,   Eb );
  add( QUARTER, Db );
  // this time with trill
  add( EIGHTH,   C );
  add( EIGHTH,   Db );
  add( EIGHTH,   C );
  add( EIGHTH,   Db );
  add( EIGHTH,   C );
  add( EIGHTH,   Db );
  add( EIGHTH,   C );
  add( EIGHTH,   Db );
  add( EIGHTH,   C );
  add( EIGHTH,   Db );
  add( EIGHTH,   C );
  add( EIGHTH,   Db );
  add( QUARTER,   C );
  
  
}
  
} // namespace Vector
} // namespace Anki
