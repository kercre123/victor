using System;
using Anki.Cozmo.Audio;
using System.ComponentModel;

namespace ScriptedSequences.Actions {
  public class PlayAudio : ScriptedSequenceAction {

    public Anki.Cozmo.Audio.GameEvent.GenericEvent EventType;

    public GameObjectType GameObjectType;

    [DefaultValue(true)]
    public bool WaitToEnd = true;

    [DefaultValue(true)]
    public bool StopSoundOnEnd = true;

    public override ISimpleAsyncToken Act() {
      
      SimpleAsyncToken token = new SimpleAsyncToken();

      if (WaitToEnd) {
        // Play with Callback
        // TODO: Need to set the Game Object Type appropriately
        UnityAudioClient.Instance.PostEvent(EventType, GameObjectType, AudioCallbackFlag.EventComplete, (c) => {
          if (c.CallbackType == AudioCallbackFlag.EventComplete || c.CallbackType == AudioCallbackFlag.EventError) {
            token.Succeed();
          }
          else {
            var err = (c.Info as AudioCallbackError);
            var errorType = err != null ? err.callbackError : CallbackErrorType.Invalid;
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

