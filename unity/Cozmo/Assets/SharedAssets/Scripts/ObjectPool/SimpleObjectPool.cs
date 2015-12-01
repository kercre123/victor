using UnityEngine;
using System.Collections.Generic;
using System;

public class SimpleObjectPool<T> where T : class {
  private readonly Func<T> _CreateFunction;
  private readonly Queue<T> _InactiveObjects;
  private readonly List<T> _ActiveObjects;
  private readonly Action<T> _ResetFunction;

  public SimpleObjectPool(Func<T> createFunction, Action<T> resetFunction, int initialPoolCount) {
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
      _InactiveObjects.Enqueue(item);
    }
  }

  public void ReturnToPool(T item) {
    if (item == null) {
      DAS.Error(this, "Tried to return a null object to the pool!");
      throw new ArgumentNullException();
    }
    if (_ResetFunction != null) {
      _ResetFunction(item);
    }
    if (_ActiveObjects.Contains(item)) {
      _ActiveObjects.Remove(item);
    }
    _InactiveObjects.Enqueue(item);
  }

  public T GetObjectFromPool() {
    T item;
    if (_InactiveObjects.Count == 0) {
      item = _CreateFunction();
    }
    else {
      item = _InactiveObjects.Dequeue();
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