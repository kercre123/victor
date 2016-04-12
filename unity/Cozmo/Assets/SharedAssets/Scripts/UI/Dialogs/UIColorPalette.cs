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

      public static Color CompleteTextColor() {
        return Instance._CompleteTextColor;
      }

      [SerializeField]
      private Color _NeutralTextColor;

      public static Color NeutralTextColor() {
        return Instance._NeutralTextColor;
      }
    }
  }
}