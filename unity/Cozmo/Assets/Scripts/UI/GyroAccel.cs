using UnityEngine;
using System.Collections;
using System.Collections.Generic;


namespace MSP_Input {

	public class GyroAccel : MonoBehaviour {

		public bool forceAccelerometer = false;
		public float smoothingTime = 0.1f;
		public float headingOffset = 0f;
		public float pitchOffset = 30f;
		public float pitchOffsetMinimum = -70f;
		public float pitchOffsetMaximum = 70f;
		public float gyroHeadingAmplifier = 1f;
		public float gyroPitchAmplifier = 1f;
		//
		private Quaternion rotation = Quaternion.identity;
		private Transform _transform = null;
		private float heading;
		private float pitch;
		private float roll;

		// STATIC VARIABLES:
		static private bool _forceAccelerometer;
		static private float _smoothingTime;
		static private float _headingOffset;
		static private float _pitchOffset;
		static private float _pitchOffsetMinimum;
		static private float _pitchOffsetMaximum;
		static private float _gyroHeadingAmplifier;
		static private float _gyroPitchAmplifier;
		//
		static private Quaternion _rotation = Quaternion.identity;
		static private float _heading;
		static private float _pitch;
		static private float _roll;

		//================================================================================

		void Awake() {
			Input.compensateSensors = true;
			//
			if (!SystemInfo.supportsGyroscope) {
				forceAccelerometer = true;
				if(!Application.isEditor) Debug.Log("No gyroscope available: forcing accelerometer");
			}
			//
			if (SystemInfo.supportsGyroscope && !forceAccelerometer) {
				Input.gyro.enabled = true;
			}	
			//
			_forceAccelerometer = forceAccelerometer;
			_smoothingTime = smoothingTime;
			_headingOffset = headingOffset;
			_pitchOffset = pitchOffset;
			_pitchOffsetMinimum = pitchOffsetMinimum;
			_pitchOffsetMaximum = pitchOffsetMaximum;
			_gyroHeadingAmplifier = gyroHeadingAmplifier;
			_gyroPitchAmplifier = gyroPitchAmplifier;
			_transform = transform;
		}

		//================================================================================
				
		void Update() {
			forceAccelerometer = _forceAccelerometer;
			smoothingTime = _smoothingTime;
			headingOffset = _headingOffset;
			pitchOffset = _pitchOffset;
			pitchOffsetMinimum = _pitchOffsetMinimum;
			pitchOffsetMaximum = _pitchOffsetMaximum;
			gyroHeadingAmplifier = _gyroHeadingAmplifier;
			gyroPitchAmplifier = _gyroPitchAmplifier;
			//
			CheckHeadingAndPitchBoundaries();
			//
			if (!forceAccelerometer) {
				UpdateGyroscopeOrientation();
			} else {
				UpdateAccelerometerOrientation ();
			}
			//
			_rotation = rotation;
			_heading = heading;
			_pitch = pitch;
			_roll = roll;
			_headingOffset = headingOffset;
			_pitchOffset = pitchOffset;
			//
			_transform.rotation = GetRotation();
		}

		//================================================================================

		void UpdateGyroscopeOrientation() {
			Quaternion gyroQuat = Input.gyro.attitude;
			Quaternion A = new Quaternion(0.5f,0.5f,-0.5f,0.5f);
			Quaternion B = new Quaternion(0f,0f,1f,0f);
			gyroQuat = A * gyroQuat * B;
			//
			float devicePitch;
			float deviceRoll;
			GetDevicePitchAndRollFromGravityVector(out devicePitch, out deviceRoll);
			//
			float rcosin = Mathf.Cos(Mathf.Deg2Rad * deviceRoll);
			float rsinus = Mathf.Sin(Mathf.Deg2Rad * deviceRoll);
			//
			float deltaHeading;
			deltaHeading = (-Input.gyro.rotationRateUnbiased.x * rsinus - Input.gyro.rotationRateUnbiased.y * rcosin);
			gyroHeadingAmplifier = Mathf.Clamp(gyroHeadingAmplifier,0.1f,4f);
			deltaHeading *= (gyroHeadingAmplifier-1f);
			headingOffset += deltaHeading;
			//
			float deltaPitch;
			deltaPitch = (-Input.gyro.rotationRateUnbiased.y * rsinus + Input.gyro.rotationRateUnbiased.x * rcosin);
			gyroPitchAmplifier = Mathf.Clamp(gyroPitchAmplifier,0.1f,4f);
			deltaPitch *= (gyroPitchAmplifier-1f);
			if (devicePitch > pitchOffsetMinimum && devicePitch < pitchOffsetMaximum) {
				pitchOffset += deltaPitch;
			}
			if (devicePitch <= pitchOffsetMinimum) {
				pitchOffset -= Mathf.Abs(deltaPitch);
			}
			if (devicePitch >= pitchOffsetMaximum) {
				pitchOffset += Mathf.Abs(deltaPitch);
			}
			//
			CheckHeadingAndPitchBoundaries();
			// PITCH OFFSET:
			Vector3 gyro_forward = gyroQuat * Vector3.forward;
			Vector3 rotAxis = Vector3.Cross(Vector3.up,gyro_forward);
			AnimationCurve devicePitchAdjustmentCurve = new AnimationCurve(new Keyframe(-90f, 0f), new Keyframe(pitchOffset, -pitchOffset), new Keyframe(90f, 0f));
			Quaternion extra_pitch = Quaternion.AngleAxis(devicePitchAdjustmentCurve.Evaluate(devicePitch),rotAxis);
			gyroQuat = extra_pitch * gyroQuat;
			// HEADING OFFSET:
			Quaternion extra_heading = Quaternion.AngleAxis(headingOffset,Vector3.up);
			gyroQuat = extra_heading * gyroQuat;
			// Smooth gyro quaternion
			float smoothFactor = (smoothingTime > Time.deltaTime) ? Time.deltaTime / smoothingTime : 1f;
			rotation = Quaternion.Slerp(rotation, gyroQuat, smoothFactor);
			// Compute heading, pitch, roll
			Vector3 rf = rotation * Vector3.forward;
			Vector3 prf = Vector3.Cross(Vector3.up,Vector3.Cross(rf,Vector3.up));
			float newHeading = Vector3.Angle(Vector3.forward,prf) * Mathf.Sign (rf.x);
			AnimationCurve headingSmoothCurve = new AnimationCurve(new Keyframe(-90f, 0f, 0f, 0f), 
			                                                       new Keyframe(-85, smoothFactor, 0f, 0f),
			                                                       new Keyframe(85f, smoothFactor, 0f, 0f),
			                                                       new Keyframe(90f,0f,0f,0f));
			heading = Mathf.LerpAngle(heading,newHeading,headingSmoothCurve.Evaluate(pitch));
			pitch = Mathf.LerpAngle(pitch,devicePitch+devicePitchAdjustmentCurve.Evaluate(devicePitch),smoothFactor);
			roll = Mathf.LerpAngle(roll,deviceRoll,smoothFactor);
		}
		
		//================================================================================

		private static AnimationCurve devicePitchAdjustmentCurve = new AnimationCurve(new Keyframe(-90f, 0f), new Keyframe(0, 0), new Keyframe(90f, 0f));
		void UpdateAccelerometerOrientation() {
			float devicePitch;
			float deviceRoll;
			GetDevicePitchAndRollFromGravityVector(out devicePitch, out deviceRoll);
			//
			devicePitchAdjustmentCurve.MoveKey(1, new Keyframe(pitchOffset, -pitchOffset));
			Quaternion accelQuat = Quaternion.identity;
			accelQuat = GetQuaternionFromHeadingPitchRoll(headingOffset, devicePitch+devicePitchAdjustmentCurve.Evaluate(devicePitch), deviceRoll);
			// Smooth gyro quaternion
			float smoothFactor = (smoothingTime > Time.deltaTime) ? Time.deltaTime / smoothingTime : 1f;
			rotation = Quaternion.Slerp(rotation, accelQuat, smoothFactor);
			// Compute heading, pitch, roll
			heading = Mathf.LerpAngle(heading,headingOffset,smoothFactor);
			pitch = Mathf.LerpAngle(pitch,devicePitch+devicePitchAdjustmentCurve.Evaluate(devicePitch),smoothFactor);
			roll = Mathf.LerpAngle(roll,deviceRoll,smoothFactor);
		}

		//================================================================================

		static private AnimationCurve rollAdjustmentCurve = new AnimationCurve(new Keyframe(-90f,0f), new Keyframe(-80f,1f), new Keyframe( 80f, 1f), new Keyframe( 90f, 0f));
		static public void GetDevicePitchAndRollFromGravityVector(out float devicePitch, out float deviceRoll) {
			// Vector holding the direction of gravity
			Vector3 gravity = SystemInfo.supportsGyroscope ? Input.gyro.gravity : Input.acceleration;
			// find the projections of the gravity vector on the YZ-plane
			Vector3 gravityProjectedOnXYplane = Vector3.Cross(Vector3.forward,Vector3.Cross(gravity,Vector3.forward));
			// calculate the pitch = rotation around x-axis ("dive forward/backward")
			devicePitch = Vector3.Angle(gravity,Vector3.forward) - 90;
			// calculate the roll = rotation around z-axis ("steer left/right")
			deviceRoll = Vector3.Angle(gravityProjectedOnXYplane,-Vector3.up) * Mathf.Sign(Vector3.Cross(gravityProjectedOnXYplane,Vector3.down).z);
			deviceRoll *= rollAdjustmentCurve.Evaluate(devicePitch);
		}

		//================================================================================

		void CheckHeadingAndPitchBoundaries() {
			if (heading > 360f) {
				heading -=360f;
			}
			if (heading < 0f) {
				heading += 360f;
			}
			//
			if (pitchOffset < pitchOffsetMinimum) {
				pitchOffset = pitchOffsetMinimum;
			}
			if (pitchOffset > pitchOffsetMaximum) {
				pitchOffset = pitchOffsetMaximum;
			}
		}

		//================================================================================
		
		static public Quaternion GetQuaternionFromHeadingPitchRoll(float inputHeading, float inputPitch, float inputRoll) {
			Quaternion returnQuat = Quaternion.Euler(0f,inputHeading,0f) * Quaternion.Euler(inputPitch,0f,0f) * Quaternion.Euler(0f,0f,inputRoll);
			return returnQuat;
		}

		//================================================================================
		//================================================================================
		//================================================================================

		//================================================================================

		static public Quaternion GetRotation() {
			return _rotation;
		}	
		
		//================================================================================
		
		static public float GetHeading() {
			return _heading;
		}	

		//================================================================================
		
		static public float GetPitch() {
			return _pitch;
		}	

		//================================================================================
		
		static public float GetRoll() {
			return _roll;
		}	

		//================================================================================

		static public void GetHeadingPitchRoll(out float h, out float p, out float r) {
			h = _heading;
			p = _pitch;
			r = _roll;
		}	
		
		//================================================================================
		
		static public void SetSmoothingTime(float smoothTime) {
			_smoothingTime = smoothTime;
		}
		
		//================================================================================
		
		static public float GetSmoothingTime() {
			return _smoothingTime;
		}
		
		//================================================================================
		
		static public void AddFloatToHeadingOffset(float extraHeadingOffset) {
			_headingOffset += extraHeadingOffset;
		}
		
		//================================================================================
		
		static public float GetHeadingOffset() {
			return _headingOffset;
		}
		
		//================================================================================
		
		static public void SetHeadingOffset(float newHeadingOffset) {
			_headingOffset = newHeadingOffset;
		}
		
		//================================================================================
		
		static public void AddFloatToPitchOffset(float extraPitchOffset) {
			_pitchOffset += extraPitchOffset;
		}
			
		//================================================================================
		
		static public float GetPitchOffset() {
			return _pitchOffset;
		}

		//================================================================================
		
		static public void SetPitchOffset(float newPitchOffset) {
			_pitchOffset = newPitchOffset;
		}

		//================================================================================

		static public void SetPitchOffsetMinumumMaximum(float newPitchOffsetMinimum, float newPitchOffsetMaximum) {
			_pitchOffsetMinimum = newPitchOffsetMinimum;
			_pitchOffsetMaximum = newPitchOffsetMaximum;
		}

		//================================================================================
		
		static public void SetGyroHeadingAmplifier(float newValue) {
			_gyroHeadingAmplifier = newValue;
		}

		//================================================================================
		
		static public float GetGyroHeadingAmplifier() {
			return _gyroHeadingAmplifier;
		}

		//================================================================================
		
		static public void SetGyroPitchAmplifier(float newValue) {
			_gyroPitchAmplifier = newValue;
		}

		//================================================================================

		static public float GetGyroPitchAmplifier() {
			return _gyroPitchAmplifier;
		}
		
		//================================================================================
		
		static public void SetForceAccelerometer(bool newValue) {
			_forceAccelerometer = newValue;
		}

		//================================================================================
		
		static public bool GetForceAccelerometer() {
			return _forceAccelerometer;
		}

		//================================================================================

	}

}