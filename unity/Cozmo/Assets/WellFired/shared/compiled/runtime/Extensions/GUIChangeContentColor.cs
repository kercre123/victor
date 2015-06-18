using UnityEngine;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class GUIChangeContentColor : IDisposable
	{
		[SerializeField]
		private Color PreviousColor
		{
			get;
			set;
		}
	
		public GUIChangeContentColor(Color newColor)
		{
			PreviousColor = GUI.contentColor;
			GUI.contentColor = newColor;
		}
	
		public void Dispose() 
		{
			GUI.contentColor = PreviousColor;
		}
	}
}