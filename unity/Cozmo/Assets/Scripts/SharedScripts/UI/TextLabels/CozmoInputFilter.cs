namespace Cozmo.UI {
  public static class CozmoInputFilter {

    public static bool IsValidInput(char c, bool allowPunctuation, bool allowDigits) {
      bool isValid = false;
      if (IsLetter(c) || IsDash(c) || IsSpace(c)) {
        isValid = true;
      }
      else if (allowPunctuation && IsPunctuation(c)) {
        isValid = true;
      }
      else if (allowDigits && char.IsDigit(c)) {
        isValid = true;
      }
      return isValid;
    }

    private static bool IsLetter(char c) {
      // char.IsLetter includes hiragana, katakana, and kanji;
      // Make sure it's not kanji since our font can't support all of them.
      return char.IsLetter(c) && !IsCJKIdeograph(c);
    }

    // Used to filter out kanji from the default "IsLetter" check since our fonts can't support all kanji.
    // See char.IsLetter spec at https://docs.microsoft.com/en-us/dotnet/api/system.char.isletter?view=netframework-4.7
    private const int kCJKFirstIdeograph = 0x4E00;
    private const int kCJKLastIdeograph = 0x9FC3;
    private static bool IsCJKIdeograph(char c) {
      return (c >= kCJKFirstIdeograph && c <= kCJKLastIdeograph);
    }

    // Hex values taken from: http://www.rikai.com/library/kanjitables/kanji_codes.unicode.shtml
    private const int kFirstHiragana = 0x3040;
    private const int kLastHiragana = 0x309F;
    private static bool IsHiragana(char c) {
      return (c >= kFirstHiragana && c <= kLastHiragana);
    }

    private const int kFirstKatakana = 0x30A0;
    private const int kLastKatakana = 0x30FF;
    private static bool IsKatakana(char c) {
      return (c >= kFirstKatakana && c <= kLastKatakana);
    }

    private static bool IsSpace(char c) {
      return (c == ' ');
    }

    // Less forgiving than char.IsPunctuation() by design
    private static bool IsPunctuation(char c) {
      return c == '.' || c == ';' || c == '\'' || c == ',' || c == '?' || c == '!' || c == ':';
    }

    // Dash is for French support
    private static bool IsDash(char c) {
      return c == '-';
    }
  }
}