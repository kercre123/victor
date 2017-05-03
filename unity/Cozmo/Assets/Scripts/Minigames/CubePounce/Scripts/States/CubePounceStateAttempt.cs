using UnityEngine;
using Anki.Cozmo.Audio;
using Anki.Cozmo.ExternalInterface;
using System.Collections;

namespace Cozmo.Challenge.CubePounce {
  public class CubePounceStateAttempt : CubePounceState {

    private float _InitialPitch_deg = 0;
    private bool _UseClosePounce = false;

    public CubePounceStateAttempt(bool useClosePounce = false) {
      _UseClosePounce = useClosePounce;
    }

    public override void Enter() {
      base.Enter();

      _CurrentRobot.SetEnableCliffSensor(false);

      _InitialPitch_deg = Mathf.Rad2Deg * _CurrentRobot.PitchAngle;

      if (!_UseClosePounce) {
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePouncePounceNormal, HandlePounceEnd);
      }
      else {
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePouncePounceClose, HandlePounceEnd);
      }
    }

    public override void Update() {
      base.Update();

      // If cozmos current pitch has differed enough from the beginning of the anim, the block was hit
      if (_CubePounceGame.PitchIndicatesPounceSuccess(_InitialPitch_deg)) {
        _StateMachine.SetNextState(new CubePounceStatePostPoint(cozmoWon: true));
      }
    }

    public override void Exit() {
      base.Exit();
      if (_CurrentRobot != null) {
        _CurrentRobot.CancelCallback(HandlePounceEnd);
      }
    }

    private void HandlePounceEnd(bool success) {
      // If we got to the end of the post animation delay without triggering the angle diff, cozmo lost
      _StateMachine.SetNextState(new CubePounceStatePostPoint(cozmoWon: false));
    }
  }
}
