// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "UI/Cozmo/AnimatedGlintShader"
{
	Properties
	{
		_GlintPeriod ("Glint Rate Modifier", Float) = 8 // how often the glint starts
		_GlintSpeed ("Glint Speed Modifier", Float) = 5 // how fast the glint moves across the object
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

			float _GlintPeriod;
			float _GlintSpeed;

			struct appdata
			{
				float4 vertex : POSITION;
				float2 uv : TEXCOORD0;
                float4 color : COLOR;
			};

			struct v2f
			{
				float2 uv : TEXCOORD0;
				float4 glintParams : TEXCOORD1;
				float4 vertex : SV_POSITION;
                float4 color : COLOR;
			};

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = UnityObjectToClipPos(v.vertex);
				o.uv = v.uv;	
                o.color = v.color;        

                // Modify the angle of the glint
                float2 glintAngle = o.vertex.xy * float2(1, 0.3);

                // Set up animation time
                fixed animTime = _Time.x * _GlintPeriod;
                fixed modAnimTime = animTime - floor(animTime);

				o.glintParams = float4(glintAngle, 				   // xy
				                       modAnimTime * _GlintSpeed, 	// z
				                       0);							// not used

				return o;
			}
			
			sampler2D _MainTex;

			fixed4 frag (v2f i) : SV_Target
			{
                // Sample masking image
		        fixed4 maskCol = tex2D(_MainTex, i.uv) * i.color;

		        // Animate glint color along x-axis
                fixed4 glintCol = fixed4(1,1,1,1);
                fixed animTime = i.glintParams.z;
                glintCol.a = 0.5 - 2 * abs(i.glintParams.x - animTime - i.glintParams.y + 1);

				// Set alpha to the lower of mask or glint
				glintCol.a = min(maskCol.a, glintCol.a);

                return glintCol;
			}
			ENDCG
		}
	}
	Fallback "UI/Cozmo/Invisible"
}
