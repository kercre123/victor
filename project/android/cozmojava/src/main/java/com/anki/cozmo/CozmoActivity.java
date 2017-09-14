package com.anki.cozmo;

import android.content.ContentResolver;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.net.Uri;
import android.util.Log;

import android.support.v4.app.ActivityCompat;

import java.io.File;
import java.util.UUID;

import com.unity3d.player.UnityPlayer;

import com.anki.daslib.DAS;
import com.anki.hockeyappandroid.NativeCrashManager;
import com.anki.util.AnkitivityDispatcher;
import com.anki.util.PermissionUtil;

import net.hockeyapp.android.CrashManager;
import net.hockeyapp.android.CrashManagerListener;
import net.hockeyapp.android.FeedbackManager;
import net.hockeyapp.android.LoginManager;
import net.hockeyapp.android.UpdateManager;
import net.hockeyapp.android.metrics.MetricsManager;

/**
 * Custom Activity implementation designed to work with AnkitivityDispatcher, so that results of permission requests
 * and other activities can be dispatched to code in the anki-util repository that doesn't know about CozmoActivity
 */
public class CozmoActivity extends HackUnityPlayerActivity implements ActivityCompat.OnRequestPermissionsResultCallback {

  private final AnkitivityDispatcher mDispatcher = new AnkitivityDispatcher();

  public static String APP_RUN_ID;
  public static String HOCKEY_APP_ID;

  static {
    System.loadLibrary("DAS");
    System.loadLibrary("cozmoEngine");
  }

  @Override
  protected void onCreate(final android.os.Bundle bundle) {
    PermissionUtil.initialize(this, mDispatcher);
    
    requestWindowFeature(1);

    super.onCreate(bundle);

    APP_RUN_ID = UUID.randomUUID().toString();
    HOCKEY_APP_ID = getManifestProperty("HOCKEYAPP_APP_ID");

    if (HOCKEY_APP_ID.length() > 0) {
      installBreakpad(NativeCrashManager.getDumpsDirectory(this) + File.separator
                      + APP_RUN_ID + NativeCrashManager.DMP_EXTENSION);
    }

    Intent intent = getIntent();
    if (intent != null) {
      onNewIntent(intent);
    }
  }

  @Override
  protected void onNewIntent(Intent intent) {
    String action = intent.getAction();
    Uri uri = intent.getData();
    if (Intent.ACTION_VIEW.equals(action) && uri != null) {
      ContentResolver resolver = getContentResolver();
      CozmoCodelabIO.openCodelabFileFromURI(resolver, uri);
    }
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
    super.onActivityResult(requestCode, resultCode, intent);
    mDispatcher.onActivityResult(requestCode, resultCode, intent);
  }

  @Override
  public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
    super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    mDispatcher.onRequestPermissionsResult(requestCode, permissions, grantResults);
  }

  public AnkitivityDispatcher getDispatcher() {
    return mDispatcher;
  }

  public String getReleaseVersion() {
    return android.os.Build.VERSION.RELEASE;
  }

  public int getSDKVersion() {
    return android.os.Build.VERSION.SDK_INT;
  }
  
  // request a permission and specify the game object/method name in unity to receive the results
  public void unityRequestPermission(final String permission, final String gameObject, final String methodCallback) {
    AnkitivityDispatcher.PermissionListener listener = new AnkitivityDispatcher.PermissionListener() {
      @Override public void onRequestPermissionsResult(String[] permissions, int[] grantResults) {
        final String result = grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED ?
          "true" : "false";
        UnityPlayer.UnitySendMessage(gameObject, methodCallback, result);
      }
    };
    PermissionUtil.askForPermission(permission, listener);
  }

  private String getManifestProperty(String propertyName) {
    try {
      ApplicationInfo appInfo = getPackageManager().getApplicationInfo(getPackageName(), PackageManager.GET_META_DATA);
      if (appInfo != null && appInfo.metaData != null) {
        return appInfo.metaData.getString(propertyName);
      }
    } catch (Exception e) {}

    return "";
  }

  public void ThrowNullPointerException() {
    throw new NullPointerException();
  }

  public void Crash() {
    runOnUiThread(new Runnable() {
        @Override
        public void run() {
          ThrowNullPointerException();
        }
    });
  }

  public void startHockeyAppManager(final String serverURL,
                                    final String appID,
                                    final String secret,
                                    final int loginMode,
                                    final boolean updateManagerEnabled,
                                    final boolean userMetricsEnabled,
                                    final boolean autoSendEnabled,
                                    final String userId,
                                    final String crashDescription) {
    runOnUiThread(new Runnable() {
        @Override
        public void run() {
          if (updateManagerEnabled) {
            registerUpdateManager(serverURL, appID);
          }
          if (userMetricsEnabled) {
            registerMetricsManager(appID);
          }
          registerCrashManager(serverURL, appID, autoSendEnabled, userId, crashDescription);
          registerFeedbackManager(serverURL, appID);
          registerLoginManager(serverURL, appID, secret, loginMode);
        }
    });
  }

  public void registerUpdateManager(final String serverURL,
                                    final String appID)
  {
    runOnUiThread(new Runnable() {
        @Override
        public void run() {
          UpdateManager.register(CozmoActivity.this, serverURL, appID, null, true);
        }
    });
  }

  public void registerMetricsManager(final String appID)
  {
    runOnUiThread(new Runnable() {
        @Override
        public void run() {
          MetricsManager.register(CozmoActivity.this, CozmoActivity.this.getApplication(), appID);
        }
    });
  }

  public void registerFeedbackManager(final String serverURL,
                                      final String appID)
  {
    runOnUiThread(new Runnable() {
        @Override
        public void run() {
          FeedbackManager.register(CozmoActivity.this, serverURL, appID, null);
        }
    });
  }

  public void registerLoginManager(final String serverURL,
                                   final String appID,
                                   final String secret,
                                   final int loginMode)
  {
    runOnUiThread(new Runnable() {
        @Override
        public void run() {
          LoginManager.register(CozmoActivity.this, appID, secret, serverURL,
                                loginMode, CozmoActivity.this.getClass());
          LoginManager.verifyLogin(CozmoActivity.this, CozmoActivity.this.getIntent());
        }
    });
  }

  public void registerCrashManager(final String serverURL,
                                   final String appID,
                                   final boolean autoSendEnabled,
                                   final String userId,
                                   final String crashDescription)
  {
    runOnUiThread(new Runnable() {
        @Override
        public void run() {
          CrashManager.register(CozmoActivity.this, serverURL, appID,
                                new CrashManagerListener() {
                                  public String getUserID() {
                                    return userId;
                                  }
                                  public String getDescription() {
                                    return crashDescription;
                                  }
                                  public boolean shouldAutoUploadCrashes() {
                                    return autoSendEnabled;
                                  }
                                });
        }
    });
  }

  private static native void installBreakpad(String path);

  private ResolveInfo getPackageForUri( Intent i, String string ) {

      final android.content.pm.PackageManager packageManager = this.getPackageManager();
      java.util.List<android.content.pm.ResolveInfo> pkgAppsList = packageManager.queryIntentActivities( i, android.content.pm.PackageManager.GET_RESOLVED_FILTER );
      int riid = -1;
      int index=0;
      for( android.content.pm.ResolveInfo resolveInfo : pkgAppsList ) {
          if( riid == -1 || resolveInfo.activityInfo.packageName.equals( string ) ) {
              riid = index;
          }
          ++index;
      }

      if( riid != -1 ) {
          return pkgAppsList.get(riid);
      }

      return null;
  }

  public void exportCodelabFile(final String projectNameString, final String projectContentString) {
    DAS.Info("Codelab.Android.exportCodelabFile", "received send email request from engine");

    File file = CozmoCodelabIO.generateFileWithNameAndText(projectNameString, projectContentString);

    if( file != null && file.exists() && file.canRead()) {

        Intent i = new Intent(android.content.Intent.ACTION_SEND);
        i.setType("text/plain");
        i.putExtra(Intent.EXTRA_STREAM, Uri.fromFile(file));

        // NOTE: "com.google.android.apps.docs" is the package ID for google drive, google docs has a different ID
        ResolveInfo pkg = getPackageForUri(i, "com.google.android.apps.docs");

        if( pkg != null && file.exists() && file.canRead() ) {
            i.setComponent(new android.content.ComponentName(pkg.activityInfo.packageName, pkg.activityInfo.name));
            startActivity(Intent.createChooser(i, ""));
        }
    }
    else
    {
      DAS.Error("Codelab.Android.exportCodelabFile.FileCreationError", "Could not properly generate a file with the specified name and text from unity");
    }
  }
}
