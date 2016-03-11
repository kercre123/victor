using UnityEngine;
using System.Collections;

namespace Cozmo {
  [CreateAssetMenu]
  public class CubePalette : ScriptableObject {

    private static CubePalette _Instance;

    public static CubePalette Instance {
      get {
        if (_Instance == null) {
          _Instance = Resources.Load<CubePalette>("Prefabs/UI/CubePalette");
        }
        return _Instance;
      }
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
  }

  [System.Serializable]
  public class CubeColor {
    public Sprite uiSprite;
    public Color lightColor;
  }
}