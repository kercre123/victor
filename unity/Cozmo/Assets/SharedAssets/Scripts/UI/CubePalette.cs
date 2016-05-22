using UnityEngine;
using System.Collections;

namespace Cozmo {
  public class CubePalette : ScriptableObject {

    private static CubePalette _sInstance;

    public static void SetInstance(CubePalette instance) {
      _sInstance = instance;
    }

    public static CubePalette Instance {
      get { return _sInstance; }
    }

    [SerializeField]
    private CubeColor _OffColor;

    public static CubeColor OffColor {
      get {
        return Instance._OffColor;
      }
    }

    [SerializeField]
    private CubeColor _InViewColor;

    public static CubeColor InViewColor {
      get {
        return Instance._InViewColor;
      }
    }

    [SerializeField]
    private CubeColor _OutOfViewColor;

    public static CubeColor OutOfViewColor {
      get {
        return Instance._OutOfViewColor;
      }
    }

    [SerializeField]
    private CubeColor _ReadyColor;

    public static CubeColor ReadyColor {
      get {
        return Instance._ReadyColor;
      }
    }

    [SerializeField]
    private CubeCycleColors _TapMeColor;

    public static CubeCycleColors TapMeColor {
      get {
        return Instance._TapMeColor;
      }
    }

    [System.Serializable]
    public class CubeColor {
      // TODO: Use generic sprites from mock tray or write a shader instead of
      // having a sprite for every color in existence
      public Sprite uiSprite;
      public Color lightColor;
    }

    [System.Serializable]
    public class CubeCycleColors {
      public Color[] lightColors;
      public float cycleIntervalSeconds;
    }
  }
}