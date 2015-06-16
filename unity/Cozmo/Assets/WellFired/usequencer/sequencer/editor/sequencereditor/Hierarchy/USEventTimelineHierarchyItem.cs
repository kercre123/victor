using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	[Serializable]
	[USCustomTimelineHierarchyItem(typeof(USTimelineEvent), "Event Timeline")]
	public class USEventTimelineHierarchyItem : IUSTimelineHierarchyItem
	{	
		public override bool ShouldDisplayMoreButton
		{
			get { return true; }
			set { ; }
		}

		private Rect MoreRect 
		{
			get;
			set;
		}

		private List<ISelectableContainer> ISelectableContainers;
	
		[SerializeField]
		public USTimelineEvent EventTimeline
		{
			get { return BaseTimeline as USTimelineEvent; }
			set { BaseTimeline = value; }
		}
		
		[SerializeField]
		private EventEditor eventEditor;

		private EventEditor EventEditor
		{
			get { return eventEditor; }
			set { eventEditor = value; }
		}
		
		public override bool IsISelectable()
		{
			return true;
		}
	
		public override List<ISelectableContainer> GetISelectableContainers()
		{
			return ISelectableContainers;
		}
		
		public override void OnEnable()
		{
			base.OnEnable();
			
			if(EventEditor == null)
				EventEditor = ScriptableObject.CreateInstance<EventEditor>();
	
			ISelectableContainers = new List<ISelectableContainer>() { EventEditor, };
		}
		
		public override void DoGUI(int depth)
		{
			using(new WellFired.Shared.GUIBeginHorizontal())
			{
				FloatingOnGUI(depth);
	
				EventEditor.Duration = EventTimeline.Sequence.Duration;
				EventEditor.XScale = XScale;
				EventEditor.XScroll = XScroll;
				EventEditor.YScroll = YScroll;
	
				if(IsExpanded)
					EventEditor.OnGUI();
				else
					EventEditor.OnCollapsedGUI();
				
				GUI.Box(FloatingBackgroundRect, "", FloatingBackground);
	
				bool wasLabelPressed = false;
				bool newExpandedState = USEditor.FoldoutLabel(FloatingBackgroundRect, IsExpanded, EventTimeline.name, out wasLabelPressed);
				if(newExpandedState != IsExpanded)
				{
					USUndoManager.PropertyChange(this, "Foldout");
					IsExpanded = newExpandedState;
					EditorWindow.Repaint();
				}

				base.FloatingOnGUI(depth, MoreRect);
			}
		}
		
		protected override void FloatingOnGUI(int depth)
		{
			GUILayout.Box("", FloatingBackground, GUILayout.MaxWidth(FloatingWidth), IsExpanded ? GUILayout.Height(50.0f) : GUILayout.Height(17.0f));
			
			if(Event.current.type == EventType.Repaint)
			{
				Rect lastRect = GUILayoutUtility.GetLastRect();
				lastRect.x += GetXOffsetForDepth(depth);
				MoreRect = lastRect;
				lastRect.width -= GetXOffsetForDepth(depth);
				if(FloatingBackgroundRect != lastRect)
				{
					EditorWindow.Repaint();
					FloatingBackgroundRect = lastRect;
				}
			}
		}

		public override void Initialize(USTimelineBase timeline)
		{
			base.Initialize(timeline);
			
			EventEditor.InitializeWithEvents(EventTimeline.Events);
			EventEditor.EventTimeline = EventTimeline;
		}
	}
}