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
        AudioClient.Instance.PostEvent(EventType, 0, AudioCallbackFlag.EventComplete, (c) => {
          if (c.CallbackType == AudioCallbackFlag.EventComplete) {
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
        AudioClient.Instance.PostEvent(EventType, 0);
        // Play without Callback
        token.Succeed();
      }
      return token;
    }
  }
}

