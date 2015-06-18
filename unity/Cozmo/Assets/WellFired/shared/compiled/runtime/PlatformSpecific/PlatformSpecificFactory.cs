using UnityEngine;
using System.Collections;
using System;

namespace WellFired.Shared
{
	public static class PlatformSpecificFactory 
	{
		private static Type cachedReflectionType;
		private static IReflectionHelper cachedReflectionHelper;
		public static IReflectionHelper ReflectionHelper
		{
			get
			{
				// determine type here
				if(cachedReflectionType == null)
					cachedReflectionType = Type.GetType("WellFired.Shared.ReflectionHelper, Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null");
				
				// create an object of the type
				if(cachedReflectionHelper == null)
					cachedReflectionHelper = (IReflectionHelper)Activator.CreateInstance(cachedReflectionType);

				return cachedReflectionHelper;
			}
			private set { ; }
		}
		
		private static Type cachedIOType;
		private static IIOHelper cachedIOHelper;
		public static IIOHelper IOHelper
		{
			get
			{
				// determine type here
				if(cachedIOType == null)
					cachedIOType = Type.GetType("WellFired.Shared.IOHelper, Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null");
				
				// create an object of the type
				if(cachedIOHelper == null)
					cachedIOHelper = (IIOHelper)Activator.CreateInstance(cachedIOType);
				
				return cachedIOHelper;
			}
			private set { ; }
		}
		
		private static Type cachedUnityEditorType;
		private static IUnityEditorHelper cachedUnityEditorHelper;
		public static IUnityEditorHelper UnityEditorHelper
		{
			get
			{
				// determine type here
				if(cachedUnityEditorType == null)
					cachedUnityEditorType = Type.GetType("WellFired.Shared.UnityEditorHelper, Assembly-CSharp, Version=0.0.0.0, Culture=neutral, PublicKeyToken=null");
				
				// create an object of the type
				if(cachedUnityEditorHelper == null)
					cachedUnityEditorHelper = (IUnityEditorHelper)Activator.CreateInstance(cachedUnityEditorType);
				
				return cachedUnityEditorHelper;
			}
			private set { ; }
		}
	}
}