// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "UI/Cozmo/GradientColorClippingShader"
{
  Properties
  {
    _ClippingSize ("Gradient Size", Vector) = (0.05, 0.05, 0.05, 0.05)
	// These represent each side of the clipping rect in the order:
	// right, top, left, bottom
    _ClippingEnd ("Clipping Ends", Vector) = (1.0, 0.1, 0.2, 0.3)
    _Color ("Tint", Color) = (0.9, 0.9, 0.9, 0.8)
  }
  SubShader
  {
    Tags
    { 
      "Queue"="Transparent" 
      "IgnoreProjector"="True" 
      "RenderType"="Transparent" 
      "CanUseSpriteAtlas"="False"
    }
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
        float2 topRightAlpha : TEXCOORD1;
        float2 bottomLeftAlpha : TEXCOORD2;
      };

      float4 _ClippingEnd;
      float4 _ClippingSize;
      float4 _Color;

      v2f vert (appdata v)
      {
        v2f o;
        o.vertex = UnityObjectToClipPos(v.vertex);
        o.uv = v.uv;

        // precalculate denominator
        float4 denominator = 2 / _ClippingSize;
        float4 offset = _ClippingEnd / _ClippingSize;

        // make the top right of the gradient frame
        float2 topRightAlpha = denominator.xy - v.uv.xy * denominator.xy - offset.xy;

        // make the bottom left of the gradient frame
        float2 bottomLeftAlpha = v.uv.xy * denominator.zw - offset.zw;

        o.topRightAlpha = topRightAlpha;
        o.bottomLeftAlpha = bottomLeftAlpha;

        return o;
      }

      fixed4 frag (v2f i) : SV_Target
      {
	    fixed4 col = _Color;

        float2 minAlpha2 = min(i.topRightAlpha,i.bottomLeftAlpha);
        col.a = min(col.a, 1 - min(minAlpha2.x, minAlpha2.y));

        return col;
      }
      ENDCG
    }
  }
}
