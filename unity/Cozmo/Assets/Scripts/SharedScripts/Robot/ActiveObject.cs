using UnityEngine;
using Anki.Cozmo;
using G2U = Anki.Cozmo.ExternalInterface;
using U2G = Anki.Cozmo.ExternalInterface;

public class ActiveObject : ObservableObject {
  public ActiveObject(int objectID, uint factoryID, ObjectFamily objectFamily, ObjectType objectType)
  : base(objectID, factoryID, objectFamily, objectType) {

  }
}
