using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	/// <summary>
	/// The USInternalCurve is our internal curve object, for it's back end, it uses a AnimationCurve.
	/// Manipulation happens (Only at edit time) and a AnimationCurve is generated from this and used at runtime, the USInternalCurve
	/// inherits from Scriptable Object so that we can get nice Inspector, Serialization and Undo / Redo support.
	/// </summary> 
	[System.Serializable]
	public class USInternalCurve : ScriptableObject 
	{
		/// <summary>
		/// Compares two keyframes for equality.
		/// 
		/// The method should return : 
		/// if the first keyframe is greater, return 1, if they are equal, return 0, if the second keyframe is greater, return -1
		/// </summary>
		/// <returns>The value result of the comparison</returns>
		/// <param name="a">The first keyframe.</param>
		/// <param name="b">The second keyframe.</param>
		public static int KeyframeComparer(USInternalKeyframe a, USInternalKeyframe b)
		{
			return (a.Time.CompareTo(b.Time));
		}

		[SerializeField]
		private AnimationCurve animationCurve;
		
		[SerializeField]
		private List<USInternalKeyframe> internalKeyframes;

		[SerializeField]
		private bool useCurrentValue = false;

		[SerializeField]
		private float duration;
		/// <summary>
		/// Gets or sets the duration of this curve
		/// </summary>
		/// <value>The duration.</value>
		public float Duration
		{
			get { return duration; }
			set { duration = value; }
		}

		/// <summary>
		/// Gets the first keyframe time.
		/// If there are no keyframes, we return 0.0f
		/// </summary>
		/// <value>The first keyframe time.</value>
		public float FirstKeyframeTime
		{
			get 
			{ 
				if(internalKeyframes.Count == 0)
					return 0.0f;
				
				return internalKeyframes[0].Time;
			}
		}

		/// <summary>
		/// Gets the first keyframe time.
		/// If there are no keyframes, we return the <see cref="WellFired.USInternalCurve.Duration"/>
		/// </summary>
		/// <value>The first keyframe time.</value>
		public float LastKeyframeTime
		{
			get 
			{ 
				if(internalKeyframes.Count == 0)
					return Duration;
				
				return internalKeyframes[internalKeyframes.Count - 1].Time;
			}
		}
		
		/// <summary>
		/// The editor internally converts <see cref="WellFired.USInternalCurve"/> to AnimationCurves, that is because the back 
		/// end of the system runs on AnimationCurves. It's probably better to interface with the USInternalCurve
		/// rather than this curve.
		/// 
		/// This should not be manipulated directly at edit time.
		/// </summary>
		public AnimationCurve UnityAnimationCurve
		{
			set 
			{ 
				animationCurve = new AnimationCurve();
				
				foreach(Keyframe keyframe in value.keys)
					animationCurve.AddKey(keyframe);
				
				BuildInternalCurveFromAnimationCurve();
			}
			get
			{
				return animationCurve;
			}
		}

		/// <summary>
		/// All of this curves <see cref="WellFired.USInternalKeyframe"/>, these will be sorted by time.
		/// </summary>
		/// <value>The keys.</value>
		public List<USInternalKeyframe> Keys
		{
			get
			{
				return internalKeyframes;
			}
		}

		/// <summary>
		/// Set this if you'd like the first keyframe to be extracted from the object rather than explicitly set. 
		/// </summary>
		/// <value><c>true</c> if use current value; otherwise, <c>false</c>.</value>
		public bool UseCurrentValue 
		{
			get { return useCurrentValue; }
			set { useCurrentValue = value; }
		}
	
		private void OnEnable()
		{
			if(internalKeyframes == null)
			{
				internalKeyframes = new List<USInternalKeyframe>();
				Duration = 10;
			}
			else
			{
				if(internalKeyframes.Count > 0)
					Duration = internalKeyframes.OrderBy(keyframe => keyframe.Time).LastOrDefault().Time;
			}
	
			if(UnityAnimationCurve == null)
				UnityAnimationCurve = new AnimationCurve();
		}
	
		public float Evaluate(float time)
		{
			return animationCurve.Evaluate(time);
		}
		
		public void BuildInternalCurveFromAnimationCurve()
		{	
			foreach(Keyframe keyframe in animationCurve.keys)
			{
				// Check we don't already have a key that is roughly equivilant to this key.
				USInternalKeyframe existingKeyframe = null;
				foreach(USInternalKeyframe internalKeyframe in internalKeyframes)
				{
					if(Mathf.Approximately(keyframe.time, internalKeyframe.Time))
					{
						existingKeyframe = internalKeyframe;
						break;
					}
				}
				
				if(existingKeyframe)
				{
					existingKeyframe.ConvertFrom(keyframe);
					continue;
				}
				
				USInternalKeyframe internalkeyframe = ScriptableObject.CreateInstance<USInternalKeyframe>();
				internalkeyframe.ConvertFrom(keyframe);
				internalkeyframe.curve = this;
				internalKeyframes.Add(internalkeyframe);
				
				internalKeyframes.Sort(KeyframeComparer);
			}
		}
		
		public void BuildAnimationCurveFromInternalCurve()
		{	
			// Clear our Animation Curve.
			while(animationCurve.keys.Length > 0)
				animationCurve.RemoveKey(0);
			
			Keyframe tmpKeyframe = new Keyframe();
			foreach(USInternalKeyframe internalKeyframe in Keys)
			{
				tmpKeyframe.value 		= internalKeyframe.Value;
				tmpKeyframe.time 		= internalKeyframe.Time;
				tmpKeyframe.inTangent 	= internalKeyframe.InTangent;
				tmpKeyframe.outTangent	= internalKeyframe.OutTangent;
				
				animationCurve.AddKey(tmpKeyframe);
			}
			
			internalKeyframes.Sort(KeyframeComparer);
		}
		
		/// <summary>
		/// If two <see cref="WellFired.USInternalKeyframe"/> have roughly similar times, remove one of them. (This happens to AnimationCurves Also)
		/// but our editor only does this when it's needed, we don't validate whilst moving for instance.
		/// </summary>
		public void ValidateKeyframeTimes()
		{
			// Remove Keyframes that are too close together.
			for(int n = Keys.Count - 1; n >= 0; n--)
			{
				if(n == Keys.Count - 1)
					continue;
				
				// Remove any keyframes that have the same time.
				if(Mathf.Approximately(Keys[n].Time, Keys[n+1].Time))
					internalKeyframes.Remove(Keys[n]);
			}
		}

		/// <summary>
		/// Adds a <see cref="WellFired.USInternalKeyframe"/> at the given time, with the given value. Allocates memory at runtime, it's advisable to use this only in edit mode.
		/// If this <see cref="WellFired.USInternalKeyframe"/> already exists, we will return the existing <see cref="WellFired.USInternalKeyframe"/>.
		/// </summary>
		/// <returns>The allocated keyframe.</returns>
		/// <param name="time">The keyframe time.</param>
		/// <param name="value">The value this keyframe should hold.</param>
		public USInternalKeyframe AddKeyframe(float time, float value)
		{
			// If a keyframe already exists at this time, use that one.
			USInternalKeyframe internalkeyframe = null;
			foreach(USInternalKeyframe keyframe in internalKeyframes)
			{
				if(Mathf.Approximately(keyframe.Time, time))
					internalkeyframe = keyframe;
					
				if(internalkeyframe != null)
					break;
			}
			
			// Didn't find a keyframe create a new one.
			if(!internalkeyframe)
			{
				internalkeyframe = ScriptableObject.CreateInstance<USInternalKeyframe>();
				internalKeyframes.Add(internalkeyframe);
			}
			
			Duration = Mathf.Max(internalKeyframes.OrderBy(keyframe => keyframe.Time).LastOrDefault().Time, time);
	
			internalkeyframe.curve = this;
			internalkeyframe.Time = time;
			internalkeyframe.Value = value;
			internalkeyframe.InTangent = 0.0f;
			internalkeyframe.OutTangent = 0.0f;
			
			internalKeyframes.Sort(KeyframeComparer);
			
			BuildAnimationCurveFromInternalCurve();
			
			return internalkeyframe;
		}

		/// <summary>
		/// Removes a <see cref="WellFired.USInternalKeyframe"/>.
		/// </summary>
		/// <param name="internalKeyframe">The keyframe to remove.</param>
		public void RemoveKeyframe(USInternalKeyframe internalKeyframe)
		{
			for(int n = internalKeyframes.Count - 1; n >= 0; n--)
			{
				if(internalKeyframes[n] == internalKeyframe)
					internalKeyframes.RemoveAt(n);
			}
			
			internalKeyframes.Sort(KeyframeComparer);
	
			Duration = internalKeyframes.OrderBy(keyframe => keyframe.Time).LastOrDefault().Time;
			
			BuildAnimationCurveFromInternalCurve();
		}
		
		/// <summary>
		/// A helper function to get the next <see cref="WellFired.USInternalKeyframe"/>.
		/// if something goes wrong, this method returns null.
		/// </summary>
		/// <returns>The next keyframe.</returns>
		/// <param name="keyframe">A Keyframe.</param>
		public USInternalKeyframe GetNextKeyframe(USInternalKeyframe keyframe)
		{
			// There are no keys, bail.
			if(internalKeyframes.Count == 0)
				return null;
			
			// this is the last keyframe, bail.
			if(internalKeyframes[internalKeyframes.Count - 1] == keyframe)
				return null;
			
			// Find the next keyframe.
			int index = -1;
			for(int n = 0; n < internalKeyframes.Count; n++)
			{
				if(internalKeyframes[n] == keyframe)
					index = n;
				
				if(index != -1)
					break;
			}
			
			return internalKeyframes[index + 1];
		}
		
		/// <summary>
		/// A helper function to get the previous <see cref="WellFired.USInternalKeyframe"/>.
		/// if something goes wrong, this method returns null.
		/// </summary>
		/// <returns>The previous keyframe.</returns>
		/// <param name="keyframe">A Keyframe.</param>
		public USInternalKeyframe GetPrevKeyframe(USInternalKeyframe keyframe)
		{
			// There are no keys, bail.
			if(internalKeyframes.Count == 0)
				return null;
			
			// this is the first keyframe, bail.
			if(internalKeyframes[0] == keyframe)
				return null;
			
			// Find the next keyframe.
			int index = -1;
			for(int n = 0; n < internalKeyframes.Count; n++)
			{
				if(internalKeyframes[n] == keyframe)
					index = n;
				
				if(index != -1)
					break;
			}
			
			return internalKeyframes[index - 1];
		}

		/// <summary>
		/// Does this curve contain the passed <see cref="WellFired.USInternalKeyframe"/>
		/// </summary>
		/// <param name="keyframe">A Keyframe.</param>
		public bool Contains(USInternalKeyframe keyframe)
		{
			// Remove Keyframes that are too close together.
			for(int n = Keys.Count - 1; n >= 0; n--)
			{
				if(Keys[n] == keyframe)
					return true;
			}
			
			return false;
		}

		/// <summary>
		/// A helper utility to find the next <see cref="WellFired.USInternalKeyframe"/> after a given time
		/// </summary>
		/// <returns>The next keyframe time.</returns>
		/// <param name="time">The comparison time.</param>
		public float FindNextKeyframeTime(float time)
		{	
			float keyframeTime = Duration;
			for(int n = Keys.Count - 1; n >= 0; n--)
			{
				if(Keys[n].Time < keyframeTime && Keys[n].Time > time)
					keyframeTime = Keys[n].Time;
			}
			
			return keyframeTime;
		}

		/// <summary>
		/// A helper utility to find the previous <see cref="WellFired.USInternalKeyframe"/> after a given time
		/// </summary>
		/// <returns>The previous keyframe time.</returns>
		/// <param name="time">The comparison time.</param>
		public float FindPrevKeyframeTime(float time)
		{
			float keyframeTime = 0.0f;
			for(int n = 0; n < Keys.Count; n++)
			{
				if(Keys[n].Time > keyframeTime && Keys[n].Time < time)
					keyframeTime = Keys[n].Time;
			}
			
			return keyframeTime;
		}

		public bool CanSetKeyframeToTime(float newTime)
		{
			foreach(var keyframe in internalKeyframes)
			{
				if(Mathf.Approximately(keyframe.Time, newTime))
					return false;
			}

			return true;
		}
	}
}