using System;
using UnityEngine;
using UnityEngine.UI;

namespace Cozmo {
  namespace UI {
    public class ProgressBar : MonoBehaviour {

      private const float kTweenDuration = 0.5f;

      public Action ProgressUpdateCompleted;

      [SerializeField]
      private Image _FilledForegroundImage;

      [SerializeField]
      private bool _UseEndCap = false;

      [SerializeField]
      private Image _EndCap;

      [SerializeField]
      private Color _NormalColor;

      [SerializeField]
      private Color _DecreasingColor;

      [SerializeField]
      private Color _IncreasingColor;

      private float _AnimStartValue = 0f;
      private float _TargetValue = 0f;
      private float CurrentValue {
        get { return _FilledForegroundImage.fillAmount; }
        set { _FilledForegroundImage.fillAmount = value; }
      }

      private float _CurrentTweenTime_sec = 0f;

      private enum BarAnimationState {
        INIT,
        IDLE,
        TWEEN_MAIN
      }

      private BarAnimationState _CurrentAnimState = BarAnimationState.INIT;

      private void Start() {
        // Force filled type so that the progress bar works.
        _FilledForegroundImage.type = Image.Type.Filled;

        // Target values are sometimes set during Awake; don't override in Start
        // _TargetValue = 0f;
        // _CurrentAnimState = BarAnimationState.IDLE;

        if (_CurrentAnimState == BarAnimationState.INIT) {
          _AnimStartValue = 0;
          _CurrentTweenTime_sec = 0;
          CurrentValue = 0;
          _CurrentAnimState = BarAnimationState.IDLE;
        }

        if (_UseEndCap && _EndCap == null) {
          DAS.Warn("ProgressBar.Start", "UseEndCap enabled but EndCap is NULL");
          _UseEndCap = false;
        }
        if (_EndCap != null && ((!_UseEndCap) ||
            ((CurrentValue * _FilledForegroundImage.rectTransform.rect.width) < _EndCap.rectTransform.rect.width))) {
          _EndCap.gameObject.SetActive(false);
        }
      }

      private void Update() {
        switch (_CurrentAnimState) {
        case BarAnimationState.TWEEN_MAIN:
          _CurrentTweenTime_sec += Time.deltaTime;
          if (_CurrentTweenTime_sec > kTweenDuration) {
            _CurrentTweenTime_sec = kTweenDuration;
          }
          CurrentValue = EaseOutQuad(_CurrentTweenTime_sec, _AnimStartValue,
            _TargetValue - _AnimStartValue, kTweenDuration);

          // If Endcap is enabled and != null, update position based on the width and fill amount
          // of the Foreground Image.
          if (_UseEndCap) {
            PositionEndCap();
          }

          if (_TargetValue > CurrentValue) {
            _FilledForegroundImage.color = _IncreasingColor;
          }
          else if (_TargetValue < CurrentValue) {
            _FilledForegroundImage.color = _DecreasingColor;
          }
          else {
            _FilledForegroundImage.color = _NormalColor;
            _AnimStartValue = CurrentValue;
            _CurrentTweenTime_sec = 0;

            if (ProgressUpdateCompleted != null) {
              ProgressUpdateCompleted.Invoke();
            }
            _CurrentAnimState = BarAnimationState.IDLE;
          }
          break;
        // IDLE / default - do nothing
        case BarAnimationState.IDLE:
          break;
        case BarAnimationState.INIT:
          break;
        default:
          break;
        }
      }

      private float EaseOutQuad(float currentTimeSeconds, float startValue, float changeInValue, float duration) {
        currentTimeSeconds /= duration;
        return -changeInValue * currentTimeSeconds * (currentTimeSeconds - 2) + startValue;
      }

      public void ResetProgress() {
        SetValueInstantInternal(0f);
      }

      /// <summary>
      /// Sets the progress of the bar.
      /// </summary>
      /// <param name="targetProgress">Progress.</param>
      /// <param name="instant">If set to <c>true</c> cut to progress instantly without firing events.</param>
      public void SetTargetAndAnimate(float targetProgress) {
        SetTarget(targetProgress);
        AnimateToTarget();
      }

      public void SetValueInstant(float targetProgress) {
        SetValueInstantInternal(targetProgress);
        if (ProgressUpdateCompleted != null) {
          ProgressUpdateCompleted.Invoke();
        }
      }

      private void SetValueInstantInternal(float targetProgress) {
        SetTarget(targetProgress);
        _AnimStartValue = _TargetValue;
        CurrentValue = _TargetValue;
        _CurrentAnimState = BarAnimationState.IDLE;
        _CurrentTweenTime_sec = 0f;
        if (_UseEndCap) {
          PositionEndCap();
        }
      }

      public void SetTarget(float targetProgress) {
        if (targetProgress < 0 || targetProgress > 1) {
          DAS.Warn("ProgressBar.SetTarget.OutOfRange", "Tried to set progress to value="
                   + targetProgress + " which is not in the range of 0 to 1! Clamping.");
        }
        _TargetValue = Mathf.Clamp(targetProgress, 0f, 1f);
      }

      public void AnimateToTarget() {
        _CurrentAnimState = BarAnimationState.TWEEN_MAIN;
      }

      private void PositionEndCap() {
        float capPos = 0.0f;
        capPos = CurrentValue * _FilledForegroundImage.rectTransform.rect.width;

        if (capPos < _EndCap.rectTransform.rect.width || (CurrentValue >= 1.0f)) {
          _EndCap.gameObject.SetActive(false);
        }
        else {
          _EndCap.gameObject.SetActive(true);
          _EndCap.rectTransform.localPosition = new Vector3(capPos, _EndCap.rectTransform.localPosition.y);
        }
      }
    }
  }
}
