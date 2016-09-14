using System;
using UnityEngine;
using UnityEngine.UI;
using UnityEngine.EventSystems;
using System.Collections.Generic;
using DG.Tweening;
using Cozmo;
using Cozmo.UI;
using DataPersistence;

public class ContextManager : MonoBehaviour {

  public Action OnAppHoldStart;
  public Action OnAppHoldEnd;

  private static ContextManager _Instance;

  public static ContextManager Instance {
    get {
      if (_Instance == null) {
        DAS.Error("ContextManager.NullInstance", "Do not access ContextManager until start: " + System.Environment.StackTrace);
      }
      return _Instance;
    }
    private set {
      if (_Instance != null) {
        DAS.Error("ContextManager.DuplicateInstance", "ContextManager Instance already exists");
      }
      _Instance = value;
    }
  }

  private bool _AnticipationAudioPlaying = false;
  private ushort _AnticipationAudioID;
  private bool _ManagerBusy = false;
  public bool ManagerBusy {
    get {
      return _ManagerBusy;
    }
  }

  [SerializeField]
  private Image _OverlayForeground;
  private Sequence _ForegroundTweener = null;
  private UIDefaultTransitionSettings _DefaultSettings;

  private float _StartHoldTimestamp = -1;
  private float _CurrentHoldDuration = -1;

  private List<Action> _AppHoldCallbackList = new List<Action>();
  private List<Action> _CozmoHoldCallbackList = new List<Action>();

  void Awake() {
    Instance = this;
    _DefaultSettings = UIDefaultTransitionSettings.Instance;
    _OverlayForeground.color = Color.clear;
  }


  /// <summary>
  /// Does a quick bright flash to draw attention to the App.
  /// </summary>
  public void AppFlash(bool playChime = false) {
    if (_ManagerBusy) { return; }
    if (_ForegroundTweener != null) {
      _ForegroundTweener.Kill();
    }
    if (playChime) {
      Anki.Cozmo.Audio.GameAudioClient.PostUIEvent(Anki.Cozmo.Audio.GameEvent.Ui.Attention_Device);
    }
    Color transparentFlashColor = new Color(_DefaultSettings.ContextFlashColor.r, _DefaultSettings.ContextFlashColor.g, _DefaultSettings.ContextFlashColor.b, 0);
    _OverlayForeground.color = transparentFlashColor;
    _ForegroundTweener.Append(_OverlayForeground.DOFade(_DefaultSettings.ContextFlashAlpha, _DefaultSettings.ContextFlashDuration).SetLoops(2, LoopType.Yoyo));
    _ForegroundTweener.Play();
  }

  public void AppHoldStart(bool anticipation, Action eventThatEndsAppHold) {
    if (_ManagerBusy) { return; }
    if (eventThatEndsAppHold != null) {
      eventThatEndsAppHold += AppHoldEnd;
      _AppHoldCallbackList.Add(eventThatEndsAppHold);
    }
    AppHoldStart(anticipation);
  }

  public void AppHoldStart(bool anticipation, float duration) {
    if (_ManagerBusy) { return; }
    if (duration > 0.0f) {
      _StartHoldTimestamp = Time.time;
      _CurrentHoldDuration = duration;
    }
    AppHoldStart(anticipation);
  }

  /// <summary>
  /// Darken screen, stop inputs, wait until an Action fires before ending. This variant can also be used
  /// for explicit start and end calls, or for setting a specific duration
  /// </summary>
  /// <param name="anticipation">If we should play the appropriate background sound as far of the anticipation.</param>
  public void AppHoldStart(bool anticipation) {
    if (_ForegroundTweener != null) {
      _ForegroundTweener.Kill();
    }
    if (anticipation) {
      // If we are doing the drumroll sound
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Context_Switch_Loop_Play);

      _AnticipationAudioPlaying = true;
    }

    if (OnAppHoldStart != null) {
      OnAppHoldStart.Invoke();
    }
    ShowForeground();
    _ManagerBusy = true;
  }

  void Update() {
    if (_StartHoldTimestamp >= 0) {
      if (Time.time - _StartHoldTimestamp >= _CurrentHoldDuration) {
        AppHoldEnd();
      }
    }
  }

  public void AppHoldEnd() {
    if (_AnticipationAudioPlaying) {
      Anki.Cozmo.Audio.GameAudioClient.PostSFXEvent(Anki.Cozmo.Audio.GameEvent.Sfx.Context_Switch_Loop_Stop);
    }
    if (OnAppHoldEnd != null) {
      OnAppHoldEnd.Invoke();
    }
    HideForeground();
    _AnticipationAudioPlaying = false;
    _ManagerBusy = false;
    _StartHoldTimestamp = -1;
    _CurrentHoldDuration = -1;
    for (int i = 0; i < _AppHoldCallbackList.Count; i++) {
      _AppHoldCallbackList[i] -= AppHoldEnd;
    }
    _AppHoldCallbackList.Clear();
  }

  public void ShowForeground() {
    if (_ForegroundTweener != null) {
      _ForegroundTweener.Kill();
    }
    Color transparentDimColor = new Color(_DefaultSettings.ContextDimColor.r, _DefaultSettings.ContextDimColor.g, _DefaultSettings.ContextDimColor.b, 0);
    _OverlayForeground.color = transparentDimColor;
    _ForegroundTweener.Append(_OverlayForeground.DOFade(_DefaultSettings.ContextDimAlpha, _DefaultSettings.ContextFlashDuration));
    _ForegroundTweener.Play();
  }

  public void HideForeground() {
    if (_ForegroundTweener != null) {
      _ForegroundTweener.Kill();
    }
    _ForegroundTweener.Append(_OverlayForeground.DOFade(0.0f, _DefaultSettings.ContextFlashDuration));
    _ForegroundTweener.Play();
  }

  // Only Call this during Free Play
  public void CozmoHoldFreeplayStart(Action eventThatEndsCozmoHold = null) {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.RobotStartIdle();
      if (eventThatEndsCozmoHold != null) {
        eventThatEndsCozmoHold += CozmoHoldFreeplayEnd;
        _CozmoHoldCallbackList.Add(eventThatEndsCozmoHold);
      }
    }
  }

  public void CozmoHoldFreeplayEnd() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      RobotEngineManager.Instance.CurrentRobot.RobotResumeFromIdle(true);
    }
    for (int i = 0; i < _CozmoHoldCallbackList.Count; i++) {
      _CozmoHoldCallbackList[i] -= CozmoHoldFreeplayEnd;
    }
    _CozmoHoldCallbackList.Clear();
  }

}