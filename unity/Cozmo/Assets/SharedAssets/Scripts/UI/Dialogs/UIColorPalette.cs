using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class UIColorPalette : ScriptableObject {

      private static UIColorPalette _Instance;

      public static UIColorPalette Instance {
        get {
          if (_Instance == null) {
            _Instance = Resources.Load<UIColorPalette>("Prefabs/UI/UIColorPalette");
          }
          return _Instance;
        }
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