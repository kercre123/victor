using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	public partial class ObserverEditor : ScriptableObject, ISelectableContainer
	{
		private List<UnityEngine.Object> objectsUnderMouse = new List<UnityEngine.Object>();
		
		private Dictionary<USObserverKeyframe, Vector2> sourcePositions = new Dictionary<USObserverKeyframe, Vector2>();
		private Dictionary<USObserverKeyframe, Vector2> SourcePositions
		{
			get { return sourcePositions; }
			set { sourcePositions = value; }
		}
	
		[SerializeField]
		private List<UnityEngine.Object> selectedObjects;
		public List<UnityEngine.Object> SelectedObjects
		{
			get { return selectedObjects; }
			set { selectedObjects = value; }
		}
		
		public void ResetSelection()
		{
			if(SelectedObjects.Count > 0)
			{
				USUndoManager.PropertyChange(this, "Select None");
	
				USEditorUtility.RemoveFromUnitySelection(SelectedObjects);
				SelectedObjects.Clear();
				SourcePositions.Clear();
			}
		}
	
		public void OnSelectedObjects(List<UnityEngine.Object> selectedObjects)
		{
			USUndoManager.PropertyChange(this, "Select");
			foreach(var selectedObject in selectedObjects)
			{
				if(!SelectedObjects.Contains(selectedObject))
				{
					SelectedObjects.Add(selectedObject);
					var selection = Selection.objects != null ? Selection.objects.ToList() : new List<UnityEngine.Object>();
					selection.Add(selectedObject);
					Selection.objects = selection.ToArray();
				}
			}
		}
	
		public void OnDeSelectedObjects(List<UnityEngine.Object> selectedObjects)
		{
			USUndoManager.PropertyChange(this, "DeSelect");
			foreach(var selectedObject in selectedObjects)
			{
				SelectedObjects.Remove(selectedObject);
				var selection = Selection.objects != null ? Selection.objects.ToList() : new List<UnityEngine.Object>();
				selection.Remove(selectedObject);
				Selection.objects = selection.ToArray();
			}
		} 
		
		public void StartDraggingObjects()
		{
			foreach(var selectedObject in selectedObjects)
			{
				USObserverKeyframe keyframe = selectedObject as USObserverKeyframe;
				SourcePositions[keyframe] = cachedObserverRenderData.Where(element => element.Keyframe == keyframe).Select(element => element.RenderPosition).First();
			}
		}
		
		public void ProcessDraggingObjects(Vector2 mouseDelta, bool snap, float snapValue)
		{
			if(!ObserverTimeline)
				return;
	
			USUndoManager.PropertyChange(this, "Drag Selection");
			foreach(var selectedObject in SelectedObjects)
			{
				var keyframe = selectedObject as USObserverKeyframe;
				USUndoManager.PropertyChange(keyframe, "Drag Selection");
	
				Vector3 newPosition = SourcePositions[keyframe] + mouseDelta;
				newPosition.x = newPosition.x + XScroll - DisplayArea.x;
				if(snap)
					newPosition.x = Mathf.Round(newPosition.x / snapValue) * snapValue;
				float newTime = ((newPosition.x / DisplayArea.width) * Duration) / XScale;
	
				newTime = Mathf.Clamp(newTime, 0.0f, Duration);
	
				keyframe.FireTime = newTime;
			}
			
			if(WellFired.Animation.IsInAnimationMode)
			{
				ObserverTimeline.Process(ObserverTimeline.Sequence.RunningTime, ObserverTimeline.Sequence.PlaybackRate);
			}
		}
		
		public List<UnityEngine.Object> GetSelectableObjectUnderPos(Vector2 mousePosition)
		{
			objectsUnderMouse.Clear();
			
			foreach(var cachedData in cachedObserverRenderData)
			{
				Rect slightlyEnlargedRect = cachedData.RenderRect;
				slightlyEnlargedRect.x -= 5.0f;
				slightlyEnlargedRect.width += 10.0f;
				if(slightlyEnlargedRect.Contains(mousePosition))
					objectsUnderMouse.Add(cachedData.Keyframe);
			}
	
			return objectsUnderMouse;
		}
		
		public List<UnityEngine.Object> GetSelectableObjectsUnderRect(Rect selectionRect, float yScroll)
		{
			objectsUnderMouse.Clear();
			
			foreach(var cachedData in cachedObserverRenderData)
			{
				Rect renderRect = cachedData.RenderRect;
				renderRect.y -= yScroll;
				if(USEditorUtility.DoRectsOverlap(cachedData.RenderRect, selectionRect))
					objectsUnderMouse.Add(cachedData.Keyframe);
			}
			
			return objectsUnderMouse;
		}
		
		public void DeleteSelection()
		{
			foreach(var selectedObject in SelectedObjects)
				RemoveKeyframe(selectedObject as USObserverKeyframe);
		}
		
		public void DuplicateSelection()
		{
			foreach(var selectedObject in SelectedObjects)
			{
				var observerKeyframe = selectedObject as USObserverKeyframe;
				SetCamera(null, observerKeyframe.FireTime, observerKeyframe.TransitionType, observerKeyframe.TransitionDuration, observerKeyframe.KeyframeCamera);
			}
		}
	}
}