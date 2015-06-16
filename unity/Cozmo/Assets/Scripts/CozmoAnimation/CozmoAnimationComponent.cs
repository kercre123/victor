using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;

/// <summary>
/// Base class for a single variable being animated.
/// </summary>
[Serializable]
public abstract class CozmoAnimationComponent : ScriptableObject
{
}

/// <summary>
/// Cozmo Animation storage for a single float variable.
/// </summary>
[Serializable]
public abstract class FloatComponent : CozmoAnimationComponent
{
	public float quality;
	
	/// <summary>
	/// Keyframes for the simplified runtime view of data.
	/// </summary>
	public Keyframe[] compiledKeyframes;
	
	/// <summary>
	/// Interpolated keyframes for easier display.
	/// </summary>
	public Keyframe[] interpolatedKeyframes;

	public abstract float MinimumValue { get; }
	public abstract float MaximumValue { get; }
	public abstract float MaxmimumSlope { get; }
}

/// <summary>
/// A keyframe for an image asset.
/// </summary>
[Serializable]
public struct TextureAssetKeyframe {
	
	public float time;
	
	public UnityEngine.Texture2D asset;
}

/// <summary>
/// Cozmo Animation storage for texture assets.
/// </summary>
[Serializable]
public abstract class TextureComponent : CozmoAnimationComponent
{
	public TextureAssetKeyframe[] keyframes;
}

/// <summary>
/// A keyframe for a sound asset.
/// </summary>
[Serializable]
public struct AudioAssetKeyframe {
	
	public float time;
	
	public UnityEngine.AudioClip asset;
}

/// <summary>
/// Cozmo Animation storage for simple audio queues.
/// </summary>
[Serializable]
public abstract class AudioComponent : CozmoAnimationComponent
{
	public AudioAssetKeyframe[] keyframes;
}

/// <summary>
/// A keyframe for a color.
/// </summary>
[Serializable]
public struct ColorKeyframe {
	
	public float time;
	
	public Color color;
}

/// <summary>
/// Cozmo Animation storage for colors.
/// </summary>
[Serializable]
public abstract class ColorComponent : CozmoAnimationComponent
{
	public ColorKeyframe[] keyframes;

	/// <summary>
	/// The few colors this type of ColorComponent is restricted to.
	/// Null means no restriction.
	/// </summary>
	public virtual Color[] FixedColors {
		get {
			return null;
		}
	}
}
