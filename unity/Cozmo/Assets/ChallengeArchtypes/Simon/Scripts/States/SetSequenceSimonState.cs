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

      _GameInstance.UpdateSequenceText(LocalizationKeys.kSimonGameLabelWatchPattern,
        0, _SequenceLength);

      _GameInstance.SharedMinigameView.CozmoScoreboard.Dim = _NextPlayer == PlayerType.Human;
      _GameInstance.SharedMinigameView.PlayerScoreboard.Dim = _NextPlayer != PlayerType.Human;

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      _LastSequenceTime = Time.time;
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSimonExampleStarted);
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
        if (_NextPlayer == PlayerType.Human) {
          _StateMachine.SetNextState(new WaitForPlayerGuessSimonState());
        }
        else {
          _StateMachine.SetNextState(new CozmoGuessSimonState());        
        }
      }
      else {
        if (Time.time - _LastSequenceTime > _GameInstance.TimeBetweenBeats) {
          LightUpNextCube();
        }
      }
    }

    private void LightUpNextCube() {
      _CurrentSequenceIndex++;
      _GameInstance.UpdateSequenceText(LocalizationKeys.kSimonGameLabelWatchPattern,
        _CurrentSequenceIndex + 1, _SequenceLength);
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
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }
  }

}
