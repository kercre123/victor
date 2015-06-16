using UnityEngine;
using System.Collections;

namespace WellFired
{
	[ExecuteInEditMode]
	public class AnimationTimelineController : MonoBehaviour 
	{
		private Animator animator;
		private Animator Animator
		{
			get
			{
				if(animator == null)
					animator = GetComponent<Animator>();
				
				return animator;
			}
		}

		private USTimelineAnimation animationTimeline;
		public USTimelineAnimation AnimationTimeline
		{
			private get { return animationTimeline; }
			set { animationTimeline = value; }
		}
	
		private void OnAnimatorMove()
		{
			transform.position = Animator.rootPosition;
			transform.rotation = Animator.rootRotation;
		}
	}
}