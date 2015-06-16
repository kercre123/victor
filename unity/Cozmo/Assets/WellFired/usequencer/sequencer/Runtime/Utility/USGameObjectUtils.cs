using UnityEngine;
using System.Collections;

namespace WellFired
{
	public class USGameObjectUtils 
	{
		public static void ToggleObjectActive(GameObject GO)
		{
			GO.SetActive(!IsObjectActive(GO));
		}
		
		public static void SetObjectActive(GameObject GO, bool active)
		{
			GO.SetActive(active);
		}
		
		public static bool IsObjectActive(GameObject GO)
		{
			return GO.activeSelf;
		}
	}
}