Shader "UI/Cozmo/AnimatedGlintShader"
{
	Properties
	{
		_GlintTex ("Glint Texture", 2D) = "white" {}
    	_UVOffset ("DEV ONLY UV", Vector) = (0, 0, 0, 0)
    	_AtlasUV ("DEV ONLY UV", Vector) = (0.5, 0.5, 0.5, 0.5)
    	_GlintWidthToMaskWidthRatio ("DEV ONLY UV", Float) = 1
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
			};

			struct v2f
			{
				float2 uv : TEXCOORD0;
				float4 vertex : SV_POSITION;
			};

			v2f vert (appdata v)
			{
				v2f o;
				o.vertex = mul(UNITY_MATRIX_MVP, v.vertex);
				o.uv = v.uv;
				return o;
			}
			
			sampler2D _MainTex;
			sampler2D _GlintTex;
			float4 _UVOffset;
      		float4 _AtlasUV;
      		float _GlintWidthToMaskWidthRatio;

			fixed4 frag (v2f i) : SV_Target
			{
				// sample masking image
				fixed4 col = tex2D(_MainTex, i.uv);

				// translate atlas UV to sprite UV
				float2 spriteUV = (i.uv.xy - _AtlasUV.xy) / ( _AtlasUV.zw);
				spriteUV.x = spriteUV.x * _GlintWidthToMaskWidthRatio;

				// offset UV
				spriteUV = spriteUV + _UVOffset.xy;
				spriteUV = clamp(spriteUV, 0, 1);

				// sample glint texture
				fixed4 glintCol = tex2D(_GlintTex, spriteUV);

				// set alpha to 0 if mask or glint texture says so. 
				col.a = min(col.a, glintCol.a);

				// set color to color of the glint texture
				col.rgb = glintCol.rgb;

				return col;
			}
			ENDCG
		}
	}
}
