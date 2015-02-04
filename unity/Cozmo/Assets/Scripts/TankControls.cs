using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class TankControls : MonoBehaviour {

	[SerializeField] Transform robot = null;
	[SerializeField] SpringSlider sliderLeft = null;
	[SerializeField] SpringSlider sliderRight = null;
	[SerializeField] float maxVel = 10f;
	[SerializeField] float maxTurn = 90f;

	void OnEnable () {
		//acquire the robot
		//reset default state for this control scheme test
		Debug.Log ("TankControls OnEnable");
	}

	void Update () {



		//take our two control axes and if changed, send them to the robot
		float left = sliderLeft.value;
		float right = sliderRight.value;
		float speed = (left + right) * 0.5f * maxVel;
		float turn = (left - right) * 0.5f * maxTurn;

		float turnThisFrame = Mathf.Min (maxTurn * Time.deltaTime, Mathf.Abs(turn) * maxTurn);
		if (turn < 0f) turnThisFrame = -turnThisFrame;
		robot.rotation *= Quaternion.AngleAxis (turnThisFrame, robot.up);
		
		Vector3 idealMove = robot.forward * speed;
		robot.position += idealMove * Time.deltaTime;

	}

	void OnDisable () {
		//clean up this controls test if needed
		Debug.Log ("TankControls OnDisable");
	}

}
