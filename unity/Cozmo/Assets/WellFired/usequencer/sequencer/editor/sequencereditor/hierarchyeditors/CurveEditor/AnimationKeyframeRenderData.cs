using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace WellFired
{
	public class AnimationKeyframeRenderData
	{
		public Vector2 RenderPosition
		{
			get;
			set;
		}
		
		public Rect RenderRect
		{
			get;
			set;
		}
		
		public bool IsKeyframeSelected
		{
			get;
			set;
		}
		
		public bool HasRightTangent
		{
			get;
			set;
		}
		
		public Vector2 RightTangentEnd
		{
			get;
			set;
		}
		
		public Color RightTangentColor
		{
			get;
			set;
		}
		
		public bool HasLeftTangent
		{
			get;
			set;
		}
		
		public Vector2 LeftTangentEnd
		{
			get;
			set;
		}
		
		public Color LeftTangentColor
		{
			get;
			set;
		}
		
		public List<Vector2> CurveSegments
		{
			get;
			set;
		}
		
		public bool IsMouseOverKeyframe
		{
			get;
			set;
		}
		
		public bool IsMouseOverLeftTangent
		{
			get;
			set;
		}
		
		public bool IsMouseOverRightTangent
		{
			get;
			set;
		}
		
		public AnimationKeyframeRenderData()
		{
			CurveSegments = new List<Vector2>();
		}
	}
}