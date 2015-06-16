using UnityEngine;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class GizmosChangeColor : IDisposable
	{
		[SerializeField]
		private Color PreviousColor
		{
			get;
			set;
		}
	
		public GizmosChangeColor(Color newColor)
		{
			PreviousColor = GUI.color;
			Gizmos.color = newColor;
		}
	
		public void Dispose() 
		{
			Gizmos.color = PreviousColor;
		}
	}
}