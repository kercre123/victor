using UnityEngine;
using System.Collections;
using Anki.UI;
using Anki.Cozmo.Viz;

public class RobotStateTextField : MonoBehaviour {

  [SerializeField]
  private AnkiTextLabel _RobotStateLabel;

  private static bool _sUseAnimString = false;

  public static void UseAnimString(bool enable) {
    _sUseAnimString = enable;
  }

  private void Update() {
    if (RobotEngineManager.Instance.CurrentRobot != null) {
      if (_sUseAnimString) {
        if (VizManager.Instance.AnimationName != _RobotStateLabel.text) {
          _RobotStateLabel.text = VizManager.Instance.AnimationName;
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
