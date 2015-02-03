using UnityEngine;
using System.Collections;

public class RobotEngineManager : MonoBehaviour {

	public static RobotEngineManager instance;

	// Use this for initialization
	void Awake () {
		instance = this;
	}
	
	// Update is called once per frame
	void Update () {
	
	}
}
