using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Simon {

  public class SetSequenceSimonState : State {

    private SimonGame _GameInstance;
    private int _CurrentSequenceIndex = -1;
    private IList<int> _CurrentSequence;
    private int _SequenceLength;
    // TODO: move this to config ( based on turn number )
    private const float kSequenceWaitDelay = 0.5f;
    private const float kFirstWaitDelay = 1.0f;
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

      _GameInstance.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kSimonGameHeaderWatchCozmoPattern);
      _GameInstance.UpdateSequenceText(LocalizationKeys.kSimonGameLabelWatchCozmoPattern,
        0, _SequenceLength);

      _GameInstance.SharedMinigameView.CozmoScoreboard.Dim = _NextPlayer == PlayerType.Human;
      _GameInstance.SharedMinigameView.PlayerScoreboard.Dim = _NextPlayer != PlayerType.Human;

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      _LastSequenceTime = Time.time;
      DAS.Warn(this, "Enter");
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSimonExampleStarted);
    }

    public override void Update() {
      base.Update();
      if (_CurrentSequenceIndex == -1) {
        // First in sequence
        if (Time.time - _LastSequenceTime > kFirstWaitDelay) {
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
        if (Time.time - _LastSequenceTime > kSequenceWaitDelay) {
          LightUpNextCube();
        }
      }
    }

    private void LightUpNextCube() {
      _CurrentSequenceIndex++;
      _GameInstance.UpdateSequenceText(LocalizationKeys.kSimonGameLabelWatchCozmoPattern,
        _CurrentSequenceIndex + 1, _SequenceLength);
      _LastSequenceTime = Time.time;

      int cubeId = GetCurrentTarget().ID;
      DAS.Warn(this, "LightUpNextCube cubeId " + cubeId);
      Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_GameInstance.GetAudioForBlock(cubeId));
      _GameInstance.BlinkLight(cubeId, SimonGame.kLightBlinkLengthSeconds, Color.black, _GameInstance.GetColorForBlock(cubeId));
      //_StateMachine.PushSubState(new CozmoTurnToCubeSimonState(GetCurrentTarget()));
      DAS.Warn(this, "LightUpNextCube" + _CurrentSequenceIndex);
    }

    public LightCube GetCurrentTarget() {
      return _CurrentRobot.LightCubes[_CurrentSequence[_CurrentSequenceIndex]];
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      DAS.Warn(this, "Exit");
    }
  }

}
