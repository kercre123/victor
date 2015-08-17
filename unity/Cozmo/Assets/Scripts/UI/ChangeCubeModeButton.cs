using UnityEngine;
using System.Collections;

public class ChangeCubeModeButton : MonoBehaviour {

  Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  public void ChangeMode() {
    if (GameActions.instance == null || robot == null || robot.carryingObject == null)
      return;

    Debug.Log("ChangeCubeModeButton.ChangeMode()");
    GameActions.instance.Change(true, robot.carryingObject);
  }
}
