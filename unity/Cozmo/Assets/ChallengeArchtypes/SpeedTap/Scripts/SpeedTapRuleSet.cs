using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public enum SpeedTapRuleSet {
    NoRed,
    TwoColor,
    LightCountThreeColor,
    LightCountMultiColor
  }

  public abstract class SpeedTapRulesBase {
    public abstract void SetLights(bool shouldMatch, SpeedTapGame game);

    // TODO: Move to tunable data
    private const float _kRedPercentChance = 0.33f;

    protected Color[] _Colors;

    public SpeedTapRulesBase() {
      // defaults
      _Colors = new Color[] { Color.yellow, Color.green, Color.blue, Color.magenta };
    }

    public virtual void SetUsableColors(Color[] colors) {
      _Colors = colors;
    }

    protected bool TrySetCubesRed(SpeedTapGame game) {
      bool setCubesRed = false;
      if (UnityEngine.Random.Range(0.0f, 1.0f) < _kRedPercentChance) {
        game.SetLEDs(game.CozmoBlockID, Color.red);
        game.SetLEDs(game.PlayerBlockID, Color.red);

        // If someone taps, the winner should be flashing white
        game.PlayerWinColor = Color.white;
        game.CozmoWinColor = Color.white;
        setCubesRed = true;
      }
      game.RedMatch = setCubesRed;
      return setCubesRed;
    }

    protected int PickRandomColor() {
      return UnityEngine.Random.Range(0, _Colors.Length);
    }

    protected void PickTwoDifferentColors(out int colorIndex1, out int colorIndex2) {
      colorIndex1 = UnityEngine.Random.Range(0, _Colors.Length);
      colorIndex2 = UnityEngine.Random.Range(0, _Colors.Length);
      while (colorIndex2 == colorIndex1) {
        colorIndex2 = UnityEngine.Random.Range(0, _Colors.Length);
      }
    }

    protected void SetABABLights(Color[] lightsToSet, int colorIndex1, int colorIndex2) {
      for (int i = 0; i < lightsToSet.Length; i++) {
        lightsToSet[i] = _Colors[i % 2 == 0 ? colorIndex1 : colorIndex2];
      }
    }

    protected void SetLightsRandomly(Color[] lightsToSet, int[] colorIndices, int indexToSkip = -1) {
      for (int i = 0; i < lightsToSet.Length; i++) {
        if (i != indexToSkip) {
          lightsToSet[i] = _Colors[colorIndices[UnityEngine.Random.Range(0, colorIndices.Length)]];
        }
      }
    }

    protected void CopyLights(Color[] targetColors, Color[] sourceColors) {
      for (int i = 0; i < targetColors.Length; i++) {
        targetColors[i] = sourceColors[i % sourceColors.Length];
      }
    }
  }
}