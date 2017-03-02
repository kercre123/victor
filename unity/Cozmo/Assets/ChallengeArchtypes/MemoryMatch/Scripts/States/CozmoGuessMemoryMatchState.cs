using UnityEngine;
using System.Collections.Generic;
using Anki.Cozmo;

namespace MemoryMatch {
  public class CozmoGuessMemoryMatchState : State {

    private MemoryMatchGame _GameInstance;
    private IList<int> _CurrentSequence;
    private int _CurrentSequenceIndex;
    private bool? _ShouldWinGame;
    private int _LastTargetID;
    private bool _IsAnimating = false;
    private bool _CubeLost = false;

    class MemoryMatchTurnTrigger {
      public AnimationTrigger left;
      public AnimationTrigger right;
      public int maxDegreeTurn;
      public MemoryMatchTurnTrigger(int setMaxDegree, AnimationTrigger setLeft, AnimationTrigger setRight) {
        left = setLeft;
        right = setRight;
        maxDegreeTurn = setMaxDegree;
      }
    }

    // Tweaking to look right in context.
    MemoryMatchTurnTrigger[] _Thresholds = new MemoryMatchTurnTrigger[] {
                  new MemoryMatchTurnTrigger(10,AnimationTrigger.MemoryMatchPointLeftSmallFast,AnimationTrigger.MemoryMatchPointRightSmallFast),
                  new MemoryMatchTurnTrigger(15,AnimationTrigger.MemoryMatchPointLeft1,AnimationTrigger.MemoryMatchPointRight1),
                  new MemoryMatchTurnTrigger(30,AnimationTrigger.MemoryMatchPointLeft2,AnimationTrigger.MemoryMatchPointRight2),
                  new MemoryMatchTurnTrigger(45,AnimationTrigger.MemoryMatchPointLeft3,AnimationTrigger.MemoryMatchPointRight3),
                  new MemoryMatchTurnTrigger(60,AnimationTrigger.MemoryMatchPointLeft4,AnimationTrigger.MemoryMatchPointRight4),
                  new MemoryMatchTurnTrigger(75,AnimationTrigger.MemoryMatchPointLeftBigFast,AnimationTrigger.MemoryMatchPointRightBigFast),
        };

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as MemoryMatchGame;
      _CurrentSequence = _GameInstance.GetCurrentSequence();
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
            StartTurnToTarget(_CurrentRobot.LightCubes[_LastTargetID], false);
          }
          else {
            _LastTargetID = _CurrentSequence[_CurrentSequenceIndex];
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
        fromIndex = _GameInstance.CubeIdsForGame.IndexOf(_CurrentSequence[_CurrentSequenceIndex - 1]);
      }

      LightCube fromCube = _CurrentRobot.LightCubes[_GameInstance.CubeIdsForGame[fromIndex]];

      // Because we let them set up at different degrees and animators want cozmo to look different
      // based on distance.
      AnimationTrigger animTrigger = AnimationTrigger.MemoryMatchPointCenterFast;
      if (goingToIndex != fromIndex) {
        Vector3 toFrom = fromCube.WorldPosition - _CurrentRobot.WorldPosition;
        Vector3 toTarget = target.WorldPosition - _CurrentRobot.WorldPosition;
        float angleBetween = Vector3.Angle(toFrom, toTarget);
        // unsigned selectIndex gets degrees we want. Each threshold has a left and right variation.
        // direction determined by sign of goingToIndex - fromIndex
        int selectIndex = _Thresholds.Length - 1;
        for (int i = 0; i < _Thresholds.Length; ++i) {
          if (angleBetween < _Thresholds[i].maxDegreeTurn) {
            selectIndex = i;
            break;
          }
        }
        // moving to cozmo's left
        if (goingToIndex - fromIndex > 0) {
          animTrigger = _Thresholds[selectIndex].left;
        }
        else {
          // moving to cozmo's right.
          animTrigger = _Thresholds[selectIndex].right;
        }
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
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Round_End);
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
