using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Linq;
using System.Collections.Generic;

namespace WellFired
{
	[Serializable]
	public class USAnimationTimelineTrackHierarchyItem : IUSHierarchyItem 
	{
		public override bool ShouldDisplayMoreButton
		{
			get { return true; }
			set { ; }
		}

		private List<ISelectableContainer> ISelectableContainers;

		[SerializeField]
		private AnimationEditor animationEditor;
		private AnimationEditor AnimationEditor
		{
			get { return animationEditor; }
			set { animationEditor = value; }
		}

		[SerializeField]
		private AnimationTrack animationTrack;
		public AnimationTrack AnimationTrack
		{
			get { return animationTrack; }
			set { animationTrack = value; }
		}
	
		[SerializeField]
		private USTimelineAnimation animationTimeline;
		public USTimelineAnimation AnimationTimeline
		{
			get { return animationTimeline; }
			set { animationTimeline = value; }
		}
		
		[SerializeField]
		private USAnimationTimelineHierarchyItem animationTimelineHierarchy;
		public USAnimationTimelineHierarchyItem AnimationTimelineHierarchy
		{
			get { return animationTimelineHierarchy; }
			set { animationTimelineHierarchy = value; }
		}
		
		public override List<ISelectableContainer> GetISelectableContainers()
		{
			return ISelectableContainers;
		}

		public override int SortOrder 
		{
			get 
			{
				return AnimationTrack.Layer;
			}
			set 
			{
				;
			}
		}

		public override bool IsISelectable()
		{
			return true;
		}
		
		public override void OnEnable()
		{
			base.OnEnable();
			
			if(AnimationEditor == null)
				AnimationEditor = ScriptableObject.CreateInstance<AnimationEditor>();
			
			ISelectableContainers = new List<ISelectableContainer>() { AnimationEditor, };
		}
	
		public override void DoGUI(int depth)
		{
			using(new WellFired.Shared.GUIBeginHorizontal())
			{
				AnimationEditor.YScroll = YScroll;

				FloatingOnGUI(depth);
				ContentOnGUI(depth);
			}
		}
		
		protected override void FloatingOnGUI(int depth)
		{
			GUILayout.Box("", FloatingBackground, GUILayout.Width(FloatingWidth), GUILayout.Height(17.0f));
			
			if(Event.current.type == EventType.Repaint)
			{
				Rect lastRect = GUILayoutUtility.GetLastRect();
				lastRect.x += GetXOffsetForDepth(depth);
				lastRect.width -= GetXOffsetForDepth(depth);
				FloatingBackgroundRect = lastRect;
			}
	
			var layerName = string.Format("Track (Layer : {0})", MecanimAnimationUtility.LayerIndexToName(AnimationTimeline.AffectedObject.gameObject, AnimationTrack.Layer));
			GUI.Label(FloatingBackgroundRect, layerName);
			
			Rect addRect = FloatingBackgroundRect;
			addRect.x = addRect.x + addRect.width - 22.0f - 1.0f;
			addRect.width = 22.0f;
			if(GUI.Button(addRect, "-"))
				RemoveAnimationTrack();
		}
		
		private void ContentOnGUI(int depth)
		{
			animationEditor.XScale = XScale;
			animationEditor.XScroll = XScroll;

			AnimationEditor.OnGUI();
		}

		private void RemoveAnimationTrack()
		{
			USUndoManager.RegisterCompleteObjectUndo(AnimationTimeline, "Remove Track");
			AnimationTimeline.RemoveTrack(AnimationTrack);

			AnimationTimelineHierarchy.RemoveAnimationTrack(AnimationTrack);
		}

		public override void Initialize (USTimelineBase timeline)
		{
			animationEditor.AnimationTimeline = AnimationTimeline;
			animationEditor.AnimationTrack = AnimationTrack;
			AnimationEditor.Build();
		}
	}
}