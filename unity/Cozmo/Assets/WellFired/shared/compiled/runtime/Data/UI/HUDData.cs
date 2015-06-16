using System;
using UnityEngine;

namespace WellFired.Data
{
	[Serializable]
	public class HUDData : DataBaseEntry 
	{
		[SerializeField]
		public float HealthRatio
		{
			get;
			set;
		}

		public float LeftWeaponCooldownRatio
		{
			get;
			set;
		}

		public float RightWeaponCooldownRatio
		{
			get;
			set;
		}
	}
}