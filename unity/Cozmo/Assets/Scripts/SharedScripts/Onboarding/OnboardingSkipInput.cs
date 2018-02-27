using UnityEngine;
using UnityEngine.Events;
using UnityEngine.EventSystems;
using System;
using System.Collections;

public class OnboardingSkipInput : MonoBehaviour {
  private const float kSecondsBeforeReset = 0.8f;
  private const int kNumTapsToEnable = 5;

  private Coroutine _ResetTimerCoroutine = null;
  private int _NumTaps = 0;

  [SerializeField]
  public UnityEvent OnToggleSkipOnboarding;

  void Awake() {
    _NumTaps = 0;
    _ResetTimerCoroutine = null;
  }

  void Update() {
    for (int i = 0; i < Input.touches.Length; i++) {
      Touch t = Input.touches[i];
      if (t.phase == TouchPhase.Ended) {
        RectTransform rect = transform as RectTransform;
        if (RectTransformUtility.RectangleContainsScreenPoint(rect, Input.touches[i].position, Camera.current)) {
          OnPointerClick();
          break;
        }
      }
    }
  }

  void OnPointerClick() {
    if (_ResetTimerCoroutine == null) {
      _NumTaps = 1;
      _ResetTimerCoroutine = StartCoroutine(ResetTimerFunc());
    }
    else {
      StopCoroutine(_ResetTimerCoroutine);
      ++_NumTaps;

      if (_NumTaps >= kNumTapsToEnable) {
        _ResetTimerCoroutine = null;
        OnToggleSkipOnboarding.Invoke();
      }
      else {
        _ResetTimerCoroutine = StartCoroutine(ResetTimerFunc());
      }
    }
  }

  IEnumerator ResetTimerFunc() {
    yield return new WaitForSeconds(kSecondsBeforeReset);
    _ResetTimerCoroutine = null;
    yield return null;
  }

  void OnDestroy() {
    if (_ResetTimerCoroutine != null) {
      StopCoroutine(_ResetTimerCoroutine);
      _ResetTimerCoroutine = null;
    }
  }
}
