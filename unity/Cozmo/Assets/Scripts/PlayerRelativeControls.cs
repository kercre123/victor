using UnityEngine;
using System.Collections;

public class PlayerRelativeControls : MonoBehaviour {

	[SerializeField] Transform robot = null;
	[SerializeField] Joystick moveStick = null;

	[SerializeField] float maxVel = 10f;
	[SerializeField] float maxTurn = 90f;

	void OnEnable () {
		//acquire the robot
		//reset default state for this control scheme test
		Debug.Log ("PlayerRelativeControls OnEnable");
	}

	void Update () {
		//take our v-pad axes, then map them to camera relative desired movement vector
		//compare desired movement vector to robot's current transform and wheel status
		//send updated wheel commands to robot if necessary

		float x = moveStick.Horizontal;
		float z = moveStick.Vertical; 

		if (x == 0f && z == 0f) {
			x = Input.GetAxis ("Horizontal");
			z = Input.GetAxis ("Vertical");
		}


		if (x == 0f && z == 0f) return;

		Vector3 camForwardFlat = Camera.main.transform.forward;
		camForwardFlat.y = 0f;
		camForwardFlat.Normalize ();

		Vector3 idealMove = camForwardFlat * z * maxVel + Camera.main.transform.right * x * maxVel;

		float angle = Vector3.Angle (robot.forward, idealMove.normalized);

		float turnThisFrame = Mathf.Min (maxTurn * Time.deltaTime, Mathf.Abs (angle));
		if (Vector3.Dot (robot.right, idealMove.normalized) < 0f) turnThisFrame = -turnThisFrame;

		//Debug.Log ("PlayerRelativeControls angle("+angle+") turnThisFrame("+turnThisFrame+") x("+x+") z("+z+")");

		robot.rotation *= Quaternion.AngleAxis (turnThisFrame, robot.up);

		if(Vector3.Dot (robot.forward, idealMove.normalized) > 0f) {
			robot.position += robot.forward * idealMove.magnitude * Time.deltaTime;
		}
	}

	void OnDisable () {
		//clean up this controls test if needed
		Debug.Log ("PlayerRelativeControls OnDisable");
	}

}
