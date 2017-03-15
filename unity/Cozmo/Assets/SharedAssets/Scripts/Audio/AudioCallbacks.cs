using UnityEngine;
using System.Collections;
using Anki.AudioEngine.Multiplexer;

namespace Anki {
  namespace Cozmo {
    namespace Audio {

      public struct CallbackInfo
      {
        public readonly ushort PlayId;
        public readonly AudioCallbackFlag CallbackType;
        public readonly System.Object Info;

        public CallbackInfo(AudioCallback callback) {
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

        public AudioCallbackDuration DurationInfo {
          get { return Info as AudioCallbackDuration; }
        }

        public AudioCallbackMarker MarkerInfo {
          get { return Info as AudioCallbackMarker; }
        }

        public AudioCallbackComplete CompleteInfo {
          get { return Info as AudioCallbackComplete; }
        }

        public AudioCallbackError ErrorInfo {
          get { return Info as AudioCallbackError; }
        }
      }

    }
  }
}
