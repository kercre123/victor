
public class TextToSpeech {

  // Maximum text length AFTER utf8 encoding
  public const int MAX_ENCODED_TEXTLEN = 255;

  // Trim text string to ensure that encoded representation fits within message size
  public static string Trim(string text) {
    while (System.Text.Encoding.UTF8.GetByteCount(text) > MAX_ENCODED_TEXTLEN) {
      text = text.Remove(text.Length - 1);
    }
    return text;
  }

}

