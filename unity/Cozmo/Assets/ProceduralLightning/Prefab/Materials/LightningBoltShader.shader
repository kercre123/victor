Shader "Custom/LightningBoltShader"
{
	Properties
	{
		_MainTex ("Color (RGB) Alpha (A)", 2D) = "white" {}
		_TintColor ("Tint Color (RGB)", Color) = (1, 1, 1, 1)
    }
    SubShader
	{
        Tags { "RenderType"="Transparent" "IgnoreProjector"="True" "Queue"="Transparent+10" "LightMode"="Always" "PreviewType"="Plane"}
        LOD 100

        Pass
		{
			Cull Back
            Lighting Off
			ZWrite Off
			Blend SrcAlpha One
			ColorMask RGBA

            CGPROGRAM

            #pragma vertex vert
            #pragma fragment frag
			#pragma fragmentoption ARB_precision_hint_fastest

            #include "UnityCG.cginc"

			fixed4 _TintColor;

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
            };
 
            float4 _MainTex_ST;
 			sampler2D _MainTex;

            v2f vert(appdata_t v)
            {
                v2f o;
                o.pos = mul(UNITY_MATRIX_MVP, v.vertex);
				o.texcoord = v.texcoord;
                o.color = (v.color * _TintColor);
                return o; 
            }

			// float rand(float2 co){ return frac(sin(dot(co.xy ,float2(12.9898,78.233))) * 43758.5453);}

            fixed4 frag(v2f i) : SV_Target
			{
				fixed4 c = tex2D(_MainTex, i.texcoord);
				return (c * i.color);
            }
            ENDCG
        }
	}
 
    Fallback "Particles/Additive (Soft)"
}