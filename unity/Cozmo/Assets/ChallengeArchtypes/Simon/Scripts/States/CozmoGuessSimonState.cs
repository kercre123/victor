using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace Simon {
  public class CozmoGuessSimonState : State {

    private SimonGame _GameInstance;
    private IList<int> _CurrentSequence;
    private int _CurrentSequenceIndex;
    private bool? _ShouldWinGame;
    private int _LastTargetID;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _GameInstance.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kSimonGameHeaderWatchCozmoGuess);
      _GameInstance.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kSimonGameLabelWatchCozmoGuess);
      _GameInstance.SharedMinigameView.CozmoScoreboard.Dim = false;
      _GameInstance.SharedMinigameView.PlayerScoreboard.Dim = true;
      _CurrentSequence = _GameInstance.GetCurrentSequence();
      _CurrentSequenceIndex = -1;
      _ShouldWinGame = null;
      _LastTargetID = -1;

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);
    }

    public override void Update() {
      base.Update();
      if (_ShouldWinGame.HasValue) {
        if (_ShouldWinGame.GetValueOrDefault()) {
          CozmoWinGame();
        }
        else {
          CozmoLoseGame();
        }
      }
      else {
        _CurrentSequenceIndex++;
        if (_CurrentSequenceIndex >= _CurrentSequence.Count) {
          _ShouldWinGame = true;
        }
        else {
          float rand = Random.Range(0f, 1f);
          if (rand > _GameInstance.CozmoWinPercentage.Evaluate(_CurrentSequenceIndex)) {
            _ShouldWinGame = false;
            int correctId = _CurrentSequence[_CurrentSequenceIndex];
            List<int> blockIds = new List<int>();
            foreach (int cubeId in _GameInstance.CubeIdsForGame) {
              if (cubeId != correctId) {
                blockIds.Add(cubeId);
              }
            }
            _LastTargetID = blockIds[Random.Range(0, blockIds.Count)];
            StartTurnToTarget(_CurrentRobot.LightCubes[_LastTargetID]);
          }
          else {
            StartTurnToTarget(GetCurrentTarget());
          }
        }
      }
    }

    private void StartTurnToTarget(LightCube target) {
      _CurrentRobot.SetAllBackpackBarLED(_GameInstance.GetColorForBlock(target.ID));
      _StateMachine.PushSubState(new CozmoTurnToCubeSimonState(target));
    }

    public LightCube GetCurrentTarget() {
      _LastTargetID = _CurrentSequence[_CurrentSequenceIndex];
      return _CurrentRobot.LightCubes[_LastTargetID];
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);
    }

    private void CozmoLoseGame() {
      _GameInstance.SetCubeLightsGuessWrong(_LastTargetID);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
      _StateMachine.SetNextState(new AnimationGroupState(
                                        AnimationManager.Instance.GetAnimGroupForEvent(Anki.Cozmo.GameEvent.OnSimonPlayerWin),
                                        HandleOnCozmoLoseAnimationDone));
    }

    private void CozmoWinGame() {
      _GameInstance.SetCubeLightsGuessRight();

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);

      _StateMachine.SetNextState(new AnimationGroupState(
                                  AnimationManager.Instance.GetAnimGroupForEvent(Anki.Cozmo.GameEvent.OnSimonCozmoHandComplete),
                                  HandleOnCozmoWinAnimationDone));
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSimonCozmoWin);
    }

    private void HandleOnCozmoWinAnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Human));
    }

    private void HandleOnCozmoLoseAnimationDone(bool success) {
      _GameInstance.RaiseMiniGameWin(Localization.GetWithArgs(
        LocalizationKeys.kSimonGameTextPatternLength, _CurrentSequence.Count));
    }
  }
}
