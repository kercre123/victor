using UnityEngine;
using System.Collections;

namespace Cozmo {
  public static class MaterialPool {

    private static SimpleObjectPool<Material> _sMaterialPool;

    private static SimpleObjectPool<Material> LoadMaterialPool() {
      if (_sMaterialPool == null) {
        _sMaterialPool = new SimpleObjectPool<Material>(CreateMaterial, 1);
      }
      return _sMaterialPool;
    }

    private static Material CreateMaterial() {
      Material newMaterial = new Material(ShaderHolder.Instance.DefaultMaterial);
      newMaterial.hideFlags = HideFlags.HideAndDontSave;
      return newMaterial;
    }

    public static Material GetMaterial(Shader shader, int renderQueue = -1) {
      Material newMaterial = LoadMaterialPool().GetObjectFromPool();
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
