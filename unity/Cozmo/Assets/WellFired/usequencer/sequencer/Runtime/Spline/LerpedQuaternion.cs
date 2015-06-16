using UnityEngine;
using System.Collections;

namespace WellFired
{
	public struct LerpedQuaternion
	{
		private bool slerp;
		public float duration;
		private Quaternion currentValue;
		private Quaternion target;
		private Quaternion source;	
		private float startTime;
		
		public float x
		{
			get { return currentValue.x; }
			set { SmoothValue = new Quaternion(value, target.y, target.z, target.w); }
		}
		
		public float y
		{
			get { return currentValue.y; }
			set { SmoothValue = new Quaternion(target.x, value, target.y, target.w); }
		}
		
		public float z
		{
			get { return currentValue.z; }
			set { SmoothValue = new Quaternion(target.x, target.y, value, target.w); }
		}
		
		public float w
		{
			get { return currentValue.w; }
			set { SmoothValue = new Quaternion(target.x, target.y, target.z, value); }
		}
		
		public Quaternion SmoothValue
		{
			get
			{
				var deltaTime = (Time.realtimeSinceStartup - startTime) / duration;
				
				if(!slerp)
					currentValue = Quaternion.Lerp(source, target, deltaTime);
				else
					currentValue = Quaternion.Slerp(source, target, deltaTime);
				
				return currentValue;
			}
			set
			{
				source = SmoothValue;
				startTime = Time.realtimeSinceStartup;
				target = value;
			}
		}
		
		public LerpedQuaternion(Quaternion quaternion)
		{
			slerp = true;
			currentValue = quaternion;
			source = quaternion;		
			target = quaternion;
			startTime = Time.realtimeSinceStartup;
			duration = 0.2f;
		}
	}
}