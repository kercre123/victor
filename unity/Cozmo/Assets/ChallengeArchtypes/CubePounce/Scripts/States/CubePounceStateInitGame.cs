using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateInitGame : CubePounceState {

    public override void Enter() {
      base.Enter();
      _CubePounceGame.ResetScore();
      _CubePounceGame.ResetPounceChance();
      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderSetupText);
      _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoSetupText);
      _CubePounceGame.SharedMinigameView.HideMiddleBackground();
      _CubePounceGame.SharedMinigameView.HideContinueButton();
      _CubePounceGame.UpdateScoreboard();
      _CubePounceGame.SelectCubeTarget();
      _CubePounceGame.SharedMinigameView.HideShelf();

      _CubePounceGame.SharedMinigameView.HideInstructionsVideoButton();

      LightCube target = _CubePounceGame.GetCubeTarget();
      // If we don't have a cube ready for whatever reason, go back to the initial cube state
      if (target == null || !target.IsInFieldOfView) {
        _StateMachine.SetNextState(new InitialCubesState(new CubePounceStateInitGame(), _CubePounceGame.GameConfig.NumCubesRequired()));
      }
      else {
        // Start the Game
        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Keep_Away_Between_Rounds);
        _StateMachine.SetNextState(new CubePounceStateResetPoint());
      }

      // TODO:(lc) When this getin is ready (simple getin to game with lift down and head angle correct) enable it
      //_CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceGetIn, null);
    }
  }
}
