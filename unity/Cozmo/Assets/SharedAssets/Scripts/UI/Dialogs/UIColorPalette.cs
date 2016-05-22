using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class UIColorPalette : ScriptableObject {

      private static UIColorPalette _sInstance;

      public static void SetInstance(UIColorPalette instance) {
        _sInstance = instance;
      }

      public static UIColorPalette Instance {
        get { return _sInstance; }
      }

      [SerializeField]
      private Color _CompleteTextColor;

      public static Color CompleteTextColor {
        get { return Instance._CompleteTextColor; }
      }

      [SerializeField]
      private Color _NeutralTextColor;

      public static Color NeutralTextColor {
        get { return Instance._NeutralTextColor; }
      }

      [SerializeField]
      private Color _LockedDifficultyBackgroundColor;

      public static Color LockedDifficultyBackgroundColor {
        get { return Instance._LockedDifficultyBackgroundColor; }
      }

      [SerializeField]
      private Color _GameBackgroundColor;

      public static Color GameBackgroundColor {
        get { return Instance._GameBackgroundColor; }
      }

      [SerializeField]
      private Color _GameToggleColor;

      public static Color GameToggleColor {
        get { return Instance._GameToggleColor; }
      }

      [SerializeField]
      private Color _ActivityBackgroundColor;

      public static Color ActivityBackgroundColor {
        get { return Instance._ActivityBackgroundColor; }
      }
    }
  }
}