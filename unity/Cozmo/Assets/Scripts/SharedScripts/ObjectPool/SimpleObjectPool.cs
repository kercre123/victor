using UnityEngine;
using System.Collections.Generic;
using System;

public class SimpleObjectPool<T> where T : class {
  private Func<T> _CreateFunction;
  private Queue<T> _InactiveObjects;
  private List<T> _ActiveObjects;
  private Action<T, bool> _ResetFunction;
  private bool _MarkedAsDestroyed;

  public SimpleObjectPool(Func<T> createFunction, Action<T, bool> resetFunction, int initialPoolCount) {
    this._CreateFunction = createFunction;
    this._ResetFunction = resetFunction;
    _InactiveObjects = new Queue<T>();
    _ActiveObjects = new List<T>();
    CreatePoolObjects(initialPoolCount);
    _MarkedAsDestroyed = false;
  }

  public SimpleObjectPool(Func<T> createFunction, int initialPoolCount) : this(createFunction, null, initialPoolCount) {
  }

  public int Count {
    get {
      return _InactiveObjects.Count;
    }
  }

  private void CreatePoolObjects(int numItems) {
    if (_MarkedAsDestroyed) {
      return;
    }

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
    if (this == null) {
      DAS.Error("SimpleObjectPool.ReturnToPool", "This is null");
      return;
    }
    if (_MarkedAsDestroyed) {
      return;
    }

    if (item == null) {
      DAS.Error("SimpleObjectPool.ReturnToPool", "Tried to return a null object to the pool!");
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
    if (_MarkedAsDestroyed) {
      return null;
    }

    T item;
    if (_InactiveObjects.Count == 0) {
      item = _CreateFunction();
      if (item == null) {
        DAS.Error("SimpleObjectPool.GetObjectFromPool", "Tried to create a pool object, but creation failed! " + typeof(T));
        return null;
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
    if (_MarkedAsDestroyed) {
      return;
    }

    while (_ActiveObjects.Count > 0) {
      ReturnToPool(_ActiveObjects[0]);
    }
  }

  public void DestroyPool() {
    if (_MarkedAsDestroyed) {
      return;
    }

    _MarkedAsDestroyed = true;
    _CreateFunction = null;
    _ResetFunction = null;

    MonoBehaviour script;
    foreach (T activeObject in _ActiveObjects) {
      if (activeObject is MonoBehaviour) {
        script = activeObject as MonoBehaviour;
        GameObject.Destroy(script.gameObject);
      }
    }
    _ActiveObjects = null;

    foreach (T inactiveObject in _InactiveObjects) {
      if (inactiveObject is MonoBehaviour) {
        script = inactiveObject as MonoBehaviour;
        GameObject.Destroy(script.gameObject);
      }
    }
    _InactiveObjects = null;
  }
}