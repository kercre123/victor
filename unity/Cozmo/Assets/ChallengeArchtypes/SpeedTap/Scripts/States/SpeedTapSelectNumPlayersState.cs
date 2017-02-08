using UnityEngine;
using Anki.Cozmo;
using System.Collections;

namespace SpeedTap {
  public class SpeedTapSelectNumPlayersState : State {
    private SpeedTapGame _SpeedTapGame;
    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      // Show round end slide
      GameObject numSelectSlide = _SpeedTapGame.SharedMinigameView.ShowWideGameStateSlide(
                                   _SpeedTapGame.SpeedTapNumPlayersSelectSlidePrefab.gameObject, "speedTap_round_end_slide");
      SpeedTapNumPlayerTypeSelectSlide numPlayerSelectSlideScript = numSelectSlide.GetComponent<SpeedTapNumPlayerTypeSelectSlide>();
      numPlayerSelectSlideScript.Init(MoveToNextState, _SpeedTapGame);

    }

    public override void Exit() {
      base.Exit();
    }

    private void MoveToNextState() {
      _SpeedTapGame.SharedMinigameView.HideGameStateSlide();
      _StateMachine.SetNextState(new SpeedTapCubeSelectionState());
    }
  }
}