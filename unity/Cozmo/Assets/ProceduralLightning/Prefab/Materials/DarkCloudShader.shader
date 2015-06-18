Shader "Custom/CloudShader"
{
    Properties
	{
		_MainTex ("Color (RGB) Alpha (A)", 2D) = "gray" {}
		_TintColor ("Tint Color (RGB)", Color) = (1, 1, 1, 1)
		_PointSpotLightMultiplier ("Point/Spot Light Multiplier", Range (0, 10)) = 2
		_DirectionalLightMultiplier ("Directional Light Multiplier", Range (0, 10)) = 1
		_EmissiveColor ("Emissive Color (RGB)", Color) = (0.1, 0.1, 0.1, 1)
		_AmbientLightMultiplier ("Ambient light multiplier", Range (0, 2)) = 0.1
    }
    SubShader
	{
        Tags { "RenderType"="Transparent" "IgnoreProjector"="True" "Queue"="Transparent" "LightMode"="Vertex" }
		Blend SrcAlpha OneMinusSrcAlpha
        LOD 100
 
        Pass
		{
			Cull Off
            Lighting On     
			ZWrite Off 
				 
            CGPROGRAM

            #pragma vertex vert
            #pragma fragment frag
			#pragma fragmentoption ARB_precision_hint_fastest
			#pragma multi_compile_particles

            #include "UnityCG.cginc"

			fixed4 _TintColor;
			float _DirectionalLightMultiplier;
			float _PointSpotLightMultiplier;
			fixed3 _EmissiveColor;
			fixed _AmbientLightMultiplier;

			struct appdata_t
			{
				float4 vertex : POSITION;
				fixed4 color : COLOR;
				float2 texcoord : TEXCOORD0;
				float3 normal : NORMAL;
			};

			float3 ApplyLight(int index, float3 lightColor, float3 viewPos, float3 viewNormal)
			{
				float4 lightPos = unity_LightPosition[index];
				fixed3 currentLightColor = unity_LightColor[index].rgb;

				if (lightPos.w == 0)
				{
					// directional light, the lightPos is actually the direction of the light
					float diff = max(0, dot(viewNormal, -normalize(lightPos)));
					return lightColor + (currentLightColor * diff * _DirectionalLightMultiplier);
				}
				else
				{
					float3 toLight = lightPos - viewPos.xyz;
	                float lengthSq = dot(toLight, toLight);
	                float atten = 1.0 / (1.0 + (lengthSq * unity_LightAtten[index].z));
					float diff = max(0, dot (viewNormal, normalize(toLight)));
					return lightColor + (currentLightColor * diff * atten * _PointSpotLightMultiplier);
				}
			}
 
            fixed4 ApplyLightsToVertex(float4 vertex, float3 normal, fixed4 color)
            {
                float3 viewPos = mul(UNITY_MATRIX_MV, vertex).xyz;
                float3 viewNormal = mul((float3x3)UNITY_MATRIX_IT_MV, normal);
                float3 lightColor = _EmissiveColor + (UNITY_LIGHTMODEL_AMBIENT.xyz * _AmbientLightMultiplier);

				lightColor = ApplyLight(0, lightColor, viewPos, viewNormal);
				lightColor = ApplyLight(1, lightColor, viewPos, viewNormal);
				lightColor = ApplyLight(2, lightColor, viewPos, viewNormal);
				lightColor = ApplyLight(3, lightColor, viewPos, viewNormal);

                return fixed4(lightColor, 1) * color;
            }
 
            struct v2f
            {
                half2 uv_MainTex : TEXCOORD0;
                fixed4 color : COLOR0;
                float4 pos : SV_POSITION;
            };
 
            float4 _MainTex_ST;
			sampler2D _MainTex;
			 
            v2f vert(appdata_t v)
            {
                v2f o;
                o.pos = mul(UNITY_MATRIX_MVP, v.vertex);
                o.uv_MainTex = TRANSFORM_TEX(v.texcoord, _MainTex);
                o.color = ApplyLightsToVertex(v.vertex, v.normal, v.color) * _TintColor;
                return o; 
            }
  
            fixed4 frag (v2f i) : COLOR {
            
                // base texture
                return tex2D(_MainTex, i.uv_MainTex) * i.color;
            }
            ENDCG
        }
    }
 
    Fallback "Particles/VertexLit"
}