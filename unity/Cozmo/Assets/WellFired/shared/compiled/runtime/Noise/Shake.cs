using UnityEngine;
using System;
using System.Collections;

namespace WellFired.Shared
{
	[Serializable]
	public class Shake
	{
		public ShakeType ShakeType
		{
			get { return shakeType; }
			set 
			{ 
				shakeType = value; 
				if(shakeType == ShakeType.None)
				{
					Position = Vector3.zero;
					EulerRotation = Vector3.zero;
				}
			}
		}

		public Vector3 Position 
		{
			get;
			set;
		}

		public Vector3 EulerRotation 
		{
			get;
			set;
		}
		
		public float ShakeSpeedPosition
		{
			get { return shakeSpeedPosition; }
			set { shakeSpeedPosition = value; }
		}

		public Vector3 ShakeRangePosition
		{
			get { return shakeRangePosition; }
			set { shakeRangePosition = value; }
		}

		public float ShakeSpeedRotation
		{
			get { return shakeSpeedRotation; }
			set { shakeSpeedRotation = value; }
		}

		public Vector3 ShakeRangeRotation
		{
			get { return shakeRangeRotation; }
			set { shakeRangeRotation = value; }
		}

		private ShakeType shakeType = ShakeType.None;
		private float shakeSpeedPosition = 0.38f;
		private Vector3 shakeRangePosition = new Vector3(0.4f, 0.4f, 0.4f);
		private float shakeSpeedRotation = 0.38f;
		private Vector3 shakeRangeRotation = new Vector3(4.0f, 4.0f, 4.0f);

		public void InitialiseShake(ShakeType shakeType)
		{
			ShakeType = shakeType;
			Position = Vector3.zero;
			EulerRotation = Vector3.zero;
		}
		
		public void InitialiseShake(ShakeType shakeType, float shakeSpeedPosition, Vector3 shakeRangePosition, float shakeSpeedRotation, Vector3 shakeRangeRotation)
		{
			this.shakeSpeedPosition = shakeSpeedPosition;
			this.shakeRangePosition = shakeRangePosition;
			this.shakeSpeedRotation = shakeSpeedRotation;
			this.shakeRangeRotation = shakeRangeRotation;
			ShakeType = shakeType;
			Position = Vector3.zero;
			EulerRotation = Vector3.zero;
		}

		public void Process(float time, float duration)
		{
			if(ShakeType == ShakeType.None)
				return;

			if(ShakeType == ShakeType.Position || ShakeType == ShakeType.Both)
				Position = Vector3.Scale(InterpolatedNoise.GetVector3(shakeSpeedPosition, time), shakeRangePosition);

			if(ShakeType == ShakeType.Rotation || ShakeType == ShakeType.Both) 
				EulerRotation = Vector3.Scale(InterpolatedNoise.GetVector3(shakeSpeedRotation, time), shakeRangeRotation);
		}
	}
}