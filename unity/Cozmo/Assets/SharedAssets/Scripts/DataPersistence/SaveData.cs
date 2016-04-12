using UnityEngine;
using System.Collections;
using System.Collections.Generic;

namespace DataPersistence {
  public class SaveData {
    public PlayerProfile DefaultProfile;

    public SaveData() { 
      DefaultProfile = new PlayerProfile();
    }
  }
}