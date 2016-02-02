Shader "UI/Skew"
{
  Properties
  {
    _MainTex ("Texture", 2D) = "white" {}
    _SkewFactor ("Skew Factor", Float) = 0.25

    // required for UI.Mask
    _StencilComp ("Stencil Comparison", Float) = 8
    _Stencil ("Stencil ID", Float) = 0
    _StencilOp ("Stencil Operation", Float) = 0
    _StencilWriteMask ("Stencil Write Mask", Float) = 255
    _StencilReadMask ("Stencil Read Mask", Float) = 255
    _ColorMask ("Color Mask", Float) = 15
  }
  SubShader
  {
    // No culling or depth
    Cull Off ZWrite Off ZTest Always
    Tags
    {
      "Queue"="AlphaTest"
      "IgnoreProjector"="True"
      "RenderType"="Transparent"
      "PreviewType"="Plane"
    }

    // required for UI.Mask
    Stencil
    {
      Ref [_Stencil]
      Comp [_StencilComp]
      Pass [_StencilOp] 
      ReadMask [_StencilReadMask]
      WriteMask [_StencilWriteMask]
    }
    ColorMask [_ColorMask]

    Pass {
      Blend SrcAlpha OneMinusSrcAlpha

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
        float4 vertex : SV_POSITION;
        float4 color : COLOR;
      };

      v2f vert (appdata v)
      {
        v2f o;
        o.vertex = mul(UNITY_MATRIX_MVP, v.vertex);
        o.uv = v.uv;
        o.color = v.color;
        return o;
      }

      sampler2D _MainTex;
      float _SkewFactor;

      fixed4 frag (v2f i) : SV_Target
      {
        // do skew translation on uvs
        float2 uv = i.uv;
        uv = float2(uv.x, (uv.y - 0.5) * (1 + uv.x * _SkewFactor) + 0.5);

        // use linear falloff to nicely blend the very edges of the image
        // to get a nice AA effect.
        float alpha = saturate(min(uv.y * 500, (1 - uv.y) * 500));

        fixed4 col = tex2D(_MainTex, uv) * i.color;
        col.a *= alpha;
        return col;
      }
      ENDCG
    }
 }
}
