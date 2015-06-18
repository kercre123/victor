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
		private void HandleEvent()
		{
			bool isContext = Event.current.type == EventType.MouseDown && Event.current.button == 1;
			
			if(!isContext)
				return;
	
			GenericMenu contextMenu = new GenericMenu();
			
			if(SelectedObjects.Count == 0 || GetSelectableObjectUnderPos(Event.current.mousePosition).Count == 0)
			{
				float newTime = ((Event.current.mousePosition.x + XScroll) / (DisplayArea.width * XScale)) * Duration;
				contextMenu.AddItem(new GUIContent("Add Key"), false, () => { AddKeyframeAtTime(newTime); });
			}
	
			if(SelectedObjects.Count != 0)
			{
				USInternalKeyframe keyframe = SelectedObjects[0] as USInternalKeyframe;
	
				contextMenu.AddItem(new GUIContent("Remove Key"), false, 					() => { SelectedObjects.ForEach( (selectedKeyframe) => RemoveKeyframe(selectedKeyframe as USInternalKeyframe) ); RebuildCachedCurveInformation(); AreCurvesDirty = true; });
				contextMenu.AddSeparator("");
	
				contextMenu.AddItem(new GUIContent("Broken"),	keyframe.BrokenTangents, 	() => { SelectedObjects.ForEach( (selectedKeyframe) => BreakKeyframeTangents(selectedKeyframe as USInternalKeyframe, keyframe.BrokenTangents) ); RebuildCachedCurveInformation(); AreCurvesDirty = true; });
					
				contextMenu.AddItem(new GUIContent("Smooth"), 	false, 						() => { SelectedObjects.ForEach( (selectedKeyframe) => SmoothKeyframe(selectedKeyframe as USInternalKeyframe) ); RebuildCachedCurveInformation(); AreCurvesDirty = true; });
				contextMenu.AddItem(new GUIContent("Flatten"), 	false, 						() => { SelectedObjects.ForEach( (selectedKeyframe) => FlattenKeyframe(selectedKeyframe as USInternalKeyframe) ); RebuildCachedCurveInformation(); AreCurvesDirty = true; });
						
				contextMenu.AddItem(new GUIContent("Right Tangent/Linear"), 	false, 		() => { SelectedObjects.ForEach( (selectedKeyframe) => RightKeyframeTangentLinear(selectedKeyframe as USInternalKeyframe) ); RebuildCachedCurveInformation(); AreCurvesDirty = true; });
				contextMenu.AddItem(new GUIContent("Right Tangent/Constant"), 	false, 		() => { SelectedObjects.ForEach( (selectedKeyframe) => RightKeyframeTangentConstant(selectedKeyframe as USInternalKeyframe) ); RebuildCachedCurveInformation(); AreCurvesDirty = true; });
				contextMenu.AddItem(new GUIContent("Left Tangent/Linear"), 		false, 		() => { SelectedObjects.ForEach( (selectedKeyframe) => LeftKeyframeTangentLinear(selectedKeyframe as USInternalKeyframe) ); RebuildCachedCurveInformation(); AreCurvesDirty = true; });
				contextMenu.AddItem(new GUIContent("Left Tangent/Constant"), 	false, 		() => { SelectedObjects.ForEach( (selectedKeyframe) => LeftKeyframeTangentConstant(selectedKeyframe as USInternalKeyframe) ); RebuildCachedCurveInformation(); AreCurvesDirty = true; });
			}
	
			if(contextMenu.GetItemCount() > 0)
			{
				Event.current.Use();
				contextMenu.ShowAsContext();
			}
		}
	
		public void BreakKeyframeTangents(USInternalKeyframe keyframe, bool currentState)
		{
			USUndoManager.PropertyChange(keyframe, "Break Keyframe Tangent");
			keyframe.BrokenTangents = !currentState;
		}
		
		public void RightKeyframeTangentLinear(USInternalKeyframe keyframe)
		{
			USUndoManager.PropertyChange(keyframe, "Right Tangent Linear");
			keyframe.RightTangentLinear();
		}
		
		public void RightKeyframeTangentConstant(USInternalKeyframe keyframe)
		{
			USUndoManager.PropertyChange(keyframe, "Right Tangent Constant");
			keyframe.RightTangentConstant();
		}
		
		public void LeftKeyframeTangentLinear(USInternalKeyframe keyframe)
		{
			USUndoManager.PropertyChange(keyframe, "Left Tangent Linear");
			keyframe.LeftTangentLinear();
		}
		
		public void LeftKeyframeTangentConstant(USInternalKeyframe keyframe)
		{
			USUndoManager.PropertyChange(keyframe, "Left Tangent Constant");
			keyframe.LeftTangentConstant();
		}
		
		public void SmoothKeyframe(USInternalKeyframe keyframe)
		{
			USUndoManager.PropertyChange(keyframe, "Smooth Keyframe");
			keyframe.Smooth();
		}
		
		public void FlattenKeyframe(USInternalKeyframe keyframe)
		{
			USUndoManager.PropertyChange(keyframe, "Flatten Keyframe");
			keyframe.Flatten();
		}
	
		public void RemoveKeyframe(USInternalKeyframe keyframe)
		{
			USUndoManager.PropertyChange(keyframe.curve, "Remove Keyframe");
			keyframe.curve.RemoveKeyframe(keyframe);
		}
		
		public void AddKeyframeAtTime(Modification modification)
		{
			AddKeyframe(modification.curve, modification.newTime, modification.newValue);

			CalculateBounds();
			RebuildCachedCurveInformation(); 
			AreCurvesDirty = true;
		}
		
		public void AddKeyframeAtTime(float time)
		{
			foreach(var curve in Curves)
			{
				float value = curve.Evaluate(time);

				USInternalKeyframe newKeyframe = AddKeyframe(curve, time, value);
				
				if(AutoTangentMode == CurveAutoTangentModes.Smooth)
					newKeyframe.Smooth();
				else if(AutoTangentMode == CurveAutoTangentModes.Flatten)
					newKeyframe.Flatten();
				else if(AutoTangentMode == CurveAutoTangentModes.RightLinear)
					newKeyframe.RightTangentLinear();
				else if(AutoTangentMode == CurveAutoTangentModes.RightConstant)
					newKeyframe.RightTangentConstant();
				else if(AutoTangentMode == CurveAutoTangentModes.LeftLinear)
					newKeyframe.LeftTangentLinear();
				else if(AutoTangentMode == CurveAutoTangentModes.LeftConstant)
					newKeyframe.LeftTangentConstant();
				else if(AutoTangentMode == CurveAutoTangentModes.BothLinear)
					newKeyframe.BothTangentLinear();
				else if(AutoTangentMode == CurveAutoTangentModes.BothConstant)
					newKeyframe.BothTangentConstant();
			}
	
			RebuildCachedCurveInformation(); 
			AreCurvesDirty = true;
		}

		public void AddKeyframe(USInternalKeyframe internalKeyframe, USInternalCurve curve)
		{
			USUndoManager.PropertyChange(curve, "Add Keyframe");
			curve.Keys.Add(internalKeyframe);

			if(Duration < internalKeyframe.Time)
				Duration = internalKeyframe.Time;
			
			curve.Keys.Sort(USInternalCurve.KeyframeComparer);
			curve.BuildAnimationCurveFromInternalCurve();

			RebuildCachedCurveInformation(); 
			AreCurvesDirty = true;
		}
		
		public void AddKeyframeAtTimeUsingCurrentValue(USPropertyInfo propertyInfo, float time)
		{
			USUndoManager.RegisterCompleteObjectUndo(propertyInfo, "Add Keyframe");
			propertyInfo.AddKeyframe(time, AutoTangentMode);
			USUndoManager.RegisterCompleteObjectUndo(this, "Add Keyframe");
			RebuildCurvesOnNextGUI = true;
			AreCurvesDirty = true;
			EditorWindow.Repaint();
		}
	
		private USInternalKeyframe AddKeyframe(USInternalCurve curve, float time, float value)
		{
			// If a keyframe already exists at this time, use that one.
			USInternalKeyframe internalKeyframe = null;
			foreach(USInternalKeyframe keyframe in curve.Keys)
			{
				if(Mathf.Approximately(keyframe.Time, time))
					internalKeyframe = keyframe;
				
				if(internalKeyframe != null)
					break;
			}
			
			// Didn't find a keyframe create a new one.
			if(!internalKeyframe)
			{
				internalKeyframe = ScriptableObject.CreateInstance<USInternalKeyframe>();
				USUndoManager.RegisterCreatedObjectUndo(internalKeyframe, "Add New Keyframe");
				USUndoManager.PropertyChange(curve, "Add Keyframe");
				curve.Keys.Add(internalKeyframe);
			}
			
			USUndoManager.PropertyChange(internalKeyframe, "Add Keyframe");
			internalKeyframe.curve = curve;
			internalKeyframe.Time = time;
			internalKeyframe.Value = value;
			internalKeyframe.InTangent = 0.0f;
			internalKeyframe.OutTangent = 0.0f;

			if(AnimationCurveEditor.AutoTangentMode == CurveAutoTangentModes.Smooth)
				internalKeyframe.Smooth();
			else if(AnimationCurveEditor.AutoTangentMode == CurveAutoTangentModes.Flatten)
				internalKeyframe.Flatten();
			else if(AnimationCurveEditor.AutoTangentMode == CurveAutoTangentModes.RightLinear)
				internalKeyframe.RightTangentLinear();
			else if(AnimationCurveEditor.AutoTangentMode == CurveAutoTangentModes.RightConstant)
				internalKeyframe.RightTangentConstant();
			else if(AnimationCurveEditor.AutoTangentMode == CurveAutoTangentModes.LeftLinear)
				internalKeyframe.LeftTangentLinear();
			else if(AnimationCurveEditor.AutoTangentMode == CurveAutoTangentModes.LeftConstant)
				internalKeyframe.LeftTangentConstant();
			else if(AnimationCurveEditor.AutoTangentMode == CurveAutoTangentModes.BothLinear)
				internalKeyframe.BothTangentLinear();
			else if(AnimationCurveEditor.AutoTangentMode == CurveAutoTangentModes.BothConstant)
				internalKeyframe.BothTangentConstant();
			
			curve.Keys.Sort(USInternalCurve.KeyframeComparer);
			
			curve.BuildAnimationCurveFromInternalCurve();
			
			return internalKeyframe;
		}
	}
}