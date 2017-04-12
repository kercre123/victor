// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "UI/Cozmo/AnimatedGlintShader"
{
	Properties
	{
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

        // modify the angle of the glint
        float2 glintParams = o.vertex.xy * float2(1, 0.3);

				o.glintParams = float4(glintParams, (_Time.x * 8 - floor(_Time.x * 8)) * 5, 0);

				return o;
			}
			
			sampler2D _MainTex;

			fixed4 frag (v2f i) : SV_Target
			{
        // sample masking image
				fixed4 col = tex2D(_MainTex, i.uv) * i.color;

        fixed4 glintCol = fixed4(1,1,1,1);

        glintCol.a = 0.5 - 2 * abs(i.glintParams.x - i.glintParams.z - i.glintParams.y + 1);

				// set alpha to 0 if mask or glint color says so. 
				glintCol.a = min(col.a, glintCol.a);

        return glintCol;
			}
			ENDCG
		}
	}
	Fallback "UI/Cozmo/Invisible"
}
