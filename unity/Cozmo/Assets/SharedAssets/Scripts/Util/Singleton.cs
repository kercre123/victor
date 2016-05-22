using System;

// Basic singleton, has a public readonly field, no thread protection
public static class Singleton<T> where T : class, new() {
  public static readonly T Instance = new T();
}

// Thread Static singleton creates an instance of the the object per thread
public static class ThreadStaticSingleton<T> where T : class, new() {
  [ThreadStatic]
  private static T _Instance;
  public static T Instance {
    get {
      if (_Instance == default(T)) {
        _Instance = new T();
      }
      return _Instance;
    }
  }
}

public static class Empty<T> {
  public static readonly T[] Instance = new T[0];
}