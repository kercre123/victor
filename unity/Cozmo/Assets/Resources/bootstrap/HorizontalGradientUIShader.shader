// Upgrade NOTE: replaced 'mul(UNITY_MATRIX_MVP,*)' with 'UnityObjectToClipPos(*)'

Shader "UI/Cozmo/HorizontalGradientUIShader"
{
  Properties 
  {
    // Unity yells if we don't have _MainTex even if we don't use it
    _MainTex ("Base (RGB)", 2D) = "white" {}
    _LeftColor ("Left Color", Color) = (1,1,1,1)
    _RightColor ("Right Color", Color) = (0,0,0,1)
  }
  SubShader 
  {
    Tags {"Queue"="Transparent" "IgnoreProjector"="True" "RenderType"="Transparent"}
    Cull Off
    ZWrite Off
	Blend SrcAlpha OneMinusSrcAlpha 

    Pass 
    {
      CGPROGRAM
      #pragma vertex vert  
      #pragma fragment frag
      #include "UnityCG.cginc"
      
      fixed4 _LeftColor;
      fixed4 _RightColor;

      struct v2f 
      {
        float4 pos : SV_POSITION;
        fixed4 col : COLOR;
      };

      v2f vert (appdata_full v)
      {
        v2f o;
        o.pos = UnityObjectToClipPos (v.vertex);
        o.col = lerp(_LeftColor,_RightColor, v.texcoord.x );
        return o;
      }
      float4 frag (v2f i) : COLOR 
      {
        return i.col;
      }
      ENDCG
    }
  }
}
