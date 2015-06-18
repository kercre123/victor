Shader "Custom/LightningBoltShaderMeshNoGlow"
{
	Properties
	{
		_MainTex ("Color (RGB) Alpha (A)", 2D) = "white" {}
		_TintColor ("Tint Color (RGB)", Color) = (1, 1, 1, 1)
		_InvFade ("Soft Particles Factor", Range(0.01, 3.0)) = 1.0
		_JitterMultiplier ("Jitter Multiplier (Float)", Float) = 0
    }

    SubShader
	{
        Tags { "RenderType"="Transparent" "IgnoreProjector"="True" "Queue"="Transparent+10" "LightMode"="Always" "PreviewType"="Plane"}
		UsePass "Custom/LightningBoltShaderMesh/LINEPASS"
    }
 
    Fallback "Particles/Additive (Soft)"
}