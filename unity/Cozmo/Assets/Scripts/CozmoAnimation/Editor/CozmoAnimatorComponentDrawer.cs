using UnityEngine;
using UnityEditor;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Linq;

/// <summary>
/// Cozmo Animator generic drawer.
/// </summary>
[Serializable]
public abstract class CozmoAnimatorComponentDrawer : ScriptableObject
{
	/// <summary>
	/// The duration of one tick mark.
	/// </summary>
	private const float normalSecondsPerTick = 0.1f;

	/// <summary>
	/// The factor multiplied to the tick duration to get the benchmark duration.
	/// </summary>
	private const float ticksPerBenchmark = 10;

	/// <summary>
	/// The minimum number of tick marks to draw horizontally.
	/// </summary>
	private const float minimumPixelsPerTick = 5;

	/// <summary>
	/// The maximum number of tick marks to draw horizontally.
	/// </summary>
	private const float maximumPixelsPerTick = 20;

	private const float tickScalingMultiplier = 2;
	
	/// <summary>
	/// The amount of space to give around the edges.
	/// </summary>
	private const float tickEdgeDisplacement = 0.05f;

	private static readonly Color tickColor = new Color(.4f, .4f, .4f, .75f);
	
	private static readonly Color benchmarkColor = new Color(.8f, .6f, .6f, .75f);
	
	[SerializeField]
	private CozmoAnimatorContent content;
	public CozmoAnimatorContent Content
	{
		get { return content; }
		set { content = value; }
	}

	[SerializeField]
	private Rect position;
	public Rect Position {
		get {
			return position;
		}
		set {
			position = value;
		}
	}

	private static Action ApplyWireMaterial;

	static CozmoAnimatorComponentDrawer()
	{
		ApplyWireMaterial = (Action)Delegate.CreateDelegate(typeof(Action), typeof(HandleUtility).GetMethod ("ApplyWireMaterial", System.Reflection.BindingFlags.Static | System.Reflection.BindingFlags.NonPublic));
	}
	
	public virtual void SetComponent(CozmoAnimatorContent content, Rect position, CozmoAnimationComponent component)
	{
		this.content = content;
		this.position = position;
	}

	public abstract void DoGUI ();

	public void DrawGridHorizontal()
	{
		if (Event.current.type != EventType.Repaint) {
			return;
		}

		ApplyWireMaterial ();

		float minTime = Mathf.Max (0, content.GetTimeForX (Position.xMin + tickEdgeDisplacement));
		float maxTime = Mathf.Min (content.CurrentAnimation.duration, content.GetTimeForX (Position.xMax - tickEdgeDisplacement));

		float secondsPerTick = normalSecondsPerTick;
		float secondsPerPixel = (maxTime - minTime) / Position.width;
		while (secondsPerTick < minimumPixelsPerTick * secondsPerPixel) {
			secondsPerTick /= tickScalingMultiplier;
		}
		while (secondsPerTick > maximumPixelsPerTick * secondsPerPixel) {
			secondsPerTick *= tickScalingMultiplier;
		}

		float secondsPerBenchmark = secondsPerTick * ticksPerBenchmark;

		minTime = Mathf.Ceil (minTime / secondsPerTick) * secondsPerTick;
		maxTime = Mathf.Floor (maxTime / secondsPerTick) * secondsPerTick;
		float nextBenchmark = Mathf.Ceil (minTime / secondsPerBenchmark) * secondsPerBenchmark;

		GL.Color (Color.white);
		GL.Begin (GL.TRIANGLE_STRIP);
		GL.Vertex(new Vector3(Position.xMin, Position.yMax, 0));
		GL.Vertex(new Vector3(Position.xMin, Position.yMin, 0));
		GL.Vertex(new Vector3(Position.xMax, Position.yMax, 0));
		GL.Vertex(new Vector3(Position.xMax, Position.yMin, 0));
		GL.End ();


		GL.Begin (GL.LINES);
		GL.Color (tickColor);
		for (float currentTime = minTime; currentTime <= maxTime; currentTime += secondsPerTick) {
			float x = content.GetXForTime(currentTime);

			bool benchmark = (currentTime >= nextBenchmark);
			if (benchmark) {
				GL.Color (benchmarkColor);
			}

			GL.Vertex(new Vector3(x, Position.yMin, 0));
			GL.Vertex(new Vector3(x, Position.yMin + .25f * Position.height, 0));

			if (benchmark) {
//				Rect textPos = new Rect(x - 16, position.yMin + 16, x + 16, 16);
//				GUI.TextField(textPos, currentTime.ToString(), EditorStyles.miniButton);

				GL.Color (tickColor);
				nextBenchmark += secondsPerBenchmark;
			}
		}
		GL.End ();
	}
}
