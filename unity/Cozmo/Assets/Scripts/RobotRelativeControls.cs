using UnityEngine;
using System.Collections;

public class RobotRelativeControls : MonoBehaviour {

	[SerializeField] Transform robot = null;
	[SerializeField] Joystick moveStick = null;

	[SerializeField] float maxVel = 10f;
	[SerializeField] float maxTurn = 90f;

	void OnEnable () {
		//acquire the robot
		//reset default state for this control scheme test
		Debug.Log ("RobotRelativeControls OnEnable");
	}

	void Update () {
		//take our v-pad control axes and calc translate to robot
		
		float x = moveStick.Horizontal;//Input.GetAxis ("Horizontal");
		float z = moveStick.Vertical; //Input.GetAxis ("Vertical");
		if (x == 0f && z == 0f) return;

		float turn = Mathf.Min (maxTurn * Time.deltaTime, Mathf.Abs(x) * maxTurn);
		if (x < 0f) turn = -turn;
		robot.rotation *= Quaternion.AngleAxis (turn, robot.up);

		Vector3 idealMove = robot.forward * z * maxVel;
		robot.position += idealMove * Time.deltaTime;
	}

	void OnDisable () {
		//clean up this controls test if needed
		Debug.Log ("RobotRelativeControls OnDisable");
	}

}
