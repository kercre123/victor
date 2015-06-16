using UnityEngine;
using System;
using System.Collections;

namespace WellFired
{
	[Serializable]
	public class SplineKeyframe : ScriptableObject 
	{
		[SerializeField]
		private Vector3 position = Vector3.zero;
		public Vector3 Position
		{
			get { return position; }
			set { position = value; }
		}
	}
}