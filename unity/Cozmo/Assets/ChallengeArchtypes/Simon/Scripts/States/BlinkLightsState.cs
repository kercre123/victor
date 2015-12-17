using System;
using UnityEngine;

namespace Simon {
  public class BlinkLightsState : State {
    private CozmoSetSimonState _Parent;

    private float _StartLightBlinkTime;

    public override void Enter() {
      base.Enter();
      _Parent = (CozmoSetSimonState)_StateMachine.GetParentState();
      SetSimonNodeBlink();
    }

    public override void Update() {
      base.Update();
      if (Time.time - _StartLightBlinkTime > 2.0f) {
        StopSimonNodeBlink();
        _StateMachine.PopState();
      }
    }

    private void SetSimonNodeBlink() {
      _StartLightBlinkTime = Time.time;
      LightCube currentCube = _Parent.GetCurrentTarget();
      _CurrentRobot.SendAnimation(AnimationName.kSeeOldPattern);
      currentCube.SetFlashingLEDs(currentCube.Lights[0].OnColor, 100, 100, 0);
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    private void StopSimonNodeBlink() {
      LightCube currentCube = _Parent.GetCurrentTarget();
      currentCube.SetLEDs(currentCube.Lights[0].OnColor, 0, uint.MaxValue, 0, 0, 0);
    }
  }
}

