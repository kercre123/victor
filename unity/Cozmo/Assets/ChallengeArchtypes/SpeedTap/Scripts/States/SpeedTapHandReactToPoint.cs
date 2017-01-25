using UnityEngine;
using System.Collections;
using Anki.Cozmo;
using Anki.Cozmo.Audio;

namespace SpeedTap {
  public enum PointWinner {
    Cozmo,
    Player
  }

  public class SpeedTapHandReactToPoint : State {

    private enum CubeLightState {
      WinnerCycling,
      WinnerFlashing,
      LoserFlashing,
      None
    }

    private SpeedTapGame _SpeedTapGame;

    private PointWinner _CurrentWinner;
    private bool _WasMistakeMade;

    private LightCube _WinningCube;

    private bool _EndRobotAnimFinished = false;
    private bool _EndCubeAnimFinished = false;

    private bool _IsPlayingWinGameAnimation = false;

    public SpeedTapHandReactToPoint(PointWinner winner, bool mistakeMade) {
      _CurrentWinner = winner;
      _WasMistakeMade = mistakeMade;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      _SpeedTapGame.AddPoint(_CurrentWinner == PointWinner.Player);
      // Count towards player mistake if cozmo wins a point off of the player tapping wrong.
      if (_WasMistakeMade) {
        if (_CurrentWinner == PointWinner.Player) {
          _SpeedTapGame.CozmoMistake();
        }
        else {
          _SpeedTapGame.PlayerMistake();
        }
      }
      // Depends on points being scored first
      _SpeedTapGame.UpdateUI();

      if (_SpeedTapGame.IsRoundComplete()) {
        GameAudioClient.SetMusicState(_SpeedTapGame.BetweenRoundsMusic);
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Round_End);

        _SpeedTapGame.EndCurrentRound();
        // Hide Current Round in between rounds
        _SpeedTapGame.SharedMinigameView.InfoTitleText = string.Empty;

        if (_SpeedTapGame.IsGameComplete()) {
          UpdateBlockLights(_CurrentWinner, wasMistakeMade: false);
          _SpeedTapGame.UpdateUIForGameEnd();
          PlayReactToGameAnimationAndSendEvent();
        }
        else {
          UpdateBlockLights(_CurrentWinner, _WasMistakeMade);
          _StateMachine.SetNextState(new SpeedTapReactToRoundEnd(_CurrentWinner));
        }
      }
      else {
        UpdateBlockLights(_CurrentWinner, _WasMistakeMade);
        PlayReactToHandAnimation();
      }
    }

    public override void Exit() {
      base.Exit();
    }

    public override void Pause(PauseReason reason, Anki.Cozmo.ReactionTrigger reactionaryBehavior) {
      // COZMO-2033; some of the win game animations cause Cozmo's cliff sensor to trigger
      // So in those cases don't show the "Cozmo Moved; Quit Game" dialog
      if (!_IsPlayingWinGameAnimation) {
        base.Pause(reason, reactionaryBehavior);
      }
    }

    private void UpdateBlockLights(PointWinner winner, bool wasMistakeMade) {
      LightCube losingBlock;
      if (winner == PointWinner.Player) {
        _WinningCube = _SpeedTapGame.GetPlayerBlock();
        losingBlock = _SpeedTapGame.GetCozmoBlock();
      }
      else {
        _WinningCube = _SpeedTapGame.GetCozmoBlock();
        losingBlock = _SpeedTapGame.GetPlayerBlock();
      }

      if (wasMistakeMade) {
        _WinningCube.SetLEDsOff();
        _CurrentRobot.PlayCubeAnimationTrigger(losingBlock, CubeAnimationTrigger.SpeedTapLose, (success) => { _EndCubeAnimFinished = true; HandleHandEndAnimDone(success); });
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Lose);
      }
      else {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_St_Win);
        losingBlock.SetLEDsOff();
        _CurrentRobot.PlayCubeAnimationTrigger(_WinningCube, CubeAnimationTrigger.SpeedTapWin, (success) => { _EndCubeAnimFinished = true; HandleHandEndAnimDone(success); });
      }
    }

    private void PlayReactToHandAnimation() {
      AnimationTrigger animationEventToSend = (_CurrentWinner == PointWinner.Player) ?
              AnimationTrigger.OnSpeedtapHandPlayerWin : AnimationTrigger.OnSpeedtapHandCozmoWin;
      ListenForAnimationEnd(animationEventToSend, (success) => { _EndRobotAnimFinished = true; HandleHandEndAnimDone(success); });
    }

    private void PlayReactToGameAnimationAndSendEvent() {
      AnimationTrigger animationEventToSend = AnimationTrigger.Count;
      bool highIntensity = _SpeedTapGame.IsHighIntensityGame();
      if (DebugMenuManager.Instance.DemoMode) {
        animationEventToSend = (_CurrentWinner == PointWinner.Player) ?
                            AnimationTrigger.DemoSpeedTapCozmoLose : AnimationTrigger.DemoSpeedTapCozmoWin;
      }
      else {
        if (_CurrentWinner == PointWinner.Player) {
          animationEventToSend = (highIntensity) ?
                    AnimationTrigger.OnSpeedtapGamePlayerWinHighIntensity : AnimationTrigger.OnSpeedtapGamePlayerWinLowIntensity;
        }
        else {
          animationEventToSend = (highIntensity) ?
                    AnimationTrigger.OnSpeedtapGameCozmoWinHighIntensity : AnimationTrigger.OnSpeedtapGameCozmoWinLowIntensity;
        }
      }
      _IsPlayingWinGameAnimation = true;
      ListenForAnimationEnd(animationEventToSend, HandleEndGameAnimDone);
    }

    private void ListenForAnimationEnd(AnimationTrigger animEvent, RobotCallback animationCallback) {
      _CurrentRobot.SendAnimationTrigger(animEvent, animationCallback);
    }

    private void HandleHandEndAnimDone(bool success) {
      if (_EndCubeAnimFinished && _EndRobotAnimFinished) {
        _StateMachine.SetNextState(new SpeedTapHandCubesOff());
      }
    }

    private void HandleEndGameAnimDone(bool success) {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Shared_Game_End);
      _SpeedTapGame.ClearWinningLightPatterns();
      _SpeedTapGame.StartRoundBasedGameEnd();
      _SpeedTapGame.StartEndMusic();
    }
  }
}
