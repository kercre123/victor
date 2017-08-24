package universal.tools.notifications;

import android.content.Context;
import android.content.Intent;

import com.google.android.gms.common.ConnectionResult;
import com.google.android.gms.common.GoogleApiAvailability;

class FCMProvider implements IPushNotificationsProvider {
//public
    public static final String Name = "FCM";

    public static boolean isAvailable(Context context) {
        try {
            GoogleApiAvailability apiAvailability = GoogleApiAvailability.getInstance();
            return apiAvailability != null && apiAvailability.isGooglePlayServicesAvailable(context) == ConnectionResult.SUCCESS;
        } catch (Throwable e) {
            return false;
        }
    }

    // Provide Application Context here
    public FCMProvider(Context context, String firebaseSenderID) {
        this.context = context;
        this.firebaseSenderID = firebaseSenderID;
    }

    @Override
    public void enable() {
        startGcmInstanceIDListenerService(firebaseSenderID);
    }

    @Override
    public void disable() {
        startGcmInstanceIDListenerService(GcmInstanceIDListenerService.UNREGISTER);
    }

    private void startGcmInstanceIDListenerService(String value) {
        Intent intent = new Intent(context, GcmInstanceIDListenerService.class);
        intent.putExtra(GcmInstanceIDListenerService.SENDER_ID, value);
        context.startService(intent);
    }

    private final Context context;
    private final String firebaseSenderID;
}
