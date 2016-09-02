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

      _CubePounceGame.ResetPounceChance();

      if (_CozmoWon) {
        _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderCozmoWinPoint);
        _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoCozmoWinPoint);
        _CubePounceGame.AddPoint(false);
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Lose_Shared);
        _CubePounceGame.StartCycleCubeSingleColor(_CubePounceGame.GetCubeTarget().ID, new Color[] { Color.red }, _CubePounceGame.GameConfig.CubeLightFlashInterval_s, Color.black);

        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Keep_Away_Pounce);
      }
      else {
        _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderPlayerWinPoint);
        _CubePounceGame.SharedMinigameView.ShowNarrowInfoTextSlideWithKey(LocalizationKeys.kCubePounceInfoPlayerWinPoint);
        _CubePounceGame.AddPoint(true);
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Win_Shared);
        _CubePounceGame.StartCycleCubeSingleColor(_CubePounceGame.GetCubeTarget().ID, new Color[] { Color.green }, _CubePounceGame.GameConfig.CubeLightFlashInterval_s, Color.white);

        Anki.Cozmo.Audio.GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Keep_Away_Player_Fail);
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
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Game_End);
        if (_CubePounceGame.CozmoRoundsWon > _CubePounceGame.PlayerRoundsWon) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceWinSession, HandleEndGameAnimFinish);
        }
        else {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceLoseSession, HandleEndGameAnimFinish);
        }
      }
      else {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Round_End);
        GameAudioClient.SetMusicState(_CubePounceGame.GameConfig.BetweenRoundMusic);
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
      _CubePounceGame.StartRoundBasedGameEnd();
    }

    public override void Exit() {
      base.Exit();
      _CurrentRobot.CancelCallback(HandleEndHandAnimFinish);
      _CurrentRobot.CancelCallback(HandleEndRoundAnimFinish);
      _CurrentRobot.CancelCallback(HandleEndGameAnimFinish);
    }
  }
}
