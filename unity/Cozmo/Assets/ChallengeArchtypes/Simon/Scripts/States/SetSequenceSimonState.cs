using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Simon {

  public class SetSequenceSimonState : State {

    private SimonGame _GameInstance;
    private int _CurrentSequenceIndex = -1;
    private IList<int> _CurrentSequence;
    private int _SequenceLength;
    private float _LastSequenceTime = -1;

    private PlayerType _NextPlayer;

    public SetSequenceSimonState(PlayerType nextPlayer) {
      _NextPlayer = nextPlayer;
    }

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _SequenceLength = _GameInstance.GetNewSequenceLength(_NextPlayer);
      _GameInstance.GenerateNewSequence(_SequenceLength);
      _CurrentSequence = _GameInstance.GetCurrentSequence();

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      _LastSequenceTime = Time.time;
    }

    public override void Update() {
      base.Update();
      if (_CurrentSequenceIndex == -1) {
        // First in sequence
        if (Time.time - _LastSequenceTime > _GameInstance.TimeWaitFirstBeat) {
          LightUpNextCube();
        }
      }
      else if (_CurrentSequenceIndex >= _CurrentSequence.Count - 1) {
        // Last in sequence

        Anki.Cozmo.AnimationTrigger trigger = _GameInstance.IsSoloMode() ? Anki.Cozmo.AnimationTrigger.MemoryMatchReactToPatternSolo :
                                                                           Anki.Cozmo.AnimationTrigger.MemoryMatchReactToPattern;


        // Let the player go without Cozmo's reaction being done.
        if (_NextPlayer == PlayerType.Human) {
          _CurrentRobot.SendAnimationTrigger(trigger);
          HandEndAnimationDone(true);
        }
        else {
          _StateMachine.SetNextState(new AnimationGroupState(trigger, HandEndAnimationDone));
        }

      }
      else {
        if (Time.time - _LastSequenceTime > _GameInstance.TimeBetweenBeats) {
          LightUpNextCube();
        }
      }
    }

    public void HandEndAnimationDone(bool success) {
      if (_NextPlayer == PlayerType.Human) {
        _StateMachine.SetNextState(new WaitForPlayerGuessSimonState());
      }
      else {
        _StateMachine.SetNextState(new CozmoGuessSimonState());
      }
    }

    private void LightUpNextCube() {
      _CurrentSequenceIndex++;
      _LastSequenceTime = Time.time;

      int cubeId = GetCurrentTarget().ID;
      Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_GameInstance.GetAudioForBlock(cubeId));
      _GameInstance.BlinkLight(cubeId, SimonGame.kLightBlinkLengthSeconds, Color.black, _GameInstance.GetColorForBlock(cubeId));
    }

    public LightCube GetCurrentTarget() {
      return _CurrentRobot.LightCubes[_CurrentSequence[_CurrentSequenceIndex]];
    }

    public override void Exit() {
      base.Exit();
      if (_CurrentRobot != null) {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }
    }
  }

}
