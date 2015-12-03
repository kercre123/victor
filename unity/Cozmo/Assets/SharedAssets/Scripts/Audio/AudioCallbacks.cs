using UnityEngine;
using System.Collections;

namespace Anki {
  namespace Cozmo {
    namespace Audio {

      public struct CallbackInfo
      {
        public readonly ushort PlayId;
        public readonly AudioCallbackFlag CallbackType;
        public readonly System.Object Info;

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

        public Anki.Cozmo.Audio.AudioCallbackDuration DurationInfo {
          get { return Info as Anki.Cozmo.Audio.AudioCallbackDuration; }
        }

        public Anki.Cozmo.Audio.AudioCallbackMarker MarkerInfo {
          get { return Info as Anki.Cozmo.Audio.AudioCallbackMarker; }
        }

        public Anki.Cozmo.Audio.AudioCallbackComplete CompleteInfo {
          get { return Info as Anki.Cozmo.Audio.AudioCallbackComplete; }
        }

        public Anki.Cozmo.Audio.AudioCallbackError ErrorInfo {
          get { return Info as Anki.Cozmo.Audio.AudioCallbackError; }
        }
      }

    }
  }
}
