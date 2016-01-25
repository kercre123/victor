Shader "UI/Cozmo/Unlit/GradientClippingShader"
{
	Properties
  	{
    	_TopClipping ("Top Clipping", Float) = 0.5
    	_BottomClipping ("Bottom Clipping", Float) = 0.5
    	_LeftClipping ("Left Clipping", Float) = 0.5
    	_RightClipping ("Right Clipping", Float) = 0.5

    	_XMinUV ("DEV ONLY X Min UV", Float) = 0.5
    	_XMaxUV ("DEV ONLY X Max UV", Float) = 0.5
    	_YMinUV ("DEV ONLY Y Min UV", Float) = 0.5
    	_YMaxUV ("DEV ONLY Y Max UV", Float) = 0.5
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

      		float _TopClipping;
      		float _BottomClipping;
      		float _LeftClipping;
      		float _RightClipping;

      		float _XMinUV;
      		float _XMaxUV;
      		float _YMinUV;
      		float _YMaxUV;
			
			sampler2D _MainTex;

			fixed4 frag (v2f i) : SV_Target
			{
				fixed4 col = tex2D(_MainTex, i.uv);

				// translate atlas UV to sprite UV
				float xSpriteUV = (i.uv.x - _XMinUV) / (_XMaxUV - _XMinUV);
				float ySpriteUV = (i.uv.y - _YMinUV) / (_YMaxUV - _YMinUV);

				// make the top of the gradient frame
				float topClipPosition = (1 - ySpriteUV) * 2 / _TopClipping;
				float topAlpha = 1 - topClipPosition;

				// make the bottom of the gradient frame
				float bottomClipPosition = (ySpriteUV) * 2 / _BottomClipping;
				float bottomAlpha = 1 - bottomClipPosition;

				float verticalAlpha = max(topAlpha,bottomAlpha);

				// make the left of the gradient frame
				float leftClipPosition = (1 - xSpriteUV) * 2 / _LeftClipping;
				float leftAlpha = 1 - leftClipPosition;

				// make the right of the gradient frame
				float rightClipPosition = (xSpriteUV) * 2 / _RightClipping;
				float rightAlpha = 1 - rightClipPosition;

				float horizontalAlpha = max(leftAlpha,rightAlpha);

				col.a = min(col.a, max(verticalAlpha, horizontalAlpha));

				return col;
			}
			ENDCG
		}
	}
}
