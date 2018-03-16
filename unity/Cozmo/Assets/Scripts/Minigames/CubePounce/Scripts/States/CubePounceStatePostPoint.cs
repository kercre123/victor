using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;

namespace Cozmo.Challenge.CubePounce {
  public class CubePounceStatePostPoint : CubePounceState {

    bool _CozmoWon = false;

    public CubePounceStatePostPoint(bool cozmoWon) {
      _CozmoWon = cozmoWon;
    }

    private Anki.AudioMetaData.GameState.Music _NextMusicState = Anki.AudioMetaData.GameState.Music.Invalid;

    private Anki.Cozmo.CubeAnimationTrigger _CubeAnim;

    public const string kCubePounceIdle = "cube_pounce_idle";

    public override void Enter() {
      base.Enter();

      GameAudioClient.SetMusicState(Anki.AudioMetaData.GameState.Music.Minigame__Keep_Away_Pounce);

      _CubePounceGame.ResetPounceChance();

      if (_CozmoWon) {
        _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderCozmoWinPoint);
        _CubePounceGame.AddPoint(false);
        _CubeAnim = Anki.Cozmo.CubeAnimationTrigger.CubePouncePlayerLose;
      }
      else {
        _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderPlayerWinPoint);
        _CubePounceGame.AddPoint(true);
        _CubeAnim = Anki.Cozmo.CubeAnimationTrigger.CubePouncePlayerWin;
      }

      _CubePounceGame.GetCubeTarget().PlayAnim(_CubeAnim);

      bool roundIsOver = _CubePounceGame.CheckAndUpdateRoundScore();
      if (roundIsOver) {
        DoRoundEndLogic();
      }
      else {
        _NextMusicState = Anki.AudioMetaData.GameState.Music.Minigame__Keep_Away_Between_Rounds;
        if (_CozmoWon) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceWinHand, HandleEndHandAnimFinish);
        }
        else {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceLoseHand, HandleEndHandAnimFinish);
        }

        // play the "score" sound, but skip it when the round is over (because we're playing another sound for that)
        Anki.AudioMetaData.GameEvent.Sfx evt = _CozmoWon ? Anki.AudioMetaData.GameEvent.Sfx.Gp_St_Lose : Anki.AudioMetaData.GameEvent.Sfx.Gp_St_Win;
        GameAudioClient.PostSFXEvent(evt);
      }

      _CubePounceGame.UpdateScoreboard();
    }

    private void DoRoundEndLogic() {
      if (_CubePounceGame.AllRoundsCompleted) {
        _NextMusicState = Anki.AudioMetaData.GameState.Music.Minigame__Setup;
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Game_End);

        if (_CubePounceGame.CozmoRoundsWon > _CubePounceGame.HumanRoundsWon) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceWinSession, HandleEndGameAnimFinish);
        }
        else {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceLoseSession, HandleEndGameAnimFinish);
        }
      }
      else {
        _NextMusicState = Anki.AudioMetaData.GameState.Music.Minigame__Keep_Away_Between_Rounds;
        GameAudioClient.PostSFXEvent(Anki.AudioMetaData.GameEvent.Sfx.Gp_Shared_Round_End);

        if (_CubePounceGame.CozmoScore > _CubePounceGame.HumanScore) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceWinRound, HandleEndRoundAnimFinish);
        }
        else {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceLoseRound, HandleEndRoundAnimFinish);
        }
      }
    }

    private void HandleEndHandAnimFinish(bool success) {
      _CubePounceGame.GetCubeTarget().StopAnim(_CubeAnim);
      _StateMachine.SetNextState(new CubePounceStateResetPoint());
    }

    private void HandleEndRoundAnimFinish(bool success) {
      _CubePounceGame.GetCubeTarget().StopAnim(_CubeAnim);
      _StateMachine.SetNextState(new CubePounceStateInitGame());
    }

    private void HandleEndGameAnimFinish(bool success) {
      _CubePounceGame.GetCubeTarget().StopAnim(_CubeAnim);
      _CubePounceGame.StartRoundBasedGameEnd();
      _CubePounceGame.UpdateUIForGameEnd();
      if (_CurrentRobot != null) {
        _CurrentRobot.TryResetHeadAndLift(null);
        _CurrentRobot.PushIdleAnimation(Anki.Cozmo.AnimationTrigger.CubePounceIdleLiftDown, kCubePounceIdle);
      }
    }

    public override void Exit() {
      base.Exit();
      // Update next music state
      if (Anki.AudioMetaData.GameState.Music.Invalid != _NextMusicState) {
        GameAudioClient.SetMusicState(_NextMusicState);
        _NextMusicState = Anki.AudioMetaData.GameState.Music.Invalid;
      }

      if (_CurrentRobot != null) {
        _CurrentRobot.CancelCallback(HandleEndHandAnimFinish);
        _CurrentRobot.CancelCallback(HandleEndRoundAnimFinish);
        _CurrentRobot.CancelCallback(HandleEndGameAnimFinish);

        _CurrentRobot.SetEnableCliffSensor(true);
      }
    }
  }
}
