Shader "Custom/LightningBoltShaderMesh"
{
  Properties
  {
    _MainTex ("Color (RGB) Alpha (A)", 2D) = "white" {}
    _GlowTex ("Glow Color (RGB) Alpha (A)", 2D) = "white" {}
    _TintColor ("Tint Color (RGB)", Color) = (1, 1, 1, 1)
    _GlowTintColor ("Glow Tint Color (RGB)", Color) = (1, 1, 1, 1)
    _InvFade ("Soft Particles Factor", Range(0.01, 3.0)) = 1.0
    _JitterMultiplier ("Jitter Multiplier (Float)", Float) = 0
    }

    SubShader
  {
        Tags { "RenderType"="Transparent" "IgnoreProjector"="True" "Queue"="Transparent+10" "LightMode"="Always" "PreviewType"="Plane"}
    Cull Off
    Lighting Off
    ZWrite Off
    Blend SrcAlpha One
    ColorMask RGBA

    CGINCLUDE
    
    #include "UnityCG.cginc"

    #pragma vertex vert
        #pragma fragment frag
    #pragma multi_compile_particles
    #pragma fragmentoption ARB_precision_hint_fastest
    #pragma glsl_no_auto_normalization
    #pragma multi_compile __ USE_LINE_GLOW

    #if defined(SOFTPARTICLES_ON)

    float _InvFade;

    #endif

    float _JitterMultiplier;

    struct appdata_t
    {
      float4 vertex : POSITION;
      float4 color : COLOR;
      float4 dir : TANGENT;
      float2 texcoord : TEXCOORD0;
      float2 glowModifiers : TEXCOORD1;
      float3 dir2 : NORMAL;
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

    inline float rand(float4 pos)
    {
      return frac(sin(dot(_Time.yzw * pos.xyz, float3(12.9898, 78.233, 45.5432))) * 43758.5453);
    }

    inline fixed4 lerpColor(float4 c)
    {
      // the vertex will fade in, stay at full color, then fade out
      // r = start time
      // g = peak start time
      // b = peak end time
      // a = end time

      // debug
      // return fixed4(1,1,1,1);

      float t = _Time.y;

      if (t < c.g)
      {
        return lerp(fixed4(0, 0, 0, 0), fixed4(1, 1, 1, 1), ((t - c.r) / (c.g - c.r)));
      }
      else if (t < c.b)
      {
        return fixed4(1, 1, 1, 1);
      }
      return lerp(fixed4(1, 1, 1, 1), fixed4(0, 0, 0, 0), ((t - c.b) / (c.a - c.b)));
    }

    /*inline float screenWorldDistance(float4 w1, float4 w2, float d)
    {
      float4 clip1 = mul(UNITY_MATRIX_MVP, w1);
      clip1.xyz /= clip1.w;
      clip1.xyz = (clip1.xyz * 0.5) + 0.5;
      clip1.xy *= _ScreenParams.xy;

      float4 clip2 = mul(UNITY_MATRIX_MVP, w2);
      clip2.xyz /= clip2.w;
      clip2.xyz = (clip2.xyz * 0.5) + 0.5;
      clip2.xy *= _ScreenParams.xy;

      float frustumPercent = d * _CameraTangent * (_ScreenParams.w - 1.0);
      return distance(clip1.xy, clip2.xy) * frustumPercent;
    }*/

    ENDCG

    // glow pass
    Pass
    {
      Name "GlowPass"
      LOD 400

            CGPROGRAM

      fixed4 _GlowTintColor;
      float _GlowIntensityMultiplier;
            float4 _GlowTex_ST;
       sampler2D _GlowTex;

      #if defined(SOFTPARTICLES_ON)

      sampler2D _CameraDepthTexture;

      #endif

            v2f vert(appdata_t v)
            {
        v2f o;
        float dirModifier = (v.texcoord.x - 0.5) + (v.texcoord.x - 0.5);
        float absRadius = abs(v.dir.w);
        float lineWidth = absRadius + absRadius;
        float lineMultiplier = v.glowModifiers.x * lineWidth;
        float jitter = 1.0 + (rand(v.vertex) * _JitterMultiplier * 0.05);

        #if USE_LINE_GLOW

        if (unity_OrthoParams.w == 0.0)
        {
          float3 directionBackwardsNormalized = normalize(v.dir2.xyz);
          float4 directionBackwards = float4(directionBackwardsNormalized * dirModifier * lineMultiplier * 1.5, 0);
          float3 directionToCamera = normalize(_WorldSpaceCameraPos - v.vertex);
          float4 tangent = float4(cross(directionBackwardsNormalized, directionToCamera), 0);
          dirModifier = v.dir.w / absRadius;
          float4 directionSideways = (tangent * lineMultiplier * dirModifier * jitter);
          o.pos = mul(UNITY_MATRIX_MVP, v.vertex + directionBackwards + directionSideways);
        }
        else
        {
          float2 directionBackwardsNormalized = normalize(v.dir2.xy);
          float4 directionBackwards = float4(directionBackwardsNormalized * dirModifier * lineMultiplier, 0, 0);
          float2 tangent = normalize(float2(-v.dir2.y, v.dir2.x));
          dirModifier = v.dir.w / absRadius;
          float4 directionSideways = float4(tangent * lineMultiplier * dirModifier * jitter, 0, 0);
          o.pos = mul(UNITY_MATRIX_MVP, v.vertex + directionBackwards + directionSideways);
        }

        #else

        // billboard square glow
              float texCoordX = (v.texcoord.x - 0.5);
        float4 otherEnd = v.vertex + float4(v.dir2 * -texCoordX * 2.0, 0);
        float4 pivot = (v.vertex + otherEnd) * 0.5;
        //float lineWidth = screenWorldDistance(v.vertex, otherEnd, distance(_WorldSpaceCameraPos, pivot));
        lineWidth = length(v.dir.xyz);
        float particleSize = (lineWidth * v.glowModifiers.x * 2.0);

        // use right and down vectors to billboard
        float4 offset = (particleSize * ((texCoordX * UNITY_MATRIX_IT_MV[0]) + ((v.texcoord.y - 0.5) * -UNITY_MATRIX_IT_MV[1])));
        pivot += offset;
        o.pos = mul(UNITY_MATRIX_MVP, pivot);
        
        #endif

                o.color = (lerpColor(v.color) * _GlowTintColor);
        o.color.a *= v.glowModifiers.y;
        o.texcoord = v.texcoord;
        
        #if defined(SOFTPARTICLES_ON)

                o.projPos = ComputeScreenPos(o.pos);
                COMPUTE_EYEDEPTH(o.projPos.z);

                #endif

                return o; 
            }
      
            fixed4 frag(v2f i) : SV_Target
      {

        #if defined(SOFTPARTICLES_ON)

                float sceneZ = LinearEyeDepth(UNITY_SAMPLE_DEPTH(tex2Dproj(_CameraDepthTexture, UNITY_PROJ_COORD(i.projPos))));
                float partZ = i.projPos.z;
                i.color.a *= saturate (_InvFade * (sceneZ - partZ));

        #endif

        fixed4 c = tex2D(_GlowTex, i.texcoord);
        return (c * i.color);
            }
            ENDCG
        }

    // line pass
    Pass
    {
      Name "LinePass"
      LOD 100

            CGPROGRAM

      fixed4 _TintColor; 
            float4 _MainTex_ST;
       sampler2D _MainTex;

      #if defined(SOFTPARTICLES_ON)

      sampler2D _CameraDepthTexture;

      #endif

            v2f vert(appdata_t v)
            {
                v2f o;

        // face the camera
        float4 worldPos = v.vertex;
        float jitter = 1.0 + (rand(worldPos) * _JitterMultiplier);

        if (unity_OrthoParams.w == 0.0)
        {
          float3 directionToCamera = (_WorldSpaceCameraPos - worldPos);
          float3 tangent = cross(v.dir.xyz, directionToCamera);
          float4 offset = float4(normalize(tangent) * v.dir.w, 0);
          o.pos = mul(UNITY_MATRIX_MVP, worldPos + (offset * jitter));
        }
        else
        {
          float2 tangent = normalize(float2(-v.dir.y, v.dir.x));
          float4 offset = float4(tangent * v.dir.w, 0, 0);
          o.pos = mul(UNITY_MATRIX_MVP, worldPos + (offset * jitter));
        }
        o.texcoord = v.texcoord;
                o.color = ( lerpColor(v.color) * _TintColor);

        #if defined(SOFTPARTICLES_ON)

                o.projPos = ComputeScreenPos(o.pos);
                COMPUTE_EYEDEPTH(o.projPos.z);

                #endif

                return o; 
            }
      
            fixed4 frag(v2f i) : SV_Target
      {

        #if defined(SOFTPARTICLES_ON)

                float sceneZ = LinearEyeDepth(UNITY_SAMPLE_DEPTH(tex2Dproj(_CameraDepthTexture, UNITY_PROJ_COORD(i.projPos))));
                float partZ = i.projPos.z;
                i.color.a *= saturate (_InvFade * (sceneZ - partZ));

        #endif

        fixed4 c = tex2D(_MainTex, i.texcoord);
        return (c * i.color);
            }
            ENDCG
        }
    }
 
    Fallback "Particles/Additive (Soft)"
}