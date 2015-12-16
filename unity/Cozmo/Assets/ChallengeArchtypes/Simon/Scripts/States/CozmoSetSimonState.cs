using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Simon {

  public class CozmoSetSimonState : State {

    private SimonGame _GameInstance;
    private int _CurrentSequenceIndex = 0;
    private IList<int> _CurrentSequence;
    private bool _TurningToTarget = true;
    private float _StartLightBlinkTime = 0.0f;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.PickNewSequence();
      _CurrentSequence = _GameInstance.GetCurrentSequence();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    public override void Update() {
      base.Update();
      if (_CurrentSequenceIndex == _CurrentSequence.Count) {
        _StateMachine.SetNextState(new WaitForPlayerSimonState());
        return;
      }

      if (_TurningToTarget) {
        if (TurnToTarget()) {
          // done turning to target, let's blink the lights
          _TurningToTarget = false;
          SetSimonNodeBlink();
        }
      }
      else {
        // time the light blinking
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
        if (Time.time - _StartLightBlinkTime > 2.0f) {
          StopSimonNodeBlink();
          _TurningToTarget = true;
          _CurrentSequenceIndex++;
        }
      }

    }

    private bool TurnToTarget() {
      LightCube currentTarget = GetCurrentTarget();
      Vector3 robotToTarget = currentTarget.WorldPosition - _CurrentRobot.WorldPosition;
      float crossValue = Vector3.Cross(_CurrentRobot.Forward, robotToTarget).z;
      if (crossValue > 0.0f) {
        _CurrentRobot.DriveWheels(-35.0f, 35.0f);
      }
      else {
        _CurrentRobot.DriveWheels(35.0f, -35.0f);
      }
      return Vector2.Dot(robotToTarget.normalized, _CurrentRobot.Forward) > 0.98f;
    }

    private void SetSimonNodeBlink() {
      _StartLightBlinkTime = Time.time;
      LightCube currentCube = GetCurrentTarget();
      _CurrentRobot.SendAnimation(AnimationName.kSeeOldPattern);
      currentCube.SetFlashingLEDs(currentCube.Lights[0].OnColor, 100, 100, 0);
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    private LightCube GetCurrentTarget() {
      return _CurrentRobot.LightCubes[_CurrentSequence[_CurrentSequenceIndex]];
    }

    private void StopSimonNodeBlink() {
      LightCube currentCube = _CurrentRobot.LightCubes[_CurrentSequence[_CurrentSequenceIndex]];
      currentCube.SetLEDs(currentCube.Lights[0].OnColor, 0, uint.MaxValue, 0, 0, 0);
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      foreach (KeyValuePair<int, LightCube> kvp in _CurrentRobot.LightCubes) {
        kvp.Value.SetLEDs(kvp.Value.Lights[0].OnColor, 0, uint.MaxValue, 0, 0, 0);
      }
    }

  }

}
