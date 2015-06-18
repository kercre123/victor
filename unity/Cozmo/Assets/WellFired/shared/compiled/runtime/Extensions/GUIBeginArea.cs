using UnityEngine;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class GUIBeginArea : IDisposable
	{
		public GUIBeginArea(Rect area)
		{
			GUILayout.BeginArea(area);
		}
		
		public GUIBeginArea(Rect area, string content)
		{
			GUILayout.BeginArea(area, content);
		}
	
		public GUIBeginArea(Rect area, string content, string style)
		{
			GUILayout.BeginArea(area, content, style);
		}
		
		public void Dispose() 
		{
			GUILayout.EndArea();
		}
	}
}