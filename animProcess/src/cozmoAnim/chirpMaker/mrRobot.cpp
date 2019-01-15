/**
 * File: because it had to be done
 *
 */

#include "mrRobot.h"
#include "clad/types/chirpTypes.h"
#include <math.h>

namespace Anki {
namespace Vector {
  
namespace {
  
  const float Ab = 207.652f;
  const float Bb = 233.082f;
  const float C = 261.626f;
  const float Db = 277.183f;
  const float D = 293.665f;
  const float Eb = 311.127f;
  const float F = 349.228f;
  const float Gb = 369.994f;
  
  const float Ab1 = 2*Ab;
  const float Bb1 = 2*Bb;
  const float C1 =  2*C;
  const float Db1 = 2*Db;
  const float D1 =  2*D;
  const float Eb1 = 2*Eb;
//  const float F1 =  2*F;
  const float Gb1 = 2*Gb;
  
//  const float Ab_1 = Ab/2;
//  const float Bb_1 = Bb/2;
  const float C_1 =  C/2;
  const float Db_1 = Db/2;
//  const float D_1 =  D/2;
  const float Eb_1 = Eb/2;
  const float F_1 =  F/2;
  const float Gb_1 = Gb/2;
  
  
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
    
    
    EIGHTH_REST=20,
    QUARTER_REST=21,
    HALF_REST=22,
    WHOLE_REST=23,
  };
}
  
  constexpr int kEighth = 200;
  
int GetPlayedDuration(Note note) {
  // EIGHTH = 250ms
  return int(note)*kEighth - 20;
}
int GetMeasuredDuration(Note note) {
  return int(note)*kEighth;
}
  
static Chirp kRestChirp;
  
void RaiseOctave(Chirp& chirp, int octaves=1) {
  const float factor = pow(2,octaves);
  chirp.pitch0_Hz *= factor;
  chirp.pitch1_Hz *= factor;
}
  
void LowerOctave(Chirp& chirp, int octaves=-1) {
  const float factor = pow(2,octaves);
  chirp.pitch0_Hz *= factor;
  chirp.pitch1_Hz *= factor;
}
  
void MrRoboto::BasePart(std::vector<Chirp>& output, uint64_t startTime_ms)
{
  uint64_t nextStart = startTime_ms;
  uint32_t delay = 0;
  auto add = [&](Note note, float pitch=0.0f) -> Chirp& {
    if( note == QUARTER_REST ) {
      nextStart += kEighth*2;
      return kRestChirp;
    } else if( note == EIGHTH_REST ) {
      nextStart += kEighth;
      return kRestChirp;
    } else if( note == HALF_REST ) {
      nextStart += kEighth*4;
      return kRestChirp;
    } else if( note == WHOLE_REST ) {
      nextStart += kEighth*8;
      return kRestChirp;
    } else {
      Chirp chirp;
      chirp.startTime_ms = nextStart + delay;
      chirp.duration_ms = GetPlayedDuration(note);
      chirp.pitch0_Hz = pitch;
      chirp.pitch1_Hz = pitch;
      chirp.volume = 1.0f;
      nextStart += GetMeasuredDuration(note);
      output.push_back(std::move(chirp));
      return output.back();
    }
  };
  
  // intro
  
  add( EIGHTH,   F_1 );
  add( EIGHTH,   F_1 );
  add( EIGHTH,   F_1 );
  add( EIGHTH,   F_1 );
  add( QUARTER, F_1 );
  add( EIGHTH,   F_1 );
  add( EIGHTH,   F_1 );
  add( EIGHTH_REST );
  add( QUARTER, F_1 );
  add( EIGHTH,   F_1 );
  add( QUARTER, F_1 );
  add( QUARTER, F_1 );
  
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
  add( EIGHTH,  Gb );
  
  add( EIGHTH_REST );
  
  // I am the mouldron man(?)
  add( EIGHTH,   Bb );
  add( DOTTED_QUARTER, Ab );
  add( EIGHTH, Db );
  add( DOTTED_QUARTER, C );
  add( EIGHTH, Ab );
  add( DOTTED_QUARTER, Gb_1 );
  
  // harmonize with secret secret
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( QUARTER,  Ab1 );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Ab1 );
  
  add( QUARTER_REST );
  // repeat
  add( EIGHTH,   Bb );
  add( DOTTED_QUARTER, Ab );
  add( EIGHTH, Db );
  add( DOTTED_QUARTER, C );
  add( EIGHTH, Ab );
  add( DOTTED_QUARTER, Gb_1 );
  
  // harmonize with secret secret
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( QUARTER,  Ab1 );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Gb );
  add( EIGHTH,   Ab1 );
  
  add( QUARTER_REST );
  // repeat last time
  add( EIGHTH,   Bb );
  add( DOTTED_QUARTER, Ab );
  add( EIGHTH, Db );
  add( DOTTED_QUARTER, C );
  add( QUARTER, Db );
  add( QUARTER, Eb );
  
  add( QUARTER, C_1 );
  add( QUARTER, C_1 );
  add( QUARTER, C_1 );
  add( QUARTER, C_1 );
  add( QUARTER, C_1 );
  add( QUARTER, C_1 );
  add( QUARTER, C_1 );
  
  // take over disco bass
  delay = 30;
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  add( EIGHTH, Db_1 );
  add( EIGHTH, Db );
  
  
}
  
void MrRoboto::TreblePart(std::vector<Chirp>& output, uint64_t startTime_ms)
{
  uint64_t nextStart = startTime_ms;
  uint32_t delay = 0;
  auto add = [&](Note note, float pitch=0.0f) -> Chirp& {
    if( note == QUARTER_REST ) {
      nextStart += kEighth*2;
      return kRestChirp;
    } else if( note == EIGHTH_REST ) {
      nextStart += kEighth;
      return kRestChirp;
    } else if( note == HALF_REST ) {
      nextStart += kEighth*4;
      return kRestChirp;
    } else if( note == WHOLE_REST ) {
      nextStart += kEighth*8;
      return kRestChirp;
    } else {
      Chirp chirp;
      chirp.startTime_ms = nextStart + delay;
      chirp.duration_ms = GetPlayedDuration(note);
      chirp.pitch0_Hz = pitch;
      chirp.pitch1_Hz = pitch;
      chirp.volume = 1.0f;
      nextStart += GetMeasuredDuration(note);
      output.push_back(std::move(chirp));
      return output.back();
    }
  };
  
  add( WHOLE_REST );
  add( WHOLE_REST );
  
  add( EIGHTH,   Eb1 );
  add( EIGHTH,   Eb1 );
  add( EIGHTH,   Eb1 );
  add( EIGHTH,   Eb1 );
  add( EIGHTH,   Eb1 );
  add( EIGHTH,   D1 );
  add( EIGHTH_REST );
  add( EIGHTH,   Eb1 );
  add( EIGHTH_REST );
  add( QUARTER, Eb1 );
  add( EIGHTH,   D1 );
  add( QUARTER, Eb1 );
  add( QUARTER, D1 );
  add( QUARTER, Eb1 );
  add( QUARTER, Db1 );
  add( EIGHTH,   C1 );
  add( EIGHTH,   Db1 );
  add( EIGHTH,   Eb1 );
  add( Note(9), Db1 );
  
  // repeat
  add( EIGHTH,   Eb1 );
  add( EIGHTH,   Eb1 );
  add( EIGHTH,   Eb1 );
  add( EIGHTH,   Eb1 );
  add( EIGHTH,   Eb1 );
  add( EIGHTH,   D1 );
  add( EIGHTH_REST );
  add( EIGHTH,   Eb1 );
  add( EIGHTH_REST );
  add( QUARTER, Eb1 );
  add( EIGHTH,   D1 );
  add( QUARTER, Eb1 );
  add( QUARTER, D1 );
  add( QUARTER, Eb1 );
  add( QUARTER, Db1 );
  add( EIGHTH,   C1 );
  add( EIGHTH,   Db1 );
  add( EIGHTH,   Eb1 );
  // this time with trill
  add( EIGHTH,   Db1 );
  add( EIGHTH,   C1 );
  add( EIGHTH,   Db1 );
  add( EIGHTH,   C1 );
  add( EIGHTH,   Db1 );
  add( EIGHTH,   C1 );
  add( EIGHTH,   Db1 );
  add( EIGHTH,   C1 );
  add( EIGHTH,   Db1 );
  add( EIGHTH,   C1 );
  add( EIGHTH,   Db1 );
  add( EIGHTH,   C1 );
  add( EIGHTH,   Db1 );
  add( EIGHTH,   C1 );
  add( EIGHTH_REST );
  
  add( WHOLE_REST );
  add( HALF_REST );
  
  // secret secret, ive got a secret
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   C1 );
  add( EIGHTH_REST );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   C1 );
  
  add( QUARTER_REST );
  
  // repeat
  add( WHOLE_REST );
  add( HALF_REST );
  
  // secret secret, ive got a secret
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   C1 );
  add( EIGHTH_REST );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   Bb1 );
  add( EIGHTH,   C1 );
  
  // repeat
  add( WHOLE_REST );
  add( HALF_REST );
  
  // disco octaves
  add( DOTTED_QUARTER, C );
  add( QUARTER, C );
  add( QUARTER, C );
  add( QUARTER, C );
  add( QUARTER, C );
  add( QUARTER, C );
  add( EIGHTH, C );
  add( EIGHTH_REST );
  
  add( EIGHTH_REST );
  
  // thank you very much
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( QUARTER, F );
  add( EIGHTH, Eb );
  add( QUARTER, F );
  add( QUARTER, F );
  add( EIGHTH, Eb );
  add( QUARTER, F );
  add( QUARTER, Eb );
  
  // domo
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb_1 );
  
  add( HALF_REST );
  
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, Eb_1 );
  add( EIGHTH, Eb_1 );
  
  add( HALF_REST );
  
  // thank you very much
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( QUARTER, F );
  add( EIGHTH, Eb );
  add( QUARTER, F );
  add( QUARTER, F );
  add( EIGHTH, Eb );
  add( QUARTER, F );
  add( QUARTER, Eb );
  
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( QUARTER, F );
  add( EIGHTH, Eb );
  add( QUARTER, F );
  add( QUARTER, F );
  add( EIGHTH, Eb );
  add( QUARTER, F );
  add( QUARTER, Eb );
  
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( QUARTER, F );
  add( EIGHTH, Eb );
  add( QUARTER, F );
  add( QUARTER, F );
  add( EIGHTH, Eb );
  add( QUARTER, F );
  add( QUARTER, Eb );
  
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( EIGHTH, F );
  add( QUARTER, F );
  add( EIGHTH, Eb );
  add( QUARTER, F );
  add( QUARTER, F );
  add( EIGHTH, Eb );
  add( QUARTER, F );
  add( QUARTER, Eb );
  
  add( EIGHTH, F ).duration_ms -= 10;
  add( EIGHTH, F ).duration_ms -= 10;
  add( EIGHTH, F ).duration_ms -= 10;
  add( EIGHTH, F ).duration_ms -= 10;
  add( EIGHTH, F );
  add( EIGHTH_REST );
  add( EIGHTH, Eb ).duration_ms -= 10;
  add( EIGHTH, F );
  add( EIGHTH_REST );
  add( EIGHTH, F );
  add( EIGHTH_REST );
  add( EIGHTH, Eb ).duration_ms -= 10;
  add( EIGHTH, F );
  add( EIGHTH_REST );
  add( EIGHTH, Eb );
  add( EIGHTH, Eb );
  add( EIGHTH, F ).duration_ms -= 10;
  add( EIGHTH, F ).duration_ms -= 10;
  add( EIGHTH, F ).duration_ms -= 10;
  add( EIGHTH, Eb );
  add( EIGHTH, F ).duration_ms -= 10;
  add( EIGHTH_REST );
  add( EIGHTH, Eb ).duration_ms -= 10;
  add( QUARTER, Bb1 );
  add( EIGHTH, Ab1 );
  add( EIGHTH, Gb );
  add( EIGHTH, Ab1 );
  add( EIGHTH, Ab1 );
  add( EIGHTH, Bb1 );
  add( EIGHTH_REST );
  add( QUARTER, Bb1 );
  add( EIGHTH, Ab1 );
  add( EIGHTH, Gb );
  add( HALF_REST );
  add( HALF_REST );
  add( EIGHTH_REST );
  add( HALF, Bb1 );
  add( EIGHTH, Ab1 );
  add( EIGHTH, Gb );
  add( EIGHTH, Eb );
  add( DOTTED_HALF, Gb );
  add( HALF_REST );
  add( HALF, Gb1 );
  add( WHOLE, Eb1 );
}
  
} // namespace Vector
} // namespace Anki
