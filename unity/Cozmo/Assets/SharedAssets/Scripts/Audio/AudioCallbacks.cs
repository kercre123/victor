using UnityEngine;
using System.Collections;

namespace Anki {
  namespace Cozmo {
    namespace Audio {

      public class CallbackInfo
      {
        public ushort PlayId = 0;
        public AudioCallbackFlag CallbackType = AudioCallbackFlag.EventNone;
        public System.Object Info = null;

        public CallbackInfo(Anki.Cozmo.Audio.AudioCallbackDuration info) {
          this.CallbackType = AudioCallbackFlag.EventDuration;
          this.PlayId = info.callbackId;
          this.Info = info;
        }

        public CallbackInfo(Anki.Cozmo.Audio.AudioCallbackMarker info) {
          this.CallbackType = AudioCallbackFlag.EventMarker;
          this.PlayId = info.callbackId;
          this.Info = info;
        }

        public CallbackInfo(Anki.Cozmo.Audio.AudioCallbackComplete info) {
          this.CallbackType = AudioCallbackFlag.EventComplete;
          this.PlayId = info.callbackId;
          this.Info = info;
        }

        public CallbackInfo(Anki.Cozmo.Audio.AudioCallbackError info) {
          this.CallbackType = AudioCallbackFlag.EventError;
          this.PlayId = info.callbackId;
          this.Info = info;
        }
      }

    }
  }
}
