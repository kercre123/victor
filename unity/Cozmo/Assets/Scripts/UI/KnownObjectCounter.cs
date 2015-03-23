using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class KnownObjectCounter : MonoBehaviour {
	[SerializeField] protected Text text = null;

	void Update() {
		if(RobotEngineManager.instance != null && RobotEngineManager.instance.current != null) {
			text.text = "known: " + RobotEngineManager.instance.current.knownObjects.Count.ToString();
		}
	}
}
