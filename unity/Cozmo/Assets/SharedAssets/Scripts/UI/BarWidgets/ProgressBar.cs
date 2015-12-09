using UnityEngine;
using UnityEngine.UI;

namespace Cozmo {
  namespace UI {
    public class ProgressBar : MonoBehaviour {

      private const float kTweenDuration = 3.25f;

      [SerializeField]
      private Image _FilledForegroundImage;

      [SerializeField]
      private Color _NormalColor;

      [SerializeField]
      private Color _DecreasingColor;

      [SerializeField]
      private Color _IncreasingColor;

      private float _StartProgress = 0f;
      private float _TargetProgress = 0f;
      private float _TimePassedSeconds = 0f;

      private void Start() {
        // Force filled type so that the progress bar works.
        _FilledForegroundImage.type = Image.Type.Filled;
        _StartProgress = 0;
        _FilledForegroundImage.fillAmount = 0;
        _TimePassedSeconds = 0;
      }

      private void Update() {
        if (_FilledForegroundImage.fillAmount != _TargetProgress) {
          _TimePassedSeconds += Time.deltaTime;
          if (_TimePassedSeconds > kTweenDuration) {
            _TimePassedSeconds = kTweenDuration;
          }
          _FilledForegroundImage.fillAmount = EaseOutQuad(_TimePassedSeconds, _StartProgress, _TargetProgress, kTweenDuration);

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
          DAS.Warn(this, "Tried to set progress to value=" + progress + " which is not in the range of 0 to 1! Clamping.");
        }

        float newProgress = Mathf.Clamp(progress, 0f, 1f);
        if (newProgress != _TargetProgress) {
          _TargetProgress = newProgress;
        }
      }
    }
  }
}
