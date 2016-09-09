using UnityEngine;
using System.Collections;
using System.IO;
using System;
using Anki.Cozmo.Audio;

namespace ArtistCozmo {
  public class ArtistCozmoRevealImageState : State {

    private Texture2D _Image;

    public ArtistCozmoRevealImageState(Texture2D image) {
      _Image = image;
    }

    public override void Enter() {
      base.Enter();

      // TODO: Get specific sound for this
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Lightup);

      ArtistCozmoGame game = (ArtistCozmoGame)_StateMachine.GetGame();
      game.DisplayImage(_Image, HandleSave, HandleRetry);
    }

    public void HandleRetry() {
      _StateMachine.GetGame().SharedMinigameView.HideGameStateSlide();
      _StateMachine.SetNextState(new ArtistCozmoFindFaceState());
    }

    public void HandleSave() {
      _StateMachine.GetGame().SharedMinigameView.HideGameStateSlide();
      var jpg = _Image.EncodeToJPG();

      ArtistCozmoGame game = (ArtistCozmoGame)_StateMachine.GetGame();
      string folder = Path.Combine(Application.persistentDataPath, game.SaveFolder);

      Directory.CreateDirectory(folder);

      string fileName = DateTime.Now.ToString().Replace('/', '-').Replace(' ', '_') + ".jpg";
      File.WriteAllBytes(Path.Combine(folder, fileName), jpg);

      game.StartBaseGameEnd(true);
    }


    public override void Exit() {
      Texture2D.Destroy(_Image);
    }
  }
}
