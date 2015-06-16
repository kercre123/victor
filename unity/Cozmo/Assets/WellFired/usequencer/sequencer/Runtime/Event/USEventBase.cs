using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	/// <summary>
	/// We use this attribute to define wether or not we hide the Duration in the inspector view.
	/// </summary>
	public class USequencerEventHideDurationAttribute : Attribute
	{
		/// <summary>
		/// An Attribute that tells Unity if it should hide the events duration in the inspector.
		/// </summary>
		public USequencerEventHideDurationAttribute()
		{

		}
	}

	/// <summary>
	/// We use this attribute to define data specific to an uSequencer Editor Event
	/// </summary>
	public class USequencerEventAttribute : Attribute
	{
		/// <summary>
		/// An Attribute that tells uSequencer a class is an custom USEventBase, don't forget to add this to any custom events.
		/// </summary>
		/// <param name="myEventPath">The path this event uses in our Context Menu, Add Event/myEventPath, for instance "Debug/Log Message" would become Add Event/Debug/Log Message</param>
		public USequencerEventAttribute(string myEventPath)
		{
			eventPath = myEventPath;
		}
		
		/// <summary>
		/// The path to map this event in the context click menu.
		/// </summary>
		private string eventPath;
		public string EventPath
		{
			get { return eventPath; }
		}
	}
	
	/// <summary>
	/// We use this attribute to specify the event display name for custom events when rendering them in the uSequencer window
	/// </summary>
	public class USequencerFriendlyNameAttribute : Attribute
	{
		/// <summary>
		/// Initializes a new instance of the <see cref="WellFired.USequencerFriendlyNameAttribute"/> class.
		/// </summary>
		/// <param name="myFriendlyName">My friendly name.</param>
		public USequencerFriendlyNameAttribute(string myFriendlyName)
		{
			friendlyName = myFriendlyName;
		}
		
		/// <summary>
		/// The path to map this event in the context click menu.
		/// </summary>
		private string friendlyName;
		public string FriendlyName
		{
			get { return friendlyName; }
		}
	}

	/// <summary>
	/// Our Base event class, when creating custom events, you need to inherit from this.
	/// </summary>
	[ExecuteInEditMode]
	[Serializable]
	abstract public class USEventBase : MonoBehaviour 
	{
		[SerializeField]
		private bool fireOnSkip = false;
		[SerializeField]
		private float firetime = -1.0f;
		[SerializeField]
		private float duration = -1.0f;
		[HideInInspector][SerializeField]
		private string[] serializedAdditionalObjectsPaths = new string[] { };
		 
		/// <summary>
		/// The time at which this event will be triggered
		/// </summary>
		public float FireTime
		{
			get { return firetime; }
			set
			{
				firetime = value;
				if(firetime < 0)
					firetime = 0;
				if(firetime > Sequence.Duration)
					firetime = Sequence.Duration;
			}
		}

		/// <summary>
		/// The duration of this Event, <0 is a <see cref="WellFired.USEventBase.IsFireAndForget"/>
		/// </summary>
		/// <value>The duration.</value>
		public float Duration
		{
			get { return duration; }
			set { duration = value; }
		}
	
		/// <summary>
		/// The sequence that this event currently resides
		/// </summary>
		/// <value>The sequence.</value>
		public USSequencer Sequence
		{
			get { return Timeline?Timeline.Sequence:null; }
		}

		/// <summary>
		/// The timeline that this event currently resides
		/// </summary>
		/// <value>The timeline.</value>
		public USTimelineBase Timeline
		{
			get
			{
				if(!transform.parent)
					return null;
				
				USTimelineBase timeline = transform.parent.GetComponent<USTimelineBase>();
	
				if (!timeline)
					Debug.LogWarning(name + " does not have a parent with an attached timeline, this is a problem. " + name + "'s parent is : " + transform.parent.name);
	
				return timeline;
			}
		}

		/// <summary>
		/// The timeline container that this event currently resides
		/// </summary>
		/// <value>The timeline container.</value>
		public USTimelineContainer TimelineContainer
		{
			get { return Timeline ? Timeline.TimelineContainer : null; }
		}

		/// <summary>
		/// The Affected Object is the object that this event will act upon.
		/// </summary>
		/// <value>The affected object.</value>
		public GameObject AffectedObject
		{
			get { return TimelineContainer.AffectedObject ? TimelineContainer.AffectedObject.gameObject : null; }
		}

		public void SetSerializedAdditionalObjectsPaths(string[] paths)
		{
			serializedAdditionalObjectsPaths = paths;
		}
		
		public void FixupAdditionalObjects()
		{
			if(serializedAdditionalObjectsPaths == null || serializedAdditionalObjectsPaths.Length == 0)
				return;
			
			if(HasValidAdditionalObjects())
				return;
				
			List<Transform> additionalObjects = new List<Transform>();
			foreach(string objectPath in serializedAdditionalObjectsPaths)
			{
				GameObject additionalObject = GameObject.Find(objectPath);
				if(additionalObject)
					additionalObjects.Add(additionalObject.transform);
			} 
				
			SetAdditionalObjects(additionalObjects.ToArray());
		}
		
		/// <summary>
		/// Is this event a one shot, or does it require processing.
		/// </summary>
		public bool IsFireAndForgetEvent
		{
			get { return Duration < 0.0f; }
		}

		/// <summary>
		/// If this is set to true, when calling the method <see cref="WellFired.USSequencer.SkipTimelineTo"/> this event will be fired.
		/// </summary>
		/// <value><c>true</c> if fire on skip; otherwise, <c>false</c>.</value>
		public bool FireOnSkip
		{
			get { return fireOnSkip; }
			set { fireOnSkip = value; }
		}
		
		/// <summary>
		/// Implement this method to define the behaviour when the uSequencer scrub head passes the start point of this event.
		/// </summary>
		public abstract void FireEvent();
		
		/// <summary>
		/// This is called on any event that isn't <see cref="WellFired.USEventBase.IsFireAndForget"/>.
		/// </summary>
		/// <param name="runningTime">Running time.</param>
		public abstract void ProcessEvent(float runningTime);

		/// <summary>
		/// Optionally implement this if you want custom functionality when the user pauses a sequence.
		/// </summary>
		public virtual void PauseEvent() { ; }

		/// <summary>
		/// Optionally implement this if you want custom functionality when the user resumes a sequence.
		/// </summary>
		public virtual void ResumeEvent() { ; }

		/// <summary>
		/// Optionally implement this if you want custom functionality when the user stops a sequence.
		/// </summary>
		public virtual void StopEvent() { ; }
	
		/// <summary>
		/// Optionally implement this if you want custom functionality after an event has finished. I.E. when the <see cref="WellFired.USSequencer.RunningTime"/> has passed the end of this event.
		/// </summary>
		public virtual void EndEvent() { ; }
		
		/// <summary>
		/// Optionally implement this if you want custom functionality when the <see cref="WellFired.USSequencer.RunningTime"/> has passed the start of this event and subsequently gone back to before the start of the event.
		/// </summary>
		public virtual void UndoEvent() { ; }

		/// <summary>
		/// Optionally implement this if you want custom functionality when the user has manually set the <see cref="WellFired.USSequencer.RunningTime"/>.
		/// </summary>
		/// <param name="deltaTime">Delta time.</param>
		public virtual void ManuallySetTime(float deltaTime) { ; }

		/// <summary>
		/// This method should be implemented if you have custom serialization requirements.
		/// 
		/// This method should probably return any objects that live in the scene that uSequencer will not automatically serialize for you.
		/// Currently, only the Affected Object will be serialized.
		/// The objects returned from this method will be passed to <see cref="WellFired.USEventBase.SetAdditionalObjects"/> when this event is constructed The order is maintained.
		/// </summary>
		/// <returns>The additional objects.</returns>
		public virtual Transform[] GetAdditionalObjects() { return new Transform[] { }; }

		/// <summary>
		/// This method should be implemented if you have custom serialization requirements.
		/// 
		/// When a sequence is created from a serialized sequence, this method will be called, passing all objects that have been optionally serialized by <see cref="WellFired.USEventBase.GetAdditionalObjects"/> The order is maintained.
		/// </summary>
		/// <param name="additionalObjects">Additional objects.</param>
		public virtual void SetAdditionalObjects(Transform[] additionalObjects) { ; }

		/// <summary>
		/// This method should be implemented if you have custom serialization requirements.
		/// 
		/// Return true if you have custom serialization requirements and want <see cref="WellFired.USEventBase.SetAdditionalObjects"/> and <see cref="WellFired.USEventBase.GetAdditionalObjects"/> to be called.
		/// </summary>
		/// <returns><c>true</c> if this instance has valid additional objects; otherwise, <c>false</c>.</returns>
		public virtual bool HasValidAdditionalObjects() { return false; }

		/// <summary>
		/// If your event does anything special with Scriptable Objects, or you want to implement custom behaviour on duplication,
		/// this will be your entry point.
		/// </summary>
		public virtual void MakeUnique() { ; }
	}
}