using UnityEngine;
using UnityEditor;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class EditorGUIBeginVertical : IDisposable
	{
		public EditorGUIBeginVertical()
		{
			EditorGUILayout.BeginVertical();
		}
	
		public EditorGUIBeginVertical(params GUILayoutOption[] layoutOptions)
		{
			EditorGUILayout.BeginVertical(layoutOptions);
		}
		
		public void Dispose() 
		{
			EditorGUILayout.EndVertical();
		}
	}
}