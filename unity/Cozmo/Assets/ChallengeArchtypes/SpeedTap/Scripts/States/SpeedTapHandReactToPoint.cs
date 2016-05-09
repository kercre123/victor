using UnityEngine;
using System.Collections;
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

    private const float _kWinCycleSpeedSeconds = 0.1f;

    private SpeedTapGame _SpeedTapGame;

    private PointWinner _CurrentWinner;
    private bool _WasMistakeMade;

    private Color[] _WinColors;
    private LightCube _WinningCube;

    private float _StartWinEffectTimestampSeconds;
    private float _CubeCycleDurationSeconds;
    private float _CubeFlashingDurationSeconds;

    private CubeLightState _CurrentCubeLightState = CubeLightState.None;

    public SpeedTapHandReactToPoint(PointWinner winner, bool mistakeMade) {
      _CurrentWinner = winner;
      _WasMistakeMade = mistakeMade;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;
      _SpeedTapGame.EndCozmoPickedUpDisruptionDetection();
      _SpeedTapGame.EndCozmoCubeMovedDisruptionDetection();

      UpdateBlockLights(_CurrentWinner, _WasMistakeMade);

      if (_CurrentWinner == PointWinner.Player) {
        _SpeedTapGame.AddPlayerPoint();
      }
      else {
        _SpeedTapGame.AddCozmoPoint();
      }

      // Depends on points being scored first
      _SpeedTapGame.UpdateUI();

      if (_SpeedTapGame.IsRoundComplete()) {
        HandleRoundEnd();
      }
      else {
        ReactToHand();
      }
    }

    public override void Update() {
      float secondsSinceAnimationStarted = Time.time - _StartWinEffectTimestampSeconds;
      if (_CurrentCubeLightState == CubeLightState.WinnerCycling && secondsSinceAnimationStarted >= _CubeCycleDurationSeconds) {
        _StartWinEffectTimestampSeconds = Time.time;
        _SpeedTapGame.StopCycleCube(_WinningCube);
        SetWinningFlashingPattern(_WinningCube, _WinColors, _kWinCycleSpeedSeconds);
        _CurrentCubeLightState = CubeLightState.WinnerFlashing;
      }
      else if (_CurrentCubeLightState == CubeLightState.WinnerFlashing && secondsSinceAnimationStarted >= _CubeFlashingDurationSeconds) {
        _StartWinEffectTimestampSeconds = Time.time;
        SetWinningCyclePattern(_WinningCube, _WinColors, _kWinCycleSpeedSeconds);
        _CurrentCubeLightState = CubeLightState.WinnerCycling;
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

    private void UpdateBlockLights(PointWinner winner, bool wasMistakeMade) {
      LightCube losingBlock;
      if (winner == PointWinner.Player) {
        _WinColors = _SpeedTapGame.PlayerWinColors;
        _WinningCube = _SpeedTapGame.PlayerBlock;
        losingBlock = _SpeedTapGame.CozmoBlock;
      }
      else {
        _WinColors = _SpeedTapGame.CozmoWinColors;
        _WinningCube = _SpeedTapGame.CozmoBlock;
        losingBlock = _SpeedTapGame.PlayerBlock;
      }

      if (wasMistakeMade) {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapLose);
        _WinningCube.SetLEDsOff();
        SetLosingLightPattern(losingBlock, _kWinCycleSpeedSeconds);
        _CurrentCubeLightState = CubeLightState.LoserFlashing;
      }
      else {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.SpeedTapWin);
        _StartWinEffectTimestampSeconds = Time.time;
        losingBlock.SetLEDsOff();
        SetWinningCyclePattern(_WinningCube, _WinColors, _kWinCycleSpeedSeconds);
        _CubeCycleDurationSeconds = _SpeedTapGame.GetCycleDurationSeconds(3, _kWinCycleSpeedSeconds);
        _CubeFlashingDurationSeconds = 6 * _kWinCycleSpeedSeconds;
        _CurrentCubeLightState = CubeLightState.WinnerCycling;
      }
    }

    private void SetWinningCyclePattern(LightCube winningBlock, Color[] winColors, float cycleDurationSeconds) {
      if (winColors[0] == Color.white) {
        _SpeedTapGame.StartCycleCubeSingleColor(winningBlock, winColors, cycleDurationSeconds, Color.black);
      }
      else {
        _SpeedTapGame.StartCycleCubeSingleColor(winningBlock, winColors, cycleDurationSeconds, Color.white);
      }
    }

    private void SetWinningFlashingPattern(LightCube winningBlock, Color[] winColors, float flashDurationSeconds) {
      uint flashDurationMs = (uint)Mathf.FloorToInt(flashDurationSeconds * 1000);
      winningBlock.SetFlashingLEDs(winColors, Color.white, flashDurationMs, flashDurationMs);
    }

    private void SetLosingLightPattern(LightCube losingBlock, float flashDurationSeconds) {
      uint flashDurationMs = (uint)Mathf.FloorToInt(flashDurationSeconds * 1000);
      losingBlock.SetFlashingLEDs(Color.red, flashDurationMs, flashDurationMs);
    }

    private void ClearWinningLightPatterns() {
      _SpeedTapGame.StopCycleCube(_SpeedTapGame.PlayerBlock);
      _SpeedTapGame.PlayerBlock.SetLEDsOff();
      _SpeedTapGame.StopCycleCube(_SpeedTapGame.CozmoBlock);
      _SpeedTapGame.CozmoBlock.SetLEDsOff();
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
          GameEventManager.Instance.SendGameEventToEngine(GameEventWrapperFactory.Create(Anki.Cozmo.GameEvent.OnSpeedtapGamePlayerWin, 
            _SpeedTapGame.CurrentDifficulty));
        }
        else {
          // TODO add event listener
          AnimationManager.Instance.AddAnimationEndedCallback(Anki.Cozmo.GameEvent.OnSpeedtapGameCozmoWin, HandleEndGameAnimDone);
          GameEventManager.Instance.SendGameEventToEngine(GameEventWrapperFactory.Create(Anki.Cozmo.GameEvent.OnSpeedtapGameCozmoWin, 
            _SpeedTapGame.CurrentDifficulty));
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
