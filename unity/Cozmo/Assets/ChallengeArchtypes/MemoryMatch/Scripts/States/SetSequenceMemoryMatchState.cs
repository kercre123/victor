using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo.Audio;

namespace MemoryMatch {

  public class SetSequenceMemoryMatchState : State {

    private MemoryMatchGame _GameInstance;
    private int _CurrentSequenceIndex = -1;
    private IList<int> _CurrentSequence;
    private int _SequenceLength;
    private float _LastSequenceTime = -1;

    private PlayerType _NextPlayer;
    private const float _kBlinkTime_Sec = 0.15f;
    private bool _InCountdown = true;
    private Coroutine _CountdownCoroutine = null;

    public SetSequenceMemoryMatchState(PlayerType nextPlayer) {
      _NextPlayer = nextPlayer;
    }

    public override void Enter() {
      base.Enter();
      _GameInstance = _StateMachine.GetGame() as MemoryMatchGame;
      bool SequenceGrown;
      _SequenceLength = _GameInstance.GetNewSequenceLength(_NextPlayer, out SequenceGrown);
      _GameInstance.GenerateNewSequence(_SequenceLength);
      _CurrentSequence = _GameInstance.GetCurrentSequence();

      _CurrentRobot.DriveWheels(0.0f, 0.0f);
      _CurrentRobot.SetLiftHeight(0.0f);
      _CurrentRobot.SetHeadAngle(CozmoUtil.kIdealBlockViewHeadValue);

      _LastSequenceTime = Time.time;

      _InCountdown = true;
      // Start Sequence after audio completes
      GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Gp_Mm_Pattern_Start);
      if (SequenceGrown) {
        _GameInstance.SharedMinigameView.PlayBannerAnimation(Localization.Get(LocalizationKeys.kMemoryMatchGameLabelNextRound), null, 0.0f, false);
      }
      if (_CountdownCoroutine == null) {
        _CountdownCoroutine = _GameInstance.StartCoroutine(CountdownCoroutine());
      }
    }

    // NextRound, 3,2,1, go
    private IEnumerator CountdownCoroutine() {
      // Blink the lights fast while this is going on...
      foreach (int cubeId in _GameInstance.CubeIdsForGame) {
        Color[] color_cycle = { _GameInstance.GetColorForBlock(cubeId), Color.black, Color.black, Color.black };
        _GameInstance.StartCycleCube(cubeId, color_cycle, _kBlinkTime_Sec);
      }
      // wait for on/off time
      float cycleTime = _GameInstance.Config.CountDownTimeSec - _GameInstance.Config.HoldLightsAfterCountDownTimeSec;
      yield return new WaitForSeconds(cycleTime);

      _GameInstance.SetCubeLightsDefaultOn();
      yield return new WaitForSeconds(_GameInstance.Config.HoldLightsAfterCountDownTimeSec);
      HandleCountDownDone();
      yield break;
    }

    private void HandleCountDownDone() {
      _GameInstance.GetMemoryMatchSlide().ShowCenterText(Localization.Get(LocalizationKeys.kMemoryMatchGameLabelListen));
      _InCountdown = false;
      if (_CountdownCoroutine != null) {
        _GameInstance.StopCoroutine(_CountdownCoroutine);
        _CountdownCoroutine = null;
      }
    }

    public override void Update() {
      base.Update();
      // Wait for countdown to complete before setting the sequence.
      if (_InCountdown) {
        return;
      }
      if (_CurrentSequenceIndex == -1) {
        // First in sequence
        if (Time.time - _LastSequenceTime > _GameInstance.TimeWaitFirstBeat) {
          LightUpNextCube();
        }
      }
      else if (_CurrentSequenceIndex >= _CurrentSequence.Count - 1) {
        // Last in sequence

        Anki.Cozmo.AnimationTrigger trigger = _GameInstance.IsSoloMode() ? Anki.Cozmo.AnimationTrigger.MemoryMatchReactToPatternSolo :
                                                                           Anki.Cozmo.AnimationTrigger.MemoryMatchReactToPattern;


        // Let the player go without Cozmo's reaction being done.
        if (_NextPlayer == PlayerType.Human) {
          _CurrentRobot.SendAnimationTrigger(trigger);
          HandEndAnimationDone(true);
        }
        else {
          _StateMachine.SetNextState(new AnimationGroupState(trigger, HandEndAnimationDone));
        }

      }
      else {
        if (Time.time - _LastSequenceTime > _GameInstance.TimeBetweenBeats) {
          LightUpNextCube();
        }
      }
    }

    public void HandEndAnimationDone(bool success) {
      if (_NextPlayer == PlayerType.Human) {
        _StateMachine.SetNextState(new WaitForPlayerGuessMemoryMatchState());
      }
      else {
        _StateMachine.SetNextState(new CozmoGuessMemoryMatchState());
      }
    }

    private void LightUpNextCube() {
      _CurrentSequenceIndex++;
      _LastSequenceTime = Time.time;
      LightCube target = GetCurrentTarget();
      if (target != null) {
        int cubeId = target.ID;
        Anki.Cozmo.Audio.GameAudioClient.PostAudioEvent(_GameInstance.GetAudioForBlock(cubeId));
        _GameInstance.BlinkLight(cubeId, MemoryMatchGame.kLightBlinkLengthSeconds, Color.black, _GameInstance.GetColorForBlock(cubeId));
      }
    }

    public LightCube GetCurrentTarget() {
      if (_CurrentRobot != null &&
          _CurrentSequenceIndex > 0 && _CurrentSequenceIndex < _CurrentSequence.Count &&
          _CurrentRobot.LightCubes.ContainsKey(_CurrentSequence[_CurrentSequenceIndex])) {
        return _CurrentRobot.LightCubes[_CurrentSequence[_CurrentSequenceIndex]];
      }
      return null;
    }

    public override void Exit() {
      base.Exit();
      if (_CurrentRobot != null) {
        _CurrentRobot.DriveWheels(0.0f, 0.0f);
      }
      if (_CountdownCoroutine != null) {
        _GameInstance.StopCoroutine(_CountdownCoroutine);
        _CountdownCoroutine = null;
      }
    }
  }

}
