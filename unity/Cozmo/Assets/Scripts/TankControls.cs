using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class TankControls : MonoBehaviour {

	//[SerializeField] Transform robot = null;
	[SerializeField] VirtualStick sliderLeft = null;
	[SerializeField] VirtualStick sliderRight = null;
	//[SerializeField] float maxTurn = 2f;

	void OnEnable() {
		//acquire the robot
		//reset default state for this control scheme test
		Debug.Log("TankControls OnEnable");
	}

	void Update() {

//		if(sliderLeft.IsPressed && sliderRight.IsPressed) {
//
//			if(sliderLeft.PressedTime < 0.5f && sliderRight.PressedTime > sliderLeft.PressedTime) {
//
//			}
//
//		}

		//take our two control axes and if changed, send them to the robot
		float left = sliderLeft.Vertical;
		float right = sliderRight.Vertical;

		if(Intro.CurrentRobotID != 0) {
			RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, left * RobotEngineManager.MAX_WHEEL_SPEED, right * RobotEngineManager.MAX_WHEEL_SPEED);
			return;
		}

		//no robot, fake it in unity visualization
//		float speed = (left + right) * 0.5f * RobotEngineManager.MAX_WHEEL_SPEED;
//		float turn = (left - right) * 0.5f * maxTurn;
//
//		float turnThisFrame = Mathf.Min(maxTurn * Time.deltaTime, Mathf.Abs(turn) * maxTurn);
//		if(turn < 0f) turnThisFrame = -turnThisFrame;
//		robot.rotation *= Quaternion.AngleAxis(turnThisFrame, robot.up);
//		
//		Vector3 idealMove = robot.forward * speed;
//		robot.position += idealMove * Time.deltaTime;

	}

	void OnDisable() {
		//clean up this controls test if needed
		Debug.Log("TankControls OnDisable");
	}

}
