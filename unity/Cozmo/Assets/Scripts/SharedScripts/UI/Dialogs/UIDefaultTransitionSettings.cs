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
      private float _ContextFlashDuration = 0.2f;
      public float ContextFlashDuration { get { return _ContextFlashDuration; } }

      #endregion

      #region UI Transitions

      [SerializeField]
      private float _StartTimeStagger_sec = 0.2f;
      public float StartTimeStagger_sec { get { return _StartTimeStagger_sec; } }

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

      public void ConstructOpenFadeTween(ref Sequence openSequence, FadeTweenSettings fadeTweenSettings) {
        switch (fadeTweenSettings.staggerType) {
        case StaggerType.DefaultUniform:
          CreateUniformOpenFadeTween(ref openSequence, fadeTweenSettings.targets, _StartTimeStagger_sec);
          break;
        case StaggerType.CustomUniform:
          CreateUniformOpenFadeTween(ref openSequence, fadeTweenSettings.targets, fadeTweenSettings.uniformStagger_sec);
          break;
        case StaggerType.Custom:
          CreateCustomOpenFadeTween(ref openSequence, fadeTweenSettings.targets);
          break;
        default:
          CreateUniformOpenFadeTween(ref openSequence, fadeTweenSettings.targets, _StartTimeStagger_sec);
          break;
        }
      }

      private void CreateUniformOpenFadeTween(ref Sequence openSequence, FadeTweenTarget[] fadeTweenTargets, float stagger_sec) {
        for (int i = 0; i < fadeTweenTargets.Length; i++) {
          openSequence.Insert(stagger_sec * i, CreateOpenFadeTween(fadeTweenTargets[i]));
        }
      }

      private void CreateCustomOpenFadeTween(ref Sequence openSequence, FadeTweenTarget[] fadeTweenTargets) {
        for (int i = 0; i < fadeTweenTargets.Length; i++) {
          openSequence.Insert(fadeTweenTargets[i].openSetting.startTime_sec, CreateOpenFadeTween(fadeTweenTargets[i]));
        }
      }

      private Tweener CreateOpenFadeTween(FadeTweenTarget tweenTarget) {
        if (tweenTarget.openSetting.useCustom) {
          return CreateFadeInTween(tweenTarget.target, tweenTarget.openSetting.easing, tweenTarget.openSetting.duration_sec);
        }
        return CreateFadeInTween(tweenTarget.target);
      }

      public Tweener CreateFadeInTween(CanvasGroup targetCanvas, Ease easing = Ease.Unset, float duration = -1f) {
        if (duration <= 0f) {
          duration = _FadeInTransitionDuration_sec;
        }
        if (easing == Ease.Unset) {
          easing = _FadeInEasing;
        }
        return CreateFadeTween(targetCanvas, 1f, easing, duration);
      }

      public void ConstructCloseFadeTween(ref Sequence closeSequence, FadeTweenSettings fadeTweenSettings) {
        switch (fadeTweenSettings.staggerType) {
        case StaggerType.DefaultUniform:
          CreateUniformCloseFadeTween(ref closeSequence, fadeTweenSettings.targets, _StartTimeStagger_sec);
          break;
        case StaggerType.CustomUniform:
          CreateUniformCloseFadeTween(ref closeSequence, fadeTweenSettings.targets, fadeTweenSettings.uniformStagger_sec);
          break;
        case StaggerType.Custom:
          CreateCustomCloseFadeTween(ref closeSequence, fadeTweenSettings.targets);
          break;
        default:
          CreateUniformCloseFadeTween(ref closeSequence, fadeTweenSettings.targets, _StartTimeStagger_sec);
          break;
        }
      }

      private void CreateUniformCloseFadeTween(ref Sequence closeSequence, FadeTweenTarget[] fadeTweenTargets, float stagger_sec) {
        for (int i = 0; i < fadeTweenTargets.Length; i++) {
          closeSequence.Insert(stagger_sec * i, CreateCloseFadeTween(fadeTweenTargets[i]));
        }
      }

      private void CreateCustomCloseFadeTween(ref Sequence closeSequence, FadeTweenTarget[] fadeTweenTargets) {
        for (int i = 0; i < fadeTweenTargets.Length; i++) {
          closeSequence.Insert(fadeTweenTargets[i].closeSetting.startTime_sec, CreateCloseFadeTween(fadeTweenTargets[i]));
        }
      }

      private Tweener CreateCloseFadeTween(FadeTweenTarget tweenTarget) {
        if (tweenTarget.closeSetting.useCustom) {
          return CreateFadeOutTween(tweenTarget.target, tweenTarget.closeSetting.easing, tweenTarget.closeSetting.duration_sec);
        }
        return CreateFadeOutTween(tweenTarget.target);
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

      public void ConstructOpenMoveTween(ref Sequence openSequence, MoveTweenSettings moveTweenSettings) {
        switch (moveTweenSettings.staggerType) {
        case StaggerType.DefaultUniform:
          CreateUniformOpenMoveTween(ref openSequence, moveTweenSettings.targets, _StartTimeStagger_sec);
          break;
        case StaggerType.CustomUniform:
          CreateUniformOpenMoveTween(ref openSequence, moveTweenSettings.targets, moveTweenSettings.uniformStagger_sec);
          break;
        case StaggerType.Custom:
          CreateCustomOpenMoveTween(ref openSequence, moveTweenSettings.targets);
          break;
        default:
          CreateUniformOpenMoveTween(ref openSequence, moveTweenSettings.targets, _StartTimeStagger_sec);
          break;
        }
      }

      private void CreateUniformOpenMoveTween(ref Sequence openSequence, MoveTweenTarget[] moveTweenTargets, float stagger_sec) {
        for (int i = 0; i < moveTweenTargets.Length; i++) {
          openSequence.Insert(stagger_sec * i, CreateOpenMoveTween(moveTweenTargets[i]));
        }
      }

      private void CreateCustomOpenMoveTween(ref Sequence openSequence, MoveTweenTarget[] moveTweenTargets) {
        for (int i = 0; i < moveTweenTargets.Length; i++) {
          openSequence.Insert(moveTweenTargets[i].openSetting.startTime_sec, CreateOpenMoveTween(moveTweenTargets[i]));
        }
      }

      private Tweener CreateOpenMoveTween(MoveTweenTarget tweenTarget) {
        if (tweenTarget.openSetting.useCustom) {
          return CreateOpenMoveTween(tweenTarget.target, tweenTarget.originOffset.x, tweenTarget.originOffset.y,
                                     tweenTarget.openSetting.easing, tweenTarget.openSetting.duration_sec);
        }
        return CreateOpenMoveTween(tweenTarget.target, tweenTarget.originOffset.x, tweenTarget.originOffset.y);
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

      public void ConstructCloseMoveTween(ref Sequence closeSequence, MoveTweenSettings moveTweenSettings) {
        switch (moveTweenSettings.staggerType) {
        case StaggerType.DefaultUniform:
          CreateUniformCloseMoveTween(ref closeSequence, moveTweenSettings.targets, _StartTimeStagger_sec);
          break;
        case StaggerType.CustomUniform:
          CreateUniformCloseMoveTween(ref closeSequence, moveTweenSettings.targets, moveTweenSettings.uniformStagger_sec);
          break;
        case StaggerType.Custom:
          CreateCustomCloseMoveTween(ref closeSequence, moveTweenSettings.targets);
          break;
        default:
          CreateUniformCloseMoveTween(ref closeSequence, moveTweenSettings.targets, _StartTimeStagger_sec);
          break;
        }
      }

      private void CreateUniformCloseMoveTween(ref Sequence closeSequence, MoveTweenTarget[] moveTweenTargets, float stagger_sec) {
        for (int i = 0; i < moveTweenTargets.Length; i++) {
          closeSequence.Insert(stagger_sec * i, CreateCloseMoveTween(moveTweenTargets[i]));
        }
      }

      private void CreateCustomCloseMoveTween(ref Sequence closeSequence, MoveTweenTarget[] moveTweenTargets) {
        for (int i = 0; i < moveTweenTargets.Length; i++) {
          closeSequence.Insert(moveTweenTargets[i].closeSetting.startTime_sec, CreateCloseMoveTween(moveTweenTargets[i]));
        }
      }

      private Tweener CreateCloseMoveTween(MoveTweenTarget tweenTarget) {
        if (tweenTarget.closeSetting.useCustom) {
          return CreateCloseMoveTween(tweenTarget.target, tweenTarget.originOffset.x, tweenTarget.originOffset.y,
                                      tweenTarget.closeSetting.easing, tweenTarget.closeSetting.duration_sec);
        }
        return CreateCloseMoveTween(tweenTarget.target, tweenTarget.originOffset.x, tweenTarget.originOffset.y);
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

    public enum StaggerType {
      DefaultUniform,
      CustomUniform,
      Custom
    }

    [System.Serializable]
    public struct MoveTweenSettings {
      public StaggerType staggerType;
      public float uniformStagger_sec;
      public MoveTweenTarget[] targets;
    }

    [System.Serializable]
    public struct MoveTweenTarget {
      public Transform target;
      public Vector2 originOffset;
      public TweenSetting openSetting;
      public TweenSetting closeSetting;
    }

    [System.Serializable]
    public struct TweenSetting {
      public float startTime_sec;
      public bool useCustom;
      public float duration_sec;
      public Ease easing;
    }

    [System.Serializable]
    public struct FadeTweenSettings {
      public StaggerType staggerType;
      public float uniformStagger_sec;
      public FadeTweenTarget[] targets;
    }

    [System.Serializable]
    public struct FadeTweenTarget {
      public CanvasGroup target;
      public TweenSetting openSetting;
      public TweenSetting closeSetting;
    }
  }
}
