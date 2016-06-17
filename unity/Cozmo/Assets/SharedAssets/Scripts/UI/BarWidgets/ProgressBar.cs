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

      public Sprite FillImage {
        set {
          _FilledForegroundImage.overrideSprite = value;
        }
      }

      [SerializeField]
      private Color _NormalColor;

      [SerializeField]
      private Color _DecreasingColor;

      [SerializeField]
      private Color _IncreasingColor;

      private float _StartProgress = 0f;
      private float _TargetProgress = 0f;
      private float _TimePassedSeconds = 0f;
      private bool _ProgressUpdating = false;

      private void Start() {
        // Force filled type so that the progress bar works.
        _FilledForegroundImage.type = Image.Type.Filled;
        _StartProgress = 0;
        _FilledForegroundImage.fillAmount = 0;
        _TimePassedSeconds = 0;
        if (_UseEndCap && _EndCap == null) {
          DAS.Warn("ProgressBar.Start", "UseEndCap enabled but EndCap is NULL");
          _UseEndCap = false;
        }
        if (!_UseEndCap && _EndCap != null) {
          _EndCap.gameObject.SetActive(false);
        }
      }

      private void Update() {
        if (_FilledForegroundImage.fillAmount != _TargetProgress) {
          _TimePassedSeconds += Time.deltaTime;
          if (_TimePassedSeconds > kTweenDuration) {
            _TimePassedSeconds = kTweenDuration;
          }
          _FilledForegroundImage.fillAmount = EaseOutQuad(_TimePassedSeconds, _StartProgress, 
            _TargetProgress - _StartProgress, kTweenDuration);
          
          // If Endcap is enabled and != null, update position based on the width and fill amount
          // of the Foreground Image.
          if (_UseEndCap) {
            float capPos = 0.0f;
            capPos = _FilledForegroundImage.fillAmount * _FilledForegroundImage.sprite.rect.width;

            if (capPos < _EndCap.sprite.rect.width) {
              _EndCap.gameObject.SetActive(false);
            }
            else {
              _EndCap.gameObject.SetActive(true);
              _EndCap.rectTransform.localPosition = new Vector3(capPos, _EndCap.rectTransform.localPosition.y);
            }
          }

          if (_TargetProgress > _FilledForegroundImage.fillAmount) {
            _FilledForegroundImage.color = _IncreasingColor;
          }
          else if (_TargetProgress < _FilledForegroundImage.fillAmount) {
            _FilledForegroundImage.color = _DecreasingColor;
          }
          else {
            _FilledForegroundImage.color = _NormalColor;
            _StartProgress = _FilledForegroundImage.fillAmount;
            _TimePassedSeconds = 0;
          }
        }
        else if (_ProgressUpdating) {
          _ProgressUpdating = false;
          if (ProgressUpdateCompleted != null) {
            ProgressUpdateCompleted.Invoke();
          }
        }
      }

      private float EaseOutQuad(float currentTimeSeconds, float startValue, float changeInValue, float duration) {
        currentTimeSeconds /= duration;
        return -changeInValue * currentTimeSeconds * (currentTimeSeconds - 2) + startValue;
      }

      public void ResetProgress() {
        _TimePassedSeconds = 0;
        _StartProgress = 0;
        _TargetProgress = 0;
        _FilledForegroundImage.fillAmount = 0;
      }

      public void SetProgress(float progress) {
        if (progress < 0 || progress > 1) {
          DAS.Warn("ProgressBar.SetProgress.OutOfRange", "Tried to set progress to value=" + progress + " which is not in the range of 0 to 1! Clamping.");
        }
        _ProgressUpdating = true;
        float newProgress = Mathf.Clamp(progress, 0f, 1f);
        if (newProgress != _TargetProgress) {
          _TargetProgress = newProgress;
        }
      }
    }
  }
}
