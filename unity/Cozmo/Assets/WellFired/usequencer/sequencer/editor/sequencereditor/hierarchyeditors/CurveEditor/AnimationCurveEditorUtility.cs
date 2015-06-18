using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	public static class AnimationCurveEditorUtility 
	{
		public const int CURVE_KEYFRAME_STEP_COUNT		= 8;
		public const int BOLD_STEP						= 5;
		public const int NUM_LINES						= 11;
		public const float TANGENT_LENGTH 				= 30.0f;
		public static readonly Color ZERO_GRID 			= new Color (0.0f, 1.0f, 0.0f, 0.4f);
		public static readonly Color BOLD_GRID 			= new Color (0.4f, 0.4f, 0.4f, 0.5f);
		public static readonly Color TANGENT_COLOR		= new Color (0.4f, 0.4f, 0.4f, 0.9f);
		public static readonly Color LIGHT_GRID 		= new Color (0.4f, 0.4f, 0.4f, 0.1f);
		public static readonly Color GRID_LABEL_COLOR 	= new Color (0.67f, 0.67f, 0.67f, 1.0f);
		public static readonly Color LABEL_COLOR 		= new Color (0.0f, 0.0f, 0.0f, 0.4f);
	
		private static Texture cachedKeyframeTexture;
		public static Texture 	KeyframeTexture
		{
			set { ; }
			get
			{
				if(cachedKeyframeTexture)
					return cachedKeyframeTexture;
				
				string textureName = "DarkKeyframe";
				cachedKeyframeTexture = Resources.Load(textureName, typeof(Texture)) as Texture;
				if(!cachedKeyframeTexture)
					throw new Exception(String.Format("Couldn't find texture : {0} for USAnimationCurveEditor::ONGUI, it is possible it has been moved from the resources folder", textureName));
	
				return cachedKeyframeTexture;
			}
		}
		
		private static List<Color> colors = new List<Color>();
		public static Color GetCurveColor(int index)
		{
			if(colors.Count == 0)
			{
				colors.Add(new Color(1.0f, 0.0f, 0.0f, 1.0f));
				colors.Add(new Color(0.0f, 1.0f, 0.0f, 1.0f));
				colors.Add(new Color(0.0f, 0.0f, 1.0f, 1.0f));
				colors.Add(new Color(1.0f, 1.0f, 0.0f, 1.0f));
			}
			
			int colorIndex = index;
			if(colorIndex >= colors.Count)
			{
				while(colorIndex > 0)
				{
					colors.Add(new Color(UnityEngine.Random.Range(0.0f, 1.0f), UnityEngine.Random.Range(0.0f, 1.0f), UnityEngine.Random.Range(0.0f, 1.0f), 1.0f));
					colorIndex--;
				}
			}
			
			return colors[colorIndex];
		}
	}
}