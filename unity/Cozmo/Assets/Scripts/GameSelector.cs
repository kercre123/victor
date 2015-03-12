using UnityEngine;
using UnityEngine.UI;
using System.Collections;


public class GameSelector : GameObjectSelector {
	public void Disconnect() {
		RobotEngineManager.instance.Disconnect ();
		Application.LoadLevel("Shell");
	}
}
