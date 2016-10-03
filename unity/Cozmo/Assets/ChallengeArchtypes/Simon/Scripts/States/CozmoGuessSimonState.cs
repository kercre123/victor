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

      _GameInstance.ShowCurrentPlayerTurnStage(PlayerType.Cozmo, false);
    }

    public override void Exit() {
      base.Exit();
      if (_CurrentRobot != null) {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }
    }

    public override void Update() {
      base.Update();
      if (_IsAnimating) {
        return;
      }
      if (_ShouldWinGame.HasValue) {
        if (_ShouldWinGame.GetValueOrDefault()) {
          CozmoWinRound();
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
      Anki.Cozmo.AnimationTrigger animTrigger = Anki.Cozmo.AnimationTrigger.SimonPointCenter;
      if (goingToIndex - fromIndex == 1) {
        animTrigger = Anki.Cozmo.AnimationTrigger.SimonPointLeftSmall;
      }
      else if (goingToIndex - fromIndex == -1) {
        animTrigger = Anki.Cozmo.AnimationTrigger.SimonPointRightSmall;
      }
      if (goingToIndex - fromIndex >= 2) {
        animTrigger = Anki.Cozmo.AnimationTrigger.SimonPointLeftBig;
      }
      else if (goingToIndex - fromIndex <= -2) {
        animTrigger = Anki.Cozmo.AnimationTrigger.SimonPointRightBig;
      }

      _StateMachine.PushSubState(new CozmoBlinkLightsSimonState(target, animTrigger));
    }

    public LightCube GetCurrentTarget() {
      return _CurrentRobot.LightCubes[_LastTargetID];
    }

    private void CozmoLoseGame() {
      _GameInstance.SetCubeLightsGuessWrong(_LastTargetID);
      _GameInstance.DecrementLivesRemaining(PlayerType.Cozmo);

      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnSimonPlayerWin, HandleOnCozmoLoseAnimationDone);
      _IsAnimating = true;
    }

    private void CozmoWinRound() {
      _GameInstance.SetCubeLightsGuessRight();
      _GameInstance.ShowCenterResult(true, true);
      Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Silent);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Round_End);
      _GameInstance.AddPoint(false);
      _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.OnSimonCozmoHandComplete, HandleOnCozmoWinAnimationDone);
      _IsAnimating = true;
    }

    private void HandleOnCozmoWinAnimationDone(bool success) {
      _GameInstance.ShowCenterResult(false);
      _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Cozmo));
    }

    private void HandleOnCozmoLoseAnimationDone(bool success) {
      _GameInstance.ShowCenterResult(false);
      if (_GameInstance.GetLivesRemaining(PlayerType.Cozmo) < 0) {
        _GameInstance.FinalLifeComplete(PlayerType.Cozmo);
      }
      else {
        _StateMachine.SetNextState(new WaitForNextRoundSimonState(PlayerType.Cozmo));
      }
    }
  }
}
