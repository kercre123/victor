using UnityEngine;
using System.Collections;

public class MiniGameCreator : MonoBehaviour {

  [SerializeField]
  GameBase gamePrefab_;

  void Start() {
    RobotEngineManager.instance.RobotConnected += Connected;
  }

  void Connected(int robotID) {
    GameObject.Instantiate(gamePrefab_);
  }

}
