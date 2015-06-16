using System;
using UnityEngine;

namespace WellFired
{
	/// <summary>
	/// You should add this attribute to any USTimelineBase classes that require custom visualisation within the uSequencer Window.
	/// </summary>
	public class USCustomTimelineHierarchyItem : Attribute
	{
		private const int MAX_TIMELINES_PER_SEQUENCE = 1000;
	
		/// <summary>
		/// The most basic form of map, a Type to the display Name, Example : [USCustomTimelineHierarchyItem(typeof(USTimelineEvent), "Event Timeline")]
		/// </summary>
		/// <param name="myInspectedType">My inspected type.</param>
		/// <param name="friendlyName">Friendly name.</param>
		public USCustomTimelineHierarchyItem(Type myInspectedType, string friendlyName)
		{
			if(!myInspectedType.IsSubclassOf(typeof(USTimelineBase)))
				throw new Exception(string.Format("myInspectedType of USCustomTimelineHierarchyItem must inherit from USTimelineBase"));
	
			InspectedType = myInspectedType;
			FriendlyName = friendlyName;
			MaxNumberPerSequence = MAX_TIMELINES_PER_SEQUENCE;
			CanBeManuallyAddToSequence = true;
		}
		
		/// <summary>
		/// A Type to the display Name with a maximum number of timelines per Affected Object, Example : [USCustomTimelineHierarchyItem(typeof(USTimelineProperty), "Property Timeline", 1)]
		/// </summary>
		/// <param name="myInspectedType">My inspected type.</param>
		/// <param name="friendlyName">Friendly name.</param>
		public USCustomTimelineHierarchyItem(Type myInspectedType, string friendlyName, int maxNumberPerSequence)
		{
			if(!myInspectedType.IsSubclassOf(typeof(USTimelineBase)))
				throw new Exception(string.Format("myInspectedType of USCustomTimelineHierarchyItem must inherit from USTimelineBase"));
			
			InspectedType = myInspectedType;
			FriendlyName = friendlyName;
			MaxNumberPerSequence = maxNumberPerSequence;
			CanBeManuallyAddToSequence = true;
		}

		/// <summary>
		/// A Type to the display Name with a parameter specifying if this can be manually added to a sequence, or if it is script only., Example : [USCustomTimelineHierarchyItem(typeof(USTimelineObserver), "Observer Timeline", false)]
		/// </summary>
		/// <param name="myInspectedType">My inspected type.</param>
		/// <param name="friendlyName">Friendly name.</param>
		public USCustomTimelineHierarchyItem(Type myInspectedType, string friendlyName, bool canBeManuallyAddToSequence)
		{
			if(!myInspectedType.IsSubclassOf(typeof(USTimelineBase)))
				throw new Exception(string.Format("myInspectedType of USCustomTimelineHierarchyItem must inherit from USTimelineBase"));
	
			InspectedType = myInspectedType;
			FriendlyName = friendlyName;
			MaxNumberPerSequence = MAX_TIMELINES_PER_SEQUENCE;
			CanBeManuallyAddToSequence = canBeManuallyAddToSequence;
		}
		
		public string FriendlyName
		{
			get;
			set;
		}
	
		public Type InspectedType
		{
			get;
			set;
		}
	
		public int MaxNumberPerSequence
		{
			get;
			set;
		}
		
		public bool CanBeManuallyAddToSequence
		{
			get;
			set;
		}
	}
}