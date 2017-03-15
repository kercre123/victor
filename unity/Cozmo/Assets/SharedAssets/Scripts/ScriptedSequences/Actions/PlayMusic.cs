using System;
using Anki.Cozmo.Audio;

namespace ScriptedSequences.Actions {
  public class PlayMusic : ScriptedSequenceAction {

    public Anki.AudioMetaData.GameState.Music MusicState;

    public bool WaitToEnd = true;

    public override ISimpleAsyncToken Act() {

      GameAudioClient.SetMusicState(MusicState);

      SimpleAsyncToken token = new SimpleAsyncToken();
      token.Succeed();
      return token;
    }
  }
}

