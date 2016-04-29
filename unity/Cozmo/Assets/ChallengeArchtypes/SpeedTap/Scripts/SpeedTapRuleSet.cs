using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public enum SpeedTapRuleSet {
    NoRed,
    TwoColor,
    LightCountTwoColor,
    LightCountMultiColor
  }

  public abstract class SpeedTapRulesBase {
    public abstract void SetLights(bool shouldTap, SpeedTapGame game);

    protected Color[] _Colors;

    public SpeedTapRulesBase() {
      // defaults
      _Colors = new Color[]{ Color.yellow, Color.green, Color.blue, Color.magenta };
    }

    public virtual void SetUsableColors(Color[] colors) {
      _Colors = colors;
    }
  }
}