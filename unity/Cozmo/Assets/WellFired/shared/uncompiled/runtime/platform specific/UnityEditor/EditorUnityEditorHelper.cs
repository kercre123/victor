using System;
using System.IO;

#if UNITY_EDITOR
using UnityEditor;
#endif

#if UNITY_EDITOR
namespace WellFired.Shared
{
	public class UnityEditorHelper : IUnityEditorHelper
	{
		public void AddUpdateListener(Action listener)
		{
			listeners += listener;

			// We always remove and the re add it, so we can ensure we never add it twice.
			EditorApplication.update -= Update;
			EditorApplication.update += Update;
		}
		
		public void RemoveUpdateListener(Action listener)
		{
			listeners -= listener;
		}

		private Action listeners = delegate { };

		private void Update()
		{
			listeners();
		}
	}
}
#endif