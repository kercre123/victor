using UnityEngine;
using System.Collections;

namespace WellFired.Shared
{
	public static class ColorExtensions
	{
		public static int PackColor(Color colorToPack)
		{
			Color32 color32 = colorToPack;
			int packedColor = 0;

			packedColor |= color32.r;
			packedColor <<= 8;
			
			packedColor |= color32.g;
			packedColor <<= 8;
			
			packedColor |= color32.b;
			packedColor <<= 8;
			
			packedColor |= color32.a;

			return packedColor;
		}

		public static Color UnPackColor(int packedColor)
		{
			var a = (byte)((packedColor >> 24) & 255);
			var b = (byte)((packedColor >> 16) & 255);
			var g = (byte)((packedColor >> 8) & 255);
			var r = (byte)((packedColor >> 0) & 255);

			return new Color32(r, g, b, a);
		}
	}
}
