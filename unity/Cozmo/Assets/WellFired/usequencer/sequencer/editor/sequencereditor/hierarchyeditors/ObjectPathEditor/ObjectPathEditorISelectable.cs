using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	public partial class ObjectPathEditor : ScriptableObject, ISelectableContainer
	{
		private List<UnityEngine.Object> objectsUnderMouse = new List<UnityEngine.Object>();
		
		private Dictionary<SplineKeyframe, Vector2> sourcePositions = new Dictionary<SplineKeyframe, Vector2>();
		private Dictionary<SplineKeyframe, Vector2> SourcePositions
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
				var keyframe = selectedObject as SplineKeyframe;
				SourcePositions[keyframe] = cachedObjectPathRenderData.Where(element => element.Keyframe == keyframe).Select(element => element.RenderPosition).First();
			}
		}
		
		public void ProcessDraggingObjects(Vector2 mouseDelta, bool snap, float snapValue)
		{
			if(!ObjectPathTimeline)
				return;
			
			USUndoManager.PropertyChange(ObjectPathTimeline, "Drag Selection");
			USUndoManager.PropertyChange(this, "Drag Selection");
			foreach(var selectedObject in SelectedObjects)
			{
				var keyframe = selectedObject as SplineKeyframe;
				
				Vector3 newPosition = SourcePositions[keyframe] + mouseDelta;
				newPosition.x = newPosition.x + XScroll - DisplayArea.x;
				if(snap)
					newPosition.x = Mathf.Round(newPosition.x / snapValue) * snapValue;
				float newTime = ((newPosition.x / DisplayArea.width) * Duration) / XScale;
				
				newTime = Mathf.Clamp(newTime, 0.0f, Duration);

				if(ObjectPathTimeline.ObjectSpline.Nodes[0] == keyframe)
					ObjectPathTimeline.StartTime = newTime;
				if(ObjectPathTimeline.ObjectSpline.Nodes[ObjectPathTimeline.ObjectSpline.Nodes.Count - 1] == keyframe)
					ObjectPathTimeline.EndTime = newTime;
			}
			
			ObjectPathTimeline.Process(ObjectPathTimeline.Sequence.RunningTime, ObjectPathTimeline.Sequence.PlaybackRate);
		}
	
		public List<UnityEngine.Object> GetSelectableObjectUnderPos(Vector2 mousePosition)
		{
			objectsUnderMouse.Clear();
			
			mousePosition.y += YScroll;
			
			foreach(var cachedData in cachedObjectPathRenderData)
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
			
			selectionRect.y += YScroll;
			
			foreach(var cachedData in cachedObjectPathRenderData)
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
	
		}
		
		public void DuplicateSelection()
		{
			
		}
	}
}