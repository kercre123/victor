using System;
using Anki.Cozmo.Audio;

namespace ScriptedSequences.Actions {
  public class PlayAudio : ScriptedSequenceAction {

    public EventType EventType;

    public bool WaitToEnd = true;

    public override ISimpleAsyncToken Act() {
      
      SimpleAsyncToken token = new SimpleAsyncToken();

      if (WaitToEnd) {
        // Play with Callback
        // TODO: Need to set the Game Object Type appropriately
        AudioClient.Instance.PostEvent(EventType, Anki.Cozmo.Audio.GameObjectType.Default, AudioCallbackFlag.EventComplete, (c) => {
          if (c.CallbackType == AudioCallbackFlag.EventComplete || c.CallbackType == AudioCallbackFlag.EventError) {
            token.Succeed();
          }
          else {
            var err = (c.Info as AudioCallbackError);
            var errorType = err != null ? err.callbackError : CallbackErrorType.Invalid;
            token.Fail(new Exception("Received Error Playing event " + EventType + ": " + errorType));
          }
        });
      }
      else {
        // TODO: Need to set the Game Object Type appropriately
        AudioClient.Instance.PostEvent(EventType, Anki.Cozmo.Audio.GameObjectType.Default);
        // Play without Callback
        token.Succeed();
      }
      return token;
    }
  }
}

