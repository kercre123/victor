using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class KnownObjectCounter : MonoBehaviour {
  [SerializeField] protected Text text = null;

  Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

  void Update() {
    if(robot != null) {
      text.text = "known: " + robot.knownObjects.Count.ToString();
    }
  }
}
