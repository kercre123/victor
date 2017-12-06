// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

// Intended to be used as a framing element or rectangular vignetting element. 
// Built to emulate a "soft mask" or clipping for scrolling views and lists.
// Based on the screen as opposed to an image. Meant for full screen scrolling content
Shader "UI/Cozmo/GradientComplexScreenspaceClippingShader"
{
  Properties
  {
    // Both the _ClippingSize and _ClippingEnd vectors' element values
    // represent one side of the rect; x = right, y = top, z = left, w = bottom

    // Indicates the size of the gradient, as a half-percentage value
    // 1 would indicate "gradient should be the same size as screen edge to center"
    // Values must be greater than 0, but can be > 1
    _ClippingSize ("DEV ONLY Clipping Start", Vector) = (0.5, 0.5, 0.5, 0.5)

    // Indicates the distance from the edge that the gradient should start (aka 
    // the end of clipping). All values valid; positive closer to center negative 
    // away from center. 0 would be the screen edge. 1 would be the screen center.
    _ClippingEnd ("DEV ONLY Clipping End", Vector) = (0.5, 0.5, 0.5, 0.5)

    // Offset if using a sprite atlas because we need to sample the texture
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
        float2 topRightAlpha : TEXCOORD1;
        float2 bottomLeftAlpha : TEXCOORD2;
      };

      float4 _AtlasUV;
      float4 _ClippingEnd;
      float4 _ClippingSize;

      v2f vert (appdata v)
      {
        v2f o;
        o.vertex = UnityObjectToClipPos(v.vertex);

        o.uv = ((o.vertex + float2(1,1)) * 0.5);
        
        // Modify uv x to match screen ratio since we are using PreserveAspect.
        o.uv.x -= 0.5;

        float screenRatio = (_ScreenParams.x / _ScreenParams.y);
        o.uv.x *= 9 * screenRatio * 0.0625;
        o.uv.x += 0.5;
        o.uv *= _AtlasUV.zw;
        o.uv += _AtlasUV.xy;

        // Translate atlas UV to sprite UV
        float2 spriteUV = (v.uv.xy - _AtlasUV.xy) / ( _AtlasUV.zw);

        // Precalculate denominator to speed up calulations
        float4 denominator = 2 / _ClippingSize;
        float4 offset = _ClippingEnd / _ClippingSize;

        // Make the top right of the gradient frame
        float2 topRightAlpha = denominator.xy - spriteUV.xy * denominator.xy - offset.xy;

        // Make the bottom left of the gradient frame
        float2 bottomLeftAlpha = spriteUV.xy * denominator.zw - offset.zw;

        o.topRightAlpha = topRightAlpha;
        o.bottomLeftAlpha = bottomLeftAlpha;

        return o;
      }
      
      sampler2D _MainTex;

      fixed4 frag (v2f i) : SV_Target
      {
        // More opaque at the edges, less opaque in the middle to allow content
        fixed4 col = tex2D(_MainTex, i.uv);
        float2 minAlpha2 = min(i.topRightAlpha,i.bottomLeftAlpha);
        col.a = min(col.a, 1 - min(minAlpha2.x, minAlpha2.y));

        return col;
      }
      ENDCG
    }
  }
}
