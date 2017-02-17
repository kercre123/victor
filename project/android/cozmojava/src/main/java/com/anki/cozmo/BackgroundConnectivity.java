package com.anki.cozmo;

import android.app.Activity;
import android.app.AlarmManager;
import android.app.IntentService;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.SystemClock;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;

import java.net.HttpURLConnection;
import java.net.URL;

public class BackgroundConnectivity extends IntentService {

    private static boolean _installed = false;
    private static final String TAG = "BackgroundConnectivity";
    // run every 15 minutes
    private static final long INTERVAL_MS = 1000 * 60 * 15;

    protected static final String BROADCAST_ACTION = "com.anki.cozmo.connectivity.BROADCAST";

    private long lastUpdateTime = 0;

    public BackgroundConnectivity() {
        super("BackgroundConnectivity");
    }

    public static void install(Activity activity) {
        if (_installed) {
            return;
        }

        AlarmManager alarmManager = (AlarmManager)activity.getSystemService(Context.ALARM_SERVICE);
        Intent alarmIntent = new Intent(activity, BackgroundConnectivity.class);
        PendingIntent pending = PendingIntent.getService(activity, 0, alarmIntent, 0);
        // first run 5 seconds after install
        long firstRunDelay = 1000 * 5;

        // if this service is already running (from previous apprun?), kill it before starting again
        alarmManager.cancel(pending);
        alarmManager.setInexactRepeating(AlarmManager.ELAPSED_REALTIME,
                SystemClock.elapsedRealtime() + firstRunDelay, INTERVAL_MS, pending);

        // register our listener
        IntentFilter filter = new IntentFilter(BROADCAST_ACTION);
        LocalBroadcastManager.getInstance(activity).registerReceiver(new ConnectivityListener(), filter);

        _installed = true;
    }

    private boolean isInternetAvailable() {
        ConnectivityManager manager = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo[] networks = manager.getAllNetworkInfo();
        for (NetworkInfo info : networks) {
            if (info.getType() == ConnectivityManager.TYPE_WIFI && info.isAvailable() && info.isConnected()) {
                return testGoogleConnection();
            }
        }
        return false;
    }

    private boolean testGoogleConnection() {
        try {
            HttpURLConnection connection = (HttpURLConnection)(new URL("http://www.google.com").openConnection());
            connection.setRequestProperty("Connection", "close");
            connection.setConnectTimeout(2000);
            connection.connect();
            return (connection.getResponseCode() == HttpURLConnection.HTTP_OK);
        } catch (Exception e) {
            return false;
        }
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        // this notification can get spammed when app is inactive - if not much time has passed
        // since last update, just return
        long systemTime = SystemClock.elapsedRealtime();
        if (systemTime - lastUpdateTime < INTERVAL_MS / 2) {
            Log.d(TAG, "ignoring update");
            return;
        }
        lastUpdateTime = systemTime;

        // test connectivity
        boolean internetAvailable = isInternetAvailable();

        Log.d(TAG, "internet availability: " + internetAvailable);

        if (internetAvailable) {
            LocalBroadcastManager.getInstance(this).sendBroadcast(new Intent(BROADCAST_ACTION));
        }
    }

    private static class ConnectivityListener extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "received connectivity broadcast");
            Runnable r = new Runnable() {
                @Override public void run() {
                    BackgroundConnectivity.ExecuteBackgroundTransfers();
                }
            };
            if (android.os.Looper.getMainLooper().getThread() == Thread.currentThread()) {
                new Thread(r).start();
            }
            else {
                r.run();
            }
        }
    }

    protected static native void ExecuteBackgroundTransfers();
}
