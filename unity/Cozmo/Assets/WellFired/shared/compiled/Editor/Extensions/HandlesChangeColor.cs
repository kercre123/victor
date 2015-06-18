using UnityEngine;
using UnityEditor;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class HandlesChangeColor : IDisposable
	{
		[SerializeField]
		private Color PreviousColor
		{
			get;
			set;
		}
	
		public HandlesChangeColor(Color newColor)
		{
			PreviousColor = GUI.color;
			Handles.color = newColor;
		}
	
		public void Dispose() 
		{
			Handles.color = PreviousColor;
		}
	}
}