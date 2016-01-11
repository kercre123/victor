using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Simon {

  public class CozmoSetSequenceSimonState : State {

    private SimonGame _GameInstance;
    private int _CurrentSequenceIndex = -1;
    private IList<int> _CurrentSequence;
    private int _SequenceLength;
    private SimonGameSequencePanel _SequenceDisplay;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _SequenceLength = _GameInstance.GetNewSequenceLength(PlayerType.Cozmo);
      _GameInstance.GenerateNewSequence(_SequenceLength);
      _CurrentSequence = _GameInstance.GetCurrentSequence();

      GameObject sequenceDisplay = _GameInstance.ShowHowToPlaySlide("WatchCozmoPattern");
      _SequenceDisplay = sequenceDisplay.GetComponent<SimonGameSequencePanel>();
      _SequenceDisplay.SetSequenceText(0, _SequenceLength);

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);

      _StateMachine.PushSubState(new AnimationState(AnimationName.kShocked, HandleOnCozmoStartAnimationDone));
    }

    private void HandleOnCozmoStartAnimationDone(bool success) {
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
      _StateMachine.PopState();
    }

    public override void Update() {
      base.Update();
      _CurrentSequenceIndex++;
      if (_CurrentSequenceIndex == _CurrentSequence.Count) {
        _StateMachine.SetNextState(new WaitForPlayerGuessSimonState());
        return;
      }
      _SequenceDisplay.SetSequenceText(_CurrentSequenceIndex + 1, _SequenceLength);
      _StateMachine.PushSubState(new CozmoTurnToCubeSimonState(GetCurrentTarget(), true));
    }

    public LightCube GetCurrentTarget() {
      return _CurrentRobot.LightCubes[_CurrentSequence[_CurrentSequenceIndex]];
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
