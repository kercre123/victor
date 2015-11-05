using UnityEngine;
using System.Collections;

// Provides common interface for HubWorlds to react to games
// ending and to start/restart games. Also has interface for killing games
public abstract class GameBase : MonoBehaviour {
  [SerializeField]
  string gameName_;
  public string GameName { get { return gameName_; } private set { gameName_ = value; } }

  public Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.CurrentRobot : null; } }

  public abstract void CleanUp();
}
