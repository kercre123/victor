using UnityEngine;
using Anki.Cozmo.Audio;
using System.Collections;

namespace Cozmo.Minigame.CubePounce {
  public class CubePounceStatePostPoint : CubePounceState {

    bool _CozmoWon = false;

    public CubePounceStatePostPoint(bool cozmoWon) {
      _CozmoWon = cozmoWon;
    }

    private Anki.Cozmo.Audio.GameState.Music _nextMusicState = Anki.Cozmo.Audio.GameState.Music.Invalid;

    private Anki.Cozmo.CubeAnimationTrigger _cubeAnim;

    public override void Enter() {
      base.Enter();

      GameAudioClient.SetMusicState(Anki.Cozmo.Audio.GameState.Music.Minigame__Keep_Away_Pounce);

      _CubePounceGame.ResetPounceChance();

      if (_CozmoWon) {
        _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderCozmoWinPoint);
        _CubePounceGame.AddPoint(false);
        _cubeAnim = Anki.Cozmo.CubeAnimationTrigger.CubePouncePlayerLose;
      }
      else {
        _CubePounceGame.SharedMinigameView.InfoTitleText = Localization.Get(LocalizationKeys.kCubePounceHeaderPlayerWinPoint);
        _CubePounceGame.AddPoint(true);
        _cubeAnim = Anki.Cozmo.CubeAnimationTrigger.CubePouncePlayerWin;
      }

      _CubePounceGame.GetCubeTarget().PlayAnim(_cubeAnim);

      bool roundIsOver = _CubePounceGame.CheckAndUpdateRoundScore();
      if (roundIsOver) {
        DoRoundEndLogic();
      }
      else {
        _nextMusicState = Anki.Cozmo.Audio.GameState.Music.Minigame__Keep_Away_Between_Rounds;
        if (_CozmoWon) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceWinHand, HandleEndHandAnimFinish);
        }
        else {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceLoseHand, HandleEndHandAnimFinish);
        }

        // play the "score" sound, but skip it when the round is over (because we're playing another sound for that)
        Anki.Cozmo.Audio.GameEvent.Sfx evt = _CozmoWon ? Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Lose : Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Win;
        Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(evt);
      }

      _CubePounceGame.UpdateScoreboard();
    }

    private void DoRoundEndLogic() {
      if (_CubePounceGame.AllRoundsCompleted) {
        _nextMusicState = Anki.Cozmo.Audio.GameState.Music.Minigame__Setup;
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Game_End);

        if (_CubePounceGame.CozmoRoundsWon > _CubePounceGame.HumanRoundsWon) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceWinSession, HandleEndGameAnimFinish);
        }
        else {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceLoseSession, HandleEndGameAnimFinish);
        }
      }
      else {
        _nextMusicState = Anki.Cozmo.Audio.GameState.Music.Minigame__Keep_Away_Between_Rounds;
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Round_End);

        if (_CubePounceGame.CozmoScore > _CubePounceGame.HumanScore) {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceWinRound, HandleEndRoundAnimFinish);
        }
        else {
          _CurrentRobot.SendAnimationTrigger(Anki.Cozmo.AnimationTrigger.CubePounceLoseRound, HandleEndRoundAnimFinish);
        }
      }
    }

    private void HandleEndHandAnimFinish(bool success) {
      _CubePounceGame.GetCubeTarget().StopAnim(_cubeAnim);
      _StateMachine.SetNextState(new CubePounceStateResetPoint());
    }

    private void HandleEndRoundAnimFinish(bool success) {
      _CubePounceGame.GetCubeTarget().StopAnim(_cubeAnim);
      _StateMachine.SetNextState(new CubePounceStateInitGame());
    }

    private void HandleEndGameAnimFinish(bool success) {
      _CubePounceGame.GetCubeTarget().StopAnim(_cubeAnim);
      _CubePounceGame.StartRoundBasedGameEnd();
      _CubePounceGame.UpdateUIForGameEnd();
      _CurrentRobot.TryResetHeadAndLift(null);
    }

    public override void Exit() {
      base.Exit();
      // Update next music state
      if (Anki.Cozmo.Audio.GameState.Music.Invalid != _nextMusicState) {
        GameAudioClient.SetMusicState(_nextMusicState);
        _nextMusicState = Anki.Cozmo.Audio.GameState.Music.Invalid;
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
