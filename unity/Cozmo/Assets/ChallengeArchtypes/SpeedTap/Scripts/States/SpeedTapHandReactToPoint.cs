using UnityEngine;
using System.Collections;
using Anki.Cozmo.Audio;

namespace SpeedTap {
  public enum PointWinner {
    Cozmo,
    Player
  }

  public class SpeedTapHandReactToPoint : State {

    private const float _kWinCycleSpeed = 0.1f;

    private SpeedTapGame _SpeedTapGame;

    private PointWinner _CurrentWinner;
    private bool _WasMistakeMade;

    public SpeedTapHandReactToPoint(PointWinner winner, bool mistakeMade) {
      _CurrentWinner = winner;
      _WasMistakeMade = mistakeMade;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.EndCozmoPickedUpDisruptionDetection();
      _SpeedTapGame.EndCozmoCubeMovedDisruptionDetection();

      Color[] winColors;
      LightCube winningBlock;
      LightCube losingBlock;
      if (_CurrentWinner == PointWinner.Player) {
        _SpeedTapGame.AddPlayerPoint();
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapWin);
        winColors = _SpeedTapGame.PlayerWinColors;
        winningBlock = _SpeedTapGame.PlayerBlock;
        losingBlock = _SpeedTapGame.CozmoBlock;
      }
      else {
        _SpeedTapGame.AddCozmoPoint();
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLose);
        winColors = _SpeedTapGame.CozmoWinColors;
        winningBlock = _SpeedTapGame.CozmoBlock;
        losingBlock = _SpeedTapGame.PlayerBlock;
      }

      _SpeedTapGame.UpdateUI();

      SetWinningLightPattern(winningBlock, winColors);
      SetLosingLightPattern(losingBlock, _WasMistakeMade);

      if (_SpeedTapGame.IsRoundComplete()) {
        HandleRoundEnd();
      }
      else {
        ReactToHand();
      }
    }

    public override void Exit() {
      base.Exit();
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapHandCozmoWin, HandleHandEndAnimDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapHandPlayerWin, HandleHandEndAnimDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapRoundCozmoWin, HandleRoundEndAnimDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapRoundPlayerWin, HandleRoundEndAnimDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapGameCozmoWin, HandleEndGameAnimDone);
      AnimationManager.Instance.RemoveAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapGamePlayerWin, HandleEndGameAnimDone);
    }

    private void SetWinningLightPattern(LightCube winningBlock, Color[] winColors) {
      if (winColors[0] == Color.white) {
        _SpeedTapGame.StartCycleCubeSingleColor(winningBlock, winColors, _kWinCycleSpeed, Color.black);
      }
      else {
        _SpeedTapGame.StartCycleCubeSingleColor(winningBlock, winColors, _kWinCycleSpeed, Color.white);
      }
    }

    private void SetLosingLightPattern(LightCube losingBlock, bool madeMistake) {
      if (madeMistake) {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapRed);
        _SpeedTapGame.StartCycleCubeSingleColor(losingBlock, new Color[] { Color.red }, _kWinCycleSpeed, Color.black);
      }
      else {
        losingBlock.SetLEDs(0);
      }
    }

    private void ClearWinningLightPatterns() {
      _SpeedTapGame.StopCycleCube(_SpeedTapGame.PlayerBlock);
      _SpeedTapGame.PlayerBlock.SetLEDsOff();
      _SpeedTapGame.StopCycleCube(_SpeedTapGame.CozmoBlock);
      _SpeedTapGame.PlayerBlock.SetLEDsOff();
    }

    private void ReactToHand() {
      if (_CurrentWinner == PointWinner.Player) {
        // TODO add event listener
        AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapHandPlayerWin, HandleHandEndAnimDone);
        GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapHandPlayerWin);
      }
      else {
        // TODO add event listener
        AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapHandCozmoWin, HandleHandEndAnimDone);
        GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapHandCozmoWin);
      }
    }

    private void HandleRoundEnd() {
      GameAudioClient.SetMusicState(_SpeedTapGame.BetweenRoundsMusic);
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedRoundEnd);

      _SpeedTapGame.UpdateRoundScore();
      _SpeedTapGame.UpdateUI();

      if (_SpeedTapGame.IsGameComplete()) {
        _SpeedTapGame.UpdateUIForGameEnd();
        if (_CurrentWinner == PointWinner.Player) {
          // TODO add event listener
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapGamePlayerWin, HandleEndGameAnimDone);
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapGamePlayerWin);
        }
        else {
          // TODO add event listener
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapGameCozmoWin, HandleEndGameAnimDone);
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapGameCozmoWin);
        }
      }
      else {
        if (_CurrentWinner == PointWinner.Player) {
          // TODO add event listener
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapRoundPlayerWin, HandleRoundEndAnimDone);
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapRoundPlayerWin);
        }
        else {
          // TODO add event listener
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapRoundCozmoWin, HandleRoundEndAnimDone);
          GameEventManager.Instance.SendGameEventToEngine(Anki.Cozmo.GameEvent.OnSpeedtapRoundCozmoWin);
        }
      }
    }

    private void HandleHandEndAnimDone(bool success) {
      ClearWinningLightPatterns();
      _StateMachine.SetNextState(new SpeedTapHandCubesOff());
    }

    private void HandleRoundEndAnimDone(bool success) {
      _SpeedTapGame.ResetScore();
      ClearWinningLightPatterns();
      _StateMachine.SetNextState(new SpeedTapCozmoDriveToCube(false));
    }

    private void HandleEndGameAnimDone(bool success) {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.GameSharedEnd);
      ClearWinningLightPatterns();
      _SpeedTapGame.HandleGameEnd();
    }
  }
}