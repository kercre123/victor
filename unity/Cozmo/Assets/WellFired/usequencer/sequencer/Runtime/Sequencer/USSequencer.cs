using UnityEngine;
using System;
using System.Reflection;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	/// <summary>
	/// The uSequencer behaviour, this deals with updating and processing all our timelines.
	/// The structure for a sequencer is as such
	/// USequencer
	/// 	USTimelineContainer
	/// 		USTimelineBase
	/// </summary>
	[ExecuteInEditMode]
	[Serializable]
	public class USSequencer : MonoBehaviour 
	{
		#region Member Variables
		/// <summary>
		/// The observed objects is basically every game object referenced in the usequencer, we need this list, so 
		/// we don't have to do anything crazy like adding un-needed components to our users game objects.
		/// </summary>
		[SerializeField]
		private List<Transform> observedObjects = new List<Transform>();
		
		[SerializeField]
		private float runningTime = 0.0f;
		
		[SerializeField]
		private float playbackRate = 1.0f;
		
		[SerializeField]
		private int version = 2;
		
		[SerializeField]
		private float duration = 10.0f;
		
		[SerializeField]
		private bool isLoopingSequence = false;
		
		[SerializeField]
		private bool isPingPongingSequence = false;
		
		[SerializeField]
		private bool updateOnFixedUpdate = false;
		
		private bool playing = false;
		
		private bool isFreshPlayback = true;
		
		private float previousTime = 0.0f;
		
		private float minPlaybackRate = -100.0f;
		
		private float maxPlaybackRate = 100.0f;
		
		private float setSkipTime = -1.0f;
		
		// The structure containing our watermark data. This shouldn't be used unless you define USEQFREE, but you paid,
		// why would you want to?
	#if USEQ_FREE
		private class playBackTimeData
		{
			public Texture2D data;
			public float playbackReductionTimer = 2.35432991f;
		}
		private playBackTimeData playbackData = new playBackTimeData();
	#endif
		#endregion
		
		#region Properties
		/// <summary>
		/// Our current Version number, you can manipulate this if you would like to force the uSequencer to upgrade
		/// your sequence, for some reason.
		/// </summary>
		public int Version
		{
			get
			{
				return version;
			}
			set
			{
				version = value;
			}
		}
		
		/// <summary>
		/// This is a list of every object that the uSequencer sequences.
		/// </summary>
		public List<Transform> ObservedObjects
		{
			get
			{
				return observedObjects;
			}
		}
		
		/// <summary>
		/// The duration of this sequence, you CAN manipulate this in script if you like.
		/// </summary>
		public float Duration
		{
			get
			{
				return duration;
			}
			set
			{
				duration = value;
				if(duration <= 0.0f)
					duration = 0.1f;
			}
		}
		
		/// <summary>
		/// Gets a value indicating whether this sequence is playing.
		/// </summary>
		public bool IsPlaying
		{
			get { return playing; }
		}
		
		/// <summary>
		/// Gets a value indicating whether this sequence is looping, you CAN manipulate this through script.
		/// </summary>
		public bool IsLopping
		{
			get { return isLoopingSequence; }
			set { isLoopingSequence = value; }
		}
		
		/// <summary>
		/// Gets a value indicating whether this sequence is ping ponging, you CAN manipulate this through script.
		/// </summary>
		public bool IsPingPonging
		{
			get { return isPingPongingSequence; }
			set { isPingPongingSequence = value; }
		}

		/// <summary>
		/// Method that allows you to determine if a sequence is still playing
		/// </summary>
		/// <value><c>true</c> if this instance is complete; otherwise, <c>false</c>.</value>
		public bool IsComplete
		{
			get { return (!IsPlaying && RunningTime >= Duration); }
			set { ; }
		}
		
		/// <summary>
		/// If you need to manually set the running time of this sequence, you can do it with this attribute.
		/// You can do this in the editor, or in game. Skipping forward or backwards is supported.
		/// </summary>
		public float RunningTime
		{
			get { return runningTime; }
			set 
			{
				runningTime = value;
				if(runningTime <= 0.0f)
					runningTime = 0.0f;
				
				if(runningTime > duration)
					runningTime = duration;
			
				if(isFreshPlayback)
				{
					foreach(USTimelineContainer timelineContainer in TimelineContainers)
					{		
			   			foreach (USTimelineBase timeline in timelineContainer.Timelines)
		   	   	  	    	timeline.StartTimeline();
					}
					isFreshPlayback = false;
				}
					
				foreach(USTimelineContainer timelineContainer in TimelineContainers)
				{
					timelineContainer.ManuallySetTime(RunningTime);
					timelineContainer.ProcessTimelines(RunningTime, PlaybackRate);
				}
				
				OnRunningTimeSet(this);
			}
		}
		
		/// <summary>
		/// Our current playback rate, you CAN manipulate this through script a negative value will play this sequence in reverse.
		/// </summary>
		public float PlaybackRate
		{
			get { return playbackRate; }
			set { playbackRate = Mathf.Clamp(value, MinPlaybackRate, MaxPlaybackRate); }
		}
		
		public float MinPlaybackRate
		{
			get { return minPlaybackRate; }
		}
		
		public float MaxPlaybackRate
		{
			get { return maxPlaybackRate; }
		}

		/// <summary>
		/// This will tell you if the sequence has been played, but isn't currently playing. For instance, if someone pressed Play and the Pause.
		/// </summary>
		/// <value><c>true</c> if this instance has sequence been started; otherwise, <c>false</c>.</value>
		public bool HasSequenceBeenStarted
		{
			get { return !isFreshPlayback; }
		}
	
		public USTimelineContainer[] TimelineContainers
		{
			get
			{
				return GetComponentsInChildren<USTimelineContainer>();
			}
		}
		
		public USTimelineContainer[] SortedTimelineContainers
		{
			get
			{
				USTimelineContainer[] timelineContainers = GetComponentsInChildren<USTimelineContainer>();
				Array.Sort(timelineContainers, USTimelineContainer.Comparer);
				
				return timelineContainers;
			}
		}
		
		public int TimelineContainerCount
		{
			get
			{
				return TimelineContainers.Length;
			}
		}
		
		public int ObservedObjectCount
		{
			get
			{
				return ObservedObjects.Count;
			} 
		}
		
		public bool UpdateOnFixedUpdate
		{
			get { return updateOnFixedUpdate; } 
			set { updateOnFixedUpdate = value; } 
		}

		/// <summary>
		/// All sequences are updated on a coroutine, we yield return new WaitForSeconds(SequenceUpdateRate); on the coroutine
		/// </summary>
		/// <value>The sequence update rate.</value>
		public static float SequenceUpdateRate
		{
			get { return 0.01f * Time.timeScale; }
		}
		#endregion
		
		#region Delegates
		public delegate void PlaybackDelegate(USSequencer sequencer);
		public delegate void UpdateDelegate(USSequencer sequencer, float newRunningTime);
		/// <summary>
		/// This Delegate will be called when Playback has Started, add delegates with +=
		/// </summary>
		public PlaybackDelegate PlaybackStarted = delegate { };
		/// <summary>
		/// This Delegate will be called when Playback has Stopped, add delegates with +=
		/// </summary>
		public PlaybackDelegate PlaybackStopped = delegate { };
		/// <summary>
		/// This Delegate will be called when Playback has Paused, add delegates with +=
		/// </summary>
		public PlaybackDelegate PlaybackPaused = delegate { };
		/// <summary>
		/// This Delegate will be called when Playback has Finished, add delegates with +=
		/// </summary>
		public PlaybackDelegate PlaybackFinished = delegate { };
		/// <summary>
		/// This Delegate will be called before an update with the new runningTime, and before timelines have been processed add delegates with +=
		/// </summary>
		public UpdateDelegate BeforeUpdate = delegate { };
		/// <summary>
		/// This Delegate will be called after an update with the new runningTime, and after timelines have been processed add delegates with +=
		/// </summary>
		public UpdateDelegate AfterUpdate = delegate { };
		/// <summary>
		/// This Delegate will be called whenever the RunningTime is set add delegates with +=
		/// </summary>
		public PlaybackDelegate OnRunningTimeSet = delegate { };
		#endregion

		private void OnDestroy()
		{
			StopCoroutine("UpdateSequencerCoroutine");
		}
		
		private void Start()
		{
			// Attempt to auto fix our Event Objects
			foreach(USTimelineContainer timelineContainer in TimelineContainers)
			{	
				if(!timelineContainer)
					continue;
				
				foreach(USTimelineBase timelineBase in timelineContainer.Timelines)
				{
					USTimelineEvent timelineEvent = timelineBase as USTimelineEvent;
					if(timelineEvent)
					{
						foreach(USEventBase eventBase in timelineEvent.Events)
							eventBase.FixupAdditionalObjects();
					}

					USTimelineObjectPath timelineObjectPath = timelineBase as USTimelineObjectPath;
					if(timelineObjectPath)
					{
						timelineObjectPath.FixupAdditionalObjects();
					}
				}	
			}
		}
	
		public void TogglePlayback()
		{
			if(playing)
				Pause();
			else
				Play();
		}
		
		public void Play()
		{
			if(PlaybackStarted != null)
				PlaybackStarted(this);
			
			// Playback runs on a coroutine.
			StartCoroutine("UpdateSequencerCoroutine");
			
			// Start or resume our playback.
			if(isFreshPlayback)
			{
				foreach(USTimelineContainer timelineContainer in TimelineContainers)
				{		
			        foreach (USTimelineBase timeline in timelineContainer.Timelines)
		   	     	{
		   	     	    timeline.StartTimeline();
		        	}
				}
				isFreshPlayback = false;
			}
			else
			{
				foreach(USTimelineContainer timelineContainer in TimelineContainers)
				{		
			        foreach (USTimelineBase timeline in timelineContainer.Timelines)
		   	     	{
		   	     	    timeline.ResumeTimeline();
		        	}
				}
			}
			
			playing = true;
			previousTime = Time.time;
		}
		
		public void Pause()
		{
			if(PlaybackPaused != null)
				PlaybackPaused(this);
			
			playing = false;
			
			foreach(USTimelineContainer timelineContainer in TimelineContainers)
			{		
		        foreach (USTimelineBase timeline in timelineContainer.Timelines)
	        	{
	        	    timeline.PauseTimeline();
	        	}
			}
		}
		
		public void Stop()
		{
			if(PlaybackStopped != null)
				PlaybackStopped(this);
			
			// Playback runs on a coroutine.
			StopCoroutine("UpdateSequencerCoroutine");
			
			foreach(USTimelineContainer timelineContainer in TimelineContainers)
			{		
		        foreach (USTimelineBase timeline in timelineContainer.Timelines)
	        	{
					if(timeline.GetType() == typeof(USTimelineObserver) ||  timeline.AffectedObject != null)
						timeline.StopTimeline();
	        	}
			}
			
			isFreshPlayback = true;
			playing = false;
			runningTime = 0.0f;
		}

		/// <summary>
		///  This method will be called when the scrub head has reached the end of playback, for a 10s long sequence, this will be 10 seconds in.
		/// </summary>
		private void End()
		{
			if(PlaybackFinished != null)
				PlaybackFinished(this);

			if(isLoopingSequence || isPingPongingSequence)
				return;

			foreach(USTimelineContainer timelineContainer in TimelineContainers)
			{		
		        foreach (USTimelineBase timeline in timelineContainer.Timelines)
	        	{
					if(timeline.AffectedObject != null)
						timeline.EndTimeline();
	        	}
			}
		}
		
		/// <summary>
		/// Untility Function to creates a new timeline container, if you want to manually manipulate the sequence, 
		/// you can. Calling this function would simulate the drag drop of an object onto the uSequencer
		/// </summary>
		public USTimelineContainer CreateNewTimelineContainer(Transform affectedObject)
		{
			GameObject newTimelineContainerGO = new GameObject("Timelines for " + affectedObject.name);
			newTimelineContainerGO.transform.parent = transform;
			
			USTimelineContainer timelineContainer = newTimelineContainerGO.AddComponent<USTimelineContainer>();
			timelineContainer.AffectedObject = affectedObject;
			
			int highestIndex = 0;
			foreach(USTimelineContainer ourTimelineContainer in TimelineContainers)
			{
				if(ourTimelineContainer.Index > highestIndex)
					highestIndex = ourTimelineContainer.Index;
			}
			
			timelineContainer.Index = highestIndex + 1;
			
			return timelineContainer;
		}

		/// <summary>
		/// Utility method that enables you to find out if a sequence has a timelinecontainer for a specific Affected Object.
		/// </summary>
		/// <returns><c>true</c> if this instance has timeline container for the specified affectedObject; otherwise, <c>false</c>.</returns>
		/// <param name="affectedObject">Affected object.</param>
		public bool HasTimelineContainerFor(Transform affectedObject)
		{
			foreach(var timelineContainer in TimelineContainers)
			{
				if(timelineContainer.AffectedObject == affectedObject)
					return true;
			}
			return false;
		}

		/// <summary>
		/// Utility method that enables you to find the timelinecontainer for a specific Affected Object. Returns NULL if one does not exist
		/// </summary>
		/// <returns>The timeline container for.</returns>
		/// <param name="affectedObject">Affected object.</param>
		public USTimelineContainer GetTimelineContainerFor(Transform affectedObject)
		{
			foreach(var timelineContainer in TimelineContainers)
			{
				if(timelineContainer.AffectedObject == affectedObject)
					return timelineContainer;
			}
			return null;
		}
		
		public void DeleteTimelineContainer(USTimelineContainer timelineContainer)
		{		
			GameObject.DestroyImmediate(timelineContainer.gameObject);
		}
		
		public void RemoveObservedObject(Transform observedObject)
		{
			if(!observedObjects.Contains(observedObject))
				return;
			
			observedObjects.Remove(observedObject);
		}
		
		/// <summary>
		/// Sets the time of this sequence to the passed time. This function will only fire events that are flagged as
		/// Fire On Skip and are Fire And Forget Events (A.K.A have a duration of < 0), all other events will be ignored. 
		/// If you want to set the time and fire all events, simply set the RunningTime.
		/// 
		/// ObserverTimelines and PropertyTimelines will work as before.
		/// </summary>
		public void SkipTimelineTo(float time)
		{
			if(RunningTime <= 0.0f && !IsPlaying)
				Play();

			setSkipTime = time;
		}
	
		public void SetPlaybackRate(float rate)
		{
			PlaybackRate = rate;
		}
	
		public void SetPlaybackTime(float time)
		{
			RunningTime = time;
		}
		
		public void UpdateSequencer(float deltaTime)
		{
			// Modify for our playback rate
			deltaTime *= playbackRate;
			
			// Update our timelines.
			if(playing)
			{
				runningTime += deltaTime;
				float sampleTime = runningTime;
	
				if(sampleTime <= 0.0f)
					sampleTime = 0.0f;
				if(sampleTime > Duration)
					sampleTime = Duration;
				
				BeforeUpdate(this, runningTime);
	
				foreach (USTimelineContainer timelineContainer in TimelineContainers)
					timelineContainer.ProcessTimelines(sampleTime, PlaybackRate);

				AfterUpdate(this, runningTime);
				
				bool hasReachedEnd = false;
				if(playbackRate > 0.0f && RunningTime >= duration)
					hasReachedEnd = true;
				if(playbackRate < 0.0f && RunningTime <= 0.0f)
					hasReachedEnd = true;

				if(hasReachedEnd)
				{
					// Here we will loop our sequence, if needed.
					if(isLoopingSequence)
					{
						var newRunningTime = 0.0f;
						if(playbackRate > 0.0f && RunningTime >= Duration)
							newRunningTime = RunningTime - Duration;
						if(playbackRate < 0.0f && RunningTime <= 0.0f)
							newRunningTime = Duration + RunningTime;
	
						Stop();
					
						runningTime = newRunningTime;
						previousTime = -1.0f;
						
						Play();

						UpdateSequencer(0.0f);
						
						return;
					}
					
					if(isPingPongingSequence)
					{
						if(playbackRate > 0.0f && RunningTime >= Duration)
							runningTime = Duration + (Duration - RunningTime);
						if(playbackRate < 0.0f && RunningTime <= 0.0f)
							runningTime = -1.0f * RunningTime;
	
						playbackRate *= -1.0f;
						
						return;
					}
					
					playing = false;
					
					// Playback runs on a coroutine.
					StopCoroutine("UpdateSequencerCoroutine");

					End();
				}
			}
			
			// Update happens on a co-routine, so deal with the skip here to avoid conflicts.
			if(setSkipTime > 0.0f)
			{
				// Update the sequence with the new time
				foreach (USTimelineContainer timelineContainer in TimelineContainers)
					timelineContainer.SkipTimelineTo(setSkipTime);
	
				runningTime = setSkipTime;
				
				// Store our previous time as though our last update was just now. This is just incase we don't skip to the end.
				previousTime = Time.time;
				
				// Reset the skip time so we don't try to skip again.
				setSkipTime = -1.0f;
			}
		}
		
		// We actually run the update with a coroutine.
		private IEnumerator UpdateSequencerCoroutine()
		{
			while (true)
			{
				if(UpdateOnFixedUpdate)
					yield break;

				float currentTime = Time.time;
				UpdateSequencer(currentTime - previousTime);
				previousTime = currentTime;
				
				yield return new WaitForSeconds(SequenceUpdateRate);
			}
		}

		private void FixedUpdate()
		{
			if(!UpdateOnFixedUpdate)
				return;

			float currentTime = Time.time;
			UpdateSequencer(currentTime - previousTime);
			previousTime = currentTime;
		}
		
		// This chunk of code deals with the Watermark, for the free version.
	#if USEQ_FREE
		private void OnDisable()
		{
			if(playbackData.data)
				DestroyImmediate(playbackData.data);
		}
		
		private void Awake()
		{
			if(playbackData.data == null)
			{
				UpdateBackground();
			}
		}
		
		private void OnGUI()
		{
			playbackData.playbackReductionTimer -= Time.deltaTime;
			if(playbackData.data == null || playbackData.playbackReductionTimer < 0.0f)
			{
				UpdateBackground();
				playbackData.playbackReductionTimer = UnityEngine.Random.Range(14.8f, 26.8f);
			}
			
			if(playbackData.data == null)
			{
				Debug.LogError("Someone Removed The WaterMark For uSequencer");
				Application.Quit();
			}
			
			Rect position = new Rect(10, 10, 226, 36);
			GUI.DrawTexture(position, playbackData.data);
		}
		
		private void UpdateBackground()
		{
			if(playbackData.data)
				DestroyImmediate(playbackData.data);
			
			playbackData.data = USRuntimeUtility.GetTextureResource("Watermark Alt 2.png");
		}
	#endif
	}
}