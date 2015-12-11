Shader "Sprites/HubworldBackground"
{
    Properties
    {
        _MainTex ("Main Texture", 2D) = "clear" {}
        _Color ("Tint", Color) = (1,1,1,1)
        _GradientColor ("Gradient Tint", Color) = (1,1,1,1)
        _BackgroundColor ("Background Color", Color) = (1,1,1,1)
        _GradientScale ("GradientScale", Vector) = (1, 1, 1, 1)
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
 
        LOD 200
        Cull Off
        Lighting On
        ZWrite Off
 
        CGPROGRAM
        #pragma surface surf Lambert addshadow
 
        sampler2D _MainTex;
        fixed4 _Color;
        fixed4 _GradientColor;
        fixed4 _BackgroundColor;
        float4 _GradientScale;
 
        struct Input
        {
            float2 uv_MainTex;
        };
 
        void surf (Input IN, inout SurfaceOutput o)
        {
            float2 d = (IN.uv_MainTex * 2 - float2(1, 1)) * _GradientScale.xy;

            float2 d2 = d * d;

            float a = saturate(1 - sqrt(d2.x  + d2.y));

            fixed4 g = _GradientColor * a;

            fixed4 b = g * g.a + (1 - g.a) * _BackgroundColor;

            fixed4 c = tex2D(_MainTex, IN.uv_MainTex) * _Color;

            fixed3 r = c.rgb * c.a + (1 - c.a) * b.rgb;

            o.Albedo = r;
            o.Alpha = 1;
        }
        ENDCG
    }
}