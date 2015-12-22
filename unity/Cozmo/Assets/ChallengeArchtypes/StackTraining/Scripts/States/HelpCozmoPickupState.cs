using UnityEngine;
using System.Collections;

namespace StackTraining {
  public class HelpCozmoPickupState : State {

    private Bounds _BottomCubeBounds = new Bounds(new Vector3(100f, 0f, CozmoUtil.kBlockLengthMM * 0.5f), new Vector3(100f, 20f, 20f));

    private bool _Moving = false;

    private StackTrainingGame _Game;

    public override void Enter() {
      base.Enter();
      _Game = _StateMachine.GetGame() as StackTrainingGame;

      _CurrentRobot.SetHeadAngle(0f);
      _CurrentRobot.SetLiftHeight(0f);

      _Game.Progress = 0.5f;
    }

    public override void Update() {
      base.Update();

      var bottomCube = _Game.BottomCube;
      var topCube = _Game.TopCube;

      if (bottomCube == null || topCube == null) {
        _Game.RaiseMiniGameLose();
        return;
      }

      Vector3 bottomCubePosition = _CurrentRobot.WorldToCozmo(bottomCube.WorldPosition);

      if (_Moving) {
        return;
      }

      if (_BottomCubeBounds.Contains(bottomCubePosition) && bottomCube.MarkersVisible) {
        bottomCube.SetLEDs(Color.white);

        _Game.Progress = 0.6f;

        _Moving = true;
        _CurrentRobot.PickupObject(bottomCube, callback: (success) => {
          if (success) {
            _Game.Progress = 0.8f;
            topCube.SetLEDs(Color.blue);
            _Game.ShowHowToPlaySlide("TapCube2");
            _StateMachine.SetNextState(new TapCubeState(new HelpCozmoStackState(), topCube.ID));
          }
          else {
            if (_Game.TryDecrementAttempts()) {
              _CurrentRobot.SetLiftHeight(0.0f);
            }
            else {
              HandleFailed();
            }
          }
          _Moving = false;
        });
      }
      else {
        bottomCube.SetLEDs(Color.red);
        topCube.TurnLEDsOff();
      }
    }

    private void HandleFailed() {
      _StateMachine.SetNextState(new AnimationState(AnimationName.kShocked, HandleLoseComplete));
    }

    private void HandleLoseComplete(bool success) {
      _Game.RaiseMiniGameLose();
    }

    public override void Exit() {
      base.Exit();
    }

  }

}
