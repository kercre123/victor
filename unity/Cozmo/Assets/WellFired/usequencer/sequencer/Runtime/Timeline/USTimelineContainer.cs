using UnityEngine;
using System;
using System.Linq;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	/// <summary>
	/// The USTimelineContainer does little besides hold Timelines.
	/// The Container holds the Affected Object, this is the object which all the timelines act upon. This can be retrieved
	/// from anywhere in the hierarchy, containers, child timelines, events, keyframes, these objects simple travers
	/// up the hierarchy.
	/// 
	/// The structure for a sequencer is as such
	/// USequencer
	/// 	USTimelineContainer
	/// 		USTimelineBase
	/// </summary>
	[ExecuteInEditMode]
	public class USTimelineContainer : MonoBehaviour 
	{
		public static int Comparer(USTimelineContainer a, USTimelineContainer b)
		{
			return (a.Index.CompareTo(b.Index));
		}
		
		#region Member Variables
		[SerializeField]
		private Transform affectedObject = null;

		[SerializeField]
		private string affectedObjectPath;
		
		[SerializeField]
		private int index = -1;
		#endregion
		
		#region Properties
		/// <summary>
		/// Gets the object that this timeline container affects.
		/// </summary>
		public Transform AffectedObject
		{
			get 
			{
				if(affectedObject == null && affectedObjectPath != string.Empty)
				{
					var foundGameObject = GameObject.Find(affectedObjectPath);
					if(foundGameObject)
					{
						affectedObject = foundGameObject.transform;
						foreach(var timeline in Timelines)
							timeline.LateBindAffectedObjectInScene(affectedObject);
					}
				}

				return affectedObject; 
			}
			set
			{
				affectedObject = value;

				if(affectedObject != null)
					affectedObjectPath = affectedObject.transform.GetFullHierarchyPath();

				RenameTimelineContainer();
			}
		}
		
		/// <summary>
		/// Gets the sequence that this timeline container belongs to.
		/// </summary>
		public USSequencer Sequence
		{
			get
			{
				USSequencer sequence = transform.parent.GetComponent<USSequencer>();
	
				// Check that this has a sequence, this is simply to catch cases where things have gone wrong.
				if (!sequence)
					Debug.LogWarning(name + " does not have a parent with an attached sequence, this is a problem. " + name + "'s parent is : " + transform.parent.name);
	
				return sequence;
			}
		}
		
		/// <summary>
		/// Gets all our child timelines.
		/// </summary>
		public USTimelineBase[] Timelines
		{
			get { return GetComponentsInChildren<USTimelineBase>(); }
		}
		
		public int Index
		{
			get { return index; }
			set { index = value; }
		}

		public string AffectedObjectPath 
		{
			get { return affectedObjectPath; }
			private set { affectedObjectPath = value; }
		}

		#endregion
		
		private void Start()
		{
			if(affectedObjectPath == null)
			{
				affectedObjectPath = string.Empty;
			}
			else
			{
				if(AffectedObject == null && affectedObjectPath.Length != 0)
				{
					GameObject affectedGameObject = GameObject.Find(affectedObjectPath);
					AffectedObject = affectedGameObject.transform;
				}
			}

			foreach(var timeline in Timelines)
			{
				var propertyTimeline = timeline as USTimelineProperty;
				if(!propertyTimeline)
				{
					continue;
				}

				propertyTimeline.TryToFixComponentReferences();
			}
		}
		
		public void AddNewTimeline(USTimelineBase timeline)
		{	
			transform.parent = transform;
		}
		
		public void ProcessTimelines(float sequencerTime, float playbackRate)
		{	
			foreach(USTimelineBase timeline in Timelines)
				timeline.Process(sequencerTime, playbackRate);
		}
		
		public void SkipTimelineTo(float sequencerTime)
		{
			foreach(USTimelineBase timeline in Timelines)
				timeline.SkipTimelineTo(sequencerTime);
		}
		
		public void ManuallySetTime(float sequencerTime)
		{
			foreach(USTimelineBase timeline in Timelines)
				timeline.ManuallySetTime(sequencerTime);
		}
		
		/// <summary>
		/// This is a utility function to rename a container, simply makes it more readable.
		/// </summary>
		public void RenameTimelineContainer()
		{
			if(affectedObject)
				name = "Timelines for " + affectedObject.name;	
		}
	}
}