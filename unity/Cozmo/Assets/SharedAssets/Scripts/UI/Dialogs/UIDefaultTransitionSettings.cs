using UnityEngine;
using DG.Tweening;

namespace Cozmo {
  namespace UI {
    public class UIDefaultTransitionSettings : ScriptableObject {
      private static UIDefaultTransitionSettings _sInstance;

      public static void SetInstance(UIDefaultTransitionSettings instance) {
        _sInstance = instance;
      }

      public static UIDefaultTransitionSettings Instance {
        get { return _sInstance; }
      }

      #region Context Transitions
      [SerializeField]
      private Color _ContextFlashColor = Color.green;
      public Color ContextFlashColor { get { return _ContextFlashColor; } }

      [SerializeField]
      private Color _ContextDimColor = new Color(0.0f, 0.0f, 0.0f, 0.4f);
      public Color ContextDimColor { get { return _ContextDimColor; } }

      [SerializeField]
      private float _ContextDimAlpha = 0.4f;
      public float ContextDimAlpha { get { return _ContextDimAlpha; } }

      [SerializeField]
      private float _ContextFlashAlpha = 0.7f;
      public float ContextFlashAlpha { get { return _ContextFlashAlpha; } }

      [SerializeField]
      private float _ContextFlashDuration = 0.2f;
      public float ContextFlashDuration { get { return _ContextFlashDuration; } }

      #endregion

      #region UI Transitions

      [SerializeField]
      private float _CascadeDelay_sec = 0.1f;
      public float CascadeDelay { get { return _CascadeDelay_sec; } }

      [SerializeField]
      private float _FadeInTransitionDuration_sec;
      public float FadeInTransitionDurationSeconds { get { return _FadeInTransitionDuration_sec; } }

      [SerializeField]
      private Ease _FadeInEasing;
      public Ease FadeInEasing { get { return _FadeInEasing; } }

      [SerializeField]
      private float _FadeOutTransitionDuration_sec;
      public float FadeOutTransitionDurationSeconds { get { return _FadeOutTransitionDuration_sec; } }

      [SerializeField]
      private Ease _FadeOutEasing;
      public Ease FadeOutEasing { get { return _FadeOutEasing; } }

      [SerializeField]
      private float _MoveOpenDuration_sec;
      public float MoveOpenDurationSeconds { get { return _MoveOpenDuration_sec; } }

      [SerializeField]
      private Ease _MoveOpenEase;
      public Ease MoveOpenEase { get { return _MoveOpenEase; } }

      [SerializeField]
      private float _MoveCloseDuration_sec;
      public float MoveCloseDurationSeconds { get { return _MoveCloseDuration_sec; } }

      [SerializeField]
      private Ease _MoveCloseEase;
      public Ease MoveCloseEase { get { return _MoveCloseEase; } }

      // TODO: Create a version for Images
      public Tweener CreateFadeInTween(CanvasGroup targetCanvas, Ease easing = Ease.Unset, float duration = -1f) {
        if (duration <= 0f) {
          duration = _FadeInTransitionDuration_sec;
        }
        if (easing == Ease.Unset) {
          easing = _FadeInEasing;
        }
        return CreateFadeTween(targetCanvas, 1f, easing, duration);
      }

      public Tweener CreateFadeOutTween(CanvasGroup targetCanvas, Ease easing = Ease.Unset, float duration = -1f) {
        if (duration <= 0f) {
          duration = _FadeOutTransitionDuration_sec;
        }
        if (easing == Ease.Unset) {
          easing = _FadeOutEasing;
        }
        return CreateFadeTween(targetCanvas, 0f, easing, duration);
      }

      private Tweener CreateFadeTween(CanvasGroup targetCanvas, float alpha, Ease easing, float duration) {
        return targetCanvas.DOFade(alpha, duration).SetEase(easing);
      }

      /// <summary>
      /// Creates a default open animation sequence. 
      /// Offsets are relative to the tranform's actual position in the UI after the sequence ends.
      /// (-x for from the left, -y for from below)
      /// </summary>
      public Tweener CreateOpenMoveTween(Transform targetTransform, float xOffset, float yOffset, Ease easing = Ease.Unset, float duration = 0) {
        if (duration <= 0f) {
          duration = _MoveOpenDuration_sec;
        }
        if (easing == Ease.Unset) {
          easing = _MoveOpenEase;
        }
        return CreateMoveTween(targetTransform, xOffset, yOffset, duration, easing).From();
      }

      /// <summary>
      /// Creates a default close animation sequence. 
      /// Offsets are relative to the tranform's actual position in the UI after the sequence ends.
      /// (-x for to the left, -y for to below)
      /// </summary>
      public Tweener CreateCloseMoveTween(Transform targetTransform, float xOffset, float yOffset, Ease easing = Ease.Unset, float duration = -1f) {
        if (duration <= 0f) {
          duration = _MoveCloseDuration_sec;
        }
        if (easing == Ease.Unset) {
          easing = _MoveCloseEase;
        }
        return CreateMoveTween(targetTransform, xOffset, yOffset, duration, easing);
      }

      private Tweener CreateMoveTween(Transform targetTransform, float xOffset, float yOffset, float duration, Ease easing) {
        return targetTransform.DOLocalMove(
          new Vector3(targetTransform.localPosition.x + xOffset,
                      targetTransform.localPosition.y + yOffset,
                      targetTransform.localPosition.z),
          duration).SetEase(easing);
      }
    }
    #endregion
  }
}
