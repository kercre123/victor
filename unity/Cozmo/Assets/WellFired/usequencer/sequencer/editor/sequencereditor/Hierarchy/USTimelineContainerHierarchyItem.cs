using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	[Serializable]
	public class USTimelineContainerHierarchyItem : IUSHierarchyItem 
	{
		public override bool ShouldDisplayMoreButton
		{
			get { return USEditorUtility.CanContextClickTimelineContainer(TimelineContainer); }
			set { ; }
		}
		
		public override int SortOrder
		{
			get { return USRuntimeUtility.IsObserverContainer(timelineContainer.transform) == true ? -1 : 0; }
		}

		[SerializeField]
		private USTimelineContainer timelineContainer;
		public USTimelineContainer TimelineContainer
		{
			get { return timelineContainer; }
			set { timelineContainer = value; }
		}
		
		public override void DoGUI(int depth)
		{
			foreach (var child in Children)
			{
				if(((IUSTimelineHierarchyItem)child).BaseTimeline != null)
					((IUSTimelineHierarchyItem)child).BaseTimeline.ShouldRenderGizmos = IsExpanded && USPreferenceWindow.RenderHierarchyGizmos;
			}

			using(new WellFired.Shared.GUIBeginVertical())
			{
				using(new WellFired.Shared.GUIBeginHorizontal())
				{
					FloatingOnGUI();
					ContentPaneOnGUI();
				}
			}
		}
		
		private void FloatingOnGUI()
		{
			GUILayout.Box("", FloatingBackground, GUILayout.MaxWidth(FloatingWidth), GUILayout.Height(17.0f));
			
			if(Event.current.type == EventType.Repaint)
				FloatingBackgroundRect = GUILayoutUtility.GetLastRect();
	
			bool wasLabelPressed;
			bool newExpandedState = USEditor.FoldoutLabel(FloatingBackgroundRect, IsExpanded, USEditorUtility.GetReadableTimelineContainerName(TimelineContainer), out wasLabelPressed);
			if(newExpandedState != IsExpanded)
			{
				USUndoManager.PropertyChange(this, "Foldout");
				IsExpanded = newExpandedState;
			}

			base.FloatingOnGUI(0);

			if(!USRuntimeUtility.HasObserverTimeline(TimelineContainer.transform) && TimelineContainer.AffectedObject == null)
				IsExpanded = false;
		}
		
		private void ContentPaneOnGUI()
		{
			GUILayout.Box("", TimelineBackground, GUILayout.Height(17.0f), GUILayout.ExpandWidth(true));

			using(new WellFired.Shared.GUIBeginArea(ContentBackgroundRect))
			{
				if(!USRuntimeUtility.HasObserverTimeline(TimelineContainer.transform) && TimelineContainer.AffectedObject == null)
					GUILayout.Label(string.Format("The Affected Object {0} is not in your scene.", TimelineContainer.AffectedObjectPath));
			}

			if(Event.current.type == EventType.Repaint)
				ContentBackgroundRect = GUILayoutUtility.GetLastRect();
		}
	
		public void SetupWithTimelineContainer(USTimelineContainer timelineContainer)
		{
			TimelineContainer = timelineContainer;
			var timelines = TimelineContainer.Timelines;
			if(TimelineContainer.Timelines.Count() > Children.Count)
			{
				var notInBoth = timelines.Where(
					(timeline) => !Children.Any(
					(item) => 
					((item is IUSTimelineHierarchyItem) && (item as IUSTimelineHierarchyItem).IsForThisTimeline(timeline))
					)
					);
				
				foreach(var extraTimeline in notInBoth)
				{
					IUSHierarchyItem hierarchyItem = null;
					
					var customKeyValuePairs = USEditorUtility.CustomTimelineAttributes;
					foreach(var customKeyValuePair in customKeyValuePairs)
					{
						if(extraTimeline.GetType() == customKeyValuePair.Key.InspectedType)
						{
							hierarchyItem = ScriptableObject.CreateInstance(customKeyValuePair.Value) as IUSHierarchyItem;

							// This is here to capture the change in USTimelineBase BaseTimeline on the IUSTimelineHierarchyItem that occurse during initialize
							USUndoManager.PropertyChange(hierarchyItem, "Add New Timeline");

							hierarchyItem.Initialize(extraTimeline);
						}
					}
					
					AddChild(hierarchyItem);
				}
			}
		}

		public void AddTimeline(USTimelineBase timeline)
		{
			SetupWithTimelineContainer(TimelineContainer);

			if(Children.Count == 1)
				IsExpanded = true;
		}
	}
}