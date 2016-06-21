using UnityEngine;
using System.Collections;
using System.Collections.Generic;

#if FACTORY_TEST

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

  public string LogFilter;

  void Start() {
    _CloseButton.onClick.AddListener(ClosePanel);
    _CopyLogsButton.onClick.AddListener(CopyLogs);
  }

  public void UpdateLogText(List<string> logQueue) {
    _LogQueueCache = logQueue;
    for (int i = 0; i < _LogTextList.childCount; ++i) {
      GameObject.Destroy(_LogTextList.GetChild(i).gameObject);
    }

    int logCount = 0;
    int logIndex = 0;

    string[] filters = LogFilter.Split(',');

    while (logCount < 80 && logIndex < logQueue.Count) {
      if (Contains(logQueue[logIndex], filters)) {
        GameObject textInstance = GameObject.Instantiate(_TextPrefab.gameObject);
        textInstance.transform.SetParent(_LogTextList, false);
        textInstance.GetComponent<UnityEngine.UI.Text>().text = logQueue[logIndex];
        logCount++;
      }
      logIndex++;
    }

    _TextScrollRect.verticalNormalizedPosition = 0;
  }

  private bool Contains(string original, string[] filters) {
    for (int i = 0; i < filters.Length; ++i) {
      if (original.Contains(filters[i])) {
        return true;
      }
    }
    return false;
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

#endif