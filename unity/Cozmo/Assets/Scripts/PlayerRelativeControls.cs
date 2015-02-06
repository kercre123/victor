using UnityEngine;
using System.Collections;

public class PlayerRelativeControls : MonoBehaviour {

	[SerializeField] Transform robot = null;
	[SerializeField] Joystick moveStick = null;
	[SerializeField] float maxVel = 10f;
	[SerializeField] float maxTurn = 90f;

	Vector2 inputs = Vector2.zero;
	//Vector2 lastInputs = Vector2.zero;
	float timeSinceLastCommand = 0f;
	float refreshTime = 0.1f;
	
	float leftWheelSpeed = 0f;
	float rightWheelSpeed = 0f;

	void OnEnable() {
		//acquire the robot
		//reset default state for this control scheme test
		Debug.Log("PlayerRelativeControls OnEnable");
	}

	void FixedUpdate() {
		//take our v-pad axes, then map them to camera relative desired movement vector
		//compare desired movement vector to robot's current transform and wheel status
		//send updated wheel commands to robot if necessary

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

		//dmd translate into cozmo space here, requires cam pos * rot, and coz pos and rot

		//if((inputs - lastInputs).magnitude < 0.1f) return;
		//lastInputs = inputs;
		
		//if(Intro.CurrentRobotID != 0) {
		//	RobotEngineManager.CalcWheelSpeedsFromBotRelativeInputs(inputs, out leftWheelSpeed, out rightWheelSpeed);
		//	RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, leftWheelSpeed, rightWheelSpeed);
		//	return;
		//}
		
		//no robot, fake it in unity visualization
		if(inputs.x == 0f && inputs.y == 0f)
			return;

		Vector3 camForwardFlat = Camera.main.transform.forward;
		camForwardFlat.y = 0f;
		camForwardFlat.Normalize();
		
		Vector3 idealMove = camForwardFlat * inputs.y * maxVel + Camera.main.transform.right * inputs.x * maxVel;
		
		float angle = Vector3.Angle(robot.forward, idealMove.normalized);
		
		float turnThisFrame = Mathf.Min(maxTurn * Time.deltaTime, Mathf.Abs(angle));
		if(Vector3.Dot(robot.right, idealMove.normalized) < 0f)
			turnThisFrame = -turnThisFrame;
		
		//Debug.Log ("PlayerRelativeControls angle("+angle+") turnThisFrame("+turnThisFrame+") x("+x+") z("+z+")");
		
		robot.rotation *= Quaternion.AngleAxis(turnThisFrame, robot.up);
		
		if(Vector3.Dot(robot.forward, idealMove.normalized) > 0f) {
			robot.position += robot.forward * idealMove.magnitude * Time.deltaTime;
		}
	}

	void OnDisable() {
		//clean up this controls test if needed
		Debug.Log("PlayerRelativeControls OnDisable");
	}

}
