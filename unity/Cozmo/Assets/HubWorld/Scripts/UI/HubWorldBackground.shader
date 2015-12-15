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
            // our the inital glow will be linear falloff of 1 at the vertical 
            // center of the screen to 0 at the top .25 and bottom .25
            float glowAlpha = saturate(min(uv.y, 1 - uv.y) * 4 - 1);

            // make glow falloff quadratic
            glowAlpha *= glowAlpha;

            // we want horizontal glow to be even on both sides, 
            // so get a value 1 in the middle and 0 on both sides
            float horizontalFlow = min(uv.x, 1 - uv.x) * 2;

            // our loop is 10 seconds
            float scaledTime = _Time.y / 10;

            // we want the glow to start and end offscreen
            float timeFactor = (scaledTime - floor(scaledTime)) * 2 - 0.5;

            // get a horizontal stripe of glow where our time currently is
            float horizontalGlowAlpha = saturate(min(timeFactor - horizontalFlow + 0.4, 
                                                     horizontalFlow + 0.4 - timeFactor) * 5);

            // multiplying glowAlpha * horizontalGlowAlpha gets a small region
            // of glow. We want to keep our base glow under the wires though so add it in
            glowAlpha += glowAlpha * horizontalGlowAlpha;

            // get the glow color from our parameter
            fixed4 glow = glowAlpha * _GlowColor * _GlowColor.a;

            // add the glow to the circuit alpha. Multiply by circuit alpha because 
            // we only want to glow on the circuit.
            circuit.a += circuit.a * glow.a * 0.2;

            // now blend the circuit with the glow color
            circuit.rgb = circuit.rgb * (1 - glow.a) + glow.a * glow.rgb;

            return circuit;
        }
 
        void surf (Input IN, inout SurfaceOutput o)
        {
            float2 delta = (IN.uv_MainTex * 2 - float2(1, 1)) * _GradientScale.xy;

            float deltaLen = length(delta);

            // circular linear falloff from the center of the screen
            float gradientAlpha = saturate(1 - deltaLen);

            fixed4 gradient = _GradientColor * gradientAlpha;

            // alpha blend the background behind the gradient
            fixed4 background = gradient * gradient.a + (1 - gradient.a) * _BackgroundColor;

            fixed4 circuit = tex2D(_MainTex, IN.uv_MainTex) * _Color;

            circuit = applyGlow(IN.uv_MainTex, circuit);

            // alpha blend the background + gradient behind the circuit
            fixed3 result = circuit.rgb * circuit.a + (1 - circuit.a) * background.rgb;

            o.Albedo = result;
        }
        ENDCG
    }
}