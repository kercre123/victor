using UnityEngine;
using System.Collections;

public class GameBase : MonoBehaviour {
  public Robot GetRobot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }
}
