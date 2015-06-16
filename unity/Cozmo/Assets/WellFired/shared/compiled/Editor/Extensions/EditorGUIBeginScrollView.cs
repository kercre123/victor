using UnityEngine;
using UnityEditor;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class EditorGUIBeginScrollView : IDisposable
	{
		public Vector2 Scroll
		{
			get;
			set;
		}
	
		public EditorGUIBeginScrollView(Vector2 scrollPosition)
		{
			Scroll = EditorGUILayout.BeginScrollView(scrollPosition);
		}
		
		public EditorGUIBeginScrollView(Vector2 scrollPosition, GUIStyle horizontalScrollBar, GUIStyle verticalScrollBar)
		{
			Scroll = EditorGUILayout.BeginScrollView(scrollPosition, horizontalScrollBar, verticalScrollBar);
		}
		
		public void Dispose() 
		{
			EditorGUILayout.EndScrollView();
		}
	}
}