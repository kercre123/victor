using UnityEngine;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStateInitGame : CubePounceState {

    public override void Enter() {
      base.Enter();
      _CubePounceGame.ResetScore();
      _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderSetupText);
      _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoSetupText);
      _CubePounceGame.SharedMinigameView.HideMiddleBackground();
      _CubePounceGame.SharedMinigameView.HideContinueButton();
      _CubePounceGame.UpdateScoreboard();

      LightCube target = _CubePounceGame.GetCubeTarget();
      // If we don't have a cube ready for whatever reason, go back to the initial cube state
      if (target == null || !target.IsInFieldOfView) {
        _StateMachine.SetNextState(new InitialCubesState(new CubePounceStateInitGame(), _CubePounceGame.NumCubesRequired));
      } else {
        _StateMachine.SetNextState(new CubePounceStateResetPoint());
      }
    }
  }
}
