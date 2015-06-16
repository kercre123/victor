using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	[Serializable]
	[USCustomTimelineHierarchyItem(typeof(USTimelineObserver), "Observer Timeline", false)]
	public class USObserverTimelineHierarchyItem : IUSTimelineHierarchyItem 
	{
		public override bool ShouldDisplayMoreButton
		{
			get { return true; }
			set { ; }
		}

		private List<ISelectableContainer> ISelectableContainers;
	
		[SerializeField]
		public USTimelineObserver ObserverTimeline
		{
			get { return BaseTimeline as USTimelineObserver; }
			set { BaseTimeline = value; }
		}
		
		[SerializeField]
		private ObserverEditor ObserverEditor
		{
			get;
			set;
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
			
			if(ObserverEditor == null)
				ObserverEditor = ScriptableObject.CreateInstance<ObserverEditor>();
			
			ISelectableContainers = new List<ISelectableContainer>() { ObserverEditor, };
		}
		
		public override void DoGUI(int depth)
		{
			using(new WellFired.Shared.GUIBeginHorizontal())
			{
				GUILayout.Box("", FloatingBackground, GUILayout.MaxWidth(FloatingWidth), GUILayout.Height(17.0f));
				if(Event.current.type == EventType.Repaint)
					FloatingBackgroundRect = GUILayoutUtility.GetLastRect();
	
				if(ObserverEditor.ObserverTimeline != ObserverTimeline)
				{
					EditorWindow.Repaint();
					ObserverEditor.ObserverTimeline = ObserverTimeline;
					ObserverEditor.InitializeWithKeyframes(ObserverTimeline.observerKeyframes);
				}
				ObserverEditor.XScale = XScale;
				ObserverEditor.XScroll = XScroll;
				ObserverEditor.YScroll = YScroll;
				ObserverEditor.Duration = ObserverTimeline.Sequence.Duration;
				ObserverEditor.OnGUI();
	
				GUI.Box(FloatingBackgroundRect, "", FloatingBackground);

				var labelRect = FloatingBackgroundRect;
				labelRect.x += GetXOffsetForDepth(depth + 1);
				GUI.Label(labelRect, "Camera Cuts");
			}
		}
	}
}