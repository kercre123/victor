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
public class CozmoAnimatorAudioDrawer : CozmoAnimatorComponentDrawer
{
	[SerializeField]
	protected AudioComponent component;

	public override void SetComponent(CozmoAnimatorContent content, Rect position, CozmoAnimationComponent component)
	{
		base.SetComponent (content, position, component);
		this.component = component as AudioComponent;
		if (this.component == null) {
			throw new ArgumentException("This CozmoAnimatorComponentDrawer can only handle AudioComponents. Got " + (component != null ? component.GetType().ToString() : "null") + ".");
		}
	}
	
	public override void DoGUI ()
	{

	}
}
