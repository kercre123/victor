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

        public CallbackInfo(Anki.Cozmo.Audio.AudioCallback callback) {
          this.PlayId = callback.callbackId;
          this.CallbackType = AudioCallbackFlag.EventNone;
          this.Info = null;

          switch (callback.callbackInfo.GetTag()) {

          case AudioCallbackInfo.Tag.callbackDuration:
            this.CallbackType = AudioCallbackFlag.EventDuration;
            this.Info = callback.callbackInfo.callbackDuration;
            break;

          case AudioCallbackInfo.Tag.callbackMarker:
            this.CallbackType = AudioCallbackFlag.EventMarker;
            this.Info = callback.callbackInfo.callbackMarker;
            break;

          case AudioCallbackInfo.Tag.callbackComplete:
            this.CallbackType = AudioCallbackFlag.EventComplete;
            this.Info = callback.callbackInfo.callbackComplete;
            break;

          case AudioCallbackInfo.Tag.callbackError:
            this.CallbackType = AudioCallbackFlag.EventError;  
            this.Info = callback.callbackInfo.callbackError;
            break;

          case AudioCallbackInfo.Tag.INVALID:
            DAS.Error("CallbackInfo", "Callback Info type is INVALID!!");
            break; 
          }
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
