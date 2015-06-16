using UnityEngine;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class GUIChangeColor : IDisposable
	{
		[SerializeField]
		private Color PreviousColor
		{
			get;
			set;
		}
	
		public GUIChangeColor(Color newColor)
		{
			PreviousColor = GUI.color;
			GUI.color = newColor;
		}
	
		public void Dispose() 
		{
			GUI.color = PreviousColor;
		}
	}
}	