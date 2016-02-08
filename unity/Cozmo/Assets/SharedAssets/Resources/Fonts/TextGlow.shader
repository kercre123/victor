Shader "UI/Cozmo/Text Glow"
{
    Properties
    {
        [PerRendererData] _MainTex ("Texture", 2D) = "white" {}
        _Color ("Tint", Color) = (1,1,1,1)
        [MaterialToggle] PixelSnap ("Pixel snap", Float) = 0
    }

    SubShader
    {
        Tags
        { 
            "Queue"="Transparent" 
            "IgnoreProjector"="True" 
            "RenderType"="Transparent" 
            "PreviewType"="Plane"
            "CanUseSpriteAtlas"="True"
        }

        Cull Off
        Lighting Off
        ZWrite Off
        Fog { Mode Off }
        Blend One OneMinusSrcAlpha

        Pass
        {
        CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #pragma multi_compile DUMMY PIXELSNAP_ON
            #pragma glsl_no_auto_normalization
            #include "UnityCG.cginc"

            struct appdata_t
            {
                float4 vertex   : POSITION;
                float4 color    : COLOR;
                float2 uv       : TEXCOORD0;
                float2 uv1      : TEXCOORD1;
                float4 tangent  : TANGENT;
            };

            struct v2f
            {
                float4 vertex   : SV_POSITION;
                fixed4 color    : COLOR;
                float2 uv : TEXCOORD0;
                float2 uv1: TEXCOORD1;
                float4 tangent  : TANGENT;
            };

            fixed4 _Color;

            v2f vert(appdata_t IN)
            {
                v2f OUT;
                OUT.vertex = mul(UNITY_MATRIX_MVP, IN.vertex);
                OUT.uv = IN.uv;
                OUT.uv1 = IN.uv1;
                OUT.tangent = IN.tangent;
                OUT.color = IN.color * _Color;
                #ifdef PIXELSNAP_ON
                OUT.vertex = UnityPixelSnap (OUT.vertex);
                #endif
                return OUT;
            }

            sampler2D _MainTex;

            float getColor(float2 uv, float2 uvDelta, float4 minMaxUV, float factor)
            {
              return tex2D(_MainTex, min(max(uv + uvDelta, minMaxUV.xy), minMaxUV.zw)).a * factor;
            }

            float Glow(float2 uv, float2 delta, float4 minMaxUV) 
            {
              float x = 0;

              //Linear
              // skip our actual uv, since we can assume we display the actual text on top of it.
              //x += getColor(uv, float2(0,0), minMaxUV, 0.2);
              x += getColor(uv, float2(delta.x, delta.y), minMaxUV, 0.2);
              x += getColor(uv, float2(-delta.x, delta.y), minMaxUV, 0.2);
              x += getColor(uv, float2(delta.x, -delta.y), minMaxUV, 0.2);
              x += getColor(uv, float2(-delta.x, -delta.y), minMaxUV, 0.2);

              return x;
            }

            fixed4 frag(v2f IN) : SV_Target
            {              
              float4 c = _Color * IN.color;

              return Glow(IN.uv, IN.uv1 * 2, IN.tangent) * c;
            }
        ENDCG
        }
    }
}