using UnityEngine;
using System.Collections;

public class MiniGameCreator : MonoBehaviour {

  [SerializeField]
  GameBase gamePrefab_;

  void Start() {
    GameObject.Instantiate(gamePrefab_);
  }

}
