Shader "UI/Cozmo/GradientTextureClippingShader"
{
  Properties
  {
    _ClippingSize ("Gradient Size", Vector) = (0.05, 0.05, 0.05, 0.05)
	// These represent each side of the clipping rect in the order:
	// right, top, left, bottom
    _ClippingEnd ("DEV ONLY Clipping End", Vector) = (1.0, 0.1, 0.2, 0.3)
	// offset if using an atlas
    _AtlasUV ("Atlas UV", Vector) = (0, 0, 1.0, 1.0)
  }
  SubShader
  {
    Tags
    { 
      "Queue"="Transparent" 
      "IgnoreProjector"="True" 
      "RenderType"="Transparent" 
      "CanUseSpriteAtlas"="True"
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

      float4 _AtlasUV;
      float4 _ClippingEnd;
      float4 _ClippingSize;
      float4 _Color;

      v2f vert (appdata v)
      {
        v2f o;
        o.vertex = mul(UNITY_MATRIX_MVP, v.vertex);
        o.uv = v.uv;

        // translate atlas UV to sprite UV
        float2 spriteUV = (v.uv.xy - _AtlasUV.xy) / ( _AtlasUV.zw);

        // precalculate denominator
        float4 denominator = 2 / _ClippingSize;
        float4 offset = _ClippingEnd / _ClippingSize;

        // make the top right of the gradient frame
        float2 topRightAlpha = denominator.xy - spriteUV.xy * denominator.xy - offset.xy;

        // make the bottom left of the gradient frame
        float2 bottomLeftAlpha = spriteUV.xy * denominator.zw - offset.zw;

        o.topRightAlpha = topRightAlpha;
        o.bottomLeftAlpha = bottomLeftAlpha;

        return o;
      }
      
      sampler2D _MainTex;

      fixed4 frag (v2f i) : SV_Target
      {
        fixed4 col = tex2D(_MainTex, i.uv);

        float2 minAlpha2 = min(i.topRightAlpha,i.bottomLeftAlpha);
        col.a = min(col.a, 1 - min(minAlpha2.x, minAlpha2.y));
        col.rgb = col.rgb * _Color.rgb;

        return col;
      }
      ENDCG
    }
  }
}
