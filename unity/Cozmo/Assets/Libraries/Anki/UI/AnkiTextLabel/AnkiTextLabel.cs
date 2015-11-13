using System;
using System.Collections.Generic;
using System.Text;
using UnityEngine;

namespace Anki.UI {
  /// <summary>
  /// Labels are graphics that display text.
  /// </summary>

  [AddComponentMenu("Anki/Text", 01)]
  public class AnkiTextLabel : UnityEngine.UI.Text {

    private string _LocalizedTextKey = String.Empty;
    private string _DisplayText = String.Empty;

    // m_Text is a protected member that holds the string to display

    public override string text {
      get {
        return m_Text;
      }
      set {
        if (String.IsNullOrEmpty(value)) {
          if (String.IsNullOrEmpty(m_Text))
            return;
          m_Text = "";
          _DisplayText = "";
          SetVerticesDirty();
        }
        else if (m_Text != value) {
          m_Text = value;
          SetLocalizedText(m_Text);

          SetVerticesDirty();
          SetLayoutDirty();
        }
      }
    }

    public virtual string DisplayText {
      get {
        if (String.IsNullOrEmpty(_DisplayText)) {
          return m_Text;
        }
        else {
          return _DisplayText;
        }
      }
    }

    private bool IsLocalizationKey(string text) {
      return ((text != String.Empty) && (text.Length > 1) && ('#' == text[0]));
    }

    private void SetLocalizedText(string text) {
      if (!IsLocalizationKey(text)) {
        m_Text = text;
        _DisplayText = text;
        return;
      }

      _LocalizedTextKey = text.Substring(1);
      _DisplayText = Localization.Get(_LocalizedTextKey);
    }

    protected override void Awake() {
      SetLocalizedText(m_Text);
      SetVerticesDirty();
      SetLayoutDirty();
    }

    //
    // Override UI.Text methods to use LocalizedString
    //
    protected override void OnPopulateMesh(UnityEngine.UI.VertexHelper vh) { 
      #if UNITY_EDITOR
      if (m_Text != DisplayText && !UnityEditor.EditorApplication.isPlaying) {
        SetLocalizedText(m_Text);
      }

      #endif

      string saveText = null;

      if (m_Text != DisplayText) {
        saveText = m_Text;
        m_Text = _DisplayText;
      }

      base.OnPopulateMesh(vh);

      if (null != saveText) {
        m_Text = saveText;
      }
    }

    public override float preferredWidth {
      get {
        var settings = GetGenerationSettings(Vector2.zero);
        return cachedTextGeneratorForLayout.GetPreferredWidth(DisplayText, settings) / pixelsPerUnit;
      }
    }

    public override float preferredHeight {
      get {
        var settings = GetGenerationSettings(new Vector2(rectTransform.rect.size.x, 0.0f));
        return cachedTextGeneratorForLayout.GetPreferredHeight(DisplayText, settings) / pixelsPerUnit;
      }
    }

  }
}