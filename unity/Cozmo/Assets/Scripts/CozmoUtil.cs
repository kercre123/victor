
using UnityEngine;
using System;

public static class CozmoUtil {

	public const float MAX_WHEEL_SPEED 		= 150f;
	public const float MAX_ANALOG_RADIUS   	= 300f;
	public const float HALF_PI             	= Mathf.PI * 0.5f;
	public const float wheelDistHalfMM 		= 23.85f; //47.7f / 2.0; // distance b/w the front wheels
	
	public static void CalcWheelSpeedsFromBotRelativeInputs(Vector2 inputs, out float leftWheelSpeed, out float rightWheelSpeed) {
		
		leftWheelSpeed = 0f;
		rightWheelSpeed = 0f;
		
		if(inputs.x == 0f && inputs.y == 0f)
			return;
		
		// Compute speed
		float xyMag = Mathf.Min(1f, inputs.magnitude);
		
		// Driving forward?
		float fwd = inputs.y > 0f ? 1f : -1f;
		
		// Curving right?
		float right = inputs.x > 0f ? 1f : -1f;
		
		// Base wheel speed based on magnitude of input and whether or not robot is driving forward
		float baseWheelSpeed = MAX_WHEEL_SPEED * xyMag * fwd;
		
		// Angle of xy input used to determine curvature
		float xyAngle = 0f;
		if(inputs.x != 0f) xyAngle = Mathf.Abs(Mathf.Atan((float)inputs.y / (float)inputs.x)) * right;
		
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
	
}
