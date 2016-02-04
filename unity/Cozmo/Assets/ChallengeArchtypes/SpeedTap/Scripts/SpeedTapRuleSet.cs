using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public enum SpeedTapRuleSet {
    Default,
    NoRed,
    TwoColor,
    LightCountTwoColor,
    LightCountMultiColor,
    LightCountNoColor,
    LightCountSameColorNoTap,
  }

  public interface ISpeedTapRules {
    void SetLights(bool shouldTap, SpeedTapGame game);
  }
}