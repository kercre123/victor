Shader "UI/Cozmo/NeedsMeterParticleScroll"
{
	Properties
	{
		// Red   = Masking to use with bar full, where redder = more visible
		// Green = Particle texture
		// Blue  = Masking to use with bar empty, where bluer = more visible
		_ParticleTex ("Particle Texture", 2D) = "white" {}

		// Red   = Masking for the shape of the bar
		// Green = Masking used to display the value of the bar, based on a cutoff. Should run from 
		//         green -> black for full -> empty
		// Blue  = Not used
		_MaskTex ("Mask Texture", 2D) = "white" {}

		// Used to display particles on a part of the bar
		[HideInInspector] _Cutoff ("Cutoff Value", Range(0.0,1.0)) = 0.5

		// Holds four UV values: xy / wz are two pairs of uvs.
		[HideInInspector] _UVOffset ("UV Offset", Vector) = (0,0,0,0)
		[HideInInspector] _UVOffset2 ("UV Offset 2", Vector) = (0,0,0,0)

		// Used for cloud masking
		[HideInInspector] _CutoffSqr ("CutoffSqr Value", Range(0.0,1.0)) = 0.5
		[HideInInspector] _CloudUVOffset ("Cloud UV Offset", Vector) = (0,0,0,0)
	}
	SubShader
	{
		// No culling or depth
		Cull Off ZWrite Off ZTest Always

		// Alpha blending
		Blend SrcAlpha OneMinusSrcAlpha 

		Pass
		{
			CGPROGRAM
			#pragma vertex vert
			#pragma fragment frag
			
			#include "UnityCG.cginc"

			struct appdata
			{
				fixed4 vertex : POSITION;
				fixed2 mainUV : TEXCOORD0;
			};

			struct v2f
			{
				fixed4 vertex : SV_POSITION;
				fixed2 mainUV : TEXCOORD0;
				fixed4 particleUV1 :TEXCOORD1;
				fixed4 particleUV2 :TEXCOORD2;
				fixed2 cloudUV :TEXCOORD3;
			};

		    fixed4 _UVOffset;
		    fixed4 _UVOffset2;
		    fixed4 _CloudUVOffset;

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);

				// Set main UV for masking
				o.mainUV = v.mainUV;

				// Scroll particles in four directions
				o.particleUV1.xy = v.mainUV + _UVOffset.xy;
				o.particleUV1.zw = v.mainUV + _UVOffset.zw;
				o.particleUV2.xy = v.mainUV + _UVOffset2.xy;
				o.particleUV2.zw = v.mainUV + _UVOffset2.zw;

				// Scroll cloud mask for flicker effect
				o.cloudUV = v.mainUV + _CloudUVOffset.xy;

				return o;
			}
			
			sampler2D _ParticleTex;
			sampler2D _MaskTex;
		    fixed _Cutoff;
		    fixed _CutoffSqr;

			fixed4 frag (v2f i) : SV_Target
			{
			    fixed4 col = (0,0,0,0);
				fixed4 cutoffTex = tex2D(_MaskTex, i.mainUV);

				// Only calculate alpha if it's below the cutoff / inside the bar
				if (cutoffTex.g < _Cutoff){
					// Compile particle color
					col.rgb = 1;
					col.a = tex2D(_ParticleTex, i.particleUV1.xy).g; 	        // Direction 1
					col.a = col.a + tex2D(_ParticleTex, i.particleUV1.zw).g;	// Direction 2
					col.a = col.a + tex2D(_ParticleTex, i.particleUV2.xy).g;	// Direction 3
					col.a = col.a + tex2D(_ParticleTex, i.particleUV2.zw).g;	// Direction 4

					// Mask based on cloud - show fewer particles if cutoff is lower
					fixed4 cloudMaskCol = tex2D(_ParticleTex, i.cloudUV.xy);
					col.a = min(col.a, lerp(cloudMaskCol.b, cloudMaskCol.r, _CutoffSqr));

					// Mask out outside of bar
					col.a = min(col.a, cutoffTex.r);
				}

				return col;
			}
			ENDCG
		}
	}
	Fallback "UI/Cozmo/Invisible"
}
