package universal.tools.notifications;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.SystemClock;
import android.support.v4.app.NotificationCompat;
import android.util.Log;

import com.unity3d.player.UnityPlayer;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.io.InputStream;
import java.net.URLConnection;
import java.net.URL;
import java.net.URLDecoder;
import java.util.Iterator;

import me.leolin.shortcutbadger.ShortcutBadger;

public class Manager {
//public
    public static boolean initialize(boolean firebasePushNotificationsEnabled, boolean amazonPushNotificationsEnabled, String firebaseSenderID, boolean willHandleReceivedNotifications, int startId, boolean incrementalId, int showNotificationsMode, boolean restoreScheduledOnReboot, int notificationsGroupingMode, boolean showLatestNotificationOnly, String titleFieldName, String textFieldName, String userDataFieldName, String notificationProfileFieldName, String idFieldName, String badgeFieldName) {
        m_nextPushNotificationId = startId;

        Context context = UnityPlayer.currentActivity.getApplicationContext();

        SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putBoolean(WILL_HANDLE_RECEIVED_NOTIFICATIONS, willHandleReceivedNotifications);
        editor.putBoolean(INCREMENTAL_ID, incrementalId);
        editor.putInt(START_ID, startId);
        editor.putInt(SHOW_NOTIFICATIONS_MODE, showNotificationsMode);
        editor.putInt(NOTIFICATIONS_GROUPING_MODE, notificationsGroupingMode);
        editor.putBoolean(SHOW_LATEST_NOTIFICATIONS_ONLY, showLatestNotificationOnly);
        editor.putString(TITLE_FIELD_NAME, titleFieldName);
        editor.putString(TEXT_FIELD_NAME, textFieldName);
        editor.putString(USER_DATA_FIELD_NAME, userDataFieldName);
        editor.putString(NOTIFICATION_PROFILE_FIELD_NAME, notificationProfileFieldName);
        editor.putString(ID_FIELD_NAME, idFieldName);
        editor.putString(BADGE_FIELD_NAME, badgeFieldName);
        editor.commit();

        ScheduledNotificationsRestorer.setRestoreScheduledOnReboot(context, restoreScheduledOnReboot);

        if (firebasePushNotificationsEnabled) {
            if (fcmProviderAvailable()) {
                m_provider = new FCMProvider(context, firebaseSenderID);
            }
        }
        if (amazonPushNotificationsEnabled) {
            if (admProviderAvailable()) {
                m_provider = new ADMProvider(context);
            }
        }

        if (m_provider != null) {
            if (pushNotificationsEnabled(context)) {
                m_provider.enable();
            } else {
                m_provider.disable();
            }

            return true;
        }

        return !firebasePushNotificationsEnabled && !amazonPushNotificationsEnabled;
    }

    public static boolean fcmProviderAvailable() {
        return FCMProvider.isAvailable(UnityPlayer.currentActivity.getApplicationContext());
    }

    public static boolean admProviderAvailable() {
        return ADMProvider.isAvailable();
    }

    public static void postNotification(String title, String text, int id, String[] userData, String notificationProfile, int badgeNumber) {
        postNotification(UnityPlayer.currentActivity, title, text, id, userDataToBundle(userData, title, text, notificationProfile, badgeNumber), notificationProfile, badgeNumber, true);
    }

    public static void scheduleNotification(int triggerInSeconds, String title, String text, int id, String[] userData, String notificationProfile, int badgeNumber) {
        scheduleNotificationCommon(UnityPlayer.currentActivity, false, triggerInSeconds, 0, title, text, id, userDataToBundle(userData, title, text, notificationProfile, badgeNumber), notificationProfile, badgeNumber);
    }

    public static void scheduleNotificationRepeating(int firstTriggerInSeconds, int intervalSeconds, String title, String text, int id, String[] userData, String notificationProfile, int badgeNumber) {
        scheduleNotificationCommon(UnityPlayer.currentActivity, true, firstTriggerInSeconds, intervalSeconds, title, text, id, userDataToBundle(userData, title, text, notificationProfile, badgeNumber), notificationProfile, badgeNumber);
    }

    public static boolean notificationsEnabled() {
        return notificationsEnabled(UnityPlayer.currentActivity.getApplicationContext());
    }

    public static void setNotificationsEnabled(boolean enabled) {
        Context context = UnityPlayer.currentActivity.getApplicationContext();

        SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putBoolean(NOTIFICATIONS_ENABLED, enabled);
        editor.commit();
    }

    public static boolean pushNotificationsEnabled() {
        Context context = UnityPlayer.currentActivity.getApplicationContext();
        return m_provider != null && notificationsEnabled(context) && pushNotificationsEnabled(context);
    }

    public static void setPushNotificationsEnabled(boolean enabled) {
        Context context = UnityPlayer.currentActivity.getApplicationContext();

        if (pushNotificationsEnabled(context) != enabled) {
            SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
            SharedPreferences.Editor editor = prefs.edit();
            editor.putBoolean(PUSH_NOTIFICATIONS_ENABLED, enabled);
            editor.commit();

            if (m_provider != null) {
                if (enabled) {
                    m_provider.enable();
                } else {
                    m_provider.disable();
                }
            }
        }
    }

    public static void hideNotification(int id) {
        NotificationManager notificationManager = (NotificationManager) UnityPlayer.currentActivity.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancel(id);
    }

    public static void cancelNotification(int id) {
        hideNotification(id);

        Context context = UnityPlayer.currentActivity.getApplicationContext();

        PendingIntent pendingIntent = getPendingIntentForScheduledNotification(context, id, null, false);
        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        alarmManager.cancel(pendingIntent);

        clearSheduledNotificationId(context, id);
    }

    public static void hideAllNotifications() {
        hideAllNotifications(UnityPlayer.currentActivity);
    }

    public static void cancelAllNotifications() {
        hideAllNotifications();
        for (int id : getStoredScheduledNotificationIds(UnityPlayer.currentActivity.getApplicationContext())) {
            cancelNotification(id);
        }
    }

    public static String getClickedNotificationPacked() {
        Intent activityIntent = getIntent();
        if (activityIntent != null) {
            String result = activityIntent.getStringExtra(KEY_NOTIFICATION);
            setIntent(null);
            return result;
        } else {
            return null;
        }
    }

    public static String getReceivedNotificationsPacked() {
        Context context = UnityPlayer.currentActivity.getApplicationContext();
        String res = readReceivedNotificationsPacked(context);

        SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        editor.remove(RECEIVED_NOTIFICATIONS);
        editor.commit();

        return "[" + res + "]";
    }

    public static void setBackgroundMode(boolean backgroundMode) {
        m_backgroundMode = backgroundMode;
    }

    public static void setBadge(int badgeNumber) {
        setBadge(UnityPlayer.currentActivity.getApplicationContext(), badgeNumber);
    }

    public static int getBadge() {
        return getBadge(UnityPlayer.currentActivity.getApplicationContext());
    }

//public constants
    public static final String KEY_ID = "__UT_ID";
    public static final String KEY_REPEATED = "__UT_REPEATED";
    public static final String KEY_USER_DATA = "__UT_USER_DATA";
    public static final String KEY_TITLE = "__UT_TITLE";
    public static final String KEY_TEXT = "__UT_TEXT";
    public static final String KEY_NOTIFICATION_PROFILE = "__UT_NOTIFICATION_PROFILE";
    public static final String KEY_BADGE_NUMBER = "__UT_BADGE_NUMBER";
    public static final String KEY_NOTIFICATION = "universal.tools.notifications.__notification";

//package
    static void postNotification(Context _context, final String title, final String text, final int id, final Bundle userData, final String notificationProfile, final int badgeNumber, final boolean repeatedOrNotScheduled) {
        final Context context = _context.getApplicationContext();

        if (userData != null && userData.containsKey(IMAGE_URL)) {
            AsyncTask<String, Void, Bitmap> downloadImageAsyncTask = new AsyncTask<String, Void, Bitmap>() {
                @Override
                protected Bitmap doInBackground(String... params) {
                    InputStream in = null;

                    try {
                        URL url = new URL(userData.getString(IMAGE_URL));
                        URLConnection connection = url.openConnection();
                        connection.setDoInput(true);
                        connection.connect();
                        in = connection.getInputStream();
                        return BitmapFactory.decodeStream(in);
                    } catch (Throwable e) {
                        e.printStackTrace();
                        return null;
                    } finally {
                        if (in != null) {
                            try {
                                in.close();
                            } catch (Throwable e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }

                @Override
                protected void onPostExecute(Bitmap result) {
                    super.onPostExecute(result);

                    postNotificationPrepared(context, prepareNotification(context, title, text, id, notificationProfile, userData, result), id, userData, badgeNumber, repeatedOrNotScheduled);
                }
            };

            downloadImageAsyncTask.execute();
        } else {
            postNotificationPrepared(context, prepareNotification(context, title, text, id, notificationProfile, userData, null), id, userData, badgeNumber, repeatedOrNotScheduled);
        }
    }

    static void postPushNotification(Context context, Bundle extras) {
        context = context.getApplicationContext();

        if (!pushNotificationsEnabled(context)) {
            return;
        }

        SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
        String titleKey = prefs.getString(TITLE_FIELD_NAME, "title");
        String textKey = prefs.getString(TEXT_FIELD_NAME, "text");
        String userDataKey = prefs.getString(USER_DATA_FIELD_NAME, "");
        String notificationProfileKey = prefs.getString(NOTIFICATION_PROFILE_FIELD_NAME, "notification_profile");
        String idKey = prefs.getString(ID_FIELD_NAME, "id");
        String badgeKey = prefs.getString(BADGE_FIELD_NAME, "badge_number");

        String title = extractStringFromBundle(extras, titleKey);
        if (title == null) {
            title = "\"data/" + titleKey + "\" not present! Configure Advanced->Push Payload Format in the UTNotifications settings";
        }

        String text = extractStringFromBundle(extras, textKey);
        if (text == null) {
            // OneSignal format support
            text = extras.getString(ALERT);
            extras.remove(ALERT);
        }
        if (text == null) {
            // "body" format support
            text = extras.getString(BODY);
            extras.remove(BODY);
        }
        if (text == null) {
            text = "\"data/" + textKey + "\" not present! Configure Advanced->Push Payload Format in the UTNotifications settings";
        }

        String notificationProfile = extractStringFromBundle(extras, notificationProfileKey);
        int badgeNumber = extractIntFromBundle(extras, badgeKey, -1);
        int id = extractIntFromBundle(extras, idKey, -1);

        Bundle userData = getSubBundle(extras, userDataKey.split("\\/"), false);
        if (userData == null) {
            userData = extras;
        }

        userData.putString(KEY_TITLE, title);
        userData.putString(KEY_TEXT, text);

        Manager.postNotification(context, title, text, (id != -1) ? id : Manager.getNextPushNotificationId(context), userData, notificationProfile, badgeNumber, true);
    }

    static void onRegistered(String providerName, String id) {
        if (UnityPlayer.currentActivity != null) {
            JSONArray json = new JSONArray();
            json.put(providerName);
            json.put(id);

            UnityPlayer.UnitySendMessage("UTNotificationsManager", "_OnAndroidIdReceived", json.toString());
        }
    }

    static boolean notificationsEnabled(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
        return prefs.getBoolean(NOTIFICATIONS_ENABLED, true);
    }

    static boolean pushNotificationsEnabled(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
        return prefs.getBoolean(PUSH_NOTIFICATIONS_ENABLED, true);
    }

    static int getNextPushNotificationId(Context context) {
        context = context.getApplicationContext();

        if (m_nextPushNotificationId == -1) {
            m_nextPushNotificationId = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE).getInt(START_ID, 0);
        }

        if (context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE).getBoolean(INCREMENTAL_ID, false)) {
            return m_nextPushNotificationId++;
        } else {
            return m_nextPushNotificationId;
        }
    }

    static int[] getStoredScheduledNotificationIds(Context context) {
        String scheduledNotificationIdsString = getStoredScheduledNotificationIdsString(context);

        if (scheduledNotificationIdsString == null || scheduledNotificationIdsString.isEmpty()) {
            return new int[0];
        }

        String[] split = scheduledNotificationIdsString.split(",");
        int[] res = new int[split.length];
        for (int i = 0; i < split.length; ++i) {
            res[i] = Integer.parseInt(split[i]);
        }

        return res;
    }

    static void scheduleNotificationCommon(Context context, boolean repeated, int triggerInSeconds, int intervalSeconds, String title, String text, int id, Bundle userData, String notificationProfile, int badgeNumber) {
        context = context.getApplicationContext();

        PendingIntent pendingIntent = getPendingIntentForScheduledNotification(context, id, userData, repeated);

        final long elapsedRealtime = SystemClock.elapsedRealtime();
        AlarmManager alarmManager = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
        if (repeated) {
            // http://developer.android.com/reference/android/app/AlarmManager.html#setInexactRepeating(int, long, long, android.app.PendingIntent)
            // As of API 19, all repeating alarms are inexact. Because this method has been available since API 3, your application can safely call
            // it and be assured that it will get similar behavior on both current and older versions of Android.
            alarmManager.setInexactRepeating(AlarmManager.ELAPSED_REALTIME, elapsedRealtime + (long) triggerInSeconds * 1000L, (long) intervalSeconds * 1000L, pendingIntent);
        } else {
            alarmManager.set(AlarmManager.ELAPSED_REALTIME, elapsedRealtime + (long) triggerInSeconds * 1000L, pendingIntent);
        }

        storeScheduledNotificationId(context, repeated, triggerInSeconds, intervalSeconds, title, text, id, userData, notificationProfile, badgeNumber);
    }

    static void addMainArgumentsToBundle(Bundle bundle, String title, String text, String notificationProfile, int badgeNumber) {
        bundle.putString(KEY_TITLE, title);
        bundle.putString(KEY_TEXT, text);
        bundle.putString(KEY_NOTIFICATION_PROFILE, notificationProfile);
        bundle.putInt(KEY_BADGE_NUMBER, badgeNumber);
    }

    static boolean backgroundMode() {
        return m_backgroundMode;
    }

    static void setIntent(Intent intent_) {
        intent = intent_;
    }

//private
    private static Intent getIntent() {
        return intent;
    }

    private static void hideAllNotifications(Context context) {
        NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.cancelAll();
    }

    private static void postNotificationPrepared(Context context, Notification notification, int id, Bundle userData, int badgeNumber, boolean repeatedOrNotScheduled) {
        if (notificationsEnabled(context)) {
            if (checkShowMode(context)) {
                if (context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE).getBoolean(SHOW_LATEST_NOTIFICATIONS_ONLY, false)) {
                    hideAllNotifications(context);
                }

                NotificationManager notificationManager = (NotificationManager) context.getSystemService(Context.NOTIFICATION_SERVICE);
                notificationManager.notify(id, notification);

                if (badgeNumber >= 0) {
                    setBadge(context, badgeNumber);
                }
            }

            notificationReceived(context.getApplicationContext(), id, userData);
        }

        if (!repeatedOrNotScheduled) {
            clearSheduledNotificationId(context, id);
        }
    }

    private static PendingIntent getPendingIntentForScheduledNotification(Context context, int id, Bundle userData, boolean repeated) {
        Intent notificationIntent = new Intent(context, AlarmBroadcastReceiver.class);
        notificationIntent.setData(Uri.parse("custom://ut." + id));
        notificationIntent.putExtra(KEY_ID, id);
        if (repeated) {
            notificationIntent.putExtra(KEY_REPEATED, true);
        }
        if (userData != null) {
            notificationIntent.putExtra(KEY_USER_DATA, userData);
        }

        return PendingIntent.getBroadcast(context, id, notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);
    }

    private static int getIcon(Context context, Resources res, String notificationProfile) {
        int icon = 0;

        //Check if need to use Android 5.0+ version of the icon
        if (android.os.Build.VERSION.SDK_INT >= 21) {
            icon = res.getIdentifier(notificationProfile + "_android5plus", "drawable", context.getPackageName());
        }

        if (icon == 0) {
            icon = res.getIdentifier(notificationProfile, "drawable", context.getPackageName());
        }

        if (icon == 0) {
            if (DEFAULT_PROFILE_NAME.equals(notificationProfile)) {
                return res.getIdentifier("app_icon", "drawable", context.getPackageName());
            } else {
                return getIcon(context, res, DEFAULT_PROFILE_NAME);
            }
        }

        return icon;
    }

    private static int getLargeIcon(Context context, Resources res, String notificationProfile) {
        int largeIcon = res.getIdentifier(notificationProfile + "_large", "drawable", context.getPackageName());
        if (largeIcon == 0 && !DEFAULT_PROFILE_NAME.equals(notificationProfile)) {
            return getLargeIcon(context, res, DEFAULT_PROFILE_NAME);
        } else {
            return largeIcon;
        }
    }

    private static String getSoundUri(Context context, Resources res, String notificationProfile) {
        int soundId = res.getIdentifier(notificationProfile, "raw", context.getPackageName());
        if (soundId == 0 && !DEFAULT_PROFILE_NAME.equals(notificationProfile)) {
            return getSoundUri(context, res, DEFAULT_PROFILE_NAME);
        } else {
            if (soundId != 0) {
                return "android.resource://" + context.getPackageName() + "/raw/" + notificationProfile;
            } else {
                return null;
            }
        }
    }

    @SuppressWarnings("deprecation")
    private static Notification prepareNotification(Context context, String title, String text, int id, String notificationProfile, Bundle userData, Bitmap bitmap) {
        Resources res = context.getResources();

        if (notificationProfile == null || notificationProfile.isEmpty()) {
            notificationProfile = DEFAULT_PROFILE_NAME;
        }

        //Find an icon
        int icon = getIcon(context, res, notificationProfile);

        //Find a custom large icon if specified
        int largeIcon = getLargeIcon(context, res, notificationProfile);

        //Find a custom sound if specified
        String soundUri = getSoundUri(context, res, notificationProfile);

        title = URLDecoder.decode(title);
        text = URLDecoder.decode(text);

        Intent notificationIntent = new Intent(context, NotificationIntentService.class);
        notificationIntent.putExtra(KEY_NOTIFICATION, packReceivedNotification(id, userData));
        PendingIntent contentIntent = PendingIntent.getService(context, id, notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);

        NotificationCompat.Builder builder = new NotificationCompat.Builder(context)
                .setSmallIcon(icon)
                .setContentTitle(title)
                .setDefaults((soundUri == null) ? NotificationCompat.DEFAULT_ALL : NotificationCompat.DEFAULT_LIGHTS | NotificationCompat.DEFAULT_VIBRATE)
                .setContentText(text)
                .setContentIntent(contentIntent)
                .setAutoCancel(true);

        // Android version prior 5 crop oversized icons instead of scaling them. Let's help it.
        if (android.os.Build.VERSION.SDK_INT < 21 && android.os.Build.VERSION.SDK_INT >= 11) {
            if (largeIcon == 0) {
                largeIcon = icon;
            }

            int targetWidth = res.getDimensionPixelSize(android.R.dimen.notification_large_icon_width);
            int targetHeight = res.getDimensionPixelSize(android.R.dimen.notification_large_icon_height);

            builder.setLargeIcon(Bitmap.createScaledBitmap(BitmapFactory.decodeResource(res, largeIcon), targetWidth, targetHeight, false));
        } else if (largeIcon != 0) {
            builder.setLargeIcon(BitmapFactory.decodeResource(res, largeIcon));
        }

        if (soundUri != null) {
            builder.setSound(Uri.parse(soundUri));
        }

        if (bitmap != null) {
            builder.setStyle(new NotificationCompat.BigPictureStyle().bigPicture(bitmap).setBigContentTitle(title).setSummaryText(text));
        } else {
            builder.setStyle(new NotificationCompat.BigTextStyle().bigText(text));
        }

        boolean hasGroup = false;
        int groupingMode = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE).getInt(NOTIFICATIONS_GROUPING_MODE, 0);
        switch (groupingMode) {
            //NONE
            case 0:
                break;

            //BY_NOTIFICATION_PROFILES
            case 1:
                builder.setGroup(notificationProfile);
                hasGroup = true;
                break;

            //FROM_USER_DATA
            case 2:
                if (userData.containsKey("notification_group")) {
                    builder.setGroup(userData.getString("notification_group"));
                    hasGroup = true;
                }
                break;

            //ALL_IN_A_SINGLE_GROUP
            case 3:
                builder.setGroup("__ALL");
                hasGroup = true;
                break;
        }

        if (hasGroup && userData.containsKey("notification_group_summary")) {
            builder.setGroupSummary(true);
        }

        return builder.build();
    }

    private static Bundle userDataToBundle(String[] userData, String title, String text, String notificationProfile, int badgeNumber) {
        Bundle bundle = new Bundle();

        addMainArgumentsToBundle(bundle, title, text, notificationProfile, badgeNumber);

        if (userData != null) {
            for (int i = 1; i < userData.length; i += 2) {
                bundle.putString(userData[i - 1], userData[i]);
            }
        }

        return bundle;
    }

    private static String packReceivedNotification(int id, Bundle userData) {
        try {
            JSONObject json = new JSONObject();

            json.put("title", userData.getString(KEY_TITLE));
            json.put("text", userData.getString(KEY_TEXT));
            json.put("id", id);
            json.put("notification_profile", userData.getString(KEY_NOTIFICATION_PROFILE));
            json.put("badgeNumber", userData.getInt(KEY_BADGE_NUMBER, -1));

            JSONObject userDataJson = new JSONObject();

            for (String key : userData.keySet()) {
                if (key != null && !KEY_TITLE.equals(key) && !KEY_TEXT.equals(key) && !KEY_NOTIFICATION_PROFILE.equals(key) && !KEY_BADGE_NUMBER.equals(key)) {
                    try {
                        String val = userData.get(key).toString();
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
            }

            json.put("user_data", userDataJson);

            return json.toString();
        } catch (JSONException e) {
            e.printStackTrace();
            return null;
        }
    }

    private static void notificationReceived(Context context, int id, Bundle userData) {
        boolean willHandleReceivedNotifications = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE).getBoolean(WILL_HANDLE_RECEIVED_NOTIFICATIONS, false);

        if (willHandleReceivedNotifications) {
            String receivedNotificationsPacked = readReceivedNotificationsPacked(context);
            String receivedPacked = packReceivedNotification(id, userData);

            if (receivedPacked != null) {
                if (receivedNotificationsPacked != null && !receivedNotificationsPacked.isEmpty()) {
                    receivedNotificationsPacked += ',' + receivedPacked;
                } else {
                    receivedNotificationsPacked = receivedPacked;
                }

                SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = prefs.edit();
                editor.putString(RECEIVED_NOTIFICATIONS, receivedNotificationsPacked);
                editor.commit();
            }
        }
    }

    private static String readReceivedNotificationsPacked(Context context) {
        return context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE).getString(RECEIVED_NOTIFICATIONS, "");
    }

    private static String getStoredScheduledNotificationIdsString(Context context) {
        return context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE).getString(SCHEDULED_NOTIFICATION_IDS, "");
    }

    private static void storeScheduledNotificationId(Context context, boolean repeated, int triggerInSeconds, int intervalSeconds, String title, String text, int id, Bundle userData, String notificationProfile, int badgeNumber) {
        ScheduledNotificationsRestorer.register(context, repeated, triggerInSeconds, intervalSeconds, title, text, id, userData, notificationProfile, badgeNumber);

        int[] ids = getStoredScheduledNotificationIds(context);
        for (int storedId : ids) {
            if (storedId == id) {
                return;
            }
        }

        String scheduledNotificationIdsString = getStoredScheduledNotificationIdsString(context);
        if (scheduledNotificationIdsString == null || scheduledNotificationIdsString.isEmpty()) {
            scheduledNotificationIdsString = "" + id;
        } else {
            scheduledNotificationIdsString += "," + id;
        }

        SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putString(SCHEDULED_NOTIFICATION_IDS, scheduledNotificationIdsString);
        editor.commit();
    }

    private static void clearSheduledNotificationId(Context context, int id) {
        String idAsString = Integer.toString(id);

        String scheduledNotificationIdsString = getStoredScheduledNotificationIdsString(context);
        if (scheduledNotificationIdsString != null && !scheduledNotificationIdsString.isEmpty()) {
            boolean found = false;
            if (scheduledNotificationIdsString == idAsString) {
                scheduledNotificationIdsString = "";
                found = true;
            } else {
                if (scheduledNotificationIdsString.indexOf("," + idAsString + ",") >= 0) {
                    scheduledNotificationIdsString = scheduledNotificationIdsString.replace("," + idAsString + ",", ",");
                    found = true;
                } else if (scheduledNotificationIdsString.startsWith(idAsString + ",")) {
                    scheduledNotificationIdsString = scheduledNotificationIdsString.substring(idAsString.length() + 1);
                    found = true;
                } else if (scheduledNotificationIdsString.endsWith("," + idAsString)) {
                    scheduledNotificationIdsString = scheduledNotificationIdsString.substring(0, scheduledNotificationIdsString.length() - (idAsString.length() + 1));
                    found = true;
                }
            }

            if (found) {
                SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
                SharedPreferences.Editor editor = prefs.edit();
                editor.putString(SCHEDULED_NOTIFICATION_IDS, scheduledNotificationIdsString);
                editor.commit();
            }
        }

        ScheduledNotificationsRestorer.cancel(context, id);
    }

    private static boolean checkShowMode(Context context) {
        int showNotificationsMode = context.getApplicationContext().getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE).getInt(SHOW_NOTIFICATIONS_MODE, 0);

        switch (showNotificationsMode) {
            //WHEN_CLOSED_OR_IN_BACKGROUND
            case 0:
                return UnityPlayer.currentActivity == null || backgroundMode();

            //WHEN_CLOSED
            case 1:
                return UnityPlayer.currentActivity == null;

            //ALWAYS
            default:
                return true;
        }
    }

    private static void setBadge(Context context, int badgeNumber) {
        if (badgeNumber != 0) {
            ShortcutBadger.applyCount(context, badgeNumber);
        } else {
            ShortcutBadger.removeCount(context);
        }

        SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
        SharedPreferences.Editor editor = prefs.edit();
        editor.putInt(KEY_BADGE_NUMBER, badgeNumber);
        editor.commit();
    }

    private static int getBadge(Context context) {
        SharedPreferences prefs = context.getSharedPreferences(Manager.class.getName(), Context.MODE_PRIVATE);
        return prefs.getInt(KEY_BADGE_NUMBER, 0);
    }

    private static Bundle getSubBundle(Bundle bundle, String[] path, boolean exceptOfTheLast) {
        if (path == null || path.length == 0 || (path.length == 1 && (path[0] == null || path[0].isEmpty()))) {
            return bundle;
        }

        int end = (exceptOfTheLast ? path.length - 1 : path.length);
        if (end > 0) {
            Object obj = bundle.get(path[0]);
            if (obj instanceof String) {
                Bundle asBundle = jsonToBundle((String)obj);
                if (asBundle != null) {
                    bundle.putBundle(path[0], asBundle);
                }
            }
        } else {
            return bundle;
        }

        for (int i = 0; i < end; ++i) {
            try {
                bundle = bundle.getBundle(path[i]);
            } catch (Throwable e) {
                return null;
            }

            if (bundle == null) {
                return null;
            }
        }

        return bundle;
    }

    private static String extractStringFromBundle(Bundle bundle, String key) {
        String[] path = key.split("\\/");
        bundle = getSubBundle(bundle, path, true);
        if (bundle != null) {
            try {
                key = path[path.length - 1];
                String value = bundle.getString(key);
                bundle.remove(key);
                return value;
            } catch (Throwable e) {
                return null;
            }
        } else {
            return null;
        }
    }

    private static int extractIntFromBundle(Bundle bundle, String key, int defaultValue) {
        int value = defaultValue;

        String[] path = key.split("\\/");
        bundle = getSubBundle(bundle, path, true);
        if (bundle != null) {
            key = path[path.length - 1];
            value = bundle.getInt(key, defaultValue);

            if (value == defaultValue) {
                try {
                    String valueAsString = bundle.getString(key);
                    value = Integer.parseInt(valueAsString);
                } catch (Throwable e) {}
            }

            bundle.remove(key);
        }

        return value;
    }

    private static Bundle jsonToBundle(String json) {
        try {
            JSONObject jsonObject = new JSONObject(json);
            return jsonToBundle(jsonObject);
        } catch (JSONException e) {
            e.printStackTrace();
            return null;
        }
    }

    private static Bundle jsonToBundle(JSONObject json) throws JSONException {
        Bundle bundle = new Bundle();

        Iterator<String> keys = json.keys();
        while (keys.hasNext()) {
            String key = keys.next();
            Object value = json.get(key);

            if (value instanceof String) {
                bundle.putString(key, (String)value);
            } else if (value instanceof Integer) {
                bundle.putInt(key, (Integer)value);
            } else if (value instanceof Long) {
                bundle.putInt(key, (int)(long)(Long)value);
            } else if (value instanceof JSONObject) {
                bundle.putBundle(key, jsonToBundle((JSONObject)value));
            } else {
                bundle.putString(key, value.toString());
            }
        }
        return bundle;

    }

    @SuppressWarnings("unused")
    private static IPushNotificationsProvider m_provider;
    private static Intent intent;
    private static final String WILL_HANDLE_RECEIVED_NOTIFICATIONS = "WILL_HANDLE_RECEIVED_NOTIFICATIONS";
    private static final String INCREMENTAL_ID = "INCREMENTAL_ID";
    private static final String START_ID = "START_ID";
    private static final String SHOW_NOTIFICATIONS_MODE = "SHOW_NOTIFICATIONS_MODE";
    private static final String NOTIFICATIONS_GROUPING_MODE = "NOTIFICATIONS_GROUPING_MODE";
    private static final String SHOW_LATEST_NOTIFICATIONS_ONLY = "SHOW_LATEST_NOTIFICATIONS_ONLY";
    private static final String NOTIFICATIONS_ENABLED = "NOTIFICATIONS_ENABLED";
    private static final String PUSH_NOTIFICATIONS_ENABLED = "PUSH_NOTIFICATIONS_ENABLED";
    private static final String RECEIVED_NOTIFICATIONS = "RECEIVED_NOTIFICATIONS";
    private static final String SCHEDULED_NOTIFICATION_IDS = "SCHEDULED_NOTIFICATION_IDS";
    private static final String TITLE_FIELD_NAME = "TITLE_FIELD_NAME";
    private static final String TEXT_FIELD_NAME = "TEXT_FIELD_NAME";
    private static final String USER_DATA_FIELD_NAME = "USER_DATA_FIELD_NAME";
    private static final String NOTIFICATION_PROFILE_FIELD_NAME = "NOTIFICATION_PROFILE_FIELD_NAME";
    private static final String ID_FIELD_NAME = "ID_FIELD_NAME";
    private static final String BADGE_FIELD_NAME = "BADGE_FIELD_NAME";
    private static final String ALERT = "alert";
    private static final String BODY = "body";
    private static final String IMAGE_URL = "image_url";
    private static final String DEFAULT_PROFILE_NAME = "__default_profile";
    private static int m_nextPushNotificationId = -1;
    private static boolean m_backgroundMode = false;
}
