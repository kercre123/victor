using UnityEngine;

namespace UiParticles {
  static class SetPropertyUtility {
    public static bool SetColor(ref Color currentValue, Color newValue) {
      if (Mathf.Abs(currentValue.r - newValue.r) < float.Epsilon
          && Mathf.Abs(currentValue.g - newValue.g) < float.Epsilon
          && Mathf.Abs(currentValue.b - newValue.b) < float.Epsilon
          && Mathf.Abs(currentValue.a - newValue.a) < float.Epsilon) {
        return false;
      }

      currentValue = newValue;
      return true;
    }

    public static bool SetStruct<T>(ref T currentValue, T newValue) where T : struct {
      if (currentValue.Equals(newValue))
        return false;

      currentValue = newValue;
      return true;
    }

    public static bool SetClass<T>(ref T currentValue, T newValue) where T : class {
      if ((currentValue == null && newValue == null) || (currentValue != null && currentValue.Equals(newValue)))
        return false;

      currentValue = newValue;
      return true;
    }
  }
}
