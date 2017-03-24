package com.anki.util;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.NetworkInfo.DetailedState;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

/**
 * This class handles the tracking of an attempt to connect to a wifi network.
 *
 * When a connection is requested to a given network, WifiConnectionAttempt registers
 * a listener for the network state, initiates the connection attempt, and runs the
 * given callback when it determines that a connection attempt has either succeeded or
 * failed.
 */
public final class WifiConnectionAttempt {

  public interface ConnectionListener {
    void onFinished(boolean success, NetworkInfo info);
  }

  private final String TAG = "AnkiUtil";

  // config/settings
  private Context mContext;
  private WifiManager mWifiManager;
  private ConnectivityManager mConnectivityManager;
  private int mNetworkId;
  private String mSSID;
  private ConnectionListener mListener;

  // connection state tracking
  private boolean mSupplicantStarted;
  private boolean mNetworkStarted;
  private boolean mFailed;
  private boolean mAuthenticationError;

  private BroadcastReceiver mReceiver;
  private final Handler mHandler = new Handler(Looper.getMainLooper());

  public WifiConnectionAttempt(final int networkId, final String SSID, final Context context, final ConnectionListener listener) {
    mContext = context;
    mNetworkId = networkId;
    mSSID = SSID;
    mListener = listener;

    mWifiManager = (WifiManager)context.getSystemService(Context.WIFI_SERVICE);
    mConnectivityManager = (ConnectivityManager)context.getSystemService(Context.CONNECTIVITY_SERVICE);
  }

  public boolean connect(final int timeoutMs) {
    if (mAuthenticationError) {
      Log.d(TAG, "WCA error, need to create new attempt on new network after authentication error");
      return false;
    }
    reset();
    if (mNetworkId < 0) {
      return false;
    }
    killReceiver();
    mReceiver = new ConnectionBroadcastReceiver();
    final IntentFilter intentFilter = new IntentFilter();
    intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
    intentFilter.addAction(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION);
    mContext.registerReceiver(mReceiver, intentFilter);

    final boolean enableResult = mWifiManager.enableNetwork(mNetworkId, true);
    final boolean reconnectResult = enableResult && mWifiManager.reconnect();
    if (!enableResult || !reconnectResult) {
      Log.d(TAG, "WCA enable/reconnect failed: " + enableResult + "/" + reconnectResult);
    }

    if (reconnectResult) {
      // set up task to detect timeout
      mHandler.postDelayed(new Runnable() {
        @Override public void run() {
          Log.v(TAG, "WCA detected connection timeout of " + timeoutMs + "ms, failing");
          killReceiver();
          failed();
        }
      }, timeoutMs);
    }

    return reconnectResult;
  }

  public boolean didPasswordFail() {
    return mAuthenticationError;
  }

  public void unregister() {
    killReceiver();
  }

  private class ConnectionBroadcastReceiver extends BroadcastReceiver {
    @Override
    public void onReceive(final Context context, final Intent intent) {
      if (isInitialStickyBroadcast()) {
        // this notification isn't current, ignore
        Log.v(TAG, "WCA ignoring sticky broadcast");
        return;
      }

      final String action = intent.getAction();
      if (action.equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)) {
        final NetworkInfo info = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
        handleNetworkUpdate(info);
      }
      else if (action.equals(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION)) {
        final SupplicantState supplicantState = intent.getParcelableExtra(WifiManager.EXTRA_NEW_STATE);
        handleSupplicantStateChanged(supplicantState, intent.getIntExtra(WifiManager.EXTRA_SUPPLICANT_ERROR, 0));
      }
    }
  };

  /**
   * Much of the logic in this function is based on attempting to handle observed behavior.
   * For example, sometimes phones will report an unchanged status of "disconnected" after
   * initiating a connection before later reporting success, so "disconnected" doesn't
   * necessarily mean "failure" unless we've received a non-"disconnected" state since
   * initiating the connection attempt.
   *
   * In this function, as in handleNetworkUpdate below, we wait until receiving a state
   * that indicates the phone is actively trying to attempt a connection before marking
   * the supplicant or network as "started"; only after that happens (or if a timeout
   * period elapses) will we mark the connection as having failed if we then receive a
   * "disconnected" state.
   */
  private void handleSupplicantStateChanged(final SupplicantState state, final int errorCode) {
    if (errorCode == WifiManager.ERROR_AUTHENTICATING) {
      mAuthenticationError = true;

      // it seems the password is incorrect, so try to delete the existing configuration
      final boolean disableResult = mWifiManager.disableNetwork(mNetworkId);
      final boolean removeResult = disableResult && mWifiManager.removeNetwork(mNetworkId);
      if (!disableResult || !removeResult) {
        Log.d(TAG, "WCA problem removing network: disable/remove result is " + disableResult + "/" + removeResult);
      }
      else {
        Log.d(TAG, "WCA supplicant error, removed network " + mNetworkId);
      }
      failed();
      killReceiver();
      return;
    }
    else if (mFailed) {
      // after failure, we only keep the receiver alive to see if we get a supplicant error.
      // we didn't, so kill the receiver
      killReceiver();
    }

    Log.v(TAG, "WCA supplicant state: " + state.toString());
    if (!mSupplicantStarted && state != SupplicantState.DISCONNECTED) {
      Log.v(TAG, "WCA marking supplicant started");
      mSupplicantStarted = true;
    }

    if (mSupplicantStarted && state == SupplicantState.DISCONNECTED) {
      Log.v(TAG, "WCA supplicant failed");
      failed();
    }
  }

  private void handleNetworkUpdate(final NetworkInfo info) {
    if (mFailed) {
      // ignore updates after failure until reconnect is attempted
      return;
    }

    final boolean isTargetNetwork = WifiUtil.dequoteSSID(info.getExtraInfo()).equals(WifiUtil.dequoteSSID(mSSID));

    if (!mNetworkStarted && (isTargetNetwork || info.getDetailedState() != DetailedState.DISCONNECTED)) {
      Log.v(TAG, "WCA marking network started");
      mNetworkStarted = true;
    }
    if (!mNetworkStarted) {
      // nothing for us to do until we get some indication that the phone has
      // started trying to connect
      return;
    }

    if (info.getDetailedState() == DetailedState.DISCONNECTED) {
      Log.v(TAG, "WCA connect failed");
      failed();
    }
    else if (isTargetNetwork && WifiUtil.isConnectedState(info.getDetailedState())) {
      Log.v(TAG, "WCA connect succeeded");
      succeeded(info);
    }
  }

  private void reset() {
    // to support reconnects using same info, reset internal state
    killReceiver();
    mFailed = false;
    mSupplicantStarted = false;
    mNetworkStarted = false;
    mAuthenticationError = false;
  }

  private void killReceiver() {
    mHandler.removeCallbacksAndMessages(null);
    if (mReceiver != null) {
      try {
        mContext.unregisterReceiver(mReceiver);
      } catch (IllegalArgumentException e) {} // suppress "wasn't registered" exception
      mReceiver = null;
    }
  }

  private void failed() {
    if (mFailed) {
      // this might be called multiple times if failure is called by a network
      // update, then and then a supplicant update (which we need to keep alive
      // in order to track password failure) also triggers it
      return;
    }
    mFailed = true;
    mListener.onFinished(false, null);
  }

  private void succeeded(final NetworkInfo info) {
    killReceiver();
    mListener.onFinished(true, info);
  }
}
