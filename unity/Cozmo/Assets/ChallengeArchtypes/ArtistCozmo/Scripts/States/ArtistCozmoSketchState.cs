using UnityEngine;
using System.Collections;

namespace ArtistCozmo {
  public class ArtistCozmoSketchState : State {

    ArtistCozmoGame _Game;

    private bool _ImageReceived = false;
    private bool _AnimationComplete = false;

    private Texture2D _Image;

    public override void Enter() {
      base.Enter();
      _Game = (ArtistCozmoGame)_StateMachine.GetGame();

      _Game.ImageReceiver.OnImageReceived += HandleImageReceived;
      _Game.ImageReceiver.CaptureSnapshot();

      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.PatternGuessThinking, HandleAnimationComplete);
    }

    public override void Update() {
      if (_ImageReceived && _AnimationComplete) {
        _StateMachine.PushSubState(new ArtistCozmoRevealImageState(_Image));
      }
    }

    private void HandleImageReceived(Texture2D texture) {
      if (_Game.Style == ArtistCozmoGame.ArtStyle.Sketch) {
        _Game.StartCoroutine(ImageUtil.SketchAsync(texture, HandleDrawingSketched, _Game.ColorCount, _Game.ColorGradient));
      }
      else {
        _Game.StartCoroutine(ImageUtil.PaintAsync(texture, HandleDrawingSketched, _Game.ColorCount % 2 == 0, _Game.ColorGradient));
      }
    }

    private void HandleDrawingSketched(Texture2D texture) {
      _Image = texture;
      _ImageReceived = true;
    }

    private void HandleAnimationComplete(bool success) {
      _AnimationComplete = true;
    }
  }
}