package com.anki.util;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.content.pm.PackageManager;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;

import android.support.v4.content.ContextCompat;
import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.api.GoogleApiClient;
import com.google.android.gms.common.api.PendingResult;
import com.google.android.gms.common.api.ResultCallback;
import com.google.android.gms.common.api.Status;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.LocationSettingsRequest;
import com.google.android.gms.location.LocationSettingsResult;
import com.google.android.gms.location.LocationSettingsStatusCodes;


/**
 * Utility class for location services:
 * - detect if location service is currently turned on or off
 * - request that the user enable location services
 */
public class LocationUtil {

  private static final String TAG = "AnkiUtil";

  private static GoogleApiClient mApiClient;

  public static void requestEnableLocation(final Activity activity, final AnkitivityDispatcher dispatcher) {
    mApiClient = new GoogleApiClient.Builder(activity)
      .addApi(LocationServices.API)
      .addConnectionCallbacks(new GoogleApiClient.ConnectionCallbacks() {
        @Override public void onConnected(android.os.Bundle connectionHint) { onApiConnected(activity, dispatcher); }
        @Override public void onConnectionSuspended(int cause) {}
      })
      .addOnConnectionFailedListener(new GoogleApiClient.OnConnectionFailedListener() {
        @Override public void onConnectionFailed(ConnectionResult result) { onApiConnectionFailed(); }
      })
      .build();
    mApiClient.connect();
  }

  public static boolean isLocationServiceEnabled(final Context context) {
    boolean isAvailable = false;

    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT) {
      int locationMode = Settings.Secure.LOCATION_MODE_OFF;
      try {
        locationMode = Settings.Secure.getInt(context.getContentResolver(), Settings.Secure.LOCATION_MODE);
      } catch (Settings.SettingNotFoundException e) {
        e.printStackTrace();
      }

      isAvailable = (locationMode != Settings.Secure.LOCATION_MODE_OFF);
    } else {
      final String locationProviders = Settings.Secure.getString(context.getContentResolver(), Settings.Secure.LOCATION_PROVIDERS_ALLOWED);
      isAvailable = !TextUtils.isEmpty(locationProviders);
    }

    return isAvailable;
  }



  private static void makeLocationRequest(final Activity activity, final AnkitivityDispatcher dispatcher) {
    final LocationRequest request = LocationRequest.create();
    request.setPriority(LocationRequest.PRIORITY_BALANCED_POWER_ACCURACY);
    request.setInterval(60 * 1000);
    request.setFastestInterval(10 * 1000);

    final LocationSettingsRequest.Builder builder = new LocationSettingsRequest.Builder().addLocationRequest(request);
    builder.setAlwaysShow(true);

    final PendingResult<LocationSettingsResult> result =
      LocationServices.SettingsApi.checkLocationSettings(mApiClient, builder.build());

    result.setResultCallback(new ResultCallback<LocationSettingsResult>() {
      @Override
      public void onResult(final LocationSettingsResult result) {
        final Status status = result.getStatus();
        switch (status.getStatusCode()) {
          case LocationSettingsStatusCodes.SUCCESS:
            Log.v(TAG, "location request worked");
            sendLocationResult(true);
            break;
          case LocationSettingsStatusCodes.RESOLUTION_REQUIRED:
            Log.v(TAG, "resolution required");
            try {
              final AnkitivityDispatcher.ResultListener listener = new AnkitivityDispatcher.ResultListener() {
                @Override public void onActivityResult(final int resultCode, final Intent intent) {
                  sendLocationResult(resultCode == Activity.RESULT_OK);
                }
              };
              final int requestCode = dispatcher.subscribeActivityResult(listener);
              status.startResolutionForResult(activity, requestCode);
            }
            catch (android.content.IntentSender.SendIntentException e) {
              Log.v(TAG, "starting resolution failed");
              sendLocationResult(false);
            }
            break;
          case LocationSettingsStatusCodes.SETTINGS_CHANGE_UNAVAILABLE:
            Log.v(TAG, "can't change settings");
            sendLocationResult(false);
            break;
          default:
            Log.v(TAG, "other status result: " + status.getStatusCode());
            sendLocationResult(false);
            break;
        }
        // COZMO-9588: If we have already disconnected, don't disconnect again
        if (mApiClient != null) {
          mApiClient.disconnect();
          mApiClient = null;
        }
      }
    });
  }

  private static void onApiConnected(final Activity activity, final AnkitivityDispatcher dispatcher) {
    Log.v(TAG, "location API connected");
    makeLocationRequest(activity, dispatcher);
  }

  private static void onApiConnectionFailed() {
    Log.v(TAG, "location API failed");
    sendLocationResult(false);
    mApiClient = null;
  }

  private static void sendLocationResult(final boolean result) {
    MessageSender.sendMessage("locationResult", new String[] { result ? "true" : "false" });
  }

}
