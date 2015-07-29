using UnityEngine;
using UnityEngine.UI;
using System.Collections;

/// <summary>
/// deprecated research into more classic gaming thumbstick controls relative to the player's input device instead of the robot
/// </summary>
public class PlayerRelativeControls : MonoBehaviour
{
	
	#region INSPECTOR FIELDS

	[SerializeField] VirtualStick stick = null;
	[SerializeField] Text text_x = null;
	[SerializeField] Text text_y = null;
	[SerializeField] Text text_leftWheelSpeed = null;
	[SerializeField] Text text_rightWheelSpeed = null;

	#endregion

	#region MISC MEMBERS

	Vector2 inputs = Vector2.zero;
	float leftWheelSpeed = 0f;
	float rightWheelSpeed = 0f;
	bool moveCommandLastFrame = false;
	float robotFacing = 0f;
	float robotStartHeading = 0f;
	float compassStartHeading = 0f;

	#endregion

	#region COMPONENT CALLBACKS

	void OnEnable()
	{
		//reset default state for this control scheme test
		Debug.Log("PlayerRelativeControls OnEnable");
		Input.compass.enabled = true;

		moveCommandLastFrame = false;
		if(robot != null)
		{
			robotStartHeading = MathUtil.ClampAngle180(robot.poseAngle_rad * Mathf.Rad2Deg);
			//Debug.Log("frame("+Time.frameCount+") robotFacing(" + robotFacing + ")");
		}

		compassStartHeading = MathUtil.ClampAngle180(-Input.compass.trueHeading);
		Debug.Log("frame(" + Time.frameCount + ") robotStartHeading(" + robotStartHeading + ") compassStartHeading(" + compassStartHeading + ")");
	}

	Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

	void Update()
	{
		if(robot == null) return;

		robotFacing = MathUtil.ClampAngle180(robot.poseAngle_rad * Mathf.Rad2Deg);
		//robotFacingStale = false;
		//Debug.Log("frame("+Time.frameCount+") robotFacing(" + robotFacing + ")");

		//take our v-pad control axes and calc translate to robot
		inputs = Vector2.zero;

		if(stick != null)
		{
			inputs.x = stick.Horizontal;
			inputs.y = stick.Vertical;
		}

		if(!moveCommandLastFrame && inputs.sqrMagnitude == 0f)
		{
			return; // command not changed
		}
		if(inputs.sqrMagnitude == 0f)
		{
			robot.DriveWheels(0f, 0f);
			return; // command zero'd, let's full stop
		}

		float relativeScreenFacing = MathUtil.AngleDelta(compassStartHeading, MathUtil.ClampAngle180(-Input.compass.trueHeading));
		float relativeRobotFacing = MathUtil.AngleDelta(robotStartHeading, robotFacing);

		float screenToRobot = MathUtil.AngleDelta(relativeScreenFacing, relativeRobotFacing);
		float throwAngle = Vector2.Angle(Vector2.up, inputs.normalized) * -Mathf.Sign(Vector2.Dot(inputs.normalized, Vector2.right));
		float relativeAngle = MathUtil.AngleDelta(screenToRobot, throwAngle);

		Debug.Log("RobotRelativeControls relativeScreenFacing(" + relativeScreenFacing + ") relativeRobotFacing(" + relativeRobotFacing + ") screenToRobot(" + screenToRobot + ") throwAngle(" + throwAngle + ") relativeAngle(" + relativeAngle + ")");
		inputs = Quaternion.AngleAxis(-screenToRobot, Vector3.forward) * inputs;

		CozmoUtil.CalcWheelSpeedsForPlayerRelInputs(inputs, out leftWheelSpeed, out rightWheelSpeed);

		robot.DriveWheels(leftWheelSpeed, rightWheelSpeed);

		moveCommandLastFrame = inputs.sqrMagnitude > 0f;
		
		RefreshDebugText();
	}

	void OnDisable()
	{
		//clean up this controls test if needed
		Debug.Log("RobotRelativeControls OnDisable");
		
		if(robot != null && RobotEngineManager.instance.IsConnected)
		{
			robot.DriveWheels(0f, 0f);
		}
	}

	#endregion

	#region PRIVATE METHODS

	void RefreshDebugText()
	{
		if(text_x != null) text_x.text = "x(" + inputs.x.ToString("N") + ")";
		if(text_y != null) text_y.text = "y(" + inputs.y.ToString("N") + ")";
		
		if(text_leftWheelSpeed != null) text_leftWheelSpeed.text = "L(" + leftWheelSpeed.ToString("N") + ")";
		if(text_rightWheelSpeed != null) text_rightWheelSpeed.text = "R(" + rightWheelSpeed.ToString("N") + ")";
	}

	#endregion

	#region PUBLIC METHODS

	#endregion
	
}
