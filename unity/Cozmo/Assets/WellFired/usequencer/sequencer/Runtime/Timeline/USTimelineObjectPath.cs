using UnityEngine;
using System;
using System.Collections.Generic;

namespace WellFired
{
	[Serializable]
	/// <summary>
	/// This is at the core of all object paths, from here, you can get keyframes, modify keyframes, alter curve data in real time.
	/// </summary>
	public class USTimelineObjectPath : USTimelineBase 
	{
		[SerializeField]
		private WellFired.Shared.Easing.EasingType easingType = WellFired.Shared.Easing.EasingType.Linear;
		public WellFired.Shared.Easing.EasingType EasingType
		{
			get { return easingType; }
			set { easingType = value; }
		}

		[SerializeField]
		private WellFired.Shared.ShakeType shakeType = WellFired.Shared.ShakeType.None;
		public WellFired.Shared.ShakeType ShakeType
		{
			get { return shakeType; }
			set { shakeType = value; }
		}

		[SerializeField]
		private SplineOrientationMode splineOrientationMode = SplineOrientationMode.LookAhead;
		public SplineOrientationMode SplineOrientationMode
		{
			get { return splineOrientationMode; }
			set { splineOrientationMode = value; }
		}
		
		[SerializeField]
		private Transform lookAtTarget;
		public Transform LookAtTarget
		{
			get { return lookAtTarget; }
			set { lookAtTarget = value; lookAtTargetPath = lookAtTarget.GetFullHierarchyPath(); }
		}
		
		[SerializeField]
		private string lookAtTargetPath = "";

		[SerializeField]
		private Vector3 sourcePosition;
		public Vector3 SourcePosition
		{
			get { return sourcePosition; }
			set { sourcePosition = value; }
		}
		
		[SerializeField]
		private Quaternion sourceRotation;
		public Quaternion SourceRotation
		{
			get { return sourceRotation; }
			set { sourceRotation = value; }
		}

		[SerializeField]
		private Spline objectSpline;
		public Spline ObjectSpline
		{
			get { return objectSpline; }
			set { objectSpline = value; }
		}

		[SerializeField]
		private float startTime;
		public float StartTime
		{
			get { return startTime; }
			set { startTime = value; }
		}
		
		[SerializeField]
		private float endTime;
		public float EndTime
		{
			get { return endTime; }
			set { endTime = value; }
		}

		[SerializeField]
		private List<SplineKeyframe> keyframes; 

		[SerializeField]
		private WellFired.Shared.Shake Shake;
		
		[SerializeField]		
		private float shakeSpeedPosition = 0.38f;
		
		[SerializeField]
		private Vector3 shakeRangePosition = new Vector3(0.4f, 0.4f, 0.4f);
		
		[SerializeField]
		private float shakeSpeedRotation = 0.38f;
		
		[SerializeField]
		private Vector3 shakeRangeRotation = new Vector3(4.0f, 4.0f, 4.0f);

		/// <summary>
		/// The keyframes in this curve, you can only read from this. 
		/// </summary>
		/// <value>The keyframes that make up this curve.</value>
		public List<SplineKeyframe> Keyframes 
		{ 
			get { return keyframes; }
			private set 
			{ 
				keyframes = value; 
				BuildCurveFromKeyframes(); 
			}
		}

		/// <summary>
		/// Gets the first node.
		/// </summary>
		/// <value>The first node.</value>
		public SplineKeyframe FirstNode
		{
			get { return ObjectSpline.Nodes[0]; }
			private set { ; }
		}

		/// <summary>
		/// Gets the last node.
		/// </summary>
		/// <value>The last node.</value>
		public SplineKeyframe LastNode
		{
			get { return ObjectSpline.Nodes[ObjectSpline.Nodes.Count - 1]; }
			private set { ; }
		}

		/// <summary>
		/// Gets or sets the color of the path in the scene view.
		/// </summary>
		/// <value>The color of the path.</value>
		public Color PathColor
		{
			get { return ObjectSpline.SplineColor; }
			set { ObjectSpline.SplineColor = value; }
		}

		/// <summary>
		/// Gets or sets the display resolution. Increasing this value will make the curve appear more smooth. This is entirely visual and has
		/// no affect on runtime performance.
		/// </summary>
		/// <value>The display resolution.</value>
		public float DisplayResolution
		{
			get { return ObjectSpline.DisplayResolution; }
			set { ObjectSpline.DisplayResolution = value; }
		}

		private LerpedQuaternion SmoothedQuaternion;

		private void OnEnable()
		{
			Build();
		}

		/// <summary>
		/// This method allows you to overwrite all keyframes.
		/// </summary>
		/// <param name="keyframes">The new Keyframes.</param>
		public void SetKeyframes(List<SplineKeyframe> keyframes)
		{
			Keyframes = keyframes;
			Build();
		}

		public void Build()
		{
			if(keyframes == null)
				CreateEmpty();
			else
				BuildCurveFromKeyframes();
		}

		public void BuildShake()
		{
			Shake = new WellFired.Shared.Shake();
		}

		/// <summary>
		/// You can use this method to add a new keyframe to this Object Path Timeline, simply call this method and pass the new keyframe.
		/// </summary>
		/// <param name="keyframe">The new Keyframe.</param>
		public void AddKeyframe(SplineKeyframe keyframe)
		{
			keyframes.Add(keyframe);
			BuildCurveFromKeyframes();
		}

		/// <summary>
		/// You can call this method to add a new keyframe after an existing keyframe.
		/// </summary>
		/// <param name="keyframe">The new keyframe.</param>
		/// <param name="index">The index of the existing keyframe that you'd like to add after.</param>
		public void AddAfterKeyframe(SplineKeyframe keyframe, int index)
		{
			keyframes.Insert(index + 1, keyframe);
			BuildCurveFromKeyframes();
		}
		
		/// <summary>
		/// You can call this method to add a new keyframe before an existing keyframe.
		/// </summary>
		/// <param name="keyframe">The new keyframe.</param>
		/// <param name="index">The index of the existing keyframe that you'd like to add before.</param>
		public void AddBeforeKeyframe(SplineKeyframe keyframe, int index)
		{
			keyframes.Insert(index - 1, keyframe);
			BuildCurveFromKeyframes();
		}

		/// <summary>
		/// Use this method to alter an existing keyframe. You can pass the new position and the index of that keyframe.
		/// </summary>
		/// <param name="position">Position.</param>
		/// <param name="keyframeIndex">Keyframe index.</param>
		public void AlterKeyframe(Vector3 position, int keyframeIndex)
		{
			keyframes[keyframeIndex].Position = position;
			BuildCurveFromKeyframes();
		}

		/// <summary>
		/// Use this method to remove an existing keyframe. You can pass the keyframe you'd like to remove.
		/// </summary>
		/// <param name="keyframe">Keyframe.</param>
		public void RemoveKeyframe(SplineKeyframe keyframe)
		{
			keyframes.Remove(keyframe);
			BuildCurveFromKeyframes();
		}

		/// <summary>
		/// Call this method if you'd like to force updating a curve after modifying the keyframes.
		/// </summary>
		public void BuildCurveFromKeyframes()
		{
			ObjectSpline.BuildFromKeyframes(Keyframes);
		}

		private void CreateEmpty()
		{
			ObjectSpline = new Spline();
			Keyframes = new List<SplineKeyframe>() { ScriptableObject.CreateInstance<SplineKeyframe>(), ScriptableObject.CreateInstance<SplineKeyframe>() };

			Keyframes[0].Position = AffectedObject.transform.position;
			Keyframes[1].Position = AffectedObject.transform.position;

			StartTime = 0.0f;
			EndTime = Sequence.Duration;
		}

		public void SetStartingOrientation()
		{
			switch(SplineOrientationMode)
			{
			case SplineOrientationMode.LookAtTransform:
				AffectedObject.position = FirstNode.Position;
				AffectedObject.LookAt(LookAtTarget, Vector3.up);
				break;
			case SplineOrientationMode.LookAhead:
				var nextNodePosition = ObjectSpline.GetPositionOnPath(Sequence.PlaybackRate > 0.0f ? USSequencer.SequenceUpdateRate : -USSequencer.SequenceUpdateRate);
				AffectedObject.position = FirstNode.Position;
				AffectedObject.LookAt(nextNodePosition, Vector3.up);
				break;
			case SplineOrientationMode.ManualOrientation:
				AffectedObject.position = FirstNode.Position;
				break;
			}
		}
		
		public override void StartTimeline() 
		{
			SourcePosition = AffectedObject.transform.position;
			SourceRotation = AffectedObject.transform.rotation;
			SmoothedQuaternion = new LerpedQuaternion(AffectedObject.rotation);

			BuildShake();
		}
		
		public override void StopTimeline()
		{
			AffectedObject.transform.position = SourcePosition;
			AffectedObject.transform.rotation = SourceRotation;
		}

		public override void SkipTimelineTo(float time)
		{
			Process(time, 1.0f);
		}

		public override void Process(float sequencerTime, float playbackRate) 
		{
			if(!AffectedObject)
				return;

			if(sequencerTime < StartTime || sequencerTime > EndTime)
				return;

			if(SplineOrientationMode == SplineOrientationMode.LookAtTransform && LookAtTarget == null)
				throw new System.Exception("Spline Orientation Mode is look at object, but there is no LookAtTarget");

			var sampleTime = (sequencerTime - StartTime) / ((EndTime - StartTime));

			var easingFunction = WellFired.Shared.DoubleEasing.GetEasingFunctionFor(easingType);
			sampleTime = (float)easingFunction(sampleTime, 0.0, 1.0, 1.0);

			var modifiedRotation = sourceRotation;

			switch(SplineOrientationMode)
			{
			case SplineOrientationMode.LookAtTransform:
				AffectedObject.position = ObjectSpline.GetPositionOnPath(sampleTime);
				AffectedObject.LookAt(LookAtTarget, Vector3.up);
				modifiedRotation = AffectedObject.rotation;
				break;
			case SplineOrientationMode.LookAhead:
			{
				var nextNodePosition = ObjectSpline.GetPositionOnPath(Sequence.PlaybackRate > 0.0f ? sampleTime + USSequencer.SequenceUpdateRate : sampleTime - USSequencer.SequenceUpdateRate);
				var prevNodePosition = ObjectSpline.GetPositionOnPath(Sequence.PlaybackRate > 0.0f ? sampleTime - USSequencer.SequenceUpdateRate : sampleTime + USSequencer.SequenceUpdateRate);

				var prev = nextNodePosition.Equals(prevNodePosition);
				SmoothedQuaternion.SmoothValue = prev ? Quaternion.identity : Quaternion.LookRotation((nextNodePosition - prevNodePosition).normalized);

				AffectedObject.rotation = SmoothedQuaternion.SmoothValue;
				AffectedObject.position = ObjectSpline.GetPositionOnPath(sampleTime);

				modifiedRotation = AffectedObject.rotation;
				break;
			}
			case SplineOrientationMode.ManualOrientation:
				AffectedObject.position = ObjectSpline.GetPositionOnPath(sampleTime);
				break;
			}

			Shake.ShakeType = shakeType;
			Shake.ShakeSpeedPosition = shakeSpeedPosition;
			Shake.ShakeRangePosition = shakeRangePosition;
			Shake.ShakeSpeedRotation = shakeSpeedRotation;	
			Shake.ShakeRangeRotation = shakeRangeRotation;
			Shake.Process(sequencerTime, Sequence.Duration);

			// Must append the shake quat, so we maintain our look forward or look at object
			var shakeQuat = Quaternion.Euler(modifiedRotation.eulerAngles + Shake.EulerRotation);

			var min = 0.0f;
			var max = (Sequence.Duration * 0.1f);
			max = Mathf.Clamp(max, 0.1f, 1.0f);
			var shakeRatio = Mathf.Clamp(sequencerTime, min, max) / max;

			var shakePos = Vector3.Slerp(Vector3.zero, Shake.Position, shakeRatio);
			shakeQuat = Quaternion.Slerp(AffectedObject.localRotation, shakeQuat, shakeRatio);

			AffectedObject.localPosition += shakePos;
			AffectedObject.localRotation = shakeQuat;
		}

		private void OnDrawGizmos()
		{
			if(!ShouldRenderGizmos)
				return;

			if(ObjectSpline == null)
				return;

			ObjectSpline.OnDrawGizmos();
		}

		public void FixupAdditionalObjects()
		{
			if(!String.IsNullOrEmpty(lookAtTargetPath))
			{
				var lookAtGameObject = (GameObject.Find(lookAtTargetPath) as GameObject);
				if(lookAtGameObject != null)
					LookAtTarget = lookAtGameObject.transform;
			}

			if(LookAtTarget == null && !String.IsNullOrEmpty(lookAtTargetPath))
				Debug.LogWarning(string.Format("Tried to fixup a lookat target for this object path timeline, but it doesn't exist in this scene. (Target = {0}, ObjectPathTimeline = {1})", lookAtTargetPath, this), this);
		}

		public void RecordAdditionalObjects()
		{
			if(LookAtTarget != null)
				lookAtTargetPath = LookAtTarget.GetFullHierarchyPath();
		}
	}
}