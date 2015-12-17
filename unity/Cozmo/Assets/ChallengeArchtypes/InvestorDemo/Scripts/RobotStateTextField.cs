using UnityEngine;
using System.Collections;
using Anki.UI;

public class RobotStateTextField : MonoBehaviour {

  [SerializeField]
  private AnkiTextLabel _RobotStateLabel;

  private void Update() {
    if (transform.GetSiblingIndex() != this.transform.parent.childCount - 1) {
      transform.SetAsLastSibling();
    }

    if (RobotEngineManager.Instance.CurrentRobot != null) {
      Robot robot = RobotEngineManager.Instance.CurrentRobot;
      if (robot.CurrentBehaviorString != _RobotStateLabel.text) {
        _RobotStateLabel.text = robot.CurrentBehaviorString;
      }
    }
  }
}
