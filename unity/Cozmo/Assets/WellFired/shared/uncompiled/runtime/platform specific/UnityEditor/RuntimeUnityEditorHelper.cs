using System;
using System.IO;

#if !UNITY_EDITOR
namespace WellFired.Shared
{
	public class UnityEditorHelper : IUnityEditorHelper
	{
		public void AddUpdateListener(Action listener)
		{

		}
		
		public void RemoveUpdateListener(Action listener)
		{
			
		}
	}
}
#endif