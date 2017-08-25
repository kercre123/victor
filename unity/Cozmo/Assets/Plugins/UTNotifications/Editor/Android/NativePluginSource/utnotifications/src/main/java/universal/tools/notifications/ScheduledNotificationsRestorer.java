package universal.tools.notifications;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.util.Log;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.Iterator;

public class ScheduledNotificationsRestorer extends BroadcastReceiver {
//public
    @Override
    public void onReceive(Context context, Intent intent) {
        if ("android.intent.action.BOOT_COMPLETED".equals(intent.getAction())) {
            restoreScheduledNotifications(context);
        }
    }

//package
    static void setRestoreScheduledOnReboot(Context context, boolean restoreScheduledOnReboot) {
        context = context.getApplicationContext();
        SharedPreferences prefs = context.getSharedPreferences(ScheduledNotificationsRestorer.class.getName(), Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putBoolean(RESTORE_SCHEDULED_NOTIFICATIONS, restoreScheduledOnReboot);
        editor.commit();
    }

    static boolean getRestoreScheduledOnReboot(Context context) {
        return context.getApplicationContext().getSharedPreferences(ScheduledNotificationsRestorer.class.getName(), Context.MODE_PRIVATE).getBoolean(RESTORE_SCHEDULED_NOTIFICATIONS, false);
    }

    static void register(Context context, boolean repeated, int triggerInSeconds, int intervalSeconds, String title, String text, int id, Bundle userData, String notificationProfile, int badgeNumber) {
        context = context.getApplicationContext();
        if (getRestoreScheduledOnReboot(context)) {
            try {
                SharedPreferences prefs = context.getSharedPreferences(ScheduledNotificationsRestorer.class.getName(), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = prefs.edit();
                editor.putString(SCHEDULED_NOTIFICATIONS_STORED_PREFIX + id, packScheduledNotification(repeated, triggerInSeconds, intervalSeconds, title, text, userData, notificationProfile, badgeNumber));
                editor.commit();
            } catch (JSONException e) {
                e.printStackTrace();
            }
        }
    }

    static void cancel(Context context, int id) {
        context = context.getApplicationContext();
        SharedPreferences prefs = context.getSharedPreferences(ScheduledNotificationsRestorer.class.getName(), Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        editor.remove(SCHEDULED_NOTIFICATIONS_STORED_PREFIX + id);
        editor.commit();
    }

//private
    private static void restoreScheduledNotifications(Context context) {
        context = context.getApplicationContext();
        int[] ids = Manager.getStoredScheduledNotificationIds(context);

        if (ids != null && ids.length > 0) {
            SharedPreferences prefs = context.getSharedPreferences(ScheduledNotificationsRestorer.class.getName(), Context.MODE_PRIVATE);
            for (int id : ids) {
                String packedScheduledNotification = prefs.getString(SCHEDULED_NOTIFICATIONS_STORED_PREFIX + id, null);
                if (packedScheduledNotification != null && !packedScheduledNotification.isEmpty()) {
                    try {
                        restoreScheduledNotification(context, packedScheduledNotification, id);
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
            }
        }
    }

    private static String packScheduledNotification(boolean repeated, int triggerInSeconds, int intervalSeconds, String title, String text, Bundle userData, String notificationProfile, int badgeNumber) throws JSONException {
        JSONObject json = new JSONObject();
        json.put("repeated", repeated);
        json.put("triggerInSeconds", System.currentTimeMillis() + (long) triggerInSeconds * 1000L);
        json.put("intervalSeconds", intervalSeconds);
        if (title != null) {
            json.put("title", title);
        }
        if (text != null) {
            json.put("text", text);
        }
        if (notificationProfile != null) {
            json.put("notificationProfile", notificationProfile);
        }
        json.put("badgeNumber", badgeNumber);

        JSONObject userDataJson = new JSONObject();
        for (String key : userData.keySet()) {
            if (key != null && !key.equals(Manager.KEY_TITLE) && !key.equals(Manager.KEY_TEXT) && !key.equals(Manager.KEY_NOTIFICATION_PROFILE) && !key.equals(Manager.KEY_BADGE_NUMBER))
            try {
                String val = userData.getString(key);
                if (val != null) {
                    try {
                        userDataJson.put(key, val);
                    } catch (JSONException e) {
                        e.printStackTrace();
                    }
                }
            } catch (ClassCastException e) {
                Log.e(Manager.class.getName(), "User data argument \"" + key + "\" is not string => ignored");
            }
        }
        json.put("userData", userDataJson);

        return json.toString();
    }

    private static void restoreScheduledNotification(Context context, String packedScheduledNotification, int id) throws JSONException {
        JSONObject json = new JSONObject(packedScheduledNotification);
        boolean repeated = json.getBoolean("repeated");
        long triggerInSeconds = (json.getLong("triggerInSeconds") - System.currentTimeMillis()) / 1000L;
        int intervalSeconds = json.getInt("intervalSeconds");
        String title = json.has("title") ? json.getString("title") : null;
        String text = json.has("text") ? json.getString("text") : null;
        String notificationProfile = json.has("notificationProfile") ? json.getString("notificationProfile") : null;
        int badgeNumber = json.has("badgeNumber") ? json.getInt("badgeNumber") : -1;

        Bundle userData = new Bundle();
        JSONObject userDataJson = json.getJSONObject("userData");
        for (Iterator<?> iterator = userDataJson.keys(); iterator.hasNext(); ) {
            String key = (String) iterator.next();
            userData.putString(key, userDataJson.getString(key));
        }

        Manager.addMainArgumentsToBundle(userData, title, text, notificationProfile, badgeNumber);

        if (triggerInSeconds < 5L) {
            if (repeated) {
                triggerInSeconds = triggerInSeconds % intervalSeconds;
                if (triggerInSeconds < 5L) {
                    triggerInSeconds += intervalSeconds;
                }
            } else {
                triggerInSeconds = 5L;
            }
        }

        Manager.scheduleNotificationCommon(context, repeated, (int)triggerInSeconds, intervalSeconds, title, text, id, userData, notificationProfile, badgeNumber);
    }

    private static final String RESTORE_SCHEDULED_NOTIFICATIONS = "RESTORE_SCHEDULED_NOTIFICATIONS";
    private static final String SCHEDULED_NOTIFICATIONS_STORED_PREFIX = "SCHEDULED_NOTIFICATIONS_STORED_";
}
