using UnityEngine;
using UnityEngine.UI;
using System.Collections;


public class GameSelector : GameObjectSelector {

	public static bool InMenu { get { return instance != null && instance.index == 0; } }

	public static GameSelector instance = null;

	void Awake() {
		instance = this;
	}

	void OnDestroy() {
		if(instance == this) instance = null;
	}

	public void Disconnect() {
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.Disconnect ();
		Application.LoadLevel("Shell");
	}
}
