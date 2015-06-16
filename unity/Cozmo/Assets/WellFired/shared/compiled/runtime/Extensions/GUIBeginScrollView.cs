using UnityEngine;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class GUIBeginScrollView : IDisposable
	{
		public Vector2 Scroll
		{
			get;
			set;
		}
	
		public GUIBeginScrollView(Vector2 scrollPosition)
		{
			Scroll = GUILayout.BeginScrollView(scrollPosition);
		}
		
		public GUIBeginScrollView(Vector2 scrollPosition, params GUILayoutOption[] options)
		{
			Scroll = GUILayout.BeginScrollView(scrollPosition, options);
		}
		
		public GUIBeginScrollView(Vector2 scrollPosition, GUIStyle style)
		{
			Scroll = GUILayout.BeginScrollView(scrollPosition, style);
		}
		
		public GUIBeginScrollView(Vector2 scrollPosition, GUIStyle style, params GUILayoutOption[] options)
		{
			Scroll = GUILayout.BeginScrollView(scrollPosition, style, options);
		}
		
		public GUIBeginScrollView(Vector2 scrollPosition, GUIStyle horizontalScrollBar, GUIStyle verticalScrollBar)
		{
			Scroll = GUILayout.BeginScrollView(scrollPosition, horizontalScrollBar, verticalScrollBar);
		}
		
		public GUIBeginScrollView(Vector2 scrollPosition, GUIStyle horizontalScrollBar, GUIStyle verticalScrollBar, params GUILayoutOption[] options)
		{
			Scroll = GUILayout.BeginScrollView(scrollPosition, horizontalScrollBar, verticalScrollBar, options);
		}
		
		public void Dispose() 
		{
			GUILayout.EndScrollView();
		}
	}
}