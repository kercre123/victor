using UnityEngine;
using System.Collections;

public class RobotPerspectiveControls : MonoBehaviour {

	void OnEnable () {
		//acquire the robot
		//reset default state for this control scheme test
		Debug.Log ("RobotPerspectiveControls OnEnable");
	}

	void Update () {
		//retrieve and display last view from robot's camera
		//retrive potential target info for targets within fov
		//allow selecting available targets within robot's fov
		//if target selected send interact command to robot

		//elsewise
		//take our v-pad control axes and calc translate to robot

	}

	void OnDisable () {
		//clean up this controls test if needed
		Debug.Log ("RobotPerspectiveControls OnDisable");
	}

}
