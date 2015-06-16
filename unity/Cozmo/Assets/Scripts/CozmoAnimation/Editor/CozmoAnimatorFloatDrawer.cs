using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;

/// <summary>
/// Cozmo Animator curve editor; based loosely on uSequencer source.
/// </summary>
[Serializable]
public class CozmoAnimatorFloatDrawer : CozmoAnimatorComponentDrawer
{
	[SerializeField]
	private FloatComponent component;

	[SerializeField]
	private AnimationCurve curve;

	private void OnEnable()
	{
		if (curve == null) {
			curve = new AnimationCurve();
		}
	}

	public override void SetComponent(CozmoAnimatorContent content, Rect position, CozmoAnimationComponent component)
	{
		base.SetComponent (content, position, component);
		this.component = component as FloatComponent;
		if (this.component == null) {
			throw new ArgumentException("This CozmoAnimatorComponentDrawer can only handle FloatComponents. Got " + (component != null ? component.GetType().ToString() : "null") + ".");
		}

		curve.keys = this.component.compiledKeyframes;
	}
	
	public override void DoGUI ()
	{
		if(Event.current.commandName == "UndoRedoPerformed")
			return;

		//GUI.Box (Position, "", WellFired.USEditorUtility.USeqSkin.GetStyle ("TimelinePaneBackground"));

//		if(Event.current.type == EventType.Repaint)
//		{
//			Rect displayAreaRect = GUILayoutUtility.GetLastRect();
//			Rect previousDisplayArea = DisplayArea;
//			
//			DisplayArea = displayAreaRect;
//			
//			if(DisplayArea != previousDisplayArea)
//				RebuildCachedCurveInformation();
//		}
		
		if(Event.current.type == EventType.Repaint)
			DrawGrid();
		
//		DrawCurve(position, curve, Color.red);
//		
//		HandleEvent();
	}

	void DrawGrid()
	{
		base.DrawGridHorizontal ();


	}
}
