using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;

namespace Cozmo {
  namespace UI {
    public class ProgressBarWidget : MonoBehaviour {

      [SerializeField]
      private Image _FilledForegroundImage;

      [SerializeField]
      private Color _NormalColor;

      [SerializeField]
      private Color _DecreasingColor;

      [SerializeField]
      private Color _IncreasingColor;

      private float _TargetProgress = 0f;

      private Sequence _TweenToTargetSequence;
      private bool _IsDirty = false;

      private void Start() {
        // Force filled type so that the progress bar works.
        _FilledForegroundImage.type = Image.Type.Filled;

        if (!_IsDirty) {
          ResetProgress();
        }
      }

      private void Update() {
        if (_IsDirty) {
          _IsDirty = false;

          KillTweens();

          if (_TargetProgress < _FilledForegroundImage.fillAmount) {
            _FilledForegroundImage.color = _DecreasingColor;
          }
          else {
            _FilledForegroundImage.color = _IncreasingColor;
          }

          // TODO: Don't hardcode this
          _TweenToTargetSequence = DOTween.Sequence();
          Tweener tween = _FilledForegroundImage.DOFillAmount(_TargetProgress, 0.25f);
          tween.SetEase(Ease.OutQuad);
          _TweenToTargetSequence.Append(tween);
          _TweenToTargetSequence.OnComplete(HandleTweenFinishedCallback);
          _TweenToTargetSequence.Play();
        }
      }

      private void HandleTweenFinishedCallback() {
        _FilledForegroundImage.color = _NormalColor;
      }

      private void OnDestroy() {
        KillTweens();
      }

      public void ResetProgress() {
        KillTweens();
        _IsDirty = false;
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
          _IsDirty = true;
        }
      }

      private void KillTweens() {
        if (_TweenToTargetSequence != null) {
          _TweenToTargetSequence.Kill();
          _TweenToTargetSequence = null;
        }
      }
    }
  }
}
