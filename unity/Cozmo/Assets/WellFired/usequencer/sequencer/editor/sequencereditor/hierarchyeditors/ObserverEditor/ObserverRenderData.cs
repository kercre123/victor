using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace WellFired
{
	[Serializable]
	public class ObserverRenderData : ScriptableObject
	{
		[SerializeField]
		private Rect renderRect;
		public Rect RenderRect
		{
			get { return renderRect; }
			set { renderRect = value; }
		}
	
		[SerializeField]
		private USObserverKeyframe keyframe;
		public USObserverKeyframe Keyframe
		{
			get { return keyframe; }
			set { keyframe = value; }
		}
		
		[SerializeField]
		private Vector2 renderPosition;
		public Vector2 RenderPosition
		{
			get { return renderPosition; }
			set { renderPosition = value; }
		}
	} 
}