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

        // () => token.Succeed();
      }
      else {
        // Play without Callback
        token.Succeed();
      }
      return token;
    }
  }
}

