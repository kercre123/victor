using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;

namespace WellFired
{
	public partial class AnimationCurveEditor : ScriptableObject, ISelectableContainer
	{
		private Dictionary<USInternalKeyframe, Vector2> sourcePositions = new Dictionary<USInternalKeyframe, Vector2>();
		private Dictionary<USInternalKeyframe, Vector2> SourcePositions
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
				SourcePositions[selectedObject as USInternalKeyframe] = cachedKeyframePositions[selectedObject as USInternalKeyframe].RenderPosition;
		}
		
		public void ProcessDraggingObjects(Vector2 mouseDelta, bool snap, float snapValue)
		{
			USUndoManager.PropertyChange(this, "Drag Selection");
	
			AreCurvesDirty = true;

			mouseDelta.x /= XScale;
	
			foreach(var selectedObject in SelectedObjects)
			{
				var keyframe = selectedObject as USInternalKeyframe;
				USUndoManager.PropertyChange(keyframe, "Drag Selection");
				USUndoManager.PropertyChange(keyframe.curve, "Drag Selection");

				Vector3 newPosition;
				if(snap)
				{
					Vector2 sourcePosition = SourcePositions[keyframe];
					sourcePosition.x = Mathf.Round(sourcePosition.x / snapValue) * snapValue;
					mouseDelta.x = Mathf.Round(mouseDelta.x / snapValue) * snapValue;
					newPosition = sourcePosition + mouseDelta;
				}
				else
					newPosition = SourcePositions[keyframe] + mouseDelta;
		
				float newTime = (newPosition.x / DisplayArea.width) * Duration;
				newTime = Mathf.Clamp(newTime, 0.0f, Duration);
				keyframe.Time = newTime;
	
				float mouseDeltaY = -1.0f * (newPosition.y - DisplayArea.height);
				float keyframeRatio = mouseDeltaY / DisplayArea.height;

				bool modifyingOnlyXAxis = false;
				if(Event.current.alt)
					modifyingOnlyXAxis = true;

				if(!modifyingOnlyXAxis)
					keyframe.Value = (keyframeRatio * (MaxDisplayY - MinDisplayY)) + MinDisplayY;
	
				var curve = keyframe.curve;
				int keyframeIndex = curve.Keys.ToList().FindIndex(element => element == keyframe);
				if(keyframeIndex > 0)
					RebuildCachedKeyframeInformation(curve.Keys[keyframeIndex - 1]);
				if(keyframeIndex > 1)
					RebuildCachedKeyframeInformation(curve.Keys[keyframeIndex - 2]);
				if(keyframeIndex < curve.Keys.Count() - 1)
					RebuildCachedKeyframeInformation(curve.Keys[keyframeIndex + 1]);
				if(keyframeIndex < curve.Keys.Count() - 2)
					RebuildCachedKeyframeInformation(curve.Keys[keyframeIndex + 2]);
				RebuildCachedKeyframeInformation(keyframe);
			}
		}
	
		public List<UnityEngine.Object> GetSelectableObjectUnderPos(Vector2 mousePosition)
		{
			var selectableObjects = new List<UnityEngine.Object>();
			var selectableRenderData = new Dictionary<UnityEngine.Object, AnimationKeyframeRenderData>();
			foreach(var kvp in cachedKeyframePositions)
			{
				if(kvp.Value.IsMouseOverKeyframe)
					selectableRenderData[kvp.Key] = kvp.Value;
			}
	
			foreach(var kvp in selectableRenderData)
			{
				bool canAdd = true;
				for(int selectedObjectIndex = selectableObjects.Count - 1; selectedObjectIndex >= 0; selectedObjectIndex--)
				{
					var selectedObject = selectableObjects[selectedObjectIndex];
					if(USEditorUtility.DoRectsOverlap(kvp.Value.RenderRect, selectableRenderData[selectedObject].RenderRect))
					{
						if(Vector2.Distance(kvp.Value.RenderPosition, mousePosition) < Vector2.Distance(selectableRenderData[selectedObject].RenderPosition, mousePosition))
							selectableObjects.Remove(selectedObject);
						else
							canAdd = false;
					}
				}
	
				if(canAdd)
					selectableObjects.Add(kvp.Key);
			}
	
			return selectableObjects;
		}
		
		public List<UnityEngine.Object> GetSelectableObjectsUnderRect(Rect selectionRect, float yScroll)
		{
			var selectableObjects = new List<UnityEngine.Object>();
			foreach(var kvp in cachedKeyframePositions)
			{
				Vector2 FinalRenderPosition = kvp.Value.RenderPosition;
				FinalRenderPosition.x = (FinalRenderPosition.x * XScale) - XScroll;
				FinalRenderPosition.y -= yScroll;
				Rect keyframeRect = new Rect(FinalRenderPosition.x - 6, FinalRenderPosition.y - 6, 16, 16);
				keyframeRect.x += DisplayArea.x;
				keyframeRect.y += DisplayArea.y;
	
				if(USEditorUtility.DoRectsOverlap(keyframeRect, selectionRect))
					selectableObjects.Add(kvp.Key);
			}
			
			return selectableObjects;
		}
	
		public void DeleteSelection()
		{
			foreach(var selectedObject in SelectedObjects)
				RemoveKeyframe(selectedObject as USInternalKeyframe);
			
			RebuildCurvesOnNextGUI = true;
			AreCurvesDirty = true;
			EditorWindow.Repaint();
		}
		
		public void DuplicateSelection()
		{
			USUndoManager.RegisterCompleteObjectUndo(this, "Duplicate");

			foreach(var selectedObject in SelectedObjects)
			{
				var keyframe = selectedObject as USInternalKeyframe;
				var newKeyframe = UnityEngine.Object.Instantiate(keyframe) as USInternalKeyframe;
				newKeyframe.name = keyframe.name;
				AddKeyframe(newKeyframe, newKeyframe.curve);
				
				USUndoManager.RegisterCreatedObjectUndo(newKeyframe, "Duplicate");
			}
		}
	}
}