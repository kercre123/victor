using UnityEngine;
using System.Collections;

public class AnimateUVs : MonoBehaviour
{
  public int materialIndex = 0;
  public Vector2 uvAnimationRate = new Vector2(1.0f, 0.0f);
  public string textureName1 = "_MainTex";
  public string textureName2 = "_BumpMap";

  Vector2 uvOffset = Vector2.zero;
  
  Renderer rend;

  void Awake()
  {
    rend = GetComponent<Renderer>();
  }

  void LateUpdate()
  {
    uvOffset += (uvAnimationRate * Time.deltaTime);
    if(rend.enabled)
    {
      rend.materials[materialIndex].SetTextureOffset(textureName1, uvOffset);
      rend.materials[materialIndex].SetTextureOffset(textureName2, uvOffset);
    }
  }
}
