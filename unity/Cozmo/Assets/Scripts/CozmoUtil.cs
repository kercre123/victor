
using UnityEngine;
using System;

public static class CozmoUtil {

	public const float MAX_WHEEL_SPEED 		= 150f;

	public static void CalcWheelSpeedsForTwoAxisInputs(Vector2 inputs, out float leftWheelSpeed, out float rightWheelSpeed, float maxTurnFactor) {
		
		leftWheelSpeed = 0f;
		rightWheelSpeed = 0f;
		
		if(inputs.x == 0f && inputs.y == 0f) return;

		float speed = inputs.magnitude;
		float turn = inputs.x;

		speed = Mathf.Clamp01(speed*speed) * (inputs.y >= 0f ? 1f : -1f);
		//turn = Mathf.Clamp01(turn*turn) * (turn >= 0f ? 1f : -1f);

		if(turn == 0f) { 
			leftWheelSpeed = speed;
			rightWheelSpeed = speed;
		}
		else if(inputs.y == 0f) {
			speed *= maxTurnFactor;
			leftWheelSpeed = turn > 0f ? speed : -speed;
			rightWheelSpeed = turn > 0f ? -speed : speed;
		}
		else {
			float speedA = speed;
			float speedB = Mathf.Lerp(speed, -speed * maxTurnFactor * 0.5f, Mathf.Abs(turn));

			if(turn > 0f) {
				leftWheelSpeed = speedA;
				rightWheelSpeed = speedB;
			}
			else if(turn < 0f) {
				rightWheelSpeed = speedA;
				leftWheelSpeed = speedB;
			}
		}

		//Debug.Log("CalcWheelSpeedsFromBotRelativeInputsB speed("+speed+") turn("+turn+")");

		//scale to maximum wheel speeds
		leftWheelSpeed *= MAX_WHEEL_SPEED;
		rightWheelSpeed *= MAX_WHEEL_SPEED;
	}

	public static void CalcWheelSpeedsForThumbStickInputs(Vector2 inputs, out float leftWheelSpeed, out float rightWheelSpeed, float maxAngle, float maxTurnFactor, bool reverse=false) {
		
		leftWheelSpeed = 0f;
		rightWheelSpeed = 0f;
		
		if(inputs.x == 0f && inputs.y == 0f) return;

		float speed = inputs.magnitude;
		float turn = 0f;

		if(!reverse) {
			if(maxAngle > 0f) turn = Mathf.Clamp01(Vector2.Angle(Vector2.up, inputs) / maxAngle) * (inputs.x >= 0f ? 1f : -1f);
		}
		else {
			if(maxAngle > 0f) turn = Mathf.Clamp01(Vector2.Angle(-Vector2.up, inputs) / maxAngle) * (inputs.x >= 0f ? 1f : -1f);
			speed = -speed;
		}

		speed = speed*speed * (speed < 0f ? -1f : 1f);
		//turn = turn*turn * (turn < 0f ? -1f : 1f);

		leftWheelSpeed = speed;
		rightWheelSpeed = speed;
			
		if(turn != 0f) {
			float speedA = speed;
			float speedB = Mathf.Lerp(speed, -speed*maxTurnFactor*0.5f, Mathf.Abs(turn));

			if(turn > 0f) {
				leftWheelSpeed = speedA;
				rightWheelSpeed = speedB;
			}
			else {
				rightWheelSpeed = speedA;
				leftWheelSpeed = speedB;
			}
		}

		//Debug.Log("CalcDriveWheelSpeedsForInputs speed("+speed+") turn("+turn+")");

		//scale to maximum wheel speeds
		leftWheelSpeed *= MAX_WHEEL_SPEED;
		rightWheelSpeed *= MAX_WHEEL_SPEED;
	}

	public static void CalcTurnInPlaceWheelSpeeds(float x, out float leftWheelSpeed, out float rightWheelSpeed, float maxTurnFactor) {
		//x = x*x * (x < 0f ? -1f : 1f);

		leftWheelSpeed = x * MAX_WHEEL_SPEED * maxTurnFactor;
		rightWheelSpeed = -x * MAX_WHEEL_SPEED * maxTurnFactor;
	}

}
