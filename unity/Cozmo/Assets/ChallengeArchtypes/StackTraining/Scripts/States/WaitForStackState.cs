using UnityEngine;
using System.Collections;

namespace StackTraining {
  public class WaitForStackState : State {

    private float _StackTime = 0.0f;

    private Bounds _BottomCubeBounds = new Bounds(new Vector3(150f, 0f, CozmoUtil.kBlockLengthMM * 0.5f), new Vector3(50f, 20f, 20f));
    private Bounds _TopCubeBounds = new Bounds(new Vector3(150f, 0f, CozmoUtil.kBlockLengthMM * 1.5f), new Vector3(50f, 20f, 20f));

    private float _DistanceBetweenCubesX = 5f;

    private StackTrainingGame _Game;

    public override void Enter() {
      base.Enter();
      _Game = _StateMachine.GetGame() as StackTrainingGame;
      _Game.PickCubes();
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

      if (_BottomCubeBounds.Contains(bottomCubePosition)) {
        bottomCube.SetLEDs(Color.white);

        if (_TopCubeBounds.Contains(topCubePosition) && Mathf.Abs(topCubePosition.x - bottomCubePosition.x) < _DistanceBetweenCubesX) {
          topCube.SetLEDs(Color.white);

          if (!bottomCube.IsMoving && !topCube.IsMoving) {
            _StackTime += Time.deltaTime;

            if (_StackTime > 3f) {
              HandleComplete();
            }
            return;
          }
        }
        else {
          topCube.SetLEDs(Color.red);
        }          
      }
      else {
        bottomCube.SetLEDs(Color.red);
        topCube.TurnLEDsOff();
      }
      _StackTime = 0f;
    }

    private void HandleComplete() {
      AnimationState animState = new AnimationState();
      animState.Initialize(AnimationName.kEnjoyPattern, HandleRecognizeStack);
      _StateMachine.SetNextState(animState);
    }

    private void HandleRecognizeStack(bool success) {
      _StateMachine.SetNextState(new PlowThroughBlocksState());
    }

    public override void Exit() {
      base.Exit();
    }

  }

}
