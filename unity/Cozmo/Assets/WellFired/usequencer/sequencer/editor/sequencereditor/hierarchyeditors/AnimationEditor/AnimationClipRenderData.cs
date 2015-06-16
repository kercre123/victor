using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Reflection;

namespace WellFired
{
	[Serializable]
	public class AnimationClipRenderData : ScriptableObject
	{
		[SerializeField]
		public Rect renderRect;
		[SerializeField]
		public Rect labelRect;
		[SerializeField]
		public Rect transitionRect;
		[SerializeField]
		public Rect leftHandle;
		[SerializeField]
		public Rect rightHandle;
		[SerializeField]
		public Vector2 renderPosition;
		[SerializeField]
		public AnimationClipData animationClipData;
	}
}