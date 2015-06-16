using System;
using System.Collections;
using UnityEngine;

namespace WellFired.Shared
{
	public class Fractal
	{
		private Perlin  noise;
		private float[] exponent;
		private int     intOctaves;
		private float   floatOctaves;
		private float   lacunarity;

		public Fractal(float inH, float inLacunarity, float inOctaves) : this (inH, inLacunarity, inOctaves, null)
		{
			
		}
		
		public Fractal(float inH, float inLacunarity, float inOctaves, Perlin passedNoise)
		{
			lacunarity = inLacunarity;
			floatOctaves = inOctaves;
			intOctaves = (int)inOctaves;
			exponent = new float[intOctaves+1];
			float frequency = 1.0F;
			for (int i = 0; i < intOctaves+1; i++)
			{
				exponent[i] = (float)Math.Pow (lacunarity, -inH);
				frequency *= lacunarity;
			}
			
			if (passedNoise == null)
				noise = new Perlin();
			else
				noise = passedNoise;
		}
		
		
		public float HybridMultifractal(float x, float y, float offset)
		{
			float weight, signal, remainder, result;
			
			result = (noise.Noise (x, y)+offset) * exponent[0];
			weight = result;
			x *= lacunarity; 
			y *= lacunarity;
			int i;
			for (i=1;i<intOctaves;i++)
			{
				if (weight > 1.0F) weight = 1.0F;
				signal = (noise.Noise (x, y) + offset) * exponent[i];
				result += weight * signal;
				weight *= signal;
				x *= lacunarity; 
				y *= lacunarity;
			}
			remainder = floatOctaves - intOctaves;
			result += remainder * noise.Noise (x,y) * exponent[i];
			
			return result;
		}
		
		public float RidgedMultifractal(float x, float y, float offset, float gain)
		{
			float weight, signal, result;
			int i;
			
			signal = Mathf.Abs (noise.Noise (x, y));
			signal = offset - signal;
			signal *= signal;
			result = signal;
			weight = 1.0F;
			
			for (i=1;i<intOctaves;i++)
			{
				x *= lacunarity; 
				y *= lacunarity;
				
				weight = signal * gain;
				weight = Mathf.Clamp01 (weight);
				
				signal = Mathf.Abs (noise.Noise (x, y));
				signal = offset - signal;
				signal *= signal;
				signal *= weight;
				result += signal * exponent[i];
			}
			
			return result;
		}
		
		public float BrownianMotion(float x, float y)
		{
			float value, remainder;
			long i;
			
			value = 0.0F;
			for (i=0;i<intOctaves;i++)
			{
				value = noise.Noise (x,y) * exponent[i];
				x *= lacunarity;
				y *= lacunarity;
			}
			remainder = floatOctaves - intOctaves;
			value += remainder * noise.Noise (x,y) * exponent[i];
			
			return value;
		}
	}
}