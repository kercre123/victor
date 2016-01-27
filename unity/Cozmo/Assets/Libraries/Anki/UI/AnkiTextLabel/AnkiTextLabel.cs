using System;
using System.Collections.Generic;
using System.Text;
using UnityEngine;
using Cozmo.Util;

namespace Anki.UI {
  /// <summary>
  /// Labels are graphics that display text.
  /// </summary>

  [AddComponentMenu("Anki/Text", 01)]
  public class AnkiTextLabel : UnityEngine.UI.Text {

    [SerializeField]
    private bool _AllUppercase = false;

    private string _LocalizedTextKey = String.Empty;
    private string _DisplayText = String.Empty;

    private object[] _FormattingArgs = null;

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
          if (SetLocalizedText(m_Text)) {
            SetVerticesDirty();
            SetLayoutDirty();
          }
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

    public object[] FormattingArgs {
      get {
        return _FormattingArgs;
      }
      set {
        _FormattingArgs = value;
        Start();
      }
    }

    private bool IsLocalizationKey(string text) {
      return ((text != String.Empty) && (text.Length > 1) && ('#' == text[0]));
    }

    private bool SetLocalizedText(string text) {
      string displayText;
      if (!IsLocalizationKey(text)) {
        m_Text = text;
        displayText = text;
      }
      else {
        _LocalizedTextKey = text.Substring(1);
        displayText = Localization.Get(_LocalizedTextKey);
      }

      if (_FormattingArgs != null) {
        displayText = string.Format(displayText, _FormattingArgs);
      }

      if (_AllUppercase) {
        displayText = displayText.ToUpper();
      }
        
      if (_DisplayText != displayText) {
        _DisplayText = displayText;
        return true;
      }
      return false;
    }

    protected override void Start() {
      if (SetLocalizedText(m_Text)) {
        SetVerticesDirty();
        SetLayoutDirty();      
      }
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

    public override Material GetModifiedMaterial(Material baseMaterial) {
      Material newMaterial = base.GetModifiedMaterial(baseMaterial);
      // newMaterial = baseMaterial;
      return newMaterial;
    }
  }
}