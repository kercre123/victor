using UnityEngine;
using System.Collections;

public class GameBase : MonoBehaviour {
  public Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.CurrentRobot : null; } }
}
