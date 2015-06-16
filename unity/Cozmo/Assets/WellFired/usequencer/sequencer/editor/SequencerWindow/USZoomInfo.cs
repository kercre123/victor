using UnityEngine;
using System;
using System.Collections;

namespace WellFired
{
	[Serializable]
	public class USZoomInfo : ScriptableObject
	{	
		// our default zoom level, 1, this is an arbitrary unit.
		[SerializeField] 
		public float currentZoom = 1.0f;
		[SerializeField]
		public float meaningOfEveryMarker = 0.0f;
		[SerializeField]
		public float currentXMarkerDist = 0.0f;
		
		private void OnEnable() { hideFlags = HideFlags.HideAndDontSave; }
	
		public void Reset()
		{
			currentZoom = 1.0f;
			meaningOfEveryMarker = 0.0f;
			currentXMarkerDist = 0.0f;
		}
	}
}