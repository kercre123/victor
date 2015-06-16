using UnityEngine;
using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace WellFired
{
	/// <summary>
	/// A selection of Utility functions that can be called to help with using uSequener.
	/// </summary>
	public static class USRuntimeUtility 
	{
		public static bool CanPlaySequence(USSequencer sequence)
		{
			if(sequence.IsPlaying)
				return false;

			if(sequence.RunningTime >= sequence.Duration)
				return false;

			return true;
		}

		public static bool CanPauseSequence(USSequencer sequence)
		{
			if(!sequence.IsPlaying)
				return false;
			
			if(sequence.RunningTime <= 0.0f)
				return false;
			
			if(sequence.RunningTime >= sequence.Duration)
				return false;
			
			return true;
		}
		
		public static bool CanStopSequence(USSequencer sequence)
		{
			if(sequence.RunningTime <= 0.0f)
				return false;
			
			return true;
		}
		
		public static bool CanSkipSequence(USSequencer sequence)
		{
			if(sequence.RunningTime >= sequence.Duration)
				return false;
			
			return true;
		}

		static public bool IsObserverTimeline(Transform transform)
		{	
			return transform.GetComponent<USTimelineObserver>() != null;
		}
		
		static public bool IsObserverContainer(Transform transform)
		{
			var timelineContainer = transform.GetComponent<USTimelineContainer>();
			if(timelineContainer == null)
				return false;

			foreach(var timeline in timelineContainer.Timelines)
			{
				if(IsObserverTimeline(timeline.transform))
					return true;
			}

			return false;
		}
		
		static public bool IsTimelineContainer(Transform transform)
		{
			return transform.GetComponent<USTimelineContainer>() != null;
		}
		
		static public bool IsTimeline(Transform transform)
		{
			return transform.GetComponent<USTimelineBase>() != null;
		}
		
		static public bool IsEventTimeline(Transform transform)
		{
			return transform.GetComponent<USTimelineEvent>() != null;
		}
		
		static public bool IsPropertyTimeline(Transform transform)
		{
			return transform.GetComponent<USTimelineProperty>() != null;
		}
		
		static public bool IsEvent(Transform transform) 
		{
			return transform.GetComponent<USEventBase>() != null;
		}
	
		static public int GetNumberOfTimelinesOfType(Type type, USTimelineContainer timelineContainer)
		{
			return timelineContainer.transform.GetComponentsInChildren(type).Length;
		}
	
		static public bool HasPropertyTimeline(Transform transform)
		{
			if(!IsTimelineContainer(transform))
				return false;
			
			foreach(Transform child in transform.GetComponentsInChildren<Transform>())
			{
				if(IsPropertyTimeline(child))
					return true;
			}
			
			return false;
		}
		
		static public bool HasObserverTimeline(Transform transform)
		{
			if(!IsTimelineContainer(transform))
				return IsObserverTimeline(transform);
			
			foreach(Transform child in transform.GetComponentsInChildren<Transform>())
			{
				if(IsObserverTimeline(child))
					return true;
			}
			
			return false;
		}

		public static bool HasTimelineContainerWithAffectedObject(USSequencer sequence, Transform affectedObject)
		{
			foreach(var timelineContainers in sequence.TimelineContainers)
			{
				if(timelineContainers.AffectedObject == affectedObject)
					return true;
			}

			return false;
		}
	
		static public void CreateAndAttachObserver(USSequencer sequence)
		{
			foreach(USTimelineContainer timelineContainer in sequence.TimelineContainers)
			{
				foreach(USTimelineBase timelineBase in timelineContainer.Timelines)
				{
					if(timelineBase is USTimelineObserver)
						return;
				}
			}
			
			Camera mainCamera = null;
			Camera[] cameras = GameObject.FindObjectsOfType(typeof(Camera)) as Camera[];
			foreach(Camera camera in cameras)
			{
				if(camera.tag == "MainCamera")
					mainCamera = camera;
			}
			
			if(mainCamera == null)
				Debug.LogWarning("There is no main camera in the scene, we need one for our observer track.");
			
			GameObject spawnedContainer = new GameObject("_Timelines for Observer");
			spawnedContainer.transform.parent = sequence.transform;
			USTimelineContainer ourTimelineContainer = spawnedContainer.AddComponent(typeof(USTimelineContainer)) as USTimelineContainer;
			ourTimelineContainer.Index = -1;
			
			GameObject spawnedObserver = new GameObject("Observer");
			spawnedObserver.transform.parent = spawnedContainer.transform;
			spawnedObserver.AddComponent(typeof(USTimelineObserver));
		}

		/// <summary>
		/// Starts recording the passed sequence.
		/// </summary>
		public static void StartRecordingSequence(USSequencer sequence, string capturePath, int captureFramerate, int upScaleAmount) 
		{
			USRecordSequence recorder = USRuntimeUtility.GetOrSpawnRecorder();
			
			recorder.StartRecording();
			
			recorder.RecordOnStart = true;
			recorder.CapturePath = capturePath;
			recorder.CaptureFrameRate = captureFramerate;
			recorder.UpscaleAmount = upScaleAmount;
		}
		
		/// <summary>
		/// Stops recording any sequence currently playing back.
		/// </summary>
		public static void StopRecordingSequence() 
		{
			USRecordSequence recorder = USRuntimeUtility.GetOrSpawnRecorder();
			
			if(recorder)
				GameObject.DestroyImmediate(recorder.gameObject);
		}
		
		/// <summary>
		/// Gets or Spawns a Recorder Object.
		/// </summary>
		private static USRecordSequence GetOrSpawnRecorder()
		{
			USRecordSequence recorder = Transform.FindObjectOfType(typeof(USRecordSequence)) as USRecordSequence;
			
			if(recorder)
				return recorder;
			
			GameObject recordingObject = new GameObject("Recording Object");
			return recordingObject.AddComponent<USRecordSequence>();
		}
	
		public static float FindNextKeyframeTime(USSequencer sequence)
		{
			float keyframeTime = sequence.Duration;
			foreach(USTimelineContainer timelineContainer in sequence.TimelineContainers)
			{
				foreach(USTimelineBase timelineBase in timelineContainer.Timelines)
				{
					USTimelineProperty timelineProperty = timelineBase as USTimelineProperty;
					if(!timelineProperty)
						continue;
					
					foreach(var property in timelineProperty.Properties)
					{
						foreach(USInternalCurve curve in property.curves)
						{
							float time = curve.FindNextKeyframeTime(sequence.RunningTime);
							if(time < keyframeTime)
								keyframeTime = time;
						}
					}
				}
			}
	
			return keyframeTime;
		}

		public static string ConvertToSerializableName(string value)
		{
			var result = string.Empty;
			if(value.IndexOf('.') != -1)
				result = value.Remove(0,  value.LastIndexOf('.') + 1);
			else
				result = value;

			return result;
		}
		
		public static float FindPrevKeyframeTime(USSequencer sequence)
		{
			float keyframeTime = 0.0f;
			foreach(USTimelineContainer timelineContainer in sequence.TimelineContainers)
			{
				foreach(USTimelineBase timelineBase in timelineContainer.Timelines)
				{
					USTimelineProperty timelineProperty = timelineBase as USTimelineProperty;
					if(!timelineProperty)
						continue;
	
					foreach(var property in timelineProperty.Properties)
					{
						foreach(USInternalCurve curve in property.curves)
						{
							float time = curve.FindPrevKeyframeTime(sequence.RunningTime);
							if(time > keyframeTime)
								keyframeTime = time;
						}
					}
				}
			}
			
			return keyframeTime;
		}
		
	#if (USEQ_FREE || USEQ_LICENSE)
		public static Stream GetResourceStream(string resourceName, Assembly assembly)
		{
			if (assembly == null)
				assembly = Assembly.GetExecutingAssembly();
			
			return assembly.GetManifestResourceStream(resourceName);
		}
		
		public static Stream GetResourceStream(string resourceName)
		{
			return GetResourceStream(resourceName, null);
		}
		
		public static byte[] GetByteResource(string resourceName, Assembly assembly)
		{ 
			Stream byteStream = GetResourceStream(resourceName, assembly);
			long length = 0;
			if(byteStream != null)
				length = byteStream.Length;
			
			byte[] buffer = new byte[length];
			
			if(buffer.Length == 0)
				return buffer;
			
			byteStream.Read(buffer, 0, (int)byteStream.Length);
			byteStream.Close();
			
			return buffer;
		}
		
		public static byte[] GetByteResource(string resourceName)
		{
			return GetByteResource(resourceName, null);
		}
		
		public static Texture2D GetTextureResource(string resourceName, Assembly assembly)
		{
			Texture2D texture = new Texture2D(4, 4);
			texture.LoadImage(GetByteResource(resourceName, assembly));
			
			return texture;
		}
		
		public static Texture2D GetTextureResource (string resourceName)
		{
			return GetTextureResource(resourceName, null);
		}
	#endif
	}
}