using UnityEngine;
using System.Collections.Generic;
using System;

public class SimpleObjectPool<T> where T : class {
  private readonly Func<T> _CreateFunction;
  private readonly Queue<T> _InactiveObjects;
  private readonly List<T> _ActiveObjects;
  private readonly Action<T, bool> _ResetFunction;

  public SimpleObjectPool(Func<T> createFunction, Action<T, bool> resetFunction, int initialPoolCount) {
    this._CreateFunction = createFunction;
    this._ResetFunction = resetFunction;
    _InactiveObjects = new Queue<T>();
    _ActiveObjects = new List<T>();
    CreatePoolObjects(initialPoolCount);
  }

  public SimpleObjectPool(Func<T> createFunction, int initialPoolCount) : this(createFunction, null, initialPoolCount) {
  }

  public int Count {
    get {
      return _InactiveObjects.Count;
    }
  }

  private void CreatePoolObjects(int numItems) {
    for (var i = 0; i < numItems; i++) {
      var item = _CreateFunction();
      if (item != null) {
        _InactiveObjects.Enqueue(item);
      }
      else {
        DAS.Error("SimpleObjectPool.CreatePoolObjects", "Tried to create a pool object, but creation failed! " + typeof(T));
      }
    }
  }

  public void ReturnToPool(T item) {
    if (item == null) {
      DAS.Error(this, "Tried to return a null object to the pool!");
      throw new ArgumentNullException();
    }

    bool isDestroyedObject = false;
    if (item is MonoBehaviour) {
      MonoBehaviour script = item as MonoBehaviour;
      isDestroyedObject = (script.gameObject == null || script.transform == null);
    }
    if (!isDestroyedObject) {
      if (_ResetFunction != null) {
        _ResetFunction(item, false);
      }
      _InactiveObjects.Enqueue(item);
    }

    if (_ActiveObjects.Contains(item)) {
      _ActiveObjects.Remove(item);
    }
  }

  public T GetObjectFromPool() {
    T item;
    if (_InactiveObjects.Count == 0) {
      item = _CreateFunction();
      if (item == null) {
        DAS.Error("SimpleObjectPool.GetObjectFromPool", "Tried to create a pool object, but creation failed! " + typeof(T));
      }
    }
    else {
      item = _InactiveObjects.Dequeue();
    }
    if (_ResetFunction != null) {
      _ResetFunction(item, true);
    }
    _ActiveObjects.Add(item);
    return item;
  }

  public void ReturnAllObjectsToPool() {
    while (_ActiveObjects.Count > 0) {
      ReturnToPool(_ActiveObjects[0]);
    }
  }
}