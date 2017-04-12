// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "UI/Cozmo/Blur Filter"
{
    Properties
    {
        [PerRendererData] _MainTex ("Sprite Texture", 2D) = "white" {}
        _Color ("Tint", Color) = (1,1,1,1)
        [MaterialToggle] PixelSnap ("Pixel snap", Float) = 0
        _Spread ("Blur Spread", Vector) = (0.002, 0.002, 0, 0)
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

        GrabPass {
        }

        Pass
        {
        CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #pragma multi_compile DUMMY PIXELSNAP_ON
            #include "UnityCG.cginc"

            struct appdata_t
            {
                float4 vertex   : POSITION;
                float4 color    : COLOR;
                float2 texcoord : TEXCOORD0;
            };

            struct v2f
            {
                float4 vertex   : SV_POSITION;
                fixed4 color    : COLOR;
                half2 texcoord  : TEXCOORD0;
                float4 grabUV   : TEXCOOR1;
            };

            fixed4 _Color;

            v2f vert(appdata_t IN)
            {
                v2f OUT;
                OUT.vertex = UnityObjectToClipPos(IN.vertex);
                OUT.texcoord = IN.texcoord;
                OUT.color = IN.color * _Color;
                #ifdef PIXELSNAP_ON
                OUT.vertex = UnityPixelSnap (OUT.vertex);
                #endif

                #if UNITY_UV_STARTS_AT_TOP
                float scale = -1.0;
                #else
                float scale = 1.0;
                #endif
                OUT.grabUV.xy = (float2(OUT.vertex.x, OUT.vertex.y * scale) + OUT.vertex.w) * 0.5;
                OUT.grabUV.zw = OUT.vertex.zw;

                return OUT;
            }

            float4 _Spread;
            sampler2D _MainTex;
            sampler2D _GrabTexture;

            float4 getColor(float offset, float2 uv, float2 uvDelta, float factor)
            {
              return tex2D(_GrabTexture, uv + offset * uvDelta) * factor;
            }

            float4 verticalFilter(float2 uv) 
            {
              float4 x = float4(0,0,0,0);

              float2 uvOffset = float2(0, _Spread.y);
              // Gauss
      //        x += getColor(-3, uv, uvOffset, 0.006);
      //        x += getColor(-2, uv, uvOffset, 0.061);
      //        x += getColor(-1, uv, uvOffset, 0.242);
      //        x += getColor(0, uv, uvOffset, 0.383);
      //        x += getColor(1, uv, uvOffset, 0.242);
      //        x += getColor(2, uv, uvOffset, 0.061);
      //        x += getColor(3, uv, uvOffset, 0.006);

              //Linear
              x += getColor(-2, uv, uvOffset, 0.2);
              x += getColor(-1, uv, uvOffset, 0.2);
              x += getColor(0, uv, uvOffset, 0.2);
              x += getColor(1, uv, uvOffset, 0.2);
              x += getColor(2, uv, uvOffset, 0.2);

              return x;
            }

            fixed4 frag(v2f IN) : SV_Target
            {
              fixed4 c = tex2D(_MainTex, IN.texcoord);

              if(c.a < 0.25) {
                return float4(0,0,0,0);
              }
              float4 f = verticalFilter(IN.grabUV);
              f.a = 1;
              return f;
            }
        ENDCG
        }

        GrabPass {
        }

        Pass
        {
        CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            #pragma multi_compile DUMMY PIXELSNAP_ON
            #include "UnityCG.cginc"

            struct appdata_t
            {
                float4 vertex   : POSITION;
                float4 color    : COLOR;
                float2 texcoord : TEXCOORD0;
            };

            struct v2f
            {
                float4 vertex   : SV_POSITION;
                fixed4 color    : COLOR;
                half2 texcoord  : TEXCOORD0;
                float4 grabUV   : TEXCOOR1;
            };

            fixed4 _Color;

            v2f vert(appdata_t IN)
            {
                v2f OUT;
                OUT.vertex = UnityObjectToClipPos(IN.vertex);
                OUT.texcoord = IN.texcoord;
                OUT.color = IN.color * _Color;
                #ifdef PIXELSNAP_ON
                OUT.vertex = UnityPixelSnap (OUT.vertex);
                #endif

                #if UNITY_UV_STARTS_AT_TOP
                float scale = -1.0;
                #else
                float scale = 1.0;
                #endif
                OUT.grabUV.xy = (float2(OUT.vertex.x, OUT.vertex.y * scale) + OUT.vertex.w) * 0.5;
                OUT.grabUV.zw = OUT.vertex.zw;

                return OUT;
            }

            float4 _Spread;
            sampler2D _MainTex;
            sampler2D _GrabTexture;

            float4 getColor(float offset, float2 uv, float2 uvDelta, float factor)
            {
              return tex2D(_GrabTexture, uv + offset * uvDelta) * factor;
            }

            float4 horizontalFilter(float2 uv) 
            {
              float4 x = float4(0,0,0,0);

              float2 uvOffset = float2(_Spread.x, 0);
              // Gauss
      //        x += getColor(-3, uv, uvOffset, 0.006);
      //        x += getColor(-2, uv, uvOffset, 0.061);
      //        x += getColor(-1, uv, uvOffset, 0.242);
      //        x += getColor(0, uv, uvOffset, 0.383);
      //        x += getColor(1, uv, uvOffset, 0.242);
      //        x += getColor(2, uv, uvOffset, 0.061);
      //        x += getColor(3, uv, uvOffset, 0.006);

              //Linear
              x += getColor(-2, uv, uvOffset, 0.2);
              x += getColor(-1, uv, uvOffset, 0.2);
              x += getColor(0, uv, uvOffset, 0.2);
              x += getColor(1, uv, uvOffset, 0.2);
              x += getColor(2, uv, uvOffset, 0.2);

              return x;
            }

            fixed4 frag(v2f IN) : SV_Target
            {
              fixed4 c = tex2D(_MainTex, IN.texcoord);
              if(c.a < 0.25) {
                return float4(0,0,0,0);
              }
              float4 f = horizontalFilter(IN.grabUV);
              f.a = 1;
              return f;
            }
        ENDCG
        }
    }
    Fallback "Mobile/Diffuse"
}