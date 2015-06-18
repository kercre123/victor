using UnityEngine;
using UnityEditor;
using System.Collections;

namespace WellFired
{
	public class AnimationCurveEditorGUIUtility 
	{
		public static void KeyframeLabel(USInternalKeyframe keyframe, AnimationKeyframeRenderData renderData, bool useCurrentValue, float xScroll, float xScale)
		{
			bool isSelected = renderData.IsKeyframeSelected;
	
			Vector2 FinalRenderPosition = renderData.RenderPosition;
			FinalRenderPosition.x = (FinalRenderPosition.x * xScale) - xScroll;
	
			renderData.RenderRect = new Rect(FinalRenderPosition.x - 6, renderData.RenderPosition.y - 6, 16, 16);
	
			using(new WellFired.Shared.GUIChangeColor(isSelected ? Color.yellow : (useCurrentValue ? Color.red : Color.white)))
				GUI.Label(renderData.RenderRect, AnimationCurveEditorUtility.KeyframeTexture);
	
			renderData.IsMouseOverKeyframe = false;
			if(renderData.RenderRect.Contains(Event.current.mousePosition))
				renderData.IsMouseOverKeyframe = true;
		}
		
		public static void DrawLine(Vector2 from, Vector2 to)
		{
			if(from.Equals(to))
				return;

			Handles.DrawLine(from, to);				
		}
		
		public static void DrawLine(Vector2 from, Vector2 to, float xScroll, float xScale)
		{
			from.x = (from.x * xScale) - xScroll;
			to.x = (to.x * xScale) - xScroll;
			Handles.DrawLine(from, to);				
		}
	}
}