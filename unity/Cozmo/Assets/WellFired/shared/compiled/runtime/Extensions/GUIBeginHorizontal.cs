using UnityEngine;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class GUIBeginHorizontal : IDisposable
	{
		public GUIBeginHorizontal()
		{
			GUILayout.BeginHorizontal();
		}
		
		public GUIBeginHorizontal(params GUILayoutOption[] layoutOptions)
		{
			GUILayout.BeginHorizontal(layoutOptions);
		}
		
		public GUIBeginHorizontal(GUIStyle guiStyle, params GUILayoutOption[] layoutOptions)
		{
			GUILayout.BeginHorizontal(guiStyle, layoutOptions);
		}
		
		public void Dispose() 
		{
			GUILayout.EndHorizontal();
		}
	}
}