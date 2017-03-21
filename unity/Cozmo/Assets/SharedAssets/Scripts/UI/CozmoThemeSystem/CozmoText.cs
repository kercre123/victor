using System;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

public class CozmoText : Anki.Core.UI.Components.AnkiTextMeshPro {
#if UNITY_EDITOR
  private static bool sLocStringsLoaded = false;
#endif

  private string _LocalizedTextKey = String.Empty;
  private string _DisplayText = String.Empty;
  private string _RenderStringCache;
  private object[] _FormattingArgs = null;

  public new string text {
    get {
      return m_text;
    }
    set {
      if (String.IsNullOrEmpty(value)) {
        if (String.IsNullOrEmpty(m_text))
          return;
        m_text = "";
        _DisplayText = "";
        SetVerticesDirty();
      }
      else if (m_text != value) {
        m_text = value;
        if (SetLocalizedText(m_text)) {
          SetVerticesDirty();
          SetLayoutDirty();
        }
      }
    }
  }

  public object[] FormattingArgs {
    get {
      return _FormattingArgs;
    }
    set {
      _FormattingArgs = value;
      TrySetLocalizedText();
    }
  }

  public virtual string DisplayText {
    get {
      if (String.IsNullOrEmpty(_DisplayText)) {
        return m_text;
      }
      else {
        return _DisplayText;
      }
    }
  }

  private bool SetLocalizedText(string textParam) {
    string displayText;
    if (!Localization.IsLocalizationKey(textParam)) {
      m_text = textParam;
      displayText = textParam;
    }
    else {
      _LocalizedTextKey = textParam.Substring(1);
      displayText = Localization.Get(_LocalizedTextKey);
    }

    if (_FormattingArgs != null) {
      displayText = Localization.GetWithArgs(_LocalizedTextKey, _FormattingArgs);
    }

    if (_DisplayText != displayText) {
      _DisplayText = displayText;
      return true;
    }
    return false;
  }

  protected override void GenerateTextMesh() {
#if UNITY_EDITOR
    if (m_text != DisplayText && !UnityEditor.EditorApplication.isPlaying) {
      if (!sLocStringsLoaded) {
        Localization.LoadStrings();
        sLocStringsLoaded = true;
      }
      SetLocalizedText(m_text);
    }
#endif

    if (m_text != DisplayText) {
      _RenderStringCache = m_text;
      m_text = _DisplayText;
    }

    // parse the new m_text to be rendered
    base.ParseInputText();

    // actually do the render work
    base.GenerateTextMesh();

    // set m_text back to the localized key
    if (!string.IsNullOrEmpty(_RenderStringCache)) {
      m_text = _RenderStringCache;
    }
  }

  protected override void Start() {
    base.Start();
    TrySetLocalizedText();
  }

  private void TrySetLocalizedText() {
    if (SetLocalizedText(m_text)) {
      SetVerticesDirty();
      SetLayoutDirty();
    }
  }

}
