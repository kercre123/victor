using UnityEngine;
using Anki.Assets;

namespace Cozmo {
  namespace UI {
    public class CubePalette : ScriptableObject {
      private static CubePalette _sInstance;

      public static CubePalette Instance {
        get { return _sInstance; }
        private set {
          if (_sInstance == null) {
            _sInstance = value;
          }
        }
      }

      public static void LoadCubePalette(string assetBundleName) {
        AssetBundleManager.Instance.LoadAssetAsync(
          assetBundleName, "CubePalette", (CubePalette colorHolder) => {
            Instance = colorHolder;
          });
      }

      [SerializeField]
      private CubeColor _OffColor;
      public CubeColor OffColor { get { return _OffColor; } }

      [SerializeField]
      private CubeColor _InViewColor;
      public CubeColor InViewColor { get { return _InViewColor; } }

      [SerializeField]
      private CubeColor _OutOfViewColor;
      public CubeColor OutOfViewColor { get { return _OutOfViewColor; } }

      [SerializeField]
      private CubeColor _ReadyColor;
      public CubeColor ReadyColor { get { return _ReadyColor; } }

      [SerializeField]
      private CubeColor _ErrorColor;
      public CubeColor ErrorColor { get { return _ErrorColor; } }

      [SerializeField]
      private CubeCycleColors _TapMeColor;
      public CubeCycleColors TapMeColor { get { return _TapMeColor; } }

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
}