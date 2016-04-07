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
    }
  }

  public void CreateListener() {
    if (_SosTcpClient != null) {
      Debug.LogWarning("SOS Listener already exists");
      return;
    }
    _SosTcpClient = new SOSTCPClient();
    _SosTcpClient.StartListening();
  }

  public void CleanUp() {
    if (_SosTcpClient != null) {
      _SosTcpClient.CleanUp();
    }
  }

  void OnDestroy() {
    CleanUp();
  }

  void Update() {
    if (_SosTcpClient != null) {
      _SosTcpClient.ProcessMessages();
    }
  }

  public void RegisterListener(System.Action<string> listener) {
    if (_SosTcpClient != null) {
      _SosTcpClient.OnNewSOSLogEntry += listener;
    }
  }
}
