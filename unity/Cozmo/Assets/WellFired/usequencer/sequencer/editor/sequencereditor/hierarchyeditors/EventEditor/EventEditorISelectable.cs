using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	public partial class EventEditor : ScriptableObject, ISelectableContainer
	{
		private List<UnityEngine.Object> objectsUnderMouse = new List<UnityEngine.Object>();
		
		private Dictionary<USEventBase, Vector2> sourcePositions = new Dictionary<USEventBase, Vector2>();
		private Dictionary<USEventBase, Vector2> SourcePositions
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
				USEventBase eventBase = selectedObject as USEventBase;
				SourcePositions[eventBase] = cachedEventRenderData.Where(element => element.Event == eventBase).Select(element => element.RenderPosition).First();
			}
		}
		
		public void ProcessDraggingObjects(Vector2 mouseDelta, bool snap, float snapValue)
		{
			USUndoManager.PropertyChange(this, "Drag Selection");
			foreach(var selectedObject in SelectedObjects)
			{
				var eventBase = selectedObject as USEventBase;
				USUndoManager.PropertyChange(eventBase, "Drag Selection");
	
				Vector3 newPosition = SourcePositions[eventBase] + mouseDelta;
				newPosition.x = newPosition.x + XScroll - DisplayArea.x;
				if(snap)
					newPosition.x = Mathf.Round(newPosition.x / snapValue) * snapValue;
				float newTime = ((newPosition.x / DisplayArea.width) * Duration) / XScale;
				eventBase.FireTime = newTime;
			}
		}
	
		public List<UnityEngine.Object> GetSelectableObjectUnderPos(Vector2 mousePosition)
		{
			objectsUnderMouse.Clear();

			mousePosition.y += YScroll;
	
			foreach(var cachedData in cachedEventRenderData)
			{
				Rect slightlyEnlargedRect = cachedData.RenderRect;
				slightlyEnlargedRect.width += 5.0f;
				if(slightlyEnlargedRect.Contains(mousePosition))
				{
					objectsUnderMouse.Add(cachedData.Event);
					return objectsUnderMouse;
				}
			}
	
			return objectsUnderMouse;
		}
		
		public List<UnityEngine.Object> GetSelectableObjectsUnderRect(Rect selectionRect, float yScroll)
		{
			objectsUnderMouse.Clear();
			
			selectionRect.y += YScroll;
			
			foreach(var cachedData in cachedEventRenderData)
			{
				Rect renderRect = cachedData.RenderRect;
				if(USEditorUtility.DoRectsOverlap(renderRect, selectionRect))
					objectsUnderMouse.Add(cachedData.Event);
			}
			
			return objectsUnderMouse;
		}
		
		public void DeleteSelection()
		{
			foreach(var selectedObject in SelectedObjects)
				RemoveEvent(selectedObject as USEventBase);
		}
		
		public void DuplicateSelection()
		{
			USUndoManager.RegisterCompleteObjectUndo(this, "Duplicate");

			foreach(var selectedObject in SelectedObjects)
			{
				var originalEventBase = selectedObject as USEventBase;
				var duplicatedEventBase = GameObject.Instantiate(originalEventBase.gameObject) as GameObject;

				duplicatedEventBase.name = originalEventBase.gameObject.name;
				duplicatedEventBase.transform.parent = originalEventBase.transform.parent;

				var cachedData = ScriptableObject.CreateInstance<EventRenderData>();
				cachedData.Initialize(duplicatedEventBase.GetComponent<USEventBase>());
				cachedEventRenderData.Add(cachedData);

				USUndoManager.RegisterCreatedObjectUndo(duplicatedEventBase, "Duplicate");
				USUndoManager.RegisterCreatedObjectUndo(cachedData, "Duplicate");
			}
		}
	}
}