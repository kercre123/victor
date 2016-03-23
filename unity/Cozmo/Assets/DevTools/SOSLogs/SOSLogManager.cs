using UnityEngine;
using System.Collections;

public class SOSLogManager : MonoBehaviour {

  public static SOSLogManager Instance { get; private set; }

  private SOSTCPClient _SosTcpClient = null;

  private void OnEnable() {
    if (Instance != null && Instance != this) {
      Destroy(gameObject);
      return;
    }
    else {
      Instance = this;
      DontDestroyOnLoad(gameObject);
    }
  }

  public void CreateListener() {
    _SosTcpClient = new SOSTCPClient();
    _SosTcpClient.StartListening();
  }

  void OnDestroy() {
    if (_SosTcpClient != null) {
      _SosTcpClient.CleanUp();
    }
  }
}
