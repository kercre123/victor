using UnityEngine;
using UnityEditor;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class EditorGUIBeginHorizontal : IDisposable
	{
		public EditorGUIBeginHorizontal()
		{
			EditorGUILayout.BeginHorizontal();
		}
		
		public EditorGUIBeginHorizontal(params GUILayoutOption[] layoutOptions)
		{
			EditorGUILayout.BeginHorizontal(layoutOptions);
		}
		
		public void Dispose() 
		{
			EditorGUILayout.EndHorizontal();
		}
	}
}