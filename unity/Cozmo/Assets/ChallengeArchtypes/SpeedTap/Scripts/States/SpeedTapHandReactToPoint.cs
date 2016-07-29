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

    private const float _kWinCycleSpeedSeconds = 0.1f;

    private SpeedTapGame _SpeedTapGame;

    private PointWinner _CurrentWinner;
    private bool _WasMistakeMade;

    private Color[] _WinColors;
    private LightCube _WinningCube;

    private float _StartWinEffectTimestampSeconds;

    private float _CubeCycleDurationSeconds = 0;
    private float _CubeFlashingDurationSeconds = 0;

    private bool _IsPlayingWinGameAnimation = false;

    private CubeLightState _CurrentCubeLightState = CubeLightState.None;

    public SpeedTapHandReactToPoint(PointWinner winner, bool mistakeMade) {
      _CurrentWinner = winner;
      _WasMistakeMade = mistakeMade;
    }

    public override void Enter() {
      base.Enter();
      _SpeedTapGame = _StateMachine.GetGame() as SpeedTapGame;

      _SpeedTapGame.AddPoint(_CurrentWinner == PointWinner.Player);
      // Depends on points being scored first
      _SpeedTapGame.UpdateUI();

      if (_SpeedTapGame.IsRoundComplete()) {
        GameAudioClient.SetMusicState(_SpeedTapGame.BetweenRoundsMusic);
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.Gp_Shared_Round_End);

        _SpeedTapGame.EndCurrentRound();

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
    }

    public override void Pause(PauseReason reason, Anki.Cozmo.BehaviorType reactionaryBehavior) {
      // COZMO-2033; some of the win game animations cause Cozmo's cliff sensor to trigger
      // So in those cases don't show the "Cozmo Moved; Quit Game" dialog
      if (!_IsPlayingWinGameAnimation) {
        base.Pause(reason, reactionaryBehavior);
      }
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
        _WinningCube.SetLEDsOff();
        SetLosingLightPattern(losingBlock, _kWinCycleSpeedSeconds);
        _CurrentCubeLightState = CubeLightState.LoserFlashing;
        GameAudioClient.PostSFXEvent (Anki.Cozmo.Audio.GameEvent.SFX.Gp_St_Lose);
      }
      else {
        GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.Gp_St_Win);
        _StartWinEffectTimestampSeconds = Time.time;
        losingBlock.SetLEDsOff();
        StartWinningBlockPattern();
      }
    }

    private void StartWinningBlockPattern() {
      /*_CubeCycleDurationSeconds = _SpeedTapGame.GetCycleDurationSeconds(3, _kWinCycleSpeedSeconds);
      _CubeFlashingDurationSeconds = 6 * _kWinCycleSpeedSeconds;
      SetWinningCyclePattern(_WinningCube, _WinColors, _kWinCycleSpeedSeconds);
      _CurrentCubeLightState = CubeLightState.WinnerCycling;
      _StartWinEffectTimestampSeconds = Time.time;*/

      // INGO PR DEMO COZMO-2005
      // For press demo, switching the win animation to flashing white because that will look better.
      _CurrentCubeLightState = CubeLightState.None;
      SetWinningFlashingPattern(_WinningCube, _WinColors, _kWinCycleSpeedSeconds);
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

    private void PlayReactToHandAnimation() {
      AnimationTrigger animationEventToSend = (_CurrentWinner == PointWinner.Player) ?
              AnimationTrigger.OnSpeedtapHandPlayerWin : AnimationTrigger.OnSpeedtapHandCozmoWin;
      ListenForAnimationEnd(animationEventToSend, HandleHandEndAnimDone);
    }

    private void PlayReactToGameAnimationAndSendEvent() {
      AnimationTrigger animationEventToSend = AnimationTrigger.Count;
      bool highIntensity = _SpeedTapGame.IsHighIntensityGame();

      if (DataPersistence.DataPersistenceManager.Instance.Data.DebugPrefs.RunPressDemo) {
        highIntensity = true;
      }

      if (_CurrentWinner == PointWinner.Player) {
        animationEventToSend = (highIntensity) ?
                  AnimationTrigger.OnSpeedtapGamePlayerWinHighIntensity : AnimationTrigger.OnSpeedtapGamePlayerWinLowIntensity;
      }
      else {
        animationEventToSend = (highIntensity) ?
                  AnimationTrigger.OnSpeedtapGameCozmoWinHighIntensity : AnimationTrigger.OnSpeedtapGameCozmoWinLowIntensity;
      }
      _IsPlayingWinGameAnimation = true;
      ListenForAnimationEnd(animationEventToSend, HandleEndGameAnimDone);
    }

    private void ListenForAnimationEnd(AnimationTrigger animEvent, RobotCallback animationCallback) {
      _CurrentRobot.SendAnimationTrigger(animEvent, animationCallback);
    }

    private void HandleHandEndAnimDone(bool success) {
      _SpeedTapGame.ClearWinningLightPatterns();
      _StateMachine.SetNextState(new SpeedTapHandCubesOff());
    }

    private void HandleEndGameAnimDone(bool success) {
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.SFX.Gp_Shared_Game_End);
      _SpeedTapGame.ClearWinningLightPatterns();
      _SpeedTapGame.StartRoundBasedGameEnd();
      _SpeedTapGame.StartEndMusic();
    }
  }
}
