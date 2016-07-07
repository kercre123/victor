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
    }
  }
}
