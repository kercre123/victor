using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Simon {

  public class CozmoSetSequenceSimonState : State {

    private SimonGame _GameInstance;
    private int _CurrentSequenceIndex = -1;
    private IList<int> _CurrentSequence;
    private int _SequenceLength;
    private const float kSequenceWaitDelay = 0.3f;
    private float _LastSequenceTime = -1;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _SequenceLength = _GameInstance.GetNewSequenceLength(PlayerType.Cozmo);
      _GameInstance.GenerateNewSequence(_SequenceLength);
      _CurrentSequence = _GameInstance.GetCurrentSequence();

      _GameInstance.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kSimonGameHeaderWatchCozmoPattern);
      _GameInstance.UpdateSequenceText(LocalizationKeys.kSimonGameLabelWatchCozmoPattern,
        0, _SequenceLength);

      _GameInstance.SharedMinigameView.CozmoScoreboard.Dim = false;
      _GameInstance.SharedMinigameView.PlayerScoreboard.Dim = true;

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(-1.0f);
    }

    public override void Update() {
      base.Update();
      if (_CurrentSequenceIndex == -1) {
        // First in sequence
        LightUpNextCube();
      }
      else if (_CurrentSequenceIndex >= _CurrentSequence.Count - 1) {
        // Last in sequence
        _StateMachine.SetNextState(new WaitForPlayerGuessSimonState());
      }
      else {
        bool differentCube = (_CurrentSequence[_CurrentSequenceIndex]
                             == _CurrentSequence[_CurrentSequenceIndex + 1]);
        if (differentCube || Time.time - _LastSequenceTime > kSequenceWaitDelay) {
          LightUpNextCube();
        }
      }
    }

    private void LightUpNextCube() {
      _CurrentSequenceIndex++;
      _GameInstance.UpdateSequenceText(LocalizationKeys.kSimonGameLabelWatchCozmoPattern,
        _CurrentSequenceIndex + 1, _SequenceLength);
      _LastSequenceTime = Time.time;
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
