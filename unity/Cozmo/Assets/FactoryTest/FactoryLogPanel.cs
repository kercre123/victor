using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class FactoryLogPanel : MonoBehaviour {
  [SerializeField]
  RectTransform _LogTextList;

  [SerializeField]
  private UnityEngine.UI.ScrollRect _TextScrollRect;

  [SerializeField]
  private UnityEngine.UI.Button _CloseButton;

  [SerializeField]
  private UnityEngine.UI.Text _TextPrefab;

  [SerializeField]
  private UnityEngine.UI.Button _CopyLogsButton;

  private List<string> _LogQueueCache = null;

  void Start() {
    _CloseButton.onClick.AddListener(ClosePanel);
    _CopyLogsButton.onClick.AddListener(CopyLogs);
  }

  public void UpdateLogText(List<string> logQueue) {
    _LogQueueCache = logQueue;
    for (int i = 0; i < _LogTextList.childCount; ++i) {
      GameObject.Destroy(_LogTextList.GetChild(i).gameObject);
    }

    for (int i = 0; i < logQueue.Count; ++i) {
      GameObject textInstance = GameObject.Instantiate(_TextPrefab.gameObject);
      textInstance.transform.SetParent(_LogTextList, false);
      textInstance.GetComponent<UnityEngine.UI.Text>().text = logQueue[i];
    }

    _TextScrollRect.verticalNormalizedPosition = 0;
  }

  private void CopyLogs() {
    if (_LogQueueCache != null) {
      string logFull = "";
      for (int i = 0; i < _LogQueueCache.Count; ++i) {
        logFull += _LogQueueCache[i] + "\n";
      }
      GUIUtility.systemCopyBuffer = logFull;
      CozmoBinding.SendToClipboard(logFull);
    }
  }

  private void ClosePanel() {
    GameObject.Destroy(gameObject);
  }
}
