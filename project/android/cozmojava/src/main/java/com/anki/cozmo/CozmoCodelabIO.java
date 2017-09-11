package com.anki.cozmo;

import android.content.ContentResolver;
import android.net.Uri;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.IOException;

import com.anki.daslib.DAS;

import com.unity3d.player.UnityPlayer;

public final class CozmoCodelabIO
{
  // @NOTE: this wheel has been invented - but its in the apache-commons library which we don't include
  public static String stringFromInputStream(InputStream is)
  {
    BufferedReader br = null;
    StringBuilder sb = new StringBuilder();

    String line;
    try {
      br = new BufferedReader(new java.io.InputStreamReader(is, "UTF-8"));
      while ((line = br.readLine()) != null) {
        sb.append(line);
      }

    } catch (IOException ex) {
      DAS.Error("Codelab.Android.stringFromInputStream.Read.IOException", ex.toString());
    } finally {
      if (br != null) {
        try {
          br.close();
        } catch (IOException ex) {
          DAS.Error("Codelab.Android.stringFromInputStream.Close.IOException", ex.toString());
        }
      }
    }

    return sb.toString();
  }

  public static void openCodelabFileFromURI(ContentResolver contentResolver, Uri uri)
  {
    DAS.Info("Codelab.Android.OpenCodelabFileFromURI", "Recieved an intent to open a codelab file");  
    BufferedReader br = null;
    try {
      InputStream is = contentResolver.openInputStream(uri);
      if( is == null )
      {
        DAS.Error("Codelab.Android.openCodelabFileFromURI.NullInputStream", "Null input stream");
      }
      else
      {
        String codelabJsonString = stringFromInputStream(is);
        UnityPlayer.UnitySendMessage("StartupManager", "LoadCodelabFromRawJson", codelabJsonString );
      }
    } catch (Exception ex) {
      DAS.Error("Codelab.Android.openCodelabFileFromURI.Open.Exception", "General exception: " + ex.toString());
    } finally {
      if (br != null) {
        try {
          br.close();
        } catch (IOException ex) {
          DAS.Error("Codelab.Android.openCodelabFileFromURI.Close.Exception", ex.toString());
        }
      }
    }
  }
}
