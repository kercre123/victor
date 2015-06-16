using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	/// <summary>
	/// This is an abstract class that represents a hierarchy item for the USSequencer window. You will need to extend this if you wish to implement custom rendering 
	/// for a timeline within the uSequencer window.
	/// </summary>
	public abstract class IUSTimelineHierarchyItem : IUSHierarchyItem
	{
		[SerializeField]
		public USTimelineBase baseTimeline;
		public USTimelineBase BaseTimeline
		{
			get { return baseTimeline; }
			set { baseTimeline = value; }
		}

		/// <summary>
		/// Returns the object that will be pingable if the user is to click on this timeline in the uSequencer hierarchy.
		/// </summary>
		/// <value>The pingable object.</value>
		public virtual Transform PingableObject
		{
			get { return BaseTimeline.transform; }
			set { ; }
		}

		public USTimelineBase TimelineToDestroy()
		{
			return BaseTimeline;
		}
		
		public bool IsForThisTimeline(USTimelineBase timeline)
		{
			return BaseTimeline == timeline;
		}

		/// <summary>
		/// If you need custom initialization code for your Timeline, include it here.
		/// </summary>
		/// <param name="timeline">the Timeline.</param>
		public override void Initialize(USTimelineBase timeline)
		{
			base.Initialize(timeline);
			BaseTimeline = timeline;
		}
	}
}