using UnityEngine;

namespace WellFired
{
	public static class TransformExtensions
	{
		public static string GetFullHierarchyPath(this Transform transform)
		{
			string path = "/" + transform.name;
			while (transform.parent != null)
			{
				transform = transform.parent;
				path = "/" + transform.name + path;
			}
			return path;
		}
	}
}