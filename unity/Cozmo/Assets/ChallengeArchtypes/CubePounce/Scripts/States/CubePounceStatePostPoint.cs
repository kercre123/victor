using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStatePostPoint : CubePounceState {

    bool _CozmoWon = false;

    public CubePounceStatePostPoint(bool cozmoWon) {
      _CozmoWon = cozmoWon;
    }

    public override void Enter() {
      base.Enter();

      if (_CozmoWon) {
        _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderCozmoWinPoint);
        _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoCozmoWinPoint);
        _CubePounceGame.CozmoScore++;
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SharedLose);
        _CubePounceGame.StartCycleCubeSingleColor(_CubePounceGame.GetCubeTarget().ID, new Color[]{Color.red}, CubePounceGame.kCubeLightFlashInterval_m, Color.black);
      }
      else {
        _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderPlayerWinPoint);
        _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoPlayerWinPoint);
        _CubePounceGame.PlayerScore++;
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SharedWin);
        _CubePounceGame.StartCycleCubeSingleColor(_CubePounceGame.GetCubeTarget().ID, new Color[]{Color.green}, CubePounceGame.kCubeLightFlashInterval_m, Color.black);
      }

      bool roundIsOver = _CubePounceGame.CheckAndUpdateRoundScore();
      if (roundIsOver) {
        DoRoundEndLogic();
      }
      else {
        if (_CozmoWon) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceWinHand, HandleEndHandAnimFinish);
        }
        else {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceLoseHand, HandleEndHandAnimFinish);
        }
      }

      _CubePounceGame.UpdateScoreboard();
    }

    private void DoRoundEndLogic() {
      if (_CubePounceGame.AllRoundsCompleted) {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedEnd);
        if (_CubePounceGame.CozmoRoundsWon > _CubePounceGame.PlayerRoundsWon) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceWinSession, HandleEndGameAnimFinish);
        }
        else {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceLoseSession, HandleEndGameAnimFinish);
        }
      }
      else {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedRoundEnd);
        GameAudioClient.SetMusicState(_CubePounceGame.BetweenRoundsMusic);
        if (_CubePounceGame.CozmoScore > _CubePounceGame.PlayerScore) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceWinRound, HandleEndRoundAnimFinish);
        }
        else {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceLoseRound, HandleEndRoundAnimFinish);
        }
      }
    }

    private void HandleEndHandAnimFinish(bool success) {
      _StateMachine.SetNextState(new CubePounceStateResetPoint());
    }

    private void HandleEndRoundAnimFinish(bool success) {
      _StateMachine.SetNextState(new CubePounceStateInitGame());
    }

    private void HandleEndGameAnimFinish(bool success) {
      _CubePounceGame.StopCycleCube(_CubePounceGame.GetCubeTarget().ID);
      if (_CozmoWon) {
        _CubePounceGame.RaiseMiniGameLose();
      }
      else {
        _CubePounceGame.RaiseMiniGameWin();
      }
    }
  }
}
