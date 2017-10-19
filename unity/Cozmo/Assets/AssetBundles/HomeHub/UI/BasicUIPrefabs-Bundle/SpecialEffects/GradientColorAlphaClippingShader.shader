// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

// Intended to be used as a framing element or rectangular vignetting element. 
// Built to emulate a "soft mask" or clipping for scrolling views and lists.
// Bottom layer: Full color
// Middle: Scrolling content
// Top layer: Full color, with this shader
Shader "UI/Cozmo/GradientColorClippingShader"
{
  Properties
  {
    // Both the _ClippingSize and _ClippingEnd vectors' element values
    // represent one side of the rect; x = right, y = top, z = left, w = bottom

    // Indicates the size of the gradient, as a half-percentage value
    // 1 would indicate "gradient should be the same size as image edge to center"
    // Values must be greater than 0, but can be > 1
    _ClippingSize ("Gradient Size", Vector) = (0.05, 0.05, 0.05, 0.05)

    // Indicates the distance from the edge that the gradient should start (aka 
    // the end of clipping). All values valid; positive closer to center negative 
    // away from center. 0 would be the image edge. 1 would be the image center.
    _ClippingEnd ("Clipping Ends", Vector) = (1.0, 0.1, 0.2, 0.3)

    // General color of the frame; no texture.
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

        // Precalculate denominator to speed up calulations
        float4 denominator = 2 / _ClippingSize;
        float4 offset = _ClippingEnd / _ClippingSize;

        // Make the top right of the gradient frame
        float2 topRightAlpha = denominator.xy - v.uv.xy * denominator.xy - offset.xy;

        // Make the bottom left of the gradient frame
        float2 bottomLeftAlpha = v.uv.xy * denominator.zw - offset.zw;

        o.topRightAlpha = topRightAlpha;
        o.bottomLeftAlpha = bottomLeftAlpha;

        return o;
      }

      fixed4 frag (v2f i) : SV_Target
      {
	      fixed4 col = _Color;

        // More opaque at the edges, less opaque in the middle to allow content
        // to show through
        float2 minAlpha2 = min(i.topRightAlpha, i.bottomLeftAlpha);
        col.a = min(col.a, 1 - min(minAlpha2.x, minAlpha2.y));

        return col;
      }
      ENDCG
    }
  }
}
