Shader "Custom/LightningGlowShader"
{
	Properties
	{
		_MainTex ("Color (RGB) Alpha (A)", 2D) = "white" {}
		_TintColor ("Tint Color (RGB)", Color) = (1, 1, 1, 1)
		_InvFade ("Soft Particles Factor", Range(0.01, 3.0)) = 1.0
    }
    SubShader
	{
        Tags { "RenderType"="Transparent" "IgnoreProjector"="True" "Queue"="Transparent" "LightMode"="Always" "PreviewType"="Plane"}
        LOD 200

        Pass
		{
			Cull Off
            Lighting Off
			ZWrite Off
			Blend SrcAlpha One
			ColorMask RGBA

            CGPROGRAM

			#pragma multi_compile_particles
            #pragma vertex vert
            #pragma fragment frag
			#pragma fragmentoption ARB_precision_hint_fastest

            #include "UnityCG.cginc"

			fixed4 _TintColor;
			#if defined(SOFTPARTICLES_ON)
			float _InvFade;
			#endif

			struct appdata_t
			{
				float4 vertex : POSITION;
				fixed4 color : COLOR;
				float2 texcoord : TEXCOORD0;
			};

            struct v2f
            {
                float2 texcoord : TEXCOORD0;
                fixed4 color : COLOR0;
                float4 pos : SV_POSITION;
				#if defined(SOFTPARTICLES_ON)
                float4 projPos : TEXCOORD1;
                #endif
            };
 
            float4 _MainTex_ST;
 			sampler2D _MainTex;
			#if defined(SOFTPARTICLES_ON)
			sampler2D _CameraDepthTexture;
			#endif

            v2f vert(appdata_t v)
            {
                v2f o;
                o.pos = mul(UNITY_MATRIX_MVP, v.vertex);
				o.texcoord = v.texcoord;
                o.color = (v.color * _TintColor);

				#if defined(SOFTPARTICLES_ON)
                o.projPos = ComputeScreenPos(o.pos);
                COMPUTE_EYEDEPTH(o.projPos.z);
                #endif

                return o; 
            }

			// float rand(float2 co){ return frac(sin(dot(co.xy ,float2(12.9898,78.233))) * 43758.5453);}

            fixed4 frag(v2f i) : SV_Target
			{
				#if defined(SOFTPARTICLES_ON)
                float sceneZ = LinearEyeDepth(UNITY_SAMPLE_DEPTH(tex2Dproj(_CameraDepthTexture, UNITY_PROJ_COORD(i.projPos))));
                float partZ = i.projPos.z;
                i.color.a *= saturate(_InvFade * (sceneZ - partZ));
				#endif

				return (tex2D(_MainTex, i.texcoord) * i.color);
            }
            ENDCG
        }
    }
 
    Fallback "Particles/Additive (Soft)"
}