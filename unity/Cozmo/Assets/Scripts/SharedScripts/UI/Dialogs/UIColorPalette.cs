using UnityEngine;
using System.Collections.Generic;

namespace Cozmo {
  namespace UI {
    [System.Serializable]
    public class SparkCostTint {
      public Color CanAffordColor;
      public Color CannotAffordColor;
    }

    public class UIColorPalette : ScriptableObject {

      private static UIColorPalette _sInstance;

      public static void SetInstance(UIColorPalette instance) {
        _sInstance = instance;
      }

      public static UIColorPalette Instance {
        get { return _sInstance; }
      }

      [SerializeField]
      private Color _GeneralBackgroundColor;

      public static Color GeneralBackgroundColor {
        get { return Instance._GeneralBackgroundColor; }
      }

      [SerializeField]
      private SparkCostTint _GeneralSparkTintColor;

      public static SparkCostTint GeneralSparkTintColor {
        get { return Instance._GeneralSparkTintColor; }
      }

      [SerializeField]
      private SparkCostTint _ButtonSparkTintColor;

      public static SparkCostTint ButtonSparkTintColor {
        get { return Instance._ButtonSparkTintColor; }
      }

      [SerializeField]
      private Color _CompleteTextColor;

      public static Color CompleteTextColor {
        get { return Instance._CompleteTextColor; }
      }

      [SerializeField]
      private Color _EnergyTextColor;

      public static Color EnergyTextColor {
        get { return Instance._EnergyTextColor; }
      }

      [SerializeField]
      private Color _NeutralTextColor;

      public static Color NeutralTextColor {
        get { return Instance._NeutralTextColor; }
      }

      [SerializeField]
      private Color _SetupTextColor;

      public static Color SetupTextColor {
        get { return Instance._SetupTextColor; }
      }

      [SerializeField]
      private Color _MinigameTextColor;

      public static Color MinigameTextColor {
        get { return Instance._MinigameTextColor; }
      }

      [SerializeField]
      private Color _GameSetupColor;

      public static Color GameSetupColor {
        get { return Instance._GameSetupColor; }
      }

      [SerializeField]
      private Color _GameBackgroundColor;

      public static Color GameBackgroundColor {
        get { return Instance._GameBackgroundColor; }
      }

      [SerializeField]
      private Color _ActivityBackgroundColor;

      public static Color ActivityBackgroundColor {
        get { return Instance._ActivityBackgroundColor; }
      }

      [SerializeField]
      private Color _FreeplayBehaviorDefaultColor;

      public static Color FreeplayBehaviorDefaultColor {
        get { return Instance._FreeplayBehaviorDefaultColor; }
      }

      [SerializeField]
      private Color _FreeplayBehaviorRewardColor;

      public static Color FreeplayBehaviorRewardColor {
        get { return Instance._FreeplayBehaviorRewardColor; }
      }
    }
  }
}