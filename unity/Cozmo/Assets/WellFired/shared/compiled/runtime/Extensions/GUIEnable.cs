using UnityEngine;
using System;
using System.Collections;

namespace WellFired.Shared
{
	public class GUIEnable : IDisposable
	{
		[SerializeField]
		private bool PreviousState
		{
			get;
			set;
		}
	
		public GUIEnable(bool newState)
		{
			PreviousState = GUI.enabled;
			GUI.enabled = newState;
		}
	
		public void Dispose() 
		{
			GUI.enabled = PreviousState;
		}
	}
}