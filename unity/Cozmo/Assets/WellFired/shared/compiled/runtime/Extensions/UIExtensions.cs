using UnityEngine;

namespace WellFired.Shared
{
	public class UIExtensions
	{
	    /// <summary>
	    /// Creates a texture of the specified size, filled with the specified colour.
	    /// </summary>
	    public static Texture2D CreateTexture(int width, int height, Color colour)
	    {
			return CreateTexture(width, height, colour, true);
		}

		public static Texture2D CreateTexture(int width, int height, Color colour, bool dontSave)
		{
			var pix = new Color[width * height];
			for (int i = 0; i < pix.Length; i++)
				pix[i] = colour;
			
			var result = new Texture2D(width, height);
			result.wrapMode = TextureWrapMode.Repeat;
			result.SetPixels(pix);
			result.Apply();
			result.hideFlags = dontSave ? HideFlags.HideAndDontSave : HideFlags.None;
			
			return result;
		}
	
		/// <summary>
		/// Creates a 1 x 1 texture with the passed colour
		/// </summary>
		/// <returns>The texture.</returns>
		/// <param name="col">Col.</param>
		public static Texture2D Create1x1Texture(Color colour)
		{
			return Create1x1Texture(colour, true);
		}

		public static Texture2D Create1x1Texture(Color colour, bool dontSave)
		{
			return CreateTexture(1, 1, colour, dontSave);
		}
	}
}