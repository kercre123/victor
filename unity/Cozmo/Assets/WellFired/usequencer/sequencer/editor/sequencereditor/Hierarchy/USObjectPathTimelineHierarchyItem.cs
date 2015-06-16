using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	[Serializable]
	[USCustomTimelineHierarchyItem(typeof(USTimelineObjectPath), "Object Path Timeline")]
	public class USObjectPathTimelineHierarchyItem : IUSTimelineHierarchyItem
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
		public USTimelineObjectPath ObjectPathTimeline
		{
			get { return BaseTimeline as USTimelineObjectPath; }
			set { BaseTimeline = value; }
		}
		
		[SerializeField]
		private ObjectPathEditor objectPathEditor;
		private ObjectPathEditor ObjectPathEditor
		{
			get { return objectPathEditor; }
			set { objectPathEditor = value; }
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
			
			if(ObjectPathEditor == null)
				ObjectPathEditor = ScriptableObject.CreateInstance<ObjectPathEditor>();

			if(ObjectPathTimeline)
				ObjectPathTimeline.Build();

			ISelectableContainers = new List<ISelectableContainer>() { ObjectPathEditor, };
		}
		
		public override void DoGUI(int depth)
		{
			using(new WellFired.Shared.GUIBeginHorizontal())
			{
				using(new WellFired.Shared.GUIBeginVertical(GUILayout.MaxWidth(FloatingWidth)))
					FloatingOnGUI(depth);

				if(ObjectPathEditor.ObjectPathTimeline != ObjectPathTimeline)
					ObjectPathEditor.ObjectPathTimeline = ObjectPathTimeline;

				ObjectPathEditor.XScale = XScale;
				ObjectPathEditor.XScroll = XScroll;
				ObjectPathEditor.YScroll = YScroll;
				ObjectPathEditor.OnCollapsedGUI();
				
				GUI.Box(FloatingBackgroundRect, "", FloatingBackground);
				Rect labelRect = FloatingBackgroundRect;
				labelRect.x += 12.0f;
				labelRect.width -= 12.0f;
				GUI.Label(labelRect, ObjectPathTimeline.name);

				base.FloatingOnGUI(depth, MoreRect);
			}
		}
		
		protected override void FloatingOnGUI(int depth)
		{
			GUILayout.Box("", FloatingBackground, GUILayout.MaxWidth(FloatingWidth), GUILayout.Height(17.0f));
		
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

			ObjectPathTimeline.Build();
			ObjectPathTimeline.SourcePosition = ObjectPathTimeline.AffectedObject.transform.position;
			ObjectPathTimeline.SourceRotation = ObjectPathTimeline.AffectedObject.transform.rotation;
		}

		public override void OnSceneGUI()
		{
			if(ObjectPathTimeline == null)
				return;

			ObjectPathEditor.OnSceneGUI();
		}
	}
}