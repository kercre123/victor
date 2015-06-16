using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	[Serializable]
	[USCustomTimelineHierarchyItem(typeof(USTimelineAnimation), "Animation Timeline (Beta)")]
	public class USAnimationTimelineHierarchyItem : IUSTimelineHierarchyItem
	{
		public override bool ShouldDisplayMoreButton
		{
			get { return true; }
			set { ; }
		}

		[SerializeField]
		public USTimelineAnimation AnimationTimeline
		{
			get { return BaseTimeline as USTimelineAnimation; }
			set { BaseTimeline = value; }
		}

		[SerializeField]
		public float MaxHeight
		{
			get;
			private set;
		}
		
		[SerializeField]
		private USHierarchy hierarchy;
		private USHierarchy USHierarchy
		{
			get { return hierarchy; }
			set { hierarchy = value; }
		}
		
		public override void OnEnable()
		{
			base.OnEnable();

			if(USHierarchy == null)
				USHierarchy = ScriptableObject.CreateInstance<USHierarchy>();
		}
		
		public override List<ISelectableContainer> GetISelectableContainers()
		{
			return USHierarchy.ISelectableContainers();
		}
		
		public override void DoGUI(int depth)
		{
			CheckForOverlap();

			using(new WellFired.Shared.GUIBeginHorizontal())
			{
				using(new WellFired.Shared.GUIBeginVertical(GUILayout.MaxWidth(FloatingWidth)))
					FloatingOnGUI(depth);
				
				if(Event.current.type == EventType.Repaint)
				{
					float newMaxHeight = GUILayoutUtility.GetLastRect().height;
					
					if(MaxHeight != newMaxHeight)
					{
						EditorWindow.Repaint();
						MaxHeight = newMaxHeight;
					}
				}
				
				ContentOnGUI();
			}

			if(IsExpanded)
			{
				USHierarchy.EditorWindow = EditorWindow;
				USHierarchy.FloatingWidth = FloatingWidth;
				USHierarchy.XScale = XScale;
				USHierarchy.ScrollPos = new Vector2(XScroll, YScroll);
				USHierarchy.CheckConsistency();
				USHierarchy.DoGUI(depth + 1);
			}
		}
		
		protected override void FloatingOnGUI(int depth)
		{
			GUILayout.Box("", FloatingBackground, GUILayout.MaxWidth(FloatingWidth), GUILayout.Height(17.0f));
			
			if(Event.current.type == EventType.Repaint)
			{
				Rect lastRect = GUILayoutUtility.GetLastRect();
				lastRect.x += GetXOffsetForDepth(depth);
				lastRect.width -= GetXOffsetForDepth(depth);
				if(FloatingBackgroundRect != lastRect)
				{
					EditorWindow.Repaint();
					FloatingBackgroundRect = lastRect;
				}
			}

			bool wasLabelPressed = false;
			bool newExpandedState = USEditor.FoldoutLabel(FloatingBackgroundRect, IsExpanded, AnimationTimeline.name, out wasLabelPressed);
			if(newExpandedState != IsExpanded)
			{
				USUndoManager.PropertyChange(this, "Foldout");
				IsExpanded = newExpandedState;
				EditorWindow.Repaint();
			}

			base.FloatingOnGUI(depth);
		}
		
		public override void AddContextItems(GenericMenu contextMenu)
		{
			var animator = AnimationTimeline.AffectedObject.GetComponent<Animator>();
			if(!animator || !animator.runtimeAnimatorController)
			{
				return;
			}

			var animationLayers = MecanimAnimationUtility.GetAllLayerNames(AnimationTimeline.AffectedObject.gameObject);
			foreach(var animationLayer in animationLayers)
				contextMenu.AddItem(new GUIContent(string.Format("Add New Track/{0}", animationLayer)), 
				             false, 
				             (layer) => AddAnimationTrack((string)layer),
				             animationLayer);
		}

		private void ContentOnGUI()
		{
			GUILayout.Box("", FloatingBackground, GUILayout.ExpandWidth(true), GUILayout.Height(17.0f));
		
			if(Event.current.type == EventType.Repaint)
				ContentBackgroundRect = GUILayoutUtility.GetLastRect();

			var animator = AnimationTimeline.AffectedObject.GetComponent<Animator>();
			if(!animator)
			{
				GUI.Label(ContentBackgroundRect, string.Format("GameObject {0} needs to have an Animator component attached.", AnimationTimeline.AffectedObject.name));
				return;
			}

			if(!animator.runtimeAnimatorController)
			{
				GUI.Label(ContentBackgroundRect, string.Format("GameObject {0} needs to have an Animator controller assigned in the Animator component.", AnimationTimeline.AffectedObject.name));
				return;
			}
		}
	
		public override void Initialize(USTimelineBase timeline)
		{
			base.Initialize(timeline);
			
			if(AnimationTimeline.AffectedObject == null)
				return;
			
			var animationTracks = AnimationTimeline.AnimationTracks;

			// Removal
			if(animationTracks.Count < USHierarchy.RootItems.Count)
			{
				var notInBoth = USHierarchy.RootItems.Where((hierarchyItem) => !animationTracks.Contains((hierarchyItem as USAnimationTimelineTrackHierarchyItem).AnimationTrack));
				foreach(var missingComponent in notInBoth)
					USHierarchy.RootItems.Remove(missingComponent as IUSHierarchyItem);
			}
			
			// Addition
			if(animationTracks.Count > USHierarchy.RootItems.Count)
			{
				var notInBoth = animationTracks.Where((track) => !USHierarchy.RootItems.Any((item) => ((item is USAnimationTimelineTrackHierarchyItem) && (item as USAnimationTimelineTrackHierarchyItem).AnimationTrack == track)));
				
				foreach(var extraTrack in notInBoth)
				{
					USAnimationTimelineTrackHierarchyItem hierarchyItem = ScriptableObject.CreateInstance(typeof(USAnimationTimelineTrackHierarchyItem)) as USAnimationTimelineTrackHierarchyItem;
					hierarchyItem.AnimationTrack = extraTrack;
					hierarchyItem.AnimationTimelineHierarchy = this;
					hierarchyItem.AnimationTimeline = AnimationTimeline;
					hierarchyItem.Initialize(AnimationTimeline);
					USHierarchy.RootItems.Add(hierarchyItem as IUSHierarchyItem);
				}
			}
		}

		public void AddAnimationTrack(string layer)
		{
			var track = ScriptableObject.CreateInstance<AnimationTrack>();
			USUndoManager.RegisterCreatedObjectUndo(track, "Add New Track");

			track.Layer = MecanimAnimationUtility.LayerNameToIndex(AnimationTimeline.AffectedObject.gameObject, layer);
			
			USUndoManager.RegisterCompleteObjectUndo(AnimationTimeline, "Add New Track");
			AnimationTimeline.AddTrack(track);
			
			USAnimationTimelineTrackHierarchyItem hierarchyItem = ScriptableObject.CreateInstance(typeof(USAnimationTimelineTrackHierarchyItem)) as USAnimationTimelineTrackHierarchyItem;
			USUndoManager.RegisterCreatedObjectUndo(hierarchyItem, "Add New Track");
			hierarchyItem.AnimationTrack = track;
			hierarchyItem.AnimationTimelineHierarchy = this;
			hierarchyItem.AnimationTimeline = AnimationTimeline;
			hierarchyItem.Initialize(AnimationTimeline);

			USUndoManager.RegisterCompleteObjectUndo(USHierarchy, "Add New Track");
			USHierarchy.RootItems.Add(hierarchyItem as IUSHierarchyItem);

			if(AnimationTimeline.AnimationTracks.Count == 1)
				IsExpanded = true;
		}

		public void RemoveAnimationTrack(AnimationTrack track)
		{
			USUndoManager.RegisterCompleteObjectUndo(AnimationTimeline, "Add New Track");
			AnimationTimeline.RemoveTrack(track);

			USUndoManager.RegisterCompleteObjectUndo(USHierarchy, "Add New Track");
			USHierarchy.RootItems.Remove(USHierarchy.RootItems.Where(item => ((USAnimationTimelineTrackHierarchyItem)item).AnimationTrack == track).First());

			USUndoManager.DestroyImmediate(track);
		}

		private void CheckForOverlap()
		{
			var allClips = AnimationTimeline.AnimationTracks.SelectMany(track => track.TrackClips);
			var allDirtyClips = allClips.Where(clip => clip.Dirty);
			var anyDirty = allDirtyClips.Count() > 0;

			if(!anyDirty)
				return;

			foreach(var clip in allClips)
				clip.CrossFade = false;

			foreach(var clip in allClips)
			{
				var min = clip.StartTime;
				var max = clip.EndTime;

				// Get all the clips that the start of this clip overlaps
				var overlappedClips = allClips.Where(evaluationClip => evaluationClip != clip && (min < evaluationClip.EndTime && min > evaluationClip.StartTime) && max > evaluationClip.EndTime);
				foreach(var overlapped in overlappedClips)
				{
					var overlappingTime = overlapped.EndTime - clip.StartTime;

					clip.TransitionDuration = overlappingTime;
					clip.CrossFade = true;
				}
			}

			foreach(var clip in allDirtyClips)
				clip.Dirty = false;
			
			AnimationTimeline.Process(AnimationTimeline.Sequence.RunningTime, AnimationTimeline.Sequence.PlaybackRate);
		}
	}
}