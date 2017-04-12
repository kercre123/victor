using UnityEngine;
using System.Collections.Generic;

namespace MemoryMatch {
  public class CozmoGuessMemoryMatchState : State {

    private MemoryMatchGame _GameInstance;
    private int _CurrentSequenceIndex;
    private bool? _ShouldWinGame;
    private int _LastTargetID;
    private bool _IsAnimating = false;
    private bool _CubeLost = false;

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as MemoryMatchGame;
      _CurrentSequenceIndex = -1;
      _ShouldWinGame = null;
      _LastTargetID = -1;

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      _GameInstance.ShowCurrentPlayerTurnStage(PlayerType.Cozmo, false);

      _StateMachine.PushSubState(new CozmoMoveCloserToCubesState(null, false, 40.0f, 20.0f, false));

      _CurrentRobot.OnLightCubeRemoved += HandleOnCubeRemoved;
    }

    public override void Exit() {
      base.Exit();
      if (_CurrentRobot != null) {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
        _CurrentRobot.OnLightCubeRemoved -= HandleOnCubeRemoved;
      }
    }

    // We want to kill the game... but Cozmo should just stop when done with current action.
    // theres no error state to go to and people should just read the error message.
    private void HandleOnCubeRemoved(LightCube cube) {
      _CubeLost = true;
    }

    public override void Update() {
      base.Update();
      if (_IsAnimating || _CubeLost) {
        return;
      }
      if (_ShouldWinGame.HasValue) {
        if (_ShouldWinGame.GetValueOrDefault()) {
          CozmoWinHand();
        }
        else {
          CozmoLoseHand();
        }
      }
      else {
        _CurrentSequenceIndex++;
        if (_CurrentSequenceIndex >= _GameInstance.GetCurrentSequenceLength()) {
          _ShouldWinGame = true;
        }
        else {
          float rand = Random.Range(0f, 1f);
          // Cozmo always right first time
          if (_GameInstance.GetCurrentTurnNumber > 1 && rand > _GameInstance.CozmoWinPercentage.Evaluate(_CurrentSequenceIndex)) {
            _ShouldWinGame = false;
            int correctId = _GameInstance.GetIDInSequence(_CurrentSequenceIndex);
            List<int> blockIds = new List<int>();
            foreach (int cubeId in _GameInstance.CubeIdsForGame) {
              if (cubeId != correctId) {
                blockIds.Add(cubeId);
              }
            }
            _LastTargetID = blockIds[Random.Range(0, blockIds.Count)];
            StartTurnToTarget(_CurrentRobot.LightCubes[_LastTargetID], false);
          }
          else {
            _LastTargetID = _GameInstance.GetIDInSequence(_CurrentSequenceIndex);
            StartTurnToTarget(GetCurrentTarget(), true);
          }
        }
      }
    }

    private void StartTurnToTarget(LightCube target, bool isCorrect) {
      if (target == null) {
        return;
      }
      _CurrentRobot.SetAllBackpackBarLED(_GameInstance.GetColorForBlock(target.ID));

      int goingToIndex = _GameInstance.CubeIdsForGame.IndexOf(target.ID);
      int fromIndex = 1;
      if (_CurrentSequenceIndex > 0) {
        fromIndex = _GameInstance.CubeIdsForGame.IndexOf(_GameInstance.GetIDInSequence(_CurrentSequenceIndex - 1));
      }
      Anki.Cozmo.AnimationTrigger animTrigger = Anki.Cozmo.AnimationTrigger.MemoryMatchPointCenterFast;
      if (goingToIndex - fromIndex == 1) {
        animTrigger = Anki.Cozmo.AnimationTrigger.MemoryMatchPointLeftSmallFast;
      }
      else if (goingToIndex - fromIndex == -1) {
        animTrigger = Anki.Cozmo.AnimationTrigger.MemoryMatchPointRightSmallFast;
      }
      if (goingToIndex - fromIndex >= 2) {
        animTrigger = Anki.Cozmo.AnimationTrigger.MemoryMatchPointLeftBigFast;
      }
      else if (goingToIndex - fromIndex <= -2) {
        animTrigger = Anki.Cozmo.AnimationTrigger.MemoryMatchPointRightBigFast;
      }

      _StateMachine.PushSubState(new CozmoBlinkLightsMemoryMatchState(target, animTrigger, isCorrect));
    }

    public LightCube GetCurrentTarget() {
      if (_CurrentRobot != null && _CurrentRobot.LightCubes.ContainsKey(_LastTargetID)) {
        return _CurrentRobot.LightCubes[_LastTargetID];
      }
      return null;
    }

    private void CozmoLoseHand() {
      _GameInstance.SetCubeLightsGuessWrong(_LastTargetID);
      _GameInstance.DecrementLivesRemaining(PlayerType.Cozmo);
      _IsAnimating = true;

      // Since the next state will play the FinalLifeComplete "lose game" don't double it up with the lose hand and skip lose hand
      if (_GameInstance.GetLivesRemaining(PlayerType.Cozmo) > 0) {
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.MemoryMatchCozmoLoseHand, HandleOnCozmoLoseAnimationDone);
      }
      else {
        HandleOnCozmoLoseAnimationDone(true);
      }
    }

    private void CozmoWinHand() {
      _GameInstance.ShowCenterResult(true, true);
      _GameInstance.AddPoint(false);
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Round_End);
      if (_GameInstance.GetLivesRemaining(PlayerType.Human) > 0) {
        _GameInstance.SetCubeLightsGuessRight();
        _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.MemoryMatchCozmoWinHand, HandleOnCozmoWinAnimationDone);
      }
      else {
        // we're going to play MemoryMatchCozmoWinGame soon.
        HandleOnCozmoWinAnimationDone(true);
      }
      _IsAnimating = true;
    }

    private void HandleOnCozmoWinAnimationDone(bool success) {
      _GameInstance.ShowCenterResult(false);
      _StateMachine.SetNextState(new WaitForNextRoundMemoryMatchState(PlayerType.Human, true));
    }

    private void HandleOnCozmoLoseAnimationDone(bool success) {
      _GameInstance.ShowCenterResult(false);
      if (_GameInstance.GetLivesRemaining(PlayerType.Cozmo) > 0) {
        _StateMachine.SetNextState(new WaitForNextRoundMemoryMatchState(PlayerType.Cozmo));
      }
      else {
        _StateMachine.SetNextState(new WaitForNextRoundMemoryMatchState(PlayerType.Human, true));
      }
    }
  }
}
