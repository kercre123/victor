using UnityEngine;
using System;
using System.Collections;

namespace WellFired
{
	/// <summary>
	/// The base class for every timeline. If you want to create your own timeline, simply extend this.
	/// The structure for a sequencer is as such
	/// <see cref="WellFired.USSequencer"/>
	/// 	<see cref="WellFired.USTimelineContainer"/>
	/// 		<see cref="WellFired.USTimelineBase"/>
	/// </summary>
	public class USTimelineBase : MonoBehaviour 
	{
		/// <summary>
		/// The Affected Object is the object that this timeline will work it's magic on. It can be anything.
		/// </summary>
		public Transform AffectedObject
		{
			get
			{
				USTimelineContainer timelineContainer = transform.parent.GetComponent<USTimelineContainer>();
	
				// Check that this has a container, this is simply to catch cases where things have gone wrong.
				if (!timelineContainer)
					Debug.LogWarning(name + " does not have a parent with an attached container, this is a problem. " + name + "'s parent is : " + transform.parent.name);
	
				return timelineContainer.AffectedObject;
			}
		}
		
		/// <summary>
		/// The <see cref="WellFired.USTimelineContainer"/> that this timeline resides.
		/// </summary>
		public USTimelineContainer TimelineContainer
		{
			get
			{
				USTimelineContainer timelineContainer = transform.parent.GetComponent<USTimelineContainer>();
	
				// Check that this has a timeline container, this is simply to catch cases where things have gone wrong.
				if (!timelineContainer)
					Debug.LogWarning(name + " does not have a parent with an attached timeline Container, this is a problem. " + name + "'s parent is : " + transform.parent.name);
	
				return timelineContainer;
			}
		}
		
		/// <summary>
		/// The <see cref="WellFired.USSequencer"/> that this timeline resides.
		/// </summary>
		public USSequencer Sequence
		{
			get
			{
				return TimelineContainer.Sequence;
			}
		}
		
		/// <summary>
		/// Should this timeline render it's gizmos.
		/// </summary>
		public bool ShouldRenderGizmos
		{
			get;
			set;
		}

		/// <summary>
		/// Optionally implement this method if you need custom behaviour when the <see cref="WellFired.USSequencer"/> Stops.
		/// </summary>
		public virtual void StopTimeline() { ; }
		
		/// <summary>
		/// Optionally implement this method if you need custom behaviour when the <see cref="WellFired.USSequencer"/> Starts.
		/// </summary>
		public virtual void StartTimeline() { ; }
		
		/// <summary>
		/// Optionally implement this method if you need custom behaviour when the <see cref="WellFired.USSequencer"/> Ends.
		/// </summary>
		public virtual void EndTimeline() { ; }

		/// <summary>
		/// Optionally implement this method if you need custom behaviour when the <see cref="WellFired.USSequencer"/> Pauses.
		/// </summary>
		public virtual void PauseTimeline() { ; }
		
		/// <summary>
		/// Optionally implement this method if you need custom behaviour when the <see cref="WellFired.USSequencer"/> is Resumed.
		/// </summary>
		public virtual void ResumeTimeline() { ; }
		
		/// <summary>
		/// Optionally implement this method if you need custom behaviour when the <see cref="WellFired.USSequencer"/> Skips.
		/// </summary>
		public virtual void SkipTimelineTo(float time) { ; }
		
		/// <summary>
		/// Optionally implement this method if you need custom behaviour when the <see cref="WellFired.USSequencer"/> processes. This should happen during regular playback and when scrubbing
		/// </summary>
		public virtual void Process(float sequencerTime, float playbackRate) { ; }
		
		/// <summary>
		/// Optionally implement this method if you need custom behaviour when the <see cref="WellFired.USSequencer"/> has it's time manually set.
		/// </summary>
		public virtual void ManuallySetTime(float sequencerTime) { ; }
		
		
		/// <summary>
		/// Implement custom logic here if you need to do something special when uSequencer finds a missing AffectedObject in the scene (prefab instantiaton, with late binding).
		/// </summary>
		/// <param name="newAffectedObject">New Affected object.</param>
		public virtual void LateBindAffectedObjectInScene(Transform newAffectedObject) { ; }
	}
}