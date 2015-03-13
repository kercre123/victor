using UnityEngine;
using UnityEngine.UI;
using System.Collections;


public class GameSelector : GameObjectSelector {

	public static bool InMenu { get; private set; }


	protected override void OnEnable() {
		base.OnEnable();
		InMenu = true;
	}

	protected void OnDisable() {
		InMenu = false;
	}

	public void Disconnect() {
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.Disconnect ();
		Application.LoadLevel("Shell");
	}
}
