using System;
using System.Text;

namespace Anki {

  public static class Util {
    // Reads text from a file, replacing \uXXXX or \UXXXX escapes with the appropriate
    // Unicode character.
    // based on: http://stackoverflow.com/a/9738409/217431
    // NOTE: This is slow, but avoids using Regex which is not available in all Unity C# libs
    public static string ReadUnicodeEscapedString(System.IO.TextReader reader) {
      var sb = new StringBuilder(32);
      bool q = false;
      while (true) {
        int chrR = reader.Read();

        if (chrR == -1)
          break;
        var chr = (char)chrR;

        if (!q) {
          if (chr == '\\') {
            q = true;
            continue;
          }
          sb.Append(chr);
        }
        else {
          switch (chr) {
          case 'u':
          case 'U':
            var hexb = new char[4];
            reader.Read(hexb, 0, 4);
            try {
              chr = (char)System.Convert.ToInt32(new string(hexb), 16);
            }
            catch (OverflowException) {
              DAS.Error("Anki.Util.ReadUnicodeEscapedString.OverflowException",
                        String.Format("Unable to convert '0x{0}' to an integer.", chr));
            }
            catch (FormatException) {
              DAS.Error("Anki.Util.ReadUnicodeEscapedString.FormatException",
                        String.Format("Unable to convert '0x{0}' to an integer.", chr));
            }
            finally {
              sb.Append(chr);
            }
            break;
          default:
            // Not a Unicode escape sequence, pass through
            sb.Append(chr);
            break;
          }
          q = false;
        }
      }
      return sb.ToString();
    }
  } // static class Util
} // namespace Anki
