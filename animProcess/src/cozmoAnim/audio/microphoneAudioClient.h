/**
 * File: microphoneAudioClient.h
 *
 * Author: Jordan Rivas
 * Created: 06/20/18
 *
 * Description: Mic Direction Audio Client receives Mic Direction messages to update the audio engine with the current
 *              mic data. By providing this to the audio engine it can adjust the audio mix and volume to better serve
 *              the current enviromnent.
 *
 * Copyright: Anki, Inc. 2018
 *
 **/


#ifndef __Anki_Victor_MicrophoneAudioClient_H__
#define __Anki_Victor_MicrophoneAudioClient_H__

#ifdef USES_CLAD_CPPLITE
#define CLAD_VECTOR(ns) CppLite::Anki::Vector::ns
#else
#define CLAD_VECTOR(ns) ns
#endif

#ifdef USES_CLAD_CPPLITE
namespace CppLite {
#endif
namespace Anki {
namespace Vector {
namespace RobotInterface {
struct MicDirection;
}
}
}
#ifdef USES_CLAD_CPPLITE
}
#endif

namespace Anki {
namespace Vector {
namespace Audio {
class CozmoAudioController;


class MicrophoneAudioClient {

public:

  MicrophoneAudioClient( CozmoAudioController* audioController );
  ~MicrophoneAudioClient();

  void ProcessMessage( const CLAD_VECTOR(RobotInterface)::MicDirection& msg );


private:

  CozmoAudioController*  _audioController = nullptr;
};

}
}
}

#endif // __Anki_Victor_MicrophoneAudioClient_H__
