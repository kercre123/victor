using System;
using Anki.Cozmo.Audio;
using System.ComponentModel;

namespace ScriptedSequences.Actions {
  public class PlayAudio : ScriptedSequenceAction {

    public Anki.AudioMetaData.GameEvent.GenericEvent EventType;

    public Anki.AudioMetaData.GameObjectType GameObjectType;

    [DefaultValue(true)]
    public bool WaitToEnd = true;

    [DefaultValue(true)]
    public bool StopSoundOnEnd = true;

    public override ISimpleAsyncToken Act() {
      
      SimpleAsyncToken token = new SimpleAsyncToken();

      if (WaitToEnd) {
        // Play with Callback
        // TODO: Need to set the Game Object Type appropriately
        UnityAudioClient.Instance.PostEvent(EventType, GameObjectType, Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventComplete, (c) => {
          if (c.CallbackType == Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventComplete || c.CallbackType == Anki.AudioEngine.Multiplexer.AudioCallbackFlag.EventError) {
            token.Succeed();
          }
          else {
            var err = (c.Info as Anki.AudioEngine.Multiplexer.AudioCallbackError);
            var errorType = err != null ? err.callbackError : Anki.AudioEngine.Multiplexer.CallbackErrorType.Invalid;
            token.Fail(new Exception("Received Error Playing event " + EventType + ": " + errorType));
          }
        });
        if (StopSoundOnEnd) {
          token.OnAbort += () => {
            UnityAudioClient.Instance.StopAllAudioEvents(GameObjectType);
          };
        }
      }
      else {
        // TODO: Need to set the Game Object Type appropriately
        UnityAudioClient.Instance.PostEvent(EventType, GameObjectType);
        // Play without Callback
        token.Succeed();
      }
      return token;
    }
  }
}

