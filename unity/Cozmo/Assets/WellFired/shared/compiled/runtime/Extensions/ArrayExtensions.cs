using System;
using System.Collections.Generic;
using System.Linq;

namespace WellFired.Shared
{
	public static class ArrayExtensions
	{
		/// <summary>
		/// This method gets a sub section of another array.
		/// </summary>
		/// <returns>The array.</returns>
	    public static T[] SubArray<T>(this T[] data, int index, int length)
	    {
	        T[] result = new T[length];
	        Array.Copy(data, index, result, 0, length);
	        return result;
	    }

		/// <summary>
		/// Populates an array with the specified value.
		/// </summary>
		/// <param name="value">The value to populate this array with</param>
		public static void Populate<T>(this T[] data, T value)
		{
			for (int i = 0; i < data.Length; ++i)
			{
				data[i] = value;
			}
		}
	}
}