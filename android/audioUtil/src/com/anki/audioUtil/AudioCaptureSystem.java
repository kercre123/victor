package com.anki.audioUtil;

import com.anki.util.AnkitivityDispatcher;
import com.anki.util.PermissionUtil;
import android.content.pm.PackageManager;
import android.Manifest;

public class AudioCaptureSystem {

  public static boolean hasCapturePermission() {
    return PermissionUtil.hasPermission(Manifest.permission.RECORD_AUDIO);
  }

  public static boolean shouldShowRationale() {
    return PermissionUtil.shouldShowRationale(Manifest.permission.RECORD_AUDIO);
  }

  public static void requestCapturePermission() {
    final AnkitivityDispatcher.PermissionListener listener = new AnkitivityDispatcher.PermissionListener() {
      @Override public void onRequestPermissionsResult(final String[] permissions, final int[] grantResults) {
        if (grantResults.length > 0) {
          NativeRequestCapturePermissionCallback(grantResults[0] == PackageManager.PERMISSION_GRANTED);
        }
        else {
          NativeRequestCapturePermissionCallback(false);
        }
      }
    };
    PermissionUtil.askForPermission(Manifest.permission.RECORD_AUDIO, listener);
  }

  public static native void NativeRequestCapturePermissionCallback(final boolean permissionGranted);
}
