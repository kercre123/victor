using UnityEngine;
using System.Collections;

namespace Cozmo {
  public static class MaterialPool {

    private static SimpleObjectPool<Material> _sMaterialPool;

    private static SimpleObjectPool<Material> GetMaterialPool() {
      if (_sMaterialPool == null) {
        _sMaterialPool = new SimpleObjectPool<Material>(CreateMaterial, 1);
      }
      return _sMaterialPool;
    }

    private static Material CreateMaterial() {
      return new Material(Shader.Find("Diffuse")){ hideFlags = HideFlags.HideAndDontSave };
    }

    public static Material GetMaterial(Shader shader, int renderQueue = -1) {
      Material newMaterial = GetMaterialPool().GetObjectFromPool();
      newMaterial.shader = shader;
      #if UNITY_EDITOR
      newMaterial.name = shader.name + " Material (Generated)";
      #endif
      newMaterial.renderQueue = renderQueue;
      return newMaterial;
    }

    public static void ReturnMaterial(Material returnMaterial) {
      if (_sMaterialPool != null && returnMaterial != null) {
        _sMaterialPool.ReturnToPool(returnMaterial);
      }
    }
  }
}
