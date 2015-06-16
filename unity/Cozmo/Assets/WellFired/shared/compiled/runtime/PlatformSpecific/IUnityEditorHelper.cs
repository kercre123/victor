using System;
using System.Collections;
using System.Reflection;

namespace WellFired.Shared
{
	public interface IUnityEditorHelper
	{
		void AddUpdateListener(Action listener);
		void RemoveUpdateListener(Action listener);
	}
}