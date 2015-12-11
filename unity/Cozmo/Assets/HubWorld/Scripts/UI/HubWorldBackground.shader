Shader "Sprites/HubworldBackground"
{
    Properties
    {
        _MainTex ("Main Texture", 2D) = "clear" {}
        _Color ("Tint", Color) = (1,1,1,1)
        _GradientColor ("Gradient Tint", Color) = (1,1,1,1)
        _BackgroundColor ("Background Color", Color) = (1,1,1,1)
        _GlowColor ("Glow Color", Color) = (1,1,1,1)
        _GradientScale ("GradientScale", Vector) = (1,1,1,1)
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
        fixed4 _GlowColor;
        float4 _GradientScale;

        struct Input
        {
            float2 uv_MainTex;
        };

        // TODO: Turn off on low tier devices
        fixed4 applyGlow(float2 uv, fixed4 circuit) 
        {
            float glowAlpha = saturate(min(uv.y, 1 - uv.y) * 4 - 1);

            glowAlpha *= glowAlpha;

            float horizontalFlow = min(uv.x, 1 - uv.x) * 2;

            float sint = abs(_SinTime.x) * 2 - 1;

            float horizontalGlowAlpha = saturate(min(sint - horizontalFlow + 0.4, horizontalFlow + 0.4 - sint) * 5);

            glowAlpha += glowAlpha * horizontalGlowAlpha;

            fixed4 glow = glowAlpha * _GlowColor * _GlowColor.a;

            circuit.a += circuit.a * glow.a * 0.2;
            circuit.rgb = circuit.rgb * (1 - glow.a) + glow.a * glow.rgb;

            return circuit;
        }
 
        void surf (Input IN, inout SurfaceOutput o)
        {
            float2 delta = (IN.uv_MainTex * 2 - float2(1, 1)) * _GradientScale.xy;

            float2 deltaSqr = delta * delta;

            float gradientAlpha = saturate(1 - sqrt(deltaSqr.x  + deltaSqr.y));

            fixed4 gradient = _GradientColor * gradientAlpha;

            fixed4 background = gradient * gradient.a + (1 - gradient.a) * _BackgroundColor;

            fixed4 circuit = tex2D(_MainTex, IN.uv_MainTex) * _Color;

            circuit = applyGlow(IN.uv_MainTex, circuit);

            fixed3 result = circuit.rgb * circuit.a + (1 - circuit.a) * background.rgb;

            o.Albedo = result;
        }
        ENDCG
    }
}