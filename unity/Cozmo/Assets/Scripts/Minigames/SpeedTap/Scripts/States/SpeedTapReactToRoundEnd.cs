using UnityEngine;
using Anki.Cozmo;
using System.Collections.Generic;

namespace SpeedTap {
  public class SpeedTapReactToRoundEnd : State {

    private SpeedTapGame _SpeedTapGame;

    private PlayerInfo _Winner;

    private bool _BannerAnimDone = false;
    private bool _RobotAnimDone = false;

    public SpeedTapReactToRoundEnd(PlayerInfo winner) {
      _Winner = winner;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      // Show round end slide
      GameObject roundEndSlide = _SpeedTapGame.SharedMinigameView.ShowWideGameStateSlide(
                                   _SpeedTapGame.SpeedTapRoundEndSlidePrefab.gameObject, "speedTap_round_end_slide");
      SpeedTapRoundEndSlide roundEndSlideScript = roundEndSlide.GetComponent<SpeedTapRoundEndSlide>();

      _SpeedTapGame.WantsHideRoundLabel = true;

      // Show current winner
      Sprite winnerPortrait = null;
      string winnerName = _Winner.name;
      int roundsWinnerWon = _Winner.playerRoundsWon;
      Color winnerColor = ((SpeedTapPlayerInfo)_Winner).scoreWidget.UnDimmedPortaitTintColor;
      if (_Winner.playerType == PlayerType.Cozmo) {
        winnerPortrait = _SpeedTapGame.SharedMinigameView.CozmoPortrait;
      }
      else {
        winnerPortrait = _SpeedTapGame.SharedMinigameView.PlayerPortrait;
        // In SinglePlayer we want it just to say "player" not the profile name because of how it was before.
        if (_SpeedTapGame.GetPlayerCount() < 3) {
          winnerName = Localization.Get(LocalizationKeys.kNamePlayer);
        }
      }
      int roundsNeeded = _SpeedTapGame.RoundsNeededToWin;
      roundEndSlideScript.Initialize(winnerPortrait, winnerName, roundsWinnerWon, roundsNeeded, _SpeedTapGame.RoundsPlayed, winnerColor);

      // Play banner animation with score
      string bannerText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapTextRoundScore,
                            _SpeedTapGame.HumanRoundsWon, _SpeedTapGame.CozmoRoundsWon);
      if (_SpeedTapGame.GetPlayerCount() > 2) {
        List<object> fillArgs = new List<object>();
        int playerCount = _SpeedTapGame.GetPlayerCount();
        for (int i = 0; i < playerCount; ++i) {
          PlayerInfo playerInfo = _SpeedTapGame.GetPlayerByIndex(i);
          fillArgs.Add(playerInfo.name);
          fillArgs.Add(playerInfo.playerRoundsWon);
        }
        bannerText = Localization.GetWithArgs(LocalizationKeys.kSpeedTapTextRoundScoreMP, fillArgs.ToArray());
        _SpeedTapGame.SharedMinigameView.PlayBannerAnimation(bannerText, HandleBannerAnimDone,
                _SpeedTapGame.MPTimeBetweenRoundsSec);
      }
      else {
        // we want single player to go fast, so don't care if the banner is done or not
        _BannerAnimDone = true;
        _SpeedTapGame.SharedMinigameView.PlayBannerAnimation(bannerText, null,
                        roundEndSlideScript.BannerAnimationDurationSeconds);
      }


      ContextManager.Instance.AppFlash(playChime: true);
      // Play cozmo animation
      PlayReactToRoundAnimationAndSendEvent();
    }

    public override void Exit() {
      base.Exit();

      _SpeedTapGame.WantsHideRoundLabel = false;
      // re-enable react to pickup during the next round
      EnableExtraReactions(true);
    }

    public override void Pause(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
      // COZMO-2033; some of the win game animations cause Cozmo's cliff sensor to trigger
      // So in those cases don't show the "Cozmo Moved; Quit Game" dialog
      // Do nothing
    }
    private void EnableExtraReactions(bool enabled) {
      if (_CurrentRobot != null) {
        if (!enabled) {
          _CurrentRobot.DisableReactionsWithLock(ReactionaryBehaviorEnableGroups.kSpeedTapRoundEndId, ReactionaryBehaviorEnableGroups.kSpeedTapRoundEndTriggers);
        }
        else {
          _CurrentRobot.RemoveDisableReactionsLock(ReactionaryBehaviorEnableGroups.kSpeedTapRoundEndId);
        }

        // breaking out the big hammer. This state make cozmo lift himself up and then he drives back within the animation
        // so it's important we don't get a firmware stop.
        _CurrentRobot.SetEnableCliffSensor(enabled);
      }
    }

    private void PlayReactToRoundAnimationAndSendEvent() {
      // Don't listen to pickup in case cozmo lifts himself on top of the cube for an animation
      EnableExtraReactions(false);

      AnimationTrigger animEvent = AnimationTrigger.Count;
      bool highIntensity = _SpeedTapGame.IsHighIntensityRound();
      if (_Winner.playerType != PlayerType.Cozmo) {
        animEvent = (highIntensity) ?
                  AnimationTrigger.OnSpeedtapRoundPlayerWinHighIntensity : AnimationTrigger.OnSpeedtapRoundPlayerWinLowIntensity;
      }
      else {
        animEvent = (highIntensity) ?
                  AnimationTrigger.OnSpeedtapRoundCozmoWinHighIntensity : AnimationTrigger.OnSpeedtapRoundCozmoWinLowIntensity;
      }
      _CurrentRobot.SendAnimationTrigger(animEvent, HandleRoundEndAnimDone);
    }

    private void HandleRoundEndAnimDone(bool success) {
      _RobotAnimDone = true;
      MoveToNextState();
    }

    private void HandleBannerAnimDone() {
      _BannerAnimDone = true;
      MoveToNextState();
    }

    private void MoveToNextState() {
      if (_BannerAnimDone && _RobotAnimDone) {
        _SpeedTapGame.SharedMinigameView.HideGameStateSlide();

        _SpeedTapGame.ClearWinningLightPatterns();
        _StateMachine.SetNextState(new SpeedTapCubeSelectionState());
      }
    }
  }
}