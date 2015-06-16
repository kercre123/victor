using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	public partial class AnimationEditor : ScriptableObject, ISelectableContainer
	{
		private List<UnityEngine.Object> objectsUnderMouse = new List<UnityEngine.Object>();
		
		private Dictionary<AnimationClipData, Vector2> sourcePositions = new Dictionary<AnimationClipData, Vector2>();
		private Dictionary<AnimationClipData, Vector2> SourcePositions
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
		
		public float YScroll
		{
			get;
			set;
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
				var clipData = selectedObject as AnimationClipData;
				SourcePositions[clipData] = cachedClipRenderData.Where(element => element.animationClipData == clipData).Select(element => element.renderPosition).First();
			}
		}
		
		public void ProcessDraggingObjects(Vector2 mouseDelta, bool snap, float snapValue)
		{
			USUndoManager.PropertyChange(this, "Drag Selection");
			foreach(var selectedObject in SelectedObjects)
			{
				var animationClipData = selectedObject as AnimationClipData;
				USUndoManager.PropertyChange(animationClipData, "Drag Selection");
				
				Vector3 newPosition = SourcePositions[animationClipData] + mouseDelta;
				newPosition.x = newPosition.x + XScroll - DisplayArea.x;
				if(snap)
					newPosition.x = Mathf.Round(newPosition.x / snapValue) * snapValue;
				float newTime = ((newPosition.x / DisplayArea.width) * AnimationTimeline.Sequence.Duration) / XScale;
				newTime = Mathf.Clamp(newTime, 0.0f, AnimationTimeline.Sequence.Duration);
				animationClipData.StartTime = newTime;
			}
		}
	
		public List<UnityEngine.Object> GetSelectableObjectUnderPos(Vector2 mousePosition)
		{
			objectsUnderMouse.Clear();
			
			mousePosition.y += YScroll;
			
			foreach(var cachedData in cachedClipRenderData)
			{
				Rect slightlyEnlargedRect = cachedData.renderRect;
				if(slightlyEnlargedRect.Contains(mousePosition))
					objectsUnderMouse.Add(cachedData.animationClipData);
			}
			
			return objectsUnderMouse;
		}
		
		public List<UnityEngine.Object> GetSelectableObjectsUnderRect(Rect selectionRect, float yScroll)
		{
			objectsUnderMouse.Clear();
			
			selectionRect.y += YScroll;
			
			foreach(var cachedData in cachedClipRenderData)
			{
				if(USEditorUtility.DoRectsOverlap(cachedData.renderRect, selectionRect))
					objectsUnderMouse.Add(cachedData.animationClipData);
			}
			
			return objectsUnderMouse;
		}
		
		public void DeleteSelection()
		{
			USUndoManager.RegisterCompleteObjectUndo(this, "RemoveClip");

			foreach(var selectedObject in SelectedObjects)
				RemoveClip(selectedObject as AnimationClipData);

			SelectedObjects.Clear();
		}

		public void DuplicateSelection()
		{
//			throw new NotImplementedException();
		}
	}
}