using UnityEngine;
using System.Collections;

public class MiniGameCreator : MonoBehaviour {

  [SerializeField]
  GameBase gamePrefab_;

  void Start() {
    RobotEngineManager.instance.ConnectedToClient += Connected;
  }

  void Connected(string connectionIdentifier) {
    GameObject.Instantiate(gamePrefab_);
  }

}
