using UnityEngine;
using System.Collections;

public class RobotRelativeControls : MonoBehaviour {

	[SerializeField] Transform robot = null;
	[SerializeField] VirtualStick moveStick = null;

	[SerializeField] float maxTurn = 90f;

	Vector2 inputs = Vector2.zero;
	Vector2 lastInputs = Vector2.zero;
	float timeSinceLastCommand = 0f;
	float refreshTime = 0.1f;

	float leftWheelSpeed = 0f;
	float rightWheelSpeed = 0f;

	void OnEnable() {
		//acquire the robot
		//reset default state for this control scheme test
		Debug.Log("RobotRelativeControls OnEnable");
	}

	void FixedUpdate() {
		timeSinceLastCommand += Time.deltaTime;

		if(timeSinceLastCommand < refreshTime)
			return;

		timeSinceLastCommand = 0f;

		//take our v-pad control axes and calc translate to robot
		inputs = new Vector2(moveStick.Horizontal, moveStick.Vertical);

		if(inputs.x == 0f && inputs.y == 0f) {
			inputs.x = Input.GetAxis("Horizontal");
			inputs.y = Input.GetAxis("Vertical");
		}

		if((inputs - lastInputs).magnitude < 0.1f) return;
		lastInputs = inputs;

		if(Intro.CurrentRobotID != 0) {
			RobotEngineManager.CalcWheelSpeedsFromBotRelativeInputs(inputs, out leftWheelSpeed, out rightWheelSpeed);
			RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, leftWheelSpeed, rightWheelSpeed);
			return;
		}

		//no robot, fake it in unity visualization
		if(inputs.x == 0f && inputs.y == 0f)
			return;


		
		float turn = Mathf.Min(maxTurn * Time.deltaTime, Mathf.Abs(inputs.x) * maxTurn);
		if(inputs.x < 0f)
			turn = -turn;

		robot.rotation *= Quaternion.AngleAxis(turn, robot.up);

		Vector3 idealMove = robot.forward * inputs.y * RobotEngineManager.MAX_WHEEL_SPEED;
		robot.position += idealMove * Time.deltaTime;
	}

	void OnDisable() {
		//clean up this controls test if needed
		Debug.Log("RobotRelativeControls OnDisable");
	}

	void OnGUI() {
		GUILayout.BeginVertical();
		GUILayout.Space(100);
		GUILayout.Label("input("+inputs+")");
		GUILayout.Label("leftWheelSpeed("+leftWheelSpeed+")");
		GUILayout.Label("rightWheelSpeed("+rightWheelSpeed+")");
		GUILayout.EndVertical();
	}

}
