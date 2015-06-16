using UnityEngine;
using System.Collections;
using System;

namespace WellFired.Shared.PlatformSpecific
{
	public static class ReflectionFactory 
	{
		private static Type cachedType;
		private static IReflectionHelper cachedReflectionHelper;
		public static IReflectionHelper ReflectionHelper
		{
			get
			{
				// determine type here
				if(cachedType == null)
					cachedType = Type.GetType("WellFired.Shared.PlatformSpecific.ReflectionHelper, Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null");
				
				// create an object of the type
				if(cachedReflectionHelper == null)
					cachedReflectionHelper = (IReflectionHelper)Activator.CreateInstance(cachedType);

				return cachedReflectionHelper;
			}
			private set { ; }
		}
	}
}