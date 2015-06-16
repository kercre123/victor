using System;
using System.Collections;
using UnityEngine;

namespace WellFired.Shared
{
	public class InterpolatedNoise
	{
		private static Fractal Fractal
		{ 
			get
			{
				if(noise == null)
					noise = new Fractal(1.27f, 2.04f, 8.36f);
				return noise;
			}
		}
		
		private static Fractal noise;
		
		public static Vector3 GetVector3(float speed, float time)
		{
			var deltaTime = time * 0.01f * speed;
			return new Vector3(Fractal.HybridMultifractal(deltaTime, 15.73f, 0.0f), Fractal.HybridMultifractal(deltaTime, 63.94f, 0.0f), Fractal.HybridMultifractal(deltaTime, 0.2f, 0.0f));
		}
		
		public static float GetFloat(float speed, float time)
		{
			var deltaTime = time * 0.01f * speed;
			return Fractal.HybridMultifractal(deltaTime, 15.7f, 0.65f);
		}
	}
}