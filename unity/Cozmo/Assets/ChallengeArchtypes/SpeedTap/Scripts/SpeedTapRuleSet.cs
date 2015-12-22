using UnityEngine;
using System.Collections;

namespace SpeedTap {
  public enum SpeedTapRuleSet {
    Default,
    NoRed
  }

  public interface ISpeedTapRules {
    void SetLights(bool shouldTap, SpeedTapGame game);
  }
}