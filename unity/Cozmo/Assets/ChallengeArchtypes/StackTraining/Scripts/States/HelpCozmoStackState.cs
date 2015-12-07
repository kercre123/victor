using UnityEngine;
using System.Collections;

namespace StackTraining {
  public class HelpCozmoStackState : State {

    private Bounds _BottomCubeBounds = new Bounds(new Vector3(100f, 0f, CozmoUtil.kBlockLengthMM * 0.5f), new Vector3(100f, 20f, 20f));
    private Bounds _TopCubeBounds = new Bounds(new Vector3(100f, 0f, CozmoUtil.kBlockLengthMM * 0.5f), new Vector3(100f, 20f, 20f));


    private bool _BottomDocked = false;

    private bool _Moving = false;

    private StackTrainingGame _Game;

    public override void Enter() {
      base.Enter();
      _Game = _StateMachine.GetGame() as StackTrainingGame;

      _CurrentRobot.SetHeadAngle(0f);
      _CurrentRobot.SetLiftHeight(0f);
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
      Vector3 topCubePosition = _CurrentRobot.WorldToCozmo(topCube.WorldPosition);

      if (_Moving) {
        return;
      }

      if (!_BottomDocked) {

        if (_BottomCubeBounds.Contains(bottomCubePosition)) {
          bottomCube.SetLEDs(Color.white);

          _Moving = true;
          _CurrentRobot.PickupObject(bottomCube, callback: (success) => {
            if(success) {
              _BottomDocked = true;
            }
            else {
              if(_Game.TryDecrementAttempts()) {
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
      else {

        if (_TopCubeBounds.Contains(topCubePosition)) {
          topCube.SetLEDs(Color.white);

          _Moving = true;
          _CurrentRobot.PlaceOnObject(topCube, _CurrentRobot.PoseAngle, (success) => {
            if (success) {
              HandleComplete();
            }
            else {
              if(_Game.TryDecrementAttempts()) {
                
                if (!topCube.Equals(_CurrentRobot.CarryingObject)) {
                  _BottomDocked = false;
                  _CurrentRobot.SetLiftHeight(0.0f);
                }
                _Moving = false;
              }
              else {
                HandleFailed();
              }
            }
          });
        }
        else {
          topCube.SetLEDs(Color.red);
        }   

      }
    }

    private void HandleComplete() {
      AnimationState animState = new AnimationState();
      animState.Initialize(AnimationName.kMajorWin, HandleWinComplete);
      _StateMachine.SetNextState(animState);
    }

    private void HandleFailed() {
      AnimationState animState = new AnimationState();
      animState.Initialize(AnimationName.kShocked, HandleLoseComplete);
      _StateMachine.SetNextState(animState);
    }

    private void HandleWinComplete(bool success) {
      _Game.RaiseMiniGameWin();
    }

    private void HandleLoseComplete(bool success) {
      _Game.RaiseMiniGameLose();
    }

    public override void Exit() {
      base.Exit();
    }

  }

}
