
using UnityEngine;
using System;

public static class CozmoUtil {

	public const float MAX_WHEEL_SPEED 		= 150f;
	public const float MAX_ANALOG_RADIUS   	= 300f;
	public const float HALF_PI             	= Mathf.PI * 0.5f;
	public const float wheelDistHalfMM 		= 23.85f; //47.7f / 2.0; // distance b/w the front wheels
	
	public static void CalcWheelSpeedsFromBotRelativeInputsA(Vector2 inputs, out float leftWheelSpeed, out float rightWheelSpeed) {
		
		leftWheelSpeed = 0f;
		rightWheelSpeed = 0f;
		
		if(inputs.x == 0f && inputs.y == 0f)
			return;
		
		// Compute speed
		float xyMag = Mathf.Min(1f, inputs.magnitude);
		
		// Driving forward?
		float fwd = inputs.y >= 0f ? 1f : -1f;
		
		// Curving right?
		float right = inputs.x >= 0f ? 1f : -1f;
		
		// Base wheel speed based on magnitude of input and whether or not robot is driving forward
		float baseWheelSpeed = MAX_WHEEL_SPEED * xyMag * fwd;
		
		// Angle of xy input used to determine curvature
		float xyAngle = 0f;
		if(inputs.x != 0f) xyAngle = Mathf.Abs(Mathf.Atan((float)inputs.y / (float)inputs.x)) * right;
		//if(inputs.x != 0f) xyAngle = Vector2.Angle(inputs, Vector2.up) * right * Mathf.Deg2Rad;
		
		// Compute radius of curvature
		float roc = (xyAngle / HALF_PI) * MAX_ANALOG_RADIUS;
		
		// Compute individual wheel speeds
		if(inputs.x == 0f) { //Mathf.Abs(xyAngle) > HALF_PI - 0.1f) {
			// Straight fwd/back
			leftWheelSpeed = baseWheelSpeed;
			rightWheelSpeed = baseWheelSpeed;
		}
		else if(inputs.y == 0f) { //Mathf.Abs(xyAngle) < 0.1f) {
			// Turn in place
			leftWheelSpeed = right * xyMag * MAX_WHEEL_SPEED;
			rightWheelSpeed = -right * xyMag * MAX_WHEEL_SPEED;
		}
		else {
			
			leftWheelSpeed = baseWheelSpeed * (roc + (right * wheelDistHalfMM)) / roc;
			rightWheelSpeed = baseWheelSpeed * (roc - (right * wheelDistHalfMM)) / roc;
			
			// Swap wheel speeds
			if(fwd * right < 0f) {
				float temp = leftWheelSpeed;
				leftWheelSpeed = rightWheelSpeed;
				rightWheelSpeed = temp;
			}
		}

		// Cap wheel speed at max
		if(Mathf.Abs(leftWheelSpeed) > MAX_WHEEL_SPEED) {
			float correction = leftWheelSpeed - (MAX_WHEEL_SPEED * (leftWheelSpeed >= 0f ? 1f : -1f));
			float correctionFactor = 1f - Mathf.Abs(correction / leftWheelSpeed);
			leftWheelSpeed *= correctionFactor;
			rightWheelSpeed *= correctionFactor;
			//printf("lcorrectionFactor: %f\n", correctionFactor);
		}
		
		if(Mathf.Abs(rightWheelSpeed) > MAX_WHEEL_SPEED) {
			float correction = rightWheelSpeed - (MAX_WHEEL_SPEED * (rightWheelSpeed >= 0f ? 1f : -1f));
			float correctionFactor = 1f - Mathf.Abs(correction / rightWheelSpeed);
			leftWheelSpeed *= correctionFactor;
			rightWheelSpeed *= correctionFactor;
			//printf("rcorrectionFactor: %f\n", correctionFactor);
		}
		
	}

	public static void CalcWheelSpeedsFromBotRelativeInputsB(Vector2 inputs, out float leftWheelSpeed, out float rightWheelSpeed) {
		
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
			leftWheelSpeed = turn > 0f ? speed : -speed;
			rightWheelSpeed = turn > 0f ? -speed : speed;
		}
		else {
			float speedA = speed;
			float speedB = Mathf.Lerp(speed, -speed * 0.5f, Mathf.Abs(turn));

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

	public static void CalcDriveWheelSpeedsForInputs(Vector2 inputs, out float leftWheelSpeed, out float rightWheelSpeed, float maxAngle, bool reverse=false) {
		
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
		turn = turn*turn * (turn < 0f ? -1f : 1f);

		leftWheelSpeed = speed;
		rightWheelSpeed = speed;
			
		if(turn != 0f) {
			float speedA = speed;
			float speedB = Mathf.Lerp(speed, -speed, Mathf.Abs(turn));

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

	public static void CalcTurnInPlaceWheelSpeeds(float x, out float leftWheelSpeed, out float rightWheelSpeed) {
		x = x*x * (x < 0f ? -1f : 1f);

		leftWheelSpeed = x * MAX_WHEEL_SPEED;
		rightWheelSpeed = -x * MAX_WHEEL_SPEED;
	}

	public static void SquareInputs(ref Vector2 inputs) {
		inputs.x = inputs.x*inputs.x * (inputs.x < 0f ? -1f : 1f);
		inputs.y = inputs.y*inputs.y * (inputs.y < 0f ? -1f : 1f);
	}

}
