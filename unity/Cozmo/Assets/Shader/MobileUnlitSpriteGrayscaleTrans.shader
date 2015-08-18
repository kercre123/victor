Shader "Mobile/Unlit/Sprite Grayscale Transparent" {
 
    Properties {
       _Opacity ("Opacity", Range(0,1)) = 1
       _Desaturation ("Desaturation", Range(0,1)) = 1
       [PerRendererData] _MainTex ("Texture", 2D) = ""
    }
   
    SubShader {
        Tags { "Queue"="Transparent" }
        ZWrite Off
        Blend SrcAlpha OneMinusSrcAlpha
        Pass {
            CGPROGRAM
            #pragma vertex vert
            #pragma fragment frag
            struct v2f {
                float4 position : SV_POSITION;
                float2 uv_mainTex : TEXCOORD;
            };
           
            float _Opacity;
            float _Desaturation;
           
            uniform float4 _MainTex_ST;
            v2f vert (float4 position : POSITION, float2 uv : TEXCOORD0) {
                v2f o;
                o.position = mul (UNITY_MATRIX_MVP, position);
                o.uv_mainTex = uv * _MainTex_ST.xy + _MainTex_ST.zw;
                return o;
            }
           
            uniform sampler2D _MainTex;
            fixed4 frag(float2 uv_mainTex : TEXCOORD) : COLOR {
                fixed4 mainTex = tex2D (_MainTex, uv_mainTex);
                fixed4 fragColor;

                fragColor.rgb = dot (mainTex.rgb, fixed3 (.222, .707, .071));
                fragColor.rgb = lerp(mainTex.rgb, fragColor.rgb, _Desaturation);
                fragColor.a = mainTex.a * _Opacity;
                return fragColor;
            }
            ENDCG
        }
    }
}