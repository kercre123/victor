using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Cozmo Animation asset that stores a single animation's data.
/// </summary>
[Serializable]
public class CozmoAnimation : ScriptableObject
{
	public float duration = 5.0f;

	public CozmoAnimationComponent[] components;
}
