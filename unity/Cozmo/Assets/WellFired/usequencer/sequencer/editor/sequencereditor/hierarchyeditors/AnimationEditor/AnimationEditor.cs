using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	[Serializable]
	public partial class AnimationEditor : ScriptableObject, ISelectableContainer
	{
		[SerializeField]
		private List<AnimationClipRenderData> cachedClipRenderData;
		
		[SerializeField]
		public USTimelineAnimation AnimationTimeline
		{
			get;
			set;
		}
		
		[SerializeField]
		public AnimationTrack AnimationTrack
		{
			get;
			set;
		}
		
		[SerializeField]
		public Rect DisplayArea
		{
			get;
			private set;
		}
		
		[SerializeField]
		private Rect HeaderArea
		{
			get;
			set;
		}
		
		public float XScale
		{
			get;
			set;
		}
		
		public float XScroll
		{
			get;
			set;
		}
		
		private GUIStyle TimelineBackground
		{
			get
			{
				return USEditorUtility.USeqSkin.GetStyle("TimelinePaneBackground");
			}
		}
	
		private void OnEnable()
		{
			hideFlags = HideFlags.HideAndDontSave;
	
			if(SelectedObjects == null)
				SelectedObjects = new List<UnityEngine.Object>();
			
			if(cachedClipRenderData == null)
				cachedClipRenderData = new List<AnimationClipRenderData>();
		}
		
		private void OnDestroy()
		{

		}

		public void InitializeWithClips(IEnumerable<AnimationClipData> clips)
		{
			foreach(var animationClipData in clips)
			{
				var cachedData = ScriptableObject.CreateInstance<AnimationClipRenderData>();
				animationClipData.Track = AnimationTrack;
				cachedData.animationClipData = animationClipData;
				cachedClipRenderData.Add(cachedData);
			}
		}
		
		public void OnGUI()
		{
			GUILayout.Box("", TimelineBackground, GUILayout.MaxHeight(17.0f), GUILayout.ExpandWidth(true));
			if(Event.current.type == EventType.Repaint)
				DisplayArea = GUILayoutUtility.GetLastRect();

			foreach(var cachedData in cachedClipRenderData)
			{
				var animationClipData = cachedData.animationClipData;
				
				using(new WellFired.Shared.GUIChangeColor(SelectedObjects.Contains(animationClipData) ? Color.yellow : GUI.color))
				{
					var startX = ConvertTimeToXPos(animationClipData.StartTime);
					var endX = ConvertTimeToXPos(animationClipData.StartTime + animationClipData.PlaybackDuration);
					var transitionX = ConvertTimeToXPos(animationClipData.StartTime + animationClipData.TransitionDuration);
					var handleWidth = 2.0f;
					
					cachedData.renderPosition.x = startX;
					cachedData.renderPosition.y = DisplayArea.y;

					cachedData.renderRect.x = startX;
					cachedData.renderRect.y = DisplayArea.y;
					cachedData.renderRect.width = endX - startX;
					cachedData.renderRect.height = DisplayArea.height;
					
					cachedData.transitionRect.x = startX;
					cachedData.transitionRect.y = DisplayArea.y;
					cachedData.transitionRect.width = transitionX - startX;
					cachedData.transitionRect.height = DisplayArea.height;

					cachedData.leftHandle.x = startX;
					cachedData.leftHandle.y = DisplayArea.y;
					cachedData.leftHandle.width = handleWidth * 2.0f;
					cachedData.leftHandle.height = DisplayArea.height;
					
					cachedData.rightHandle.x = endX - (handleWidth * 2.0f);
					cachedData.rightHandle.y = DisplayArea.y;
					cachedData.rightHandle.width = handleWidth * 2.0f;
					cachedData.rightHandle.height = DisplayArea.height;

					GUI.Box(cachedData.renderRect, "");

					if(animationClipData.CrossFade)
						GUI.Box(cachedData.transitionRect, "");

					GUI.Box(cachedData.leftHandle, "");
					GUI.Box(cachedData.rightHandle, "");

					cachedData.labelRect = cachedData.renderRect;
					cachedData.labelRect.width = DisplayArea.width;
					
					if(animationClipData.CrossFade)
						cachedData.labelRect.x = cachedData.labelRect.x + (transitionX - startX);
					else
						cachedData.labelRect.x += 4.0f; // Nudge this along a bit so it isn't flush with the side.

					GUI.Label(cachedData.labelRect, animationClipData.FriendlyName);
				}
			}

			HandleEvent();
		}
		
		private void HandleEvent()
		{
			bool isContext = Event.current.type == EventType.MouseDown && Event.current.button == 1;
			
			if(!isContext)
				return;
			
			GenericMenu contextMenu = new GenericMenu();

			if(DisplayArea.Contains(Event.current.mousePosition))
			{
				string baseAdd = "Add State/{0}";
				List<string> allStates = MecanimAnimationUtility.GetAllStateNames(AnimationTimeline.AffectedObject.gameObject, AnimationTrack.Layer);
				float newTime = (((Event.current.mousePosition.x + XScroll - DisplayArea.x) / DisplayArea.width) * AnimationTimeline.Sequence.Duration) / XScale;

				foreach(var state in allStates)
					contextMenu.AddItem(
						new GUIContent(string.Format(baseAdd, state)), 
						false, 
						(obj) => AddNewState(((float)((object[])obj)[0]), ((string)((object[])obj)[1])),
						new object[] { newTime, state } // This object is passed through to the annonymous function that is passed through as the previous argument.
					);
			}
			
			if(DisplayArea.Contains(Event.current.mousePosition))
			{
				Event.current.Use();
				contextMenu.ShowAsContext();
			}
		}

		private void AddNewState(float time, string stateName)
		{
			var clipData = ScriptableObject.CreateInstance<AnimationClipData>();
			clipData.TargetObject = AnimationTimeline.AffectedObject.gameObject;
			clipData.StartTime = time;
			clipData.StateName = stateName;
			clipData.StateDuration = MecanimAnimationUtility.GetStateDuration(stateName, clipData.TargetObject);
			clipData.PlaybackDuration = MecanimAnimationUtility.GetStateDuration(stateName, clipData.TargetObject);
			clipData.Track = AnimationTrack;
			USUndoManager.RegisterCreatedObjectUndo(clipData, "Add New Clip");
			
			var cachedData = ScriptableObject.CreateInstance<AnimationClipRenderData>();
			cachedData.animationClipData = clipData;
			USUndoManager.RegisterCreatedObjectUndo(clipData, "Add New Clip");

			USUndoManager.RegisterCompleteObjectUndo(AnimationTrack, "Add New Clip");
			AnimationTrack.AddClip(clipData);
			
			USUndoManager.RegisterCompleteObjectUndo(this, "Add New Clip");
			cachedClipRenderData.Add(cachedData);

			OnSelectedObjects(new List<UnityEngine.Object>() { clipData });
		}
		
		protected float ConvertTimeToXPos(float time)
		{
			float xPos = DisplayArea.width * (time / AnimationTimeline.Sequence.Duration);
			return DisplayArea.x + ((xPos * XScale) - XScroll);
		}

		private void RemoveClip(AnimationClipData clip)
		{
			var undoName = "RemoveClip";
			var animationTracks = AnimationTimeline.AnimationTracks;
			var cachedClipDataToRemove = cachedClipRenderData.Where(cachedClipData => cachedClipData.animationClipData == clip).ToList();
			var tracksToRemoveClipFrom = animationTracks.Where(track => track.TrackClips.Contains(clip));
			
			USUndoManager.RegisterCompleteObjectUndo(this, undoName);

			foreach(var cachedClipData in cachedClipDataToRemove)
			{
				cachedClipRenderData.Remove(cachedClipData);
				USUndoManager.DestroyImmediate(cachedClipData);
			}
			
			foreach(var trackToRemoveClipFrom in tracksToRemoveClipFrom)
			{
				USUndoManager.RegisterCompleteObjectUndo(trackToRemoveClipFrom, undoName);
				trackToRemoveClipFrom.RemoveClip(clip);
				USUndoManager.DestroyImmediate(clip);
			}
		}

		public void Build()
		{
			foreach(var clipData in AnimationTrack.TrackClips)
			{
				var cachedData = ScriptableObject.CreateInstance<AnimationClipRenderData>();
				cachedData.animationClipData = clipData;
				clipData.Track = AnimationTrack;
				cachedClipRenderData.Add(cachedData);
			}
		}
	}
}