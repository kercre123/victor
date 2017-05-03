using UnityEngine;
using System.Collections;
using Anki.UI;

public class RobotStateTextField : MonoBehaviour {

  [SerializeField]
  private AnkiTextLegacy _RobotStateLabel;

  // static can be toggled when textfield doesn't exist.
  private static bool _sUseAnimString = false;

  public static void UseAnimString(bool enable) {
    _sUseAnimString = enable;
  }
  public static bool IsAnimString() {
    return _sUseAnimString;
  }

  private void Update() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      if (_sUseAnimString) {
        IRobot robot = RobotEngineManager.Instance.CurrentRobot;
        if (robot.CurrentDebugAnimationString != _RobotStateLabel.text) {
          _RobotStateLabel.text = "Anim: " + robot.CurrentDebugAnimationString;
        }
      }
      else {
        IRobot robot = RobotEngineManager.Instance.CurrentRobot;
        if (robot.CurrentBehaviorString != _RobotStateLabel.text) {
          _RobotStateLabel.text = robot.CurrentBehaviorString;
        }
      }
    }
  }
}
