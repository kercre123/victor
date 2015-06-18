#if WELLFIRED_INTERNAL
#define DEBUG_OBSERVER
#endif

using UnityEngine;
using System;
using System.Reflection;
using System.Linq;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	/// <summary>
	/// This is the core element to our Observer Timelines
	/// </summary>
	[Serializable]
	[ExecuteInEditMode]
	public class USTimelineObserver : USTimelineBase 
	{
		[Serializable]
		private class SnapShotEntry
		{
			public Camera camera;
			public AudioListener listener;
			public RenderTexture target;
			public bool state;
		}

		/// <summary>
		/// This is the list of observer Keyframes
		/// </summary>
		[SerializeField] 
		public List<USObserverKeyframe> observerKeyframes = new List<USObserverKeyframe>();
		
		/// <summary>
		/// When the sequence changes, the current states of observer keyframes are stored here,
		/// this is so we can revert them at any time. As such, this isn't serialized.
		/// </summary>
		[SerializeField]
		private List<SnapShotEntry> currentSnapshots = new List<SnapShotEntry>();

		private List<Camera> camerasAtLastSnapShot;
	
		/// <summary>
		/// The layermask that defines which layers to ignore when toggline cameras
		/// </summary>
		public LayerMask layersToIgnore = 0;

		public USObserverKeyframe CurrentlyActiveKeyframe
		{ 
			get;
			set;
		}
		
		private int KeyframeComparer(USObserverKeyframe a, USObserverKeyframe b)
		{
			if (a == null && b == null)
	   			return 0;
			else if (a == null)
	   			return -1;
			else if (b == null)
	   			return 1;
			
			return (a.FireTime.CompareTo(b.FireTime));
		}
		
		public override void StopTimeline()
		{
			RevertToSnapshot();
			
			currentSnapshots.Clear();

			if(camerasAtLastSnapShot != null)
				camerasAtLastSnapShot.Clear();
		}
		
		public override void PauseTimeline()
		{
		}
		
		public override void ResumeTimeline()
		{
		}
		
		public override void StartTimeline() 
		{
			SortKeyframes();
			CreateSnapshot();

			if(observerKeyframes.Count > 0)
				DisableAllCameras();

			Process(0.0f, Sequence.PlaybackRate);
		}

		private void DisableAllCameras()
		{
			var allCameras = AllValidCameras();
			foreach(var camera in allCameras)
			{
				camera.enabled = false;
				var audioListener = camera.gameObject.GetComponent<AudioListener>();
				if(audioListener)
					audioListener.enabled = false;
			}
		}
		
		public override void ManuallySetTime(float sequencerTime)
		{
			Process(sequencerTime, 1.0f);
		}
		
		/// <summary>
		/// This function will skip the timeline time to the time passed, we will simply Process the timeline.
		/// </summary>
		public override void SkipTimelineTo(float time)
		{
			Process(time, 1.0f);
		}
		
		public override void Process(float sequencerTime, float playbackRate)
		{
			var currentKeyframe = default(USObserverKeyframe);
			foreach(var keyframe in observerKeyframes)
			{
				if(keyframe.FireTime > sequencerTime)
					continue;

				if(currentKeyframe == null)
					currentKeyframe = keyframe;

				if(keyframe.FireTime > currentKeyframe.FireTime)
					currentKeyframe = keyframe;
			}

			if(CurrentlyActiveKeyframe != null)
			{
				if(sequencerTime >= (CurrentlyActiveKeyframe.FireTime + CurrentlyActiveKeyframe.TransitionDuration) && CurrentlyActiveKeyframe.Fired)
				{
					CurrentlyActiveKeyframe.Process(sequencerTime - CurrentlyActiveKeyframe.FireTime);
					CurrentlyActiveKeyframe.End();

					var prevKeyframe = KeyframeBefore(CurrentlyActiveKeyframe);
					if(prevKeyframe && prevKeyframe.AudioListener)
						prevKeyframe.AudioListener.enabled = false;
				}
			}

			if(currentKeyframe != CurrentlyActiveKeyframe)
			{
				var prevKeyframe = CurrentlyActiveKeyframe;
				CurrentlyActiveKeyframe = currentKeyframe;

				var allKeyframesBetween = CollectAllKeyframesBetween(prevKeyframe, CurrentlyActiveKeyframe);

				var forwards = (prevKeyframe == null && CurrentlyActiveKeyframe != null) || (CurrentlyActiveKeyframe != null && prevKeyframe.FireTime < CurrentlyActiveKeyframe.FireTime);

				if(forwards)
				{
					for(int index = 0; index < allKeyframesBetween.Count(); index++)
					{
						if(prevKeyframe != null && index == 0)
							continue;

						if(index - 1 >= 0)
							allKeyframesBetween[index-1].UnFire();

						allKeyframesBetween[index].Fire((index > 0 ? allKeyframesBetween[index-1].KeyframeCamera : default(Camera)));
						
						if(index != allKeyframesBetween.Count() - 1)
						{
							allKeyframesBetween[index].Process(sequencerTime - CurrentlyActiveKeyframe.FireTime);
							allKeyframesBetween[index].End();
						}
					}
				}
				else
				{
					for(int index = allKeyframesBetween.Count() - 1; index >= 0; index--)
					{
						if(CurrentlyActiveKeyframe != null && index == 0)
						{
							var keyframeBefore = KeyframeBefore(CurrentlyActiveKeyframe);
							allKeyframesBetween[index].Fire(keyframeBefore ? keyframeBefore.KeyframeCamera : default(Camera));
							continue;
						}

						var shouldRevert = true;

						// Simple Hack so the when playing a ping pong'd sequence there isn't a frame of no cameras
						if(Sequence.IsPingPonging && allKeyframesBetween[index].FireTime <= 0.0f)
							shouldRevert = false;

						if(shouldRevert)
							allKeyframesBetween[index].Revert();
					}
				}
			}
			
			if(CurrentlyActiveKeyframe)
				CurrentlyActiveKeyframe.Process(sequencerTime - CurrentlyActiveKeyframe.FireTime);
		}

		USObserverKeyframe KeyframeBefore(USObserverKeyframe currentlyActiveKeyframe)
		{
			if(currentlyActiveKeyframe == default(USObserverKeyframe))
				return default(USObserverKeyframe);

			var keyframeBefore = default(USObserverKeyframe);
			foreach(var keyframe in observerKeyframes)
			{
				if(keyframe.FireTime < currentlyActiveKeyframe.FireTime)
				{
					if(keyframeBefore == default(USObserverKeyframe) || keyframeBefore.FireTime < keyframe.FireTime)
						keyframeBefore = keyframe;
				}
			}

			return keyframeBefore;
		}

		List<USObserverKeyframe> CollectAllKeyframesBetween(USObserverKeyframe prevKeyframe, USObserverKeyframe currentKeyframe)
		{
			var earliestKeyframe = default(USObserverKeyframe);
			var latestKeyframe = default(USObserverKeyframe);

			if(prevKeyframe == null)
				earliestKeyframe = observerKeyframes.First();
			else if(currentKeyframe == null)
				earliestKeyframe = observerKeyframes.First();
			else
				earliestKeyframe = prevKeyframe.FireTime < currentKeyframe.FireTime ? prevKeyframe : currentKeyframe;

			if(prevKeyframe == null)
				latestKeyframe = currentKeyframe;
			else if(currentKeyframe == null)
				latestKeyframe = prevKeyframe;
			else
				latestKeyframe = earliestKeyframe == prevKeyframe ? currentKeyframe : prevKeyframe;

			var keyframes = new List<USObserverKeyframe>();

			foreach(var keyframe in observerKeyframes)
			{
				if(keyframe.FireTime >= earliestKeyframe.FireTime && keyframe.FireTime <= latestKeyframe.FireTime)
					keyframes.Add(keyframe);
			}

			return keyframes;
		}

		private void Update()
		{
			if(Application.isEditor && observerKeyframes.Count > 0 && Sequence.RunningTime > 0.0f && !ValidatePreviousSnapshot())
			{
#if DEBUG_OBSERVER
				Debug.Log("Invalid");
#endif
				RevertToSnapshot();
				CreateSnapshot();
				Process(Sequence.RunningTime, 1.0f);
			}
		}

		public USObserverKeyframe AddKeyframe(USObserverKeyframe keyframe)
		{
			keyframe.observer = this;
			observerKeyframes.Add(keyframe);
			
			SortKeyframes();

			if(observerKeyframes.Count == 1 && Sequence.HasSequenceBeenStarted)
				DisableAllCameras();

			return keyframe;
		}
		
		private void SortKeyframes()
		{
			observerKeyframes.Sort(KeyframeComparer);
		}
		
		public void RemoveKeyframe(USObserverKeyframe keyframe)
		{
			if(keyframe == null)
				return;
			
			if(!observerKeyframes.Contains(keyframe))
				return;
			
			observerKeyframes.Remove(keyframe);
		}

		public void SetKeyframes(List<USObserverKeyframe> keyframes)
		{
			observerKeyframes = keyframes;
		}

		public void OnEditorUndo()
		{
		}

		public void OnGUI()
		{
			if(CurrentlyActiveKeyframe)
				CurrentlyActiveKeyframe.ProcessFromOnGUI();
		}

		public bool IsValidCamera(Camera testCamera)
		{
			// If this camera is on the ignore layer, ignore it.
			if(((1 << testCamera.gameObject.layer) & layersToIgnore) > 0)
				return false;

			var notEdittable = (testCamera.gameObject.hideFlags & HideFlags.NotEditable) == HideFlags.NotEditable;
			var hideAndDontSave = (testCamera.gameObject.hideFlags & HideFlags.HideAndDontSave) == HideFlags.HideAndDontSave;
			var hideInHierarchy = (testCamera.gameObject.hideFlags & HideFlags.HideInHierarchy) == HideFlags.HideInHierarchy;
				
			if(notEdittable || hideAndDontSave || hideInHierarchy)
				return false;

			return true;
		}

		private bool ValidatePreviousSnapshot()
		{
			var newValidCameras = AllValidCameras();
			return (newValidCameras.Count == camerasAtLastSnapShot.Count) && !newValidCameras.Except(camerasAtLastSnapShot).Any();
		}

		public List<Camera> AllValidCameras()
		{			
			Camera[] cameras = Resources.FindObjectsOfTypeAll(typeof(Camera)) as Camera[];
			return cameras.Where(camera => IsValidCamera(camera)).ToList();
		}

		private void CreateSnapshot()
		{
#if DEBUG_OBSERVER
			Debug.LogWarning("Making a snapshot");
#endif

			currentSnapshots.Clear();

			if(camerasAtLastSnapShot != null)
				camerasAtLastSnapShot.Clear();

			camerasAtLastSnapShot = AllValidCameras();

			// Also go through all the scenes existing active cameras.
			foreach(var activeCamera in camerasAtLastSnapShot)
				currentSnapshots.Add(new SnapShotEntry() { camera = activeCamera, target = activeCamera.targetTexture, listener = activeCamera.gameObject.GetComponent<AudioListener>(), state = activeCamera.enabled });
		}

		private void RevertToSnapshot()
		{
#if DEBUG_OBSERVER
			Debug.LogWarning("Reverting to snapshot");
#endif

			foreach(var currentSnapshot in currentSnapshots)
			{
				if(currentSnapshot.camera != null)
					currentSnapshot.camera.enabled = currentSnapshot.state;
				if(currentSnapshot.listener != null)
					currentSnapshot.listener.enabled = currentSnapshot.state;

				currentSnapshot.camera.targetTexture = currentSnapshot.target;
			}
			CurrentlyActiveKeyframe = null;
		}
		
		public void FixCameraReferences()
		{
			foreach(var keyframe in observerKeyframes)
			{
				if(keyframe.cameraPath == string.Empty)
					continue;
				
				var cameraGO = GameObject.Find(keyframe.cameraPath);
				
				if(!cameraGO)
					continue;
				
				var camera = cameraGO.GetComponent<Camera>();
				
				if(!camera)
					continue;
				
				keyframe.KeyframeCamera = camera;
			}
		}
	}
}