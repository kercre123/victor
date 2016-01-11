using System;
using UnityEngine;
using Anki.Cozmo;

namespace StackTraining {
  public class PlowThroughBlocksState : State {

    private StackTrainingGame _Game;

    private Vector3 _StartPosition;
    private Quaternion _StartRotation;

    private Vector3 _BottomCubePosition;

    private bool _HasRunIntoCubes = false;

    private bool _HasStartedReturningToPosition = false;

    private bool _HasFinishedPlowing = false;

    private bool _OnCliff = false;

    private float _BackupTime = 0.0f;

    public override void Enter() {
      base.Enter();

      _Game = _StateMachine.GetGame() as StackTrainingGame;

      _StartPosition = _CurrentRobot.WorldPosition;
      _StartRotation = _CurrentRobot.Rotation;

      _BottomCubePosition = _Game.BottomCube.WorldPosition;
      _CurrentRobot.SetLiftHeight(0.5f);

      RobotEngineManager.Instance.OnCliffEvent += HandleCliffEvent;
    }

    public override void Update() {
      if (!_HasRunIntoCubes) {

        if (_CurrentRobot.WorldToCozmo(_BottomCubePosition).x > 0) {

          _CurrentRobot.DriveWheels(1000f, 1000f);
          return;
        }

        _HasRunIntoCubes = true;
        _CurrentRobot.SendAnimation(AnimationName.kShocked, (__) => {

          _CurrentRobot.SetLiftHeight(1f);
          _HasFinishedPlowing = true;

        });
      }
      else if (!_HasStartedReturningToPosition && _HasFinishedPlowing) {

        var currentEuler = _CurrentRobot.Rotation.eulerAngles;
        var startingEuler = _StartRotation.eulerAngles;

        // This isn't really a great way to test gradient of a surface
        // TODO: do better
        var initialTiltMax = Mathf.Max(Mathf.Abs(startingEuler.x), Mathf.Abs(startingEuler.y));
        var currentTiltMax = Mathf.Max(Mathf.Abs(currentEuler.x), Mathf.Abs(currentEuler.y));

        // the 1.5 seconds is a hack since we don't get pitch and roll in the quaternion yet
        if (_OnCliff || currentTiltMax > initialTiltMax * 1.5f || _BackupTime < 1.5f) {
          _BackupTime += Time.deltaTime;
          _CurrentRobot.DriveWheels(-35f, -35f);
          return;
        }
        _HasStartedReturningToPosition = true;

        _CurrentRobot.GotoPose(_StartPosition, _StartRotation, callback:(success) => {
          if(!success) {
            WaitForReturnToPosition();
          }
          else {
            HandlePrepareToStack();
          }
        });
      }
    }

    private void HandlePrepareToStack() {       
      _StateMachine.SetNextState(new PlaceCubeState());
    }

    private void HandleCliffEvent(CliffEvent evt) {
      _OnCliff = evt.detected;
    }

    private void WaitForReturnToPosition() { 
      // TODO: Something here to get us back in a valid state
      HandlePrepareToStack();
    }

    public override void Exit() {
      base.Exit();

      RobotEngineManager.Instance.OnCliffEvent -= HandleCliffEvent;
    }
  }
}

