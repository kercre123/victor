Shader "UI/CozmoFacePost"
{
  Properties
  {
      _Color ("Main Color", Color) = (1,1,1,1)
      _MainTex ("Main Texture", 2D) = "white" {}
  }
  SubShader
  {
    Tags
    {
        "Queue"="AlphaTest"
        "IgnoreProjector"="True"
        "RenderType"="Opaque"
        "PreviewType"="Plane"
    }
    ZTest LEqual

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
        float2 screenPos : TEXCOORD1;
      };

      sampler2D _MainTex;
      uniform fixed4 _Color;



      v2f vert (appdata v)
      {
        v2f o;
        o.vertex = mul(UNITY_MATRIX_MVP, v.vertex);
        o.uv = v.uv;
        o.screenPos = ComputeScreenPos(o.vertex);
        return o;
      }
      
      float mod(float x, float d)
      {
        return x - floor(x / d) * d;
      }

      fixed4 frag (v2f i) : SV_Target
      {
        float a = tex2D(_MainTex,i.uv);
        return _Color * a * mod(floor(i.screenPos.y * _ScreenParams.y * 0.5), 2);
      }
      ENDCG
    }
  }
}
