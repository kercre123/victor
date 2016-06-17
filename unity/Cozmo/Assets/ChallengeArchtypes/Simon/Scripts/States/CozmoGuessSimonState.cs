using UnityEngine;
using System.Collections.Generic;

namespace Simon {
  public class CozmoGuessSimonState : State {

    private SimonGame _GameInstance;
    private IList<int> _CurrentSequence;
    private int _CurrentSequenceIndex;
    private bool? _ShouldWinGame;
    private int _LastTargetID;
    private bool _IsAnimating = false;

    // Still under review for what we want for audio.
    // This is likely to be refactored into all different clips with unique sounds.
    private const string _kPointDefault = "ag_simon_point_center";
    private const string _kPointLeftSmall = "ag_simon_point_smallleft";
    private const string _kPointRightSmall = "ag_simon_point_smallright";
    private const string _kPointLeftBig = "ag_simon_point_bigleft";
    private const string _kPointRightBig = "ag_simon_point_bigright";

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as SimonGame;
      _CurrentSequence = _GameInstance.GetCurrentSequence();
      _CurrentSequenceIndex = -1;
      _ShouldWinGame = null;
      _LastTargetID = -1;

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSimonPlayerWin, HandleOnCozmoLoseAnimationDone);
      AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSimonCozmoHandComplete, HandleOnCozmoWinAnimationDone);

      _GameInstance.ShowCurrentPlayerTurnStage(PlayerType.Cozmo, false);
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.DriveWheels(0.0f, 0.0f);

      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSimonPlayerWin, HandleOnCozmoLoseAnimationDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSimonCozmoHandComplete, HandleOnCozmoWinAnimationDone);
    }

    public override void Update() {
      base.Update();
      if (_IsAnimating) {
        return;
      }
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
          // Cozmo always right first time
          if (_GameInstance.GetCurrentTurnNumber > 1 && rand > _GameInstance.CozmoWinPercentage.Evaluate(_CurrentSequenceIndex)) {
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
            _LastTargetID = _CurrentSequence[_CurrentSequenceIndex];
            StartTurnToTarget(GetCurrentTarget());
          }
        }
      }
    }

    private void StartTurnToTarget(LightCube target) {
      _CurrentRobot.SetAllBackpackBarLED(_GameInstance.GetColorForBlock(target.ID));

      int goingToIndex = _GameInstance.CubeIdsForGame.IndexOf(target.ID);
      int fromIndex = 1;
      if (_CurrentSequenceIndex > 0) {
        fromIndex = _GameInstance.CubeIdsForGame.IndexOf(_CurrentSequence[_CurrentSequenceIndex - 1]);
      }
      string animGroup = _kPointDefault;
      if (goingToIndex - fromIndex == 1) {
        animGroup = _kPointLeftSmall;
      }
      else if (goingToIndex - fromIndex == -1) {
        animGroup = _kPointRightSmall;
      }
      if (goingToIndex - fromIndex >= 2) {
        animGroup = _kPointLeftBig;
      }
      else if (goingToIndex - fromIndex <= -2) {
        animGroup = _kPointRightBig;
      }

      _StateMachine.PushSubState(new CozmoBlinkLightsSimonState(target, animGroup));
    }

    public LightCube GetCurrentTarget() {
      return _CurrentRobot.LightCubes[_LastTargetID];
    }

    private void CozmoLoseGame() {
      _GameInstance.SetCubeLightsGuessWrong(_LastTargetID);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSimonPlayerWin);
      _IsAnimating = true;

      _GameInstance.ShowBanner(LocalizationKeys.kSimonGameLabelYouWin);
    }

    private void CozmoWinGame() {
      _GameInstance.SetCubeLightsGuessRight();

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedRoundEnd);
      GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSimonCozmoHandComplete);
      _IsAnimating = true;
    }

    private void HandleOnCozmoWinAnimationDone(bool success) {
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Human));
    }

    private void HandleOnCozmoLoseAnimationDone(bool success) {
      _GameInstance.RaiseMiniGameWin(Localization.GetWithArgs(
        LocalizationKeys.kSimonGameTextPatternLength, (_CurrentSequence.Count - _GameInstance.MinSequenceLength + 1)));
    }
  }
}
