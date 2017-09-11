package com.anki.cozmo;

import android.content.ContentResolver;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.os.Environment;

import java.io.BufferedReader;
import java.io.File;
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

  private static File combinePaths(String... paths) {
    File file = new File(paths[0]);

    for (int i = 1; i < paths.length ; i++) {
        file = new File(file, paths[i]);
    }

    return file;
  }

  public static File generateFileWithNameAndText(final String projectNameString, final String projectContentString) {

    File result = null;
    try{
      File targetDirectory = new File( Environment.getExternalStorageDirectory(), combinePaths( "Android", "data", "com.anki.cozmo" ).getPath() );
      String fileName = projectNameString.replaceAll(" ", "_").toLowerCase() + ".codelab";
      File temp = new File(targetDirectory, fileName);

      if( temp.exists() ) {
        if( !temp.delete() ) {
          DAS.Error("Codelab.Android.generateFileWithNameAndText.DeleteFileError", "Failed to delete duplicate temp file in upload to google drive");
        }
      }

      if( !temp.createNewFile() ) {
        DAS.Error("Codelab.Android.generateFileWithNameAndText.CreateFileError", "Failed to create a temp file to upload to google drive");
      }
      else if( !temp.exists() || !temp.canRead() ) {
        DAS.Error("Codelab.Android.generateFileWithNameAndText.CreatedFileReadable", "Created temp file is not readable");
      }
      else {
        temp.deleteOnExit();

        java.io.BufferedWriter out = new java.io.BufferedWriter(new java.io.FileWriter(temp));
        out.write(projectContentString);
        out.close();

        result = temp;
      }
    } catch (IOException e) {
      e.printStackTrace();
    }
    return result;
  }
}
