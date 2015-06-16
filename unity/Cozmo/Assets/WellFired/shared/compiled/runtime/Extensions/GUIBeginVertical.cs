using UnityEngine;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class GUIBeginVertical : IDisposable
	{
		public GUIBeginVertical()
		{
			GUILayout.BeginVertical();
		}
	
		public GUIBeginVertical(params GUILayoutOption[] layoutOptions)
		{
			GUILayout.BeginVertical(layoutOptions);
		}
		
		public GUIBeginVertical(GUIStyle guiStyle, params GUILayoutOption[] layoutOptions)
		{
			GUILayout.BeginVertical(guiStyle, layoutOptions);
		}
		
		public void Dispose() 
		{
			GUILayout.EndVertical();
		}
	}
}