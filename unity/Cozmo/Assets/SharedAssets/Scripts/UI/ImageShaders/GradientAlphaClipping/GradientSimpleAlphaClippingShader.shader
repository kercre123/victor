Shader "UI/Cozmo/GradientSimpleClippingShader"
{
	Properties
  	{
    	_Clipping ("DEV ONLY Clipping", Vector) = (0.5, 0.5, 0.5, 0.5)

    	_AtlasUV ("DEV ONLY UV", Vector) = (0.5, 0.5, 0.5, 0.5)
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

      		float4 _Clipping;
      		float4 _AtlasUV;
			
			sampler2D _MainTex;

			fixed4 frag (v2f i) : SV_Target
			{
				fixed4 col = tex2D(_MainTex, i.uv);

				// translate atlas UV to sprite UV
				float2 spriteUV = (i.uv.xy - _AtlasUV.xy) / ( _AtlasUV.zw);

				// make the top right of the gradient frame
				float2 topRightClipPosition = (1 - spriteUV.xy) * 2 / _Clipping.xy;
				float2 topRightAlpha = 1 - topRightClipPosition.xy;

				float topRightMaxAlpha = max(topRightAlpha.x,topRightAlpha.y);

				// make the bottom left of the gradient frame
				float2 bottomLeftClipPosition = (spriteUV.xy) * 2 / _Clipping.zw;
				float2 bottomLeftAlpha = 1 - bottomLeftClipPosition.xy;

				float bottomLeftMaxAlpha = max(bottomLeftAlpha.x,bottomLeftAlpha.y);

				col.a = min(col.a, max(topRightMaxAlpha, bottomLeftMaxAlpha));

				return col;
			}
			ENDCG
		}
	}
}
